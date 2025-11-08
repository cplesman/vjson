#define _CRT_SECURE_NO_WARNINGS

#include "json.h"
#include <string.h>
#include <stdio.h>

const char g_spacebytes[] = {
' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
0
};
int JSON_sendTab(stream *buf, int tab) {
	int t = tab;
	if (tab > 25) {
		t = 25;
	}
	return buf->PutBytes(g_spacebytes, t);
}

int JSON_sendtext(stream *buf, char *text) {
	int i;
	int len = (int)strlen(text);
	char miniBuf[32];
	miniBuf[0] = '\"';
	int bufSize = 1;
	for (i = 0; i < len; i++) {
		if (text[i] == '\\' || text[i] == '\n' || text[i] == '\r' || text[i] == '\"' || text[i] == '\'' ) {
			miniBuf[bufSize++] = '\\'; miniBuf[bufSize++] = text[i];
		}
		else {
			miniBuf[bufSize++] = text[i];
		}
		if (bufSize > 30) { //then flush miniBuf
			int iResult = buf->PutBytes(miniBuf, bufSize);
			bufSize = 0; 
			if (iResult < 0) return iResult;
		}
	}
	miniBuf[bufSize++] = '\"';
	return buf->PutBytes( miniBuf, bufSize); //send rest of it
}

long JSON_sendchildren(stream *buf, i64 *children, unsigned long num_children, int pretty);
long JSON_send(stream *buf, _jsonobj* p_obj, int pretty);
long JSON_send(stream *buf, i64 p_obj, int pretty){
	_jsonobj* obj = (_jsonobj*)g_jsonMem->Lock(p_obj);
	long iResult = JSON_send(buf, obj, pretty);
	g_jsonMem->Unlock(p_obj);
	return iResult;
}
long JSON_sendpair(stream *buf, jsonkeypair *pair, int pretty) {
	int cpretty = pretty ? pretty + 2 : 0;
	int iResult;
	if (pair->key) { //null if array item
		iResult = JSON_sendTab(buf, pretty);
		if (iResult < 0) return iResult;
		char *keyPtr = (char*)g_jsonMem->Lock(pair->key);
		iResult = JSON_sendtext(buf, keyPtr);
		g_jsonMem->Unlock(pair->key);
		if (iResult < 0) return iResult;
		iResult = buf->PutBytes(":", 1);
		if (iResult < 0) return iResult;
	}
	return JSON_send(buf, pair->val, cpretty);
}
long JSON_send(stream *buf, _jsonobj *p_obj, int pretty) {
	int cpretty = pretty ? pretty + 2 : 0;
	int iResult;
	char tmp[32];
	char *txtPtr;
	switch (jsonobj_ftables[p_obj->m_ftable]->Type()) {
	case JSON_NUMBER:
		iResult = sprintf(tmp, "%f", ((jsonnumber*)p_obj)->num);
		return buf->PutBytes(tmp, iResult);
	case JSON_BOOLEAN:
		return buf->PutBytes( ((jsonboolean*)p_obj)->b ? "true" : "false", ((jsonboolean*)p_obj)->b ? 4 : 5);
	case JSON_TEXT:
		txtPtr = (char*)g_jsonMem->Lock(((jsonstring*)p_obj)->m_str);
		iResult = JSON_sendtext(buf, txtPtr);
		g_jsonMem->Unlock(((jsonstring*)p_obj)->m_str);
		return iResult;
	case JSON_OBJ:{
		jsonobj* obj = (jsonobj*)p_obj;
		unsigned long keycount = obj->NumKeys();
		iResult = buf->PutBytes( "{", 1);
		if (pretty && keycount && iResult > 0)
			iResult = buf->PutBytes("\r\n", 2);
		if (iResult < 0) return iResult;
		if (keycount) {
			unsigned long total = keycount,count=0;
			unsigned long tsize = obj->m_tablesize;
			i64 *table = (i64*)g_jsonMem->Lock(obj->m_tableLoc);
			for (unsigned long i = 0; i < tsize; i++) {
				i64 itr = table[i];
				while(itr){
					jsonkeypair* itrPtr = (jsonkeypair*)g_jsonMem->Lock(itr);
					iResult = JSON_sendpair(buf,itrPtr, cpretty);
					i64 next = itrPtr->next;
					g_jsonMem->Unlock(itr);
					if(iResult<0) break;
					count++;
					if (count < total) {
						iResult = buf->PutBytes(",", 1);
					}
					if (pretty)
						iResult = buf->PutBytes("\r\n", 2);
					itr = next;
				}
				if(iResult<0) break;
			}
			g_jsonMem->Unlock(obj->m_tableLoc);
			if (iResult < 0) return iResult;
			if (total&&pretty) {
				JSON_sendTab(buf, pretty);
			}
		}
		iResult = buf->PutBytes( "}", 1);
		return iResult;
	}
	case JSON_ARRAY:{
		iResult = buf->PutBytes( "[", 1);
		if (pretty && iResult > 0)
			iResult = buf->PutBytes("\r\n", 2);
		if (iResult < 0) return iResult;
		i64 *data = (i64*)g_jsonMem->Lock(((jsonarray*)p_obj)->m_dataLoc);
		iResult = JSON_sendchildren(buf, data, ((jsonarray*)p_obj)->m_size, cpretty);
		g_jsonMem->Unlock(((jsonarray*)p_obj)->m_dataLoc);
		if (iResult < 0) return iResult;
		if (pretty && iResult >= 0)
			iResult = buf->PutBytes("\r\n", 2);
		if (iResult>0&&pretty) {
			JSON_sendTab(buf, pretty);
		}
		iResult = buf->PutBytes("]", 1);
		return iResult;
	}
	default:
		return buf->PutBytes( "{}",2);
	}
}

long JSON_sendchildren(stream *buf, i64 *children, unsigned long num_children, int pretty) {
	if (!children) {
		return 0; //no children
	}
	int iResult =0;
	for(unsigned long i=0;i<num_children;i++) {
		if(pretty) 
			JSON_sendTab(buf, pretty);
		iResult = JSON_send(buf, children[i], pretty);
		if (iResult < 0) return iResult;

		if (i+1<num_children) {
			//not last one
			iResult = buf->PutBytes( ",", 1);
			if (pretty )
				iResult = buf->PutBytes("\r\n", 2);
			if (iResult < 0) return iResult;
		}
	}
	return num_children;
}

