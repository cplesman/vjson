#pragma once
#pragma warning(disable: 26495)

#include "../stream.h"
#include "../virtual_mem/virtual_mem.h"
#ifdef _DEBUG
#include <assert.h>
#endif

#define JSON_OBJ			1 //AKA. MAP
#define JSON_ARRAY			4 //ARRAY OF JSONOBJ/TEXT/NUMBER/ARRAY/RAWDATA 
#define JSON_TEXT			2 //
#define JSON_STRING			2 // SAME AS TEXT
#define JSON_NUMBER			3 //
#define JSON_BOOLEAN		5 
#define JSON_NULL			0
#define JSON_UNDEFINED		6

#define JSON_ERROR_OUTOFMEMORY -8
#define JSON_ERROR_INVALIDDATA -4

void *json_alloc(unsigned long size);
void json_free(void *ptr);
extern vmem *g_jsonMem;

class _jsonobj;
struct jsonobj_functable {
	long (*Type)();
	long (*Create)(i64 *);
	void (*Delete)(i64);
	void (*Free)(_jsonobj*);
	long (*Load)(i64, stream *, char);
};
extern jsonobj_functable jsonnull_ftable;
extern jsonobj_functable jsonobj_ftable;
extern jsonobj_functable jsonarray_ftable;
extern jsonobj_functable jsonstring_ftable;
extern jsonobj_functable jsonnumber_ftable;
extern jsonobj_functable jsonboolean_ftable;
extern jsonobj_functable jsonundefined_ftable;

extern jsonobj_functable *jsonobj_ftables[6];

class _jsonobj {
public:
	unsigned long m_ftable; //serves as json obj type
	//other variables go here
	long Send(stream *buf, int pretty=0);
};

class jsonkeypair {
public:
	i64 key; //char *
	i64 val; //__jsonobj*
	i64 next; //jsonkeypair* next;
	long Init() {
		this->key = 0;
		this->val = 0;
		this->next = 0;
		return 0;
	}
	long Init(const char* key, i64 val = 0) {
		if (key) {
			unsigned long i = 0;
			while (key[i]) i++;
			if(i>0x1ff) return JSON_ERROR_INVALIDDATA; //key too long
			this->key = g_jsonMem->Alloc(sizeof(char)*(i + 1));
			if (!this->key) return JSON_ERROR_OUTOFMEMORY;
			i=0;
			char *keyPtr = (char*)g_jsonMem->Lock(this->key);
			while (key[i]) { keyPtr[i] = key[i];i++; } keyPtr[i] = 0; //null terminate
			g_jsonMem->Unlock(this->key);
		}
		this->val = val;
		this->next = 0;
		return 0;
	}
	long Init(const char* key, unsigned long kLen, i64 val = 0) {
		if (key && kLen) {
			if(kLen>0x1ff) return JSON_ERROR_INVALIDDATA; //key too long
			this->key = g_jsonMem->Alloc(sizeof(char)*(kLen + 1));
			if (!this->key) return JSON_ERROR_OUTOFMEMORY;
			unsigned long i=0;
			char *keyPtr = (char*)g_jsonMem->Lock(this->key);
			while (key[i]) { keyPtr[i] = key[i];i++; } keyPtr[i] = 0; //null terminate
			g_jsonMem->Unlock(this->key);
		}
		this->val = val;
		this->next = 0;
		return 0;
	}
	void Free() {
		if (key) g_jsonMem->Free(key); 
		if (val) { 
			_jsonobj* valPtr = (_jsonobj*)g_jsonMem->Lock(this->val);
			jsonobj_ftables[valPtr->m_ftable]->Delete(this->val);
			g_jsonMem->Unlock(this->val);
		} 
		next = 0; val = 0; key = 0;
	}
};

class jsonobj : public _jsonobj {
public:
	unsigned long m_tablesize;
	i64 m_tableLoc;
	//jsonkeypair** m_table;
	i64 operator [] (const char *p_key) {
		return Find(p_key);
	}
	i64 operator [] (const unsigned long p_idx); //will convert integer to string first

	long ResizeTable(unsigned long size) {
		if (size == 0) { 
			FreeChildren();
			return 0; 
		} //will free key pairs too
		if( size>0x10000 ) return JSON_ERROR_INVALIDDATA; //table too big
		unsigned long size2 = 1;
		while (size2<size) {
			size2 <<= 1;
		}
		i64 newTableLoc = g_jsonMem->Alloc(sizeof(jsonkeypair*)*size2);
		if (!newTableLoc) { return JSON_ERROR_OUTOFMEMORY; }
		i64 *m_table = (i64*)g_jsonMem->Lock(newTableLoc);
		for (unsigned long i = 0; i < size2; i++) {
			m_table[i] = 0;
		}
		if(m_tablesize){ //old size
			i64 *old_data = (i64*)g_jsonMem->Lock(m_tableLoc);
			unsigned long old_size = m_tablesize;
			unsigned long i;
			for (i = 0; i < old_size; i++) {
				i64 itr = old_data[i];
				while (itr) {
					jsonkeypair* itrPtr = (jsonkeypair*)g_jsonMem->Lock(itr);
					i64 next = itrPtr->next;
					char *keyPtr = (char*)g_jsonMem->Lock(itrPtr->key);
					unsigned long k = calculateHashKey(keyPtr);
					itrPtr->next = m_table[k]; m_table[k] = itr; //insert at beginning for speed
					g_jsonMem->Unlock(itrPtr->key);
					g_jsonMem->Unlock(itr);
					itr = next;
				}
			}
			g_jsonMem->Unlock(m_tableLoc);
			g_jsonMem->Free(m_tableLoc);
		}
		m_tablesize = size2;
		m_tableLoc = newTableLoc;
		g_jsonMem->Unlock(newTableLoc);
		return 0;
	}

	unsigned long calculateHashKey(const char *p_key) {
		const unsigned long mask = m_tablesize - 1;
		unsigned long key = _keyHash(p_key) & mask;
		return key;
	}
	static unsigned long _keyHash(const char*str) {
		unsigned long hash = 5381; // Initial prime number
		int c;

		while ((c = *str++)) {
			hash = ((hash << 5) + hash) + c; // hash * 33 + c
		}

		return hash;
		// unsigned long hash = 0, i = 0;
		// while (p_key[i] && i < 256/*bytes*/) {
		// 	if (!(i & 3)) {
		// 		hash ^= (i & 7) ? 0x55555555 : 0xaaaaaaaa;
		// 	}
		// 	hash += ((unsigned long)p_key[i]) << (i & 3);
		// 	i++;
		// }
		// hash ^= i;
		// return hash;
	}

	void FreeChildren() {
		if (m_tableLoc) {
			i64 *m_table = (i64*)g_jsonMem->Lock(m_tableLoc);
			unsigned long i;
			for (i = 0; i < m_tablesize; i++) {
				i64 first = m_table[i];
				while(first) {
					jsonkeypair* firstPtr = (jsonkeypair*)g_jsonMem->Lock(first);
					i64 next = firstPtr->next;
					firstPtr->Free();
					g_jsonMem->Unlock(first);
					g_jsonMem->Free(first);
					first = next;
				}
			}
			g_jsonMem->Unlock(m_tableLoc);
			g_jsonMem->Free(m_tableLoc);
		}
		m_tableLoc = 0;
		m_tablesize = 0;
	}

	i64 Find(const char *key);
	unsigned long NumKeys(i64 p_firstChild) {
		unsigned count = 0;
		i64 next = p_firstChild;
		while(next){
			jsonkeypair* nextPtr = (jsonkeypair*)g_jsonMem->Lock(next);
			i64 nxt = nextPtr->next;
			g_jsonMem->Unlock(next);
			count++;
			next = nxt;;
		}
		return count;
	}
	unsigned long NumKeys() {
		unsigned long i,count=0;
		i64 *m_table = (i64*)g_jsonMem->Lock(m_tableLoc);
		for (i = 0; i < m_tablesize; i++) {
			count += NumKeys(m_table[i]);
		}
		g_jsonMem->Unlock(m_tableLoc);
		return count;
	}

	long addChild(i64 childLoc) {
		jsonkeypair* child = (jsonkeypair*)g_jsonMem->Lock(childLoc);
		i64 *m_table = (i64*)g_jsonMem->Lock(m_tableLoc);
		char *keyPtr = (char*)g_jsonMem->Lock(child->key);
		unsigned long key = calculateHashKey(keyPtr);
		child->next = m_table[key]; //insert at beginning for speed
		m_table[key] = childLoc;
		g_jsonMem->Unlock(child->key);
		g_jsonMem->Unlock(childLoc);
		g_jsonMem->Unlock(m_tableLoc);
		return 0;
	}

	i64 AppendText(const char *key, const char *val);
	i64 AppendText(const char *key, const char *val, const unsigned long len);
	i64 AppendText(const char *key, i64 valLoc);
	i64 AppendObj(const char *key, i64 val);
	//append empty obj
	i64 AppendObj(const char *key);
	i64 AppendNumber(const char *key, double num);
	i64 AppendBoolean(const char *key, bool p_b);
	i64 AppendNull(const char *key);
	i64 AppendUndefined(const char *key);
};
long jsonobj_Type();
void jsonobj_Delete(i64);
long jsonobj_Create(i64*);
long jsonobj_Load(i64, stream*,char);
//load from stream before '{'
long jsonobj_Load(i64, stream*);

class jsonarray : public _jsonobj {
public:
	i64 m_dataLoc;
	unsigned long m_size;
	unsigned long m_totalsize;
	i64 operator[] (const unsigned long p_idx) {
		i64* dataPtr = (i64*)g_jsonMem->Lock(m_dataLoc);
		i64 ret = dataPtr[p_idx];
		g_jsonMem->Unlock(m_dataLoc);
		return ret;
	}

	long Resize(unsigned long p_newsize) {
		//assume new never fails
		//size^2 >= p_newsize
		i64 nArrayLoc = p_newsize ? g_jsonMem->Alloc(p_newsize*sizeof(i64)) : 0;
		if (!nArrayLoc && p_newsize) return JSON_ERROR_OUTOFMEMORY;
		if (m_dataLoc) {
			unsigned long i;
			unsigned long smallerSize = p_newsize;
			i64 *nArray = nArrayLoc ? (i64*)g_jsonMem->Lock(nArrayLoc) : 0;
			i64 *m_data = (i64*)g_jsonMem->Lock(m_dataLoc);
			if (m_size < p_newsize) smallerSize = m_size;
			for (i = 0; i < smallerSize; i++) {
				nArray[i] = m_data[i]; //copy pointers
			}
			for (; i < p_newsize; i++) {
				nArray[i] = 0;
			}
			for (; i < m_size; i++) {
				//if size is smaller then new size free all objects
				_jsonobj* objPtr = (_jsonobj*)g_jsonMem->Lock(m_data[i]);
				jsonobj_ftables[objPtr->m_ftable]->Free(objPtr);
				g_jsonMem->Unlock(m_data[i]);
				g_jsonMem->Free(m_data[i]);
			}
			g_jsonMem->Unlock(m_dataLoc);
			g_jsonMem->Free(m_dataLoc);
			if(nArrayLoc) g_jsonMem->Unlock(nArrayLoc);
			m_size = smallerSize;
		}
		else {
			m_size = 0;
		}
		m_totalsize = p_newsize;
		m_dataLoc = nArrayLoc;
		return 0;
	}

	void Free(void (*free)(void*)) {
		Resize(0);
	}

	i64 AppendText(const char *val);
	i64 AppendText(const char *val, const unsigned long len);
	i64 AppendObj(i64 val);
	i64 AppendNumber(double num);
	i64 AppendBoolean(bool p_b);

};
long jsonarray_Type();
void jsonarray_Delete(i64);
long jsonarray_Create(i64*);
long jsonarray_Load(i64, stream*, char);
//load from stream before '['
long jsonarray_Load(i64, stream*);

class jsonstring : public _jsonobj {
public:
	i64 m_str;
	//make sure m_str is not allocated before calling this
	long SetString(i64 sLoc) {
		if(m_str){
			g_jsonMem->Free(m_str);
			m_str = 0;
		}
		m_str = sLoc;
		return 0;
	}
	long SetString(const char *s) {
		unsigned long len = 0;
		if (s) {
			for (;s[len]; len++) {}
		}
		return SetString(s,len);
	}
	long SetString(const char *s, unsigned long len) {
		unsigned long i = 0;
		if(m_str){
			g_jsonMem->Free(m_str);
			m_str = 0;
		}
		if (s) {
			m_str = g_jsonMem->Alloc(sizeof(char)*(len + 1));
			if(!m_str) return JSON_ERROR_OUTOFMEMORY;
			char *strPtr = (char*)g_jsonMem->Lock(m_str);
			for (i=0; s[i]; i++) {
				strPtr[i] = s[i];
			}
			strPtr[i] = 0;
		}
		return i;
	}
	char operator[](long i) {
		char *strPtr = (char*)g_jsonMem->Lock(m_str);
		char ret = strPtr[i];
		g_jsonMem->Unlock(m_str);
		return ret;
	}

};
long jsonstring_Type();
void jsonstring_Delete(i64);
long jsonstring_Create(i64*);
long jsonstring_Load(i64, stream*, char);

class jsonnumber : public _jsonobj {
public:
	double num;
};
long jsonnumber_Type();
void jsonnumber_Delete(i64);
long jsonnumber_Create(i64*);
long jsonnumber_Load(i64, stream*, char);

class jsonboolean : public _jsonobj {
public:
	bool b;
};
long jsonboolean_Type();
void jsonboolean_Delete(i64);
long jsonboolean_Create(i64*);
long jsonboolean_Load(i64, stream*, char);

class jsonnull : public _jsonobj {
public:
};
long jsonnull_Type();
void jsonnull_Delete(i64);
long jsonnull_Create(i64*);
long jsonnull_Load(i64, stream*, char);

class jsonundefined : public _jsonobj {
public:
	bool b;
};
long jsonundefined_Type();
void jsonundefined_Delete(i64);
long jsonundefined_Create(i64*);
long jsonundefined_Load(i64, stream*, char);


long JSON_movepastwhite(stream *buf);

//long JSON_sendObj(stream *buf, struct t_jsonobj *obj);
long JSON_send(stream *buf, i64 p_obj, int pretty);

long JSON_parseVal(i64* obj, char lastch, stream* buf);

long JSON_parseString_iterateNumber(char *result, stream *buf) ;
long JSON_parseString_iterateQuote(char* result, char quote, stream* buf);
long JSON_parseString_iterateQuote_getLength(char quote, stream* buf);




