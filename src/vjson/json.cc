#include "json.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>

using namespace std;

vmem *g_jsonMem;

long jsonobj_Type() { return JSON_OBJ; }
long jsonarray_Type() { return JSON_ARRAY; }
long jsonstring_Type() { return JSON_STRING; }
long jsonnumber_Type() { return JSON_NUMBER; }
long jsonboolean_Type() { return JSON_BOOLEAN; }
long jsonnull_Type() { return JSON_NULL; }
long jsonundefined::Type() { return JSON_UNDEFINED; }

long jsonobj_Create(i64* p_obj) {
	*p_obj = g_jsonMem->Alloc(sizeof(jsonobj));
	if(!*p_obj) return JSON_ERROR_OUTOFMEMORY;
	jsonobj *obj = (jsonobj*)g_jsonMem->Lock(*p_obj);
	obj->m_ftable = JSON_OBJ;
	obj->m_tablesize = 0;
	obj->m_tableLoc = 0;
	g_jsonMem->Unlock(*p_obj);
	return 0;
}
long jsonarray_Create(i64* p_obj) {
	*p_obj = g_jsonMem->Alloc(sizeof(jsonarray));
	if(!*p_obj) return JSON_ERROR_OUTOFMEMORY;
	jsonarray *obj = (jsonarray*)g_jsonMem->Lock(*p_obj);
	obj->m_dataLoc = 0;
	obj->m_size = 0;
	obj->m_totalsize = 0;
	obj->m_ftable = JSON_ARRAY;
	g_jsonMem->Unlock(*p_obj);
	return 0;
}
long jsonstring_Create(i64* p_obj) {
	*p_obj = g_jsonMem->Alloc(sizeof(jsonstring));
	if(!*p_obj) return JSON_ERROR_OUTOFMEMORY;
	jsonstring *obj = (jsonstring*)g_jsonMem->Lock(*p_obj);
	obj->m_str = 0;
	obj->m_ftable = JSON_STRING;
	g_jsonMem->Unlock(*p_obj);
	return 0;
}
long jsonnumber_Create(i64* p_obj) {
	*p_obj = g_jsonMem->Alloc(sizeof(jsonnumber));
	if(!*p_obj) return JSON_ERROR_OUTOFMEMORY;
	jsonnumber *obj = (jsonnumber*)g_jsonMem->Lock(*p_obj);
	obj->num = 0;
	obj->m_ftable = JSON_NUMBER;
	g_jsonMem->Unlock(*p_obj);
	return 0;
}
long jsonboolean_Create(i64* p_obj) {
	*p_obj = g_jsonMem->Alloc(sizeof(jsonboolean));
	if(!*p_obj) return JSON_ERROR_OUTOFMEMORY;
	jsonboolean *obj = (jsonboolean*)g_jsonMem->Lock(*p_obj);
	obj->b = false;
	obj->m_ftable = JSON_BOOLEAN;
	g_jsonMem->Unlock(*p_obj);
	return 0;
}
long jsonundefined::Create(i64* p_obj) {
	*p_obj = g_jsonMem->Alloc(sizeof(jsonundefined));
	if(!*p_obj) return JSON_ERROR_OUTOFMEMORY;
	jsonundefined *obj = (jsonundefined*)g_jsonMem->Lock(*p_obj);
	obj->m_ftable = JSON_UNDEFINED;
	g_jsonMem->Unlock(*p_obj);
	return 0;
}
long jsonnull_Create(i64* p_obj) {
	*p_obj = g_jsonMem->Alloc(sizeof(jsonnull));
	if(!*p_obj) return JSON_ERROR_OUTOFMEMORY;
	jsonnull *obj = (jsonnull*)g_jsonMem->Lock(*p_obj);
	obj->m_ftable = JSON_NULL;
	g_jsonMem->Unlock(*p_obj);
	return 0;
}

void jsonobj_Free(_jsonobj* p_obj) {
	jsonobj* obj = (jsonobj*)p_obj;
	obj->FreeChildren();
}
void jsonobj_Delete(i64 p_obj) {
	jsonobj* obj = (jsonobj*)g_jsonMem->Lock(p_obj);
	jsonobj_Free(obj);
	g_jsonMem->Unlock(p_obj);
	g_jsonMem->Free(p_obj);
}
void jsonarray_Free(_jsonobj* p_obj) {
	jsonarray* obj = (jsonarray*)p_obj;
	if(obj->m_size){
		obj->Resize(0);
	}
	if(obj->m_dataLoc){
		g_jsonMem->Free(obj->m_dataLoc);
	}
}
void jsonarray_Delete(i64 p_obj) {
	jsonarray* obj = (jsonarray*)g_jsonMem->Lock(p_obj);
	jsonarray_Free(obj);
	g_jsonMem->Unlock(p_obj);
	g_jsonMem->Free(p_obj);
}
void jsonstring_Free(_jsonobj* obj) {
	if(((jsonstring*)obj)->m_str){
		g_jsonMem->Free(((jsonstring*)obj)->m_str);
	}
}
void jsonstring_Delete(i64 p_obj) {
	jsonstring* obj = (jsonstring*)g_jsonMem->Lock(p_obj);
	jsonstring_Free(obj);
	g_jsonMem->Unlock(p_obj);
	g_jsonMem->Free(p_obj);
}
void jsonnumber_Free(_jsonobj* obj) {
	//nothing to free
}
void jsonnumber_Delete(i64 p_obj) {
	jsonnumber* obj = (jsonnumber*)g_jsonMem->Lock(p_obj);
	jsonnumber_Free(obj);
	g_jsonMem->Unlock(p_obj);
	g_jsonMem->Free(p_obj);
}
void jsonboolean_Free(_jsonobj* obj) {
	//nothing to free
}
void jsonboolean_Delete(i64 p_obj) {
	jsonboolean* obj = (jsonboolean*)g_jsonMem->Lock(p_obj);
	jsonboolean_Free(obj);
	g_jsonMem->Unlock(p_obj);
	g_jsonMem->Free(p_obj);
}

void jsonnull_Free(_jsonobj* obj) {
	//nothing to free
}
void jsonnull_Delete(i64 p_obj) {
	jsonnull* obj = (jsonnull*)g_jsonMem->Lock(p_obj);
	jsonnull_Free(obj);
	g_jsonMem->Unlock(p_obj);
	g_jsonMem->Free(p_obj);
}

void jsonundefined::Free(_jsonobj* obj) {
	//nothing to free
}
void jsonundefined::Delete(i64 p_obj) {
	jsonundefined* obj = (jsonundefined*)g_jsonMem->Lock(p_obj);
	jsonundefined::Free(obj);
	g_jsonMem->Unlock(p_obj);
	g_jsonMem->Free(p_obj);
}

jsonobj_functable jsonobj_ftable = {
	jsonobj_Type,
	jsonobj_Create,
	jsonobj_Delete,
	jsonobj_Free,
	jsonobj_Load,
};
jsonobj_functable jsonarray_ftable = {
	jsonarray_Type,
	jsonarray_Create,
	jsonarray_Delete,
	jsonarray_Free,
	jsonarray_Load
};
jsonobj_functable jsonstring_ftable = {
	jsonstring_Type,
	jsonstring_Create,
	jsonstring_Delete,
	jsonstring_Free,
	jsonstring_Load
};
jsonobj_functable jsonnumber_ftable = {
	jsonnumber_Type,
	jsonnumber_Create,
	jsonnumber_Delete,
	jsonnumber_Free,
	jsonnumber_Load
};
jsonobj_functable jsonboolean_ftable = {
	jsonboolean_Type,
	jsonboolean_Create,
	jsonboolean_Delete,
	jsonboolean_Free,
	jsonboolean_Load
};
jsonobj_functable jsonnull_ftable = {
	jsonnull_Type,
	jsonnull_Create,
	jsonnull_Delete,
	jsonnull_Free,
	jsonnull_Load
};

jsonobj_functable jsonundefined_ftable = {
	jsonundefined::Type,
	jsonundefined::Create,
	jsonundefined::Delete,
	jsonundefined::Free,
	jsonundefined::Load
};

jsonobj_functable* jsonobj_ftables[7] = {
	&jsonnull_ftable,
	&jsonobj_ftable,
	&jsonstring_ftable,
	&jsonnumber_ftable,
	&jsonarray_ftable,
	&jsonboolean_ftable,
	&jsonundefined_ftable
};

i64 jsonobj::operator [] (const unsigned long p_idx) {
	char buf[32];
	snprintf(buf, sizeof(buf), "%lu", p_idx);
	//ultoa(p_idx, buf, 10);
	return Find(buf);
}


//jsonobj *JSON_findChild(darray<jsonobj *> *p_children, const char *p_key) {
//	if (!p_children) {
//		return nullptr;
//	}
//	unsigned long size = p_children->Size();
//	for(unsigned long i=0;i<size;i++) {
//		jsonobj*itr = p_children->At(i);
//		if (!strcmp(p_key, itr->name)) {
//			return itr;
//		}
//	}
//	return 0;
//}
i64 jsonobj::Find(const char *p_key) {
	unsigned long k = calculateHashKey(p_key);
	i64 *m_table = (i64*)g_jsonMem->Lock(m_tableLoc);
	i64 itr = m_table[k];
	while (itr) {
		jsonkeypair* itrPtr = (jsonkeypair*)g_jsonMem->Lock(itr);
		char *keyPtr = (char*)g_jsonMem->Lock(itrPtr->key);
		if (!strcmp(p_key, keyPtr)) {
			g_jsonMem->Unlock(itrPtr->key);
			g_jsonMem->Unlock(itr);
			return itrPtr->val;
		}
		i64 next = itrPtr->next;
		g_jsonMem->Unlock(itrPtr->key);
		g_jsonMem->Unlock(itr);
		itr = next;
	}
	return 0; //nothing found
}
long jsonobj::Delete(const char *p_key) {
	unsigned long k = calculateHashKey(p_key);
	i64 *m_table = (i64*)g_jsonMem->Lock(m_tableLoc);
	i64 itr = m_table[k];
	i64 prev = 0;
	while (itr) {
		jsonkeypair* itrPtr = (jsonkeypair*)g_jsonMem->Lock(itr);
		char *keyPtr = (char*)g_jsonMem->Lock(itrPtr->key);
		if (!strcmp(p_key, keyPtr)) {
			g_jsonMem->Unlock(itrPtr->key); //done with key
			itrPtr->Free(); 
			i64 next = itrPtr->next; 
			g_jsonMem->Unlock(itr);
			if(!prev) {m_table[k]=next;}
			else{ itrPtr = (jsonkeypair*)g_jsonMem->Lock(prev); itrPtr->next = next; g_jsonMem->Unlock(prev); } //de link itr from chain
			g_jsonMem->Free(itr);
			return 0;
		}
		i64 next = itrPtr->next;
		g_jsonMem->Unlock(itrPtr->key);
		g_jsonMem->Unlock(itr);
		prev = itr;
		itr = next;
	}

	return JSON_ERROR_INVALIDDATA; //nothing found
}
long jsonobj::Replace(const char *p_key,i64 p_newval) {
	unsigned long k = calculateHashKey(p_key);
	i64 *m_table = (i64*)g_jsonMem->Lock(m_tableLoc);
	i64 itr = m_table[k];
	while (itr) {
		jsonkeypair* itrPtr = (jsonkeypair*)g_jsonMem->Lock(itr);
		char *keyPtr = (char*)g_jsonMem->Lock(itrPtr->key);
		if (!strcmp(p_key, keyPtr)) {
			g_jsonMem->Unlock(itrPtr->key); //done with key
			if(itrPtr->val){
				_jsonobj* valPtr = (_jsonobj*)g_jsonMem->Lock(itrPtr->val);
				g_jsonMem->Unlock(itrPtr->val);
				jsonobj_ftables[valPtr->m_ftable]->Delete(itrPtr->val);
			}
			itrPtr->val = p_newval;
			g_jsonMem->Unlock(itr);
			return 0;
		}
		i64 next = itrPtr->next;
		g_jsonMem->Unlock(itrPtr->key);
		g_jsonMem->Unlock(itr);
		itr = next;
	}

	return JSON_ERROR_INVALIDDATA; //nothing found
}

void jsonarray::Delete(unsigned long index) {
	i64 *data = (i64*)g_jsonMem->Lock(m_dataLoc);
	if(data[index]){
		_jsonobj* objPtr = (_jsonobj*)g_jsonMem->Lock(data[index]);
		i64 t = objPtr->m_ftable;
		g_jsonMem->Unlock(data[index]);
		jsonobj_ftables[t]->Delete(data[index]);
	}
	g_jsonMem->Unlock(m_dataLoc);
}
void jsonarray::Replace(unsigned long index,i64 p_newval) {
	i64 *data = (i64*)g_jsonMem->Lock(m_dataLoc);
	if(data[index]){
		_jsonobj* objPtr = (_jsonobj*)g_jsonMem->Lock(data[index]);
		i64 t = objPtr->m_ftable;
		g_jsonMem->Unlock(data[index]);
		jsonobj_ftables[t]->Delete(data[index]);
		data[index] = p_newval;
	}
	g_jsonMem->Unlock(m_dataLoc);
}

long JSON_movepastwhite(stream *buf) {
	char ch;
	do {
		int err = buf->GetBytes(&ch, 1);
		if (err < 0) return err;
	} while ((ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t'));
	return ch;
}

//void JSON_addChildren(darray<darray<jsonobj*>*> *p_table, darray<jsonobj*> *p_children) {
//	unsigned long tsize = p_table->Size();
//#ifdef _DEBUG
//	unsigned long validtablesize = tsize;
//	while (validtablesize && !(validtablesize & 1)) {
//		validtablesize >>= 1;
//	}
//	assert(validtablesize == 1);
//#endif 
//	const unsigned long mask = tsize - 1;
//	unsigned long i;
//	const unsigned long numChildren = p_children->Size();
//	for(i=0;i<numChildren;i++) {
//		jsonobj *child = p_children->At(i);
//		unsigned long key = (jsonobj::getKeyHash(child->name) & mask);
//		darray<jsonobj*> *tchildren = p_table->At(key);
//		if (!tchildren) {
//			tchildren = (p_table->At(key) = new darray<jsonobj*>(1));
//		}
//		tchildren->push(child);
//	}
//}

long JSON_parseVal(i64 *p_val, char lastch, stream *buf) {
	int err;
	char ch = lastch;
	//determine type
	if (ch == '{') { //obj in obj
		err = jsonobj_Create(p_val);
		if (err < 0) { return err; }

		err = jsonobj_Load(*p_val,buf, ch);
		if (err < 0) { jsonobj_Delete(*p_val); *p_val = 0; return err; }

		return '}'; //ch
	}
	else if (ch == '[') {
		err = jsonarray_Create(p_val);
		if (err < 0) { return err; }

		err = jsonarray_Load(*p_val,buf, ch);
		if (err < 0) { if(*p_val) jsonarray_Delete(*p_val); *p_val = 0; return err; }

		return ']';//ch
	}
	else if (ch == '\'' || ch == '\"') {
		err = jsonstring_Create(p_val);
		if (err < 0) { if(*p_val) return err; }

		err = jsonstring_Load(*p_val,buf, ch);
		if (err < 0) { jsonstring_Delete(*p_val); *p_val = 0; return err; }

		return err; 
	}
	else if ((ch >= '0'&& ch <= '9')||ch=='-') {
		err = jsonnumber_Create(p_val);
		if (err < 0) { if(*p_val) return err; }

		err = jsonnumber_Load(*p_val,buf, ch);
		if (err < 0) { jsonnumber_Delete(*p_val); *p_val = 0; return err; }

		return err; 
	}
	else if( (ch=='t') || (ch=='f') ) {
		err = jsonboolean_Create(p_val);
		if (err < 0) { if(*p_val) return err; }

		err = jsonboolean_Load(*p_val,buf, ch);
		if (err < 0) { jsonboolean_Delete(*p_val); *p_val = 0; return err; }

		return err; 
	}
	else if( (ch=='n') ) {
		err = jsonnull_Create(p_val);
		if (err < 0) { if(*p_val) return err; }

		err = jsonnull_Load(*p_val,buf, ch);
		if (err < 0) { jsonnull_Delete(*p_val); *p_val = 0; return err; }

		return err; 
	}
	else if( ch=='u' ) {
		err = jsonundefined::Create(p_val);
		if (err < 0) { if(*p_val) return err; }

		err = jsonundefined::Load(*p_val,buf, ch);
		if (err < 0) { jsonundefined::Delete(*p_val); *p_val = 0; return err; }

		return err; 
	}

	return -4; //not a valid obj
}

long JSON_parseString_iterateNumber(char *result, stream *buf) {
	int size = 0;
	char ch = 0;
	while (1) {
		int err;
		err = buf->PeekBytes(0, &ch, 1);
		if (err < 0) return err;
		if ((ch>='0'&&ch<='9') || ch=='.' || ch=='e' || ch=='-'){
			err = buf->GetBytes( &ch, 1);
			if (err < 0) return err;
		}
		else {
			break;
		}
		result[size] = ch;
		size++;
		if (size >= 30) return JSON_ERROR_INVALIDDATA;
	}
	*(result + size) = 0; //null terminate
	return size;
}
long JSON_parseString_iterateQuote_getLength(char quote, stream* buf) {
	int size = 0;
	char ch;
	while (1) {
		int err;
		err = buf->PeekBytes(size, &ch, 1);
		if (err < 0) return err;
		char ch2 = ch;
		if (ch == '\\') { //escape
			int err;
			err = buf->PeekBytes(size, &ch2, 1);
			if (err < 0) return err;
			if (ch2 == 'r') ch2 = '\r';
			else if (ch2 == 'n')ch2 = '\n';
		}
		if (ch == quote) {
			break;
		}
		size++;
	}
	size++;
	return size;
}
long JSON_parseString_iterateQuote(char* result, char quote, stream* buf) {
	int size = 0;
	char ch = 0;
	if (!result) return 0;
	while (1) {
		int err = buf->GetBytes(&ch, 1);
		if (err < 0) return err;
		char ch2 = ch;
		if (ch == '\\') { //escape
			int err;
			err = buf->GetBytes(&ch2, 1);
			if (err < 0) return err;
			if (ch2 == 'r') ch2 = '\r';
			else if (ch2 == 'n')ch2 = '\n';
		}
		if (ch == quote) {
			break;
		}
		*(result + size) = ch2;
		size++;
	}
	*(result + size) = 0; //null terminate
	size++;
	return size;
}
