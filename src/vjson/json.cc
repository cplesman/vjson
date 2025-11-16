#include "json.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>

using namespace std;

vmem *g_jsonMem;

long jsonobj::Type() { return JSON_OBJ; }
long jsonarray::Type() { return JSON_ARRAY; }
long jsonstring::Type() { return JSON_STRING; }
long jsonnumber::Type() { return JSON_NUMBER; }
long jsonboolean::Type() { return JSON_BOOLEAN; }
long jsonnull::Type() { return JSON_NULL; }
long jsonundefined::Type() { return JSON_UNDEFINED; }

long jsonobj::Create(i64* p_obj) {
	*p_obj = g_jsonMem->Alloc(sizeof(jsonobj));
	if(!*p_obj) return JSON_ERROR_OUTOFMEMORY;
	jsonobj *obj = (jsonobj*)g_jsonMem->Lock(*p_obj);
	obj->m_ftable = JSON_OBJ;
	obj->m_tablesize = 0;
	obj->m_keycount = 0;
	obj->m_tableLoc = 0;
	g_jsonMem->Unlock(*p_obj);
	return 0;
}
long jsonarray::Create(i64* p_obj) {
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
long jsonstring::Create(i64* p_obj) {
	*p_obj = g_jsonMem->Alloc(sizeof(jsonstring));
	if(!*p_obj) return JSON_ERROR_OUTOFMEMORY;
	jsonstring *obj = (jsonstring*)g_jsonMem->Lock(*p_obj);
	obj->m_str = 0;
	obj->m_ftable = JSON_STRING;
	g_jsonMem->Unlock(*p_obj);
	return 0;
}
long jsonnumber::Create(i64* p_obj) {
	*p_obj = g_jsonMem->Alloc(sizeof(jsonnumber));
	if(!*p_obj) return JSON_ERROR_OUTOFMEMORY;
	jsonnumber *obj = (jsonnumber*)g_jsonMem->Lock(*p_obj);
	obj->num = 0;
	obj->m_ftable = JSON_NUMBER;
	g_jsonMem->Unlock(*p_obj);
	return 0;
}
long jsonboolean::Create(i64* p_obj) {
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
long jsonnull::Create(i64* p_obj) {
	*p_obj = g_jsonMem->Alloc(sizeof(jsonnull));
	if(!*p_obj) return JSON_ERROR_OUTOFMEMORY;
	jsonnull *obj = (jsonnull*)g_jsonMem->Lock(*p_obj);
	obj->m_ftable = JSON_NULL;
	g_jsonMem->Unlock(*p_obj);
	return 0;
}

void jsonobj::Free(_jsonobj* p_obj) {
	jsonobj* obj = (jsonobj*)p_obj;
	obj->FreeChildren();
}
void jsonobj::Delete(i64 p_obj) {
	jsonobj* obj = (jsonobj*)g_jsonMem->Lock(p_obj);
	jsonobj::Free(obj);
	g_jsonMem->Unlock(p_obj);
	g_jsonMem->Free(p_obj);
}
void jsonarray::Free(_jsonobj* p_obj) {
	jsonarray* obj = (jsonarray*)p_obj;
	if(obj->m_size){
		obj->Resize(0);
	}
	if(obj->m_dataLoc){
		g_jsonMem->Free(obj->m_dataLoc);
	}
}
void jsonarray::Delete(i64 p_obj) {
	jsonarray* obj = (jsonarray*)g_jsonMem->Lock(p_obj);
	jsonarray::Free(obj);
	g_jsonMem->Unlock(p_obj);
	g_jsonMem->Free(p_obj);
}
void jsonstring::Free(_jsonobj* obj) {
	if(((jsonstring*)obj)->m_str){
		g_jsonMem->Free(((jsonstring*)obj)->m_str);
	}
}
void jsonstring::Delete(i64 p_obj) {
	jsonstring* obj = (jsonstring*)g_jsonMem->Lock(p_obj);
	jsonstring::Free(obj);
	g_jsonMem->Unlock(p_obj);
	g_jsonMem->Free(p_obj);
}
void jsonnumber::Free(_jsonobj* obj) {
	//nothing to free
}
void jsonnumber::Delete(i64 p_obj) {
	jsonnumber* obj = (jsonnumber*)g_jsonMem->Lock(p_obj);
	jsonnumber::Free(obj);
	g_jsonMem->Unlock(p_obj);
	g_jsonMem->Free(p_obj);
}
void jsonboolean::Free(_jsonobj* obj) {
	//nothing to free
}
void jsonboolean::Delete(i64 p_obj) {
	jsonboolean* obj = (jsonboolean*)g_jsonMem->Lock(p_obj);
	jsonboolean::Free(obj);
	g_jsonMem->Unlock(p_obj);
	g_jsonMem->Free(p_obj);
}

void jsonnull::Free(_jsonobj* obj) {
	//nothing to free
}
void jsonnull::Delete(i64 p_obj) {
	jsonnull* obj = (jsonnull*)g_jsonMem->Lock(p_obj);
	jsonnull::Free(obj);
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
	jsonobj::Type,
	jsonobj::Create,
	jsonobj::Delete,
	jsonobj::Free,
	jsonobj::Load,
};
jsonobj_functable jsonarray_ftable = {
	jsonarray::Type,
	jsonarray::Create,
	jsonarray::Delete,
	jsonarray::Free,
	jsonarray::Load
};
jsonobj_functable jsonstring_ftable = {
	jsonstring::Type,
	jsonstring::Create,
	jsonstring::Delete,
	jsonstring::Free,
	jsonstring::Load
};
jsonobj_functable jsonnumber_ftable = {
	jsonnumber::Type,
	jsonnumber::Create,
	jsonnumber::Delete,
	jsonnumber::Free,
	jsonnumber::Load
};
jsonobj_functable jsonboolean_ftable = {
	jsonboolean::Type,
	jsonboolean::Create,
	jsonboolean::Delete,
	jsonboolean::Free,
	jsonboolean::Load
};
jsonobj_functable jsonnull_ftable = {
	jsonnull::Type,
	jsonnull::Create,
	jsonnull::Delete,
	jsonnull::Free,
	jsonnull::Load
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
	if(!m_tablesize) return 0;
	unsigned long k = calculateHashKey(p_key);
	i64 *m_table = (i64*)g_jsonMem->Lock(m_tableLoc,true);
	i64 itr = m_table[k];
	while (itr) {
		jsonkeypair* itrPtr = (jsonkeypair*)g_jsonMem->Lock(itr,true);
		char *keyPtr = (char*)g_jsonMem->Lock(itrPtr->key,true);
		if (!strcmp(p_key, keyPtr)) {
			g_jsonMem->Unlock(itrPtr->key);
			g_jsonMem->Unlock(itr);
			g_jsonMem->Unlock(m_tableLoc);
			return itrPtr->val;
		}
		i64 next = itrPtr->next;
		g_jsonMem->Unlock(itrPtr->key);
		g_jsonMem->Unlock(itr);
		itr = next;
	}
	g_jsonMem->Unlock(m_tableLoc);
	return 0; //nothing found
}
long jsonobj::Delete(const char *p_key) {
	unsigned long k = calculateHashKey(p_key);
	i64 *m_table = (i64*)g_jsonMem->Lock(m_tableLoc,true); 
	i64 itr = m_table[k]; g_jsonMem->Unlock(m_tableLoc);
	i64 prev = 0;
	while (itr) {
		jsonkeypair* itrPtr = (jsonkeypair*)g_jsonMem->Lock(itr,true);
		char *keyPtr = (char*)g_jsonMem->Lock(itrPtr->key,true);
		if (!strcmp(p_key, keyPtr)) {
			g_jsonMem->Unlock(itrPtr->key); //done with key
			itrPtr->Free(); 
			i64 next = itrPtr->next; 
			g_jsonMem->Unlock(itr);
			if(!prev) {m_table = (i64*)g_jsonMem->Lock(m_tableLoc); m_table[k]=next; g_jsonMem->Unlock(m_tableLoc);}
			else{ itrPtr = (jsonkeypair*)g_jsonMem->Lock(prev); itrPtr->next = next; g_jsonMem->Unlock(prev); } //de link itr from chain
			g_jsonMem->Free(itr);
			m_keycount--;
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
	i64 *m_table = (i64*)g_jsonMem->Lock(m_tableLoc,true); 
	i64 itr = m_table[k]; g_jsonMem->Unlock(m_tableLoc);
	while (itr) {
		jsonkeypair* itrPtr = (jsonkeypair*)g_jsonMem->Lock(itr);
		char *keyPtr = (char*)g_jsonMem->Lock(itrPtr->key,true); //not replacing key so read only
		if (!strcmp(p_key, keyPtr)) {
			g_jsonMem->Unlock(itrPtr->key); //done with key
			if(itrPtr->val){
				_jsonobj* valPtr = (_jsonobj*)g_jsonMem->Lock(itrPtr->val);
				unsigned long t =valPtr->m_ftable;
				g_jsonMem->Unlock(itrPtr->val);
				jsonobj_ftables[t]->Delete(itrPtr->val);
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

void jsonarray::DeleteIdx(unsigned long index) {
	i64 *data = (i64*)g_jsonMem->Lock(m_dataLoc);
	if(data[index]){
		_jsonobj* objPtr = (_jsonobj*)g_jsonMem->Lock(data[index]);
		i64 t = objPtr->m_ftable;
		g_jsonMem->Unlock(data[index]);
		jsonobj_ftables[t]->Delete(data[index]);
		data[index] =0; //nullify index
	}
	g_jsonMem->Unlock(m_dataLoc);
}
void jsonarray::ReplaceIdx(unsigned long index,i64 p_newval) {
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
		err = jsonobj::Create(p_val);
		if (err < 0) { return err; }

		err = jsonobj::Load(*p_val,buf, ch);
		if (err < 0) { jsonobj::Delete(*p_val); *p_val = 0; return err; }

		return '}'; //ch
	}
	else if (ch == '[') {
		err = jsonarray::Create(p_val);
		if (err < 0) { return err; }

		err = jsonarray::Load(*p_val,buf, ch);
		if (err < 0) { if(*p_val) jsonarray::Delete(*p_val); *p_val = 0; return err; }

		return ']';//ch
	}
	else if (ch == '\'' || ch == '\"') {
		err = jsonstring::Create(p_val);
		if (err < 0) { if(*p_val) return err; }

		err = jsonstring::Load(*p_val,buf, ch);
		if (err < 0) { jsonstring::Delete(*p_val); *p_val = 0; return err; }

		return err; 
	}
	else if ((ch >= '0'&& ch <= '9')||ch=='-') {
		err = jsonnumber::Create(p_val);
		if (err < 0) { if(*p_val) return err; }

		err = jsonnumber::Load(*p_val,buf, ch);
		if (err < 0) { jsonnumber::Delete(*p_val); *p_val = 0; return err; }

		return err; 
	}
	else if( (ch=='t') || (ch=='f') ) {
		err = jsonboolean::Create(p_val);
		if (err < 0) { if(*p_val) return err; }

		err = jsonboolean::Load(*p_val,buf, ch);
		if (err < 0) { jsonboolean::Delete(*p_val); *p_val = 0; return err; }

		return err; 
	}
	else if( (ch=='n') ) {
		err = jsonnull::Create(p_val);
		if (err < 0) { if(*p_val) return err; }

		err = jsonnull::Load(*p_val,buf, ch);
		if (err < 0) { jsonnull::Delete(*p_val); *p_val = 0; return err; }

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
