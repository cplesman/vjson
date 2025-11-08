#include "json.h"
#include <stdlib.h>

using namespace std;

long jsonobj_Load(i64 p_obj, stream* buf, char lastCh) {
	int err;
	char ch;

	//move to significant character
	err = JSON_movepastwhite(buf);
	if (err < 0) { return err; }
	ch = (char)err;
	if ((ch == '}' && lastCh == '{') || (!ch && !lastCh)) {
		return ch; //this is an empty object
	}

	jsonobj* obj = (jsonobj*)g_jsonMem->Lock(p_obj);
	i64 keypair = 0;
	i64* next = &keypair;
	unsigned long count = 0;

	//at this point the stream should have moved past '{'
	do {
		jsonkeypair* nextPtr;
		if (ch == '\'' || ch == '\"') {
			err = JSON_parseString_iterateQuote_getLength(ch, buf);
			if (err < 0) { break; }
			(*next) = g_jsonMem->Alloc(sizeof(jsonkeypair));
			count++;
			if (!(*next)) { err = JSON_ERROR_OUTOFMEMORY; break; }
			nextPtr = (jsonkeypair*)g_jsonMem->Lock(*next); //stay locked but unlock before returning from Load
			nextPtr->Init();
			nextPtr->key = g_jsonMem->Alloc(sizeof(char) * /*size with null*/err);
			if (!nextPtr->key) { err = JSON_ERROR_OUTOFMEMORY; break; }
			char *keyPtr = (char*)g_jsonMem->Lock(nextPtr->key);
			err = JSON_parseString_iterateQuote(keyPtr, ch, buf);
			g_jsonMem->Unlock(nextPtr->key);
			if (err < 0) { break; }
			//ch stays as quote
		}
		else {
			//expected key for the key pair
			err = JSON_ERROR_INVALIDDATA;
			break;
		}

		err = JSON_movepastwhite(buf);
		if (err < 0) { break; }
		ch = (char)err;

		if (ch != ':') {
			err = JSON_ERROR_INVALIDDATA; break;
		}

		//move to next char after ':'
		err = JSON_movepastwhite(buf);
		if (err < 0) { break; }
		ch = (char)err;

		err = JSON_parseVal(&nextPtr->val, ch, buf);
		if (err < 0) { break; }
		ch = (char)err;

		//move to next char after ch
		err = JSON_movepastwhite(buf);
		if (err < 0) { break; }
		ch = (char)err;

		if (ch == '}') break; //reached end of object
		if (ch != ',') { err = JSON_ERROR_INVALIDDATA; break; }

		//move to next char after ','
		err = JSON_movepastwhite(buf);
		if (err < 0) { break; }
		ch = (char)err;

		//keep nextPtr locked so we can set its next pointer
		next = &nextPtr->next;

	} while (ch != '}'); //could be ',' then '}' which will be ok

	if(err>=0) {
		err = obj->ResizeTable(count);
	}

	if (err < 0) {
		while(keypair) {
			jsonkeypair* keypairPtr = (jsonkeypair*)g_jsonMem->Lock(keypair);
			i64 next = keypairPtr->next;
			keypairPtr->Free();
			g_jsonMem->Unlock(keypair);
			g_jsonMem->Unlock(keypair); //unlock 2 times because we Locked another time when creating key
			g_jsonMem->Free(keypair);
			keypair = next;
		}
		g_jsonMem->Unlock(p_obj);
		return err;
	}
	i64 itr = keypair;// firstpair
	i64 *table = (i64*)g_jsonMem->Lock(obj->m_tableLoc);
	while (itr) {
		jsonkeypair* itrPtr = (jsonkeypair*)g_jsonMem->Lock(itr);
		unsigned long key = obj->calculateHashKey((char*)g_jsonMem->Lock(itrPtr->key));
		g_jsonMem->Unlock(itrPtr->key);
		i64 next = itrPtr->next; //save next because we are changing itr->next
		itrPtr->next = table[key];
		table[key] = itr;
		g_jsonMem->Unlock(itr);
		g_jsonMem->Unlock(itr); //unlock 2 times because we Locked another time when creating key
		itr = next;
	}

	g_jsonMem->Unlock(obj->m_tableLoc);
	g_jsonMem->Unlock(p_obj);
	return ch;
}

long jsonobj_Load(i64 p_obj, stream* buf) {
	//move to '{'
	int err = JSON_movepastwhite(buf);
	if (err < 0) { return err; }
	char ch = (char)err;

	if (ch != '{') {
		return JSON_ERROR_INVALIDDATA; //syntax error
	}

	return jsonobj_Load(p_obj, buf, '{');
}

long jsonarray_Load(i64 p_obj, stream* buf) {
	//move to '{'
	int err = JSON_movepastwhite(buf);
	if (err < 0) { return err; }
	char ch = (char)err;

	if (ch != '[') {
		return JSON_ERROR_INVALIDDATA; //syntax error
	}

	return jsonarray_Load(p_obj, buf, '[');
}

class jsonarrayitemlink {
public:
	jsonarrayitemlink() { val = 0; next = 0; }
	i64 val;
	jsonarrayitemlink* next;
};
long jsonarray_Load(i64 p_obj, stream* buf, char lastCh) {
	int err;
	char ch;
	jsonarray* obj = (jsonarray*)g_jsonMem->Lock(p_obj);
	jsonarrayitemlink* list = 0;
	jsonarrayitemlink** next = &list;
	unsigned long count = 0;

	//at this point the stream should have moved past '['
	do {
		//move to next char after ',' or '['
		err = JSON_movepastwhite(buf);
		if (err < 0) { break; }
		ch = (char)err;

		if (ch == ']') break; //all done

		*next = new jsonarrayitemlink();
		if (!(*next)) { err = JSON_ERROR_OUTOFMEMORY; break; }
		count++;
		err = JSON_parseVal(&(*next)->val, ch, buf); //on error parseVal will delete val
		if (err < 0) { break; }
		ch = (char)err;
		next = &(*next)->next;

		err = JSON_movepastwhite(buf);
		if (err < 0) { break; }
		ch = (char)err;

		if (ch == ']') break; //all done
		if (ch != ',') {
			err = JSON_ERROR_INVALIDDATA; break;
		}
	} while (1);

	if (err >= 0 && count) {
		err = obj->Resize(count);
		if(err>=0){
			jsonarrayitemlink* itr = list;
			i64 *dataPtr = (i64*)g_jsonMem->Lock(obj->m_dataLoc);
			count = 0;
			while (itr) {
				dataPtr[count] = itr->val;
				count++;
				itr = itr->next;
			}
			g_jsonMem->Unlock(obj->m_dataLoc);
			obj->m_size = count;
		}
	}

	while(list){
		jsonarrayitemlink* next = list->next;
		delete list;
		list = next;
	}

	g_jsonMem->Unlock(p_obj);
	return err;
}

long jsonstring_Load(i64 p_obj, stream* buf, char lastCh) {
	int err = JSON_parseString_iterateQuote_getLength(lastCh, buf);
	if (err < 0) { return err; }
	jsonstring* obj = (jsonstring*)g_jsonMem->Lock(p_obj);
	if(obj->m_str) g_jsonMem->Free(obj->m_str);
	obj->m_str = g_jsonMem->Alloc(sizeof(char) * /*size with null*/err);
	if (!obj->m_str) { g_jsonMem->Unlock(p_obj); return err; }
	char *strPtr = (char*)g_jsonMem->Lock(obj->m_str);
	err = JSON_parseString_iterateQuote(strPtr, lastCh, buf);
	g_jsonMem->Unlock(obj->m_str);
	g_jsonMem->Unlock(p_obj); 
	if (err < 0) { return err; }
	return lastCh;
}

long jsonnumber_Load(i64 p_obj, stream* buf, char lastCh) {
	char numstr[32];
	numstr[0] = lastCh;
	int err = JSON_parseString_iterateNumber(numstr + 1, buf);
	if (err < 0) return err;
	jsonnumber* obj = (jsonnumber*)g_jsonMem->Lock(p_obj);
	obj->num = atof(numstr);
	g_jsonMem->Unlock(p_obj);
	return numstr[err]; //return last number as ch
}

long jsonboolean_Load(i64 p_obj, stream* buf, char lastCh) {
	const char* boolstr = (lastCh == 't') ? "true" : "false";
	int i = 1;
	while(i< (boolstr[0]=='t'?4:5) ) {
		char ch;
		int err = buf->GetBytes(&ch, 1);
		if (err < 0) return err;
		if (ch != boolstr[i]) {
			return JSON_ERROR_INVALIDDATA;
		}
		i++;
	}
	jsonboolean* obj = (jsonboolean*)g_jsonMem->Lock(p_obj);
	obj->b = boolstr[0] == 't' ? true : false;
	g_jsonMem->Unlock(p_obj);
	return 'e'; //return last char read (always 'e' in true/false)
}
