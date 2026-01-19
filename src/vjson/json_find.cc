#include "json.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>

using namespace std;

char g_find_obj_emptyString[] = "";

bool find_obj_isOR(const char *query, long start, long queryLen){
    //check if query is an OR query
    if(query[start]=='|'&&(start+1)<queryLen&&query[start+1]=='|'){
        return true;
    }
    return false;
}
bool find_obj_isAND(const char *query, long start, long queryLen){
    //check if query is an AND query
    if(query[start]=='&'&&(start+1)<queryLen&&query[start+1]=='&'){
        return true;
    }
    return false;
}
bool find_obj_isWhitespace(const char *query, long pos, long queryLen){
    if(pos>=queryLen) return false;
    char c = query[pos];
    if(c==' '||c=='\t'||c=='\r'||c=='\n'){
        return true;
    }
    return false;
}
long find_obj_skipWhitespace(const char *query, long pos, long queryLen){
    while(pos<queryLen&&find_obj_isWhitespace(query,pos,queryLen)){
        pos++;
    }
    return pos;
}
// token = query[start:returned pos]
long find_obj_readToken(const char *query, long start, long queryLen, char *outToken){
    long pos = start;
    outToken[0] = 0;
    if(query[pos]=='"'||query[pos]=='\''){
        //quoted token
        char quoteChar = query[pos];
        pos++;
        start++; //skip starting quote
        while(pos<queryLen&&query[pos]!=quoteChar){
            if(query[pos]=='\\'&&(pos+1)<queryLen){
                //escape character
                pos++;
                switch(query[pos]){
                    case 'n':
                        outToken[pos - start] = '\n';
                        break;
                    case 'r':
                        outToken[pos - start] = '\r';
                        break;
                    case 't':
                        outToken[pos - start] = '\t';
                        break;
                    default:
                        outToken[pos - start] = query[pos];
                        break;
                }
            }
            else{
                outToken[pos - start] = query[pos];
            }
            pos++;
            if(pos-start>=255){
                return -3; //token too long
            }
        }
        if(pos>=queryLen||query[pos]!=quoteChar){
            return -4; //unterminated quote
        }
        outToken[pos - start] = 0; //null terminate
        pos++; //skip ending quote
        return pos;
    }
    while(pos<queryLen&&!find_obj_isWhitespace(query,pos,queryLen)&&query[pos]!=')'){
        if(pos-start>=255){
            return -3; //token too long
        }
        outToken[pos - start] = query[pos];
        pos++;
    }
    outToken[pos - start] = 0; //null terminate
    return pos;
}
const char *find_obj_operators[] = {
    "==",
    "!=",
    "<=",
    ">=",
    "<",
    ">",
};
const long find_obj_operatorCount = sizeof(find_obj_operators)/sizeof(find_obj_operators[0]);
long find_obj_getOperatorIndex(const char *op){
    for(long i=0;i<find_obj_operatorCount;i++){
        if(strcmp(find_obj_operators[i],op)==0){
            return i;
        }
    }
    return -1;
}
long find_obj_readSubQuery(char *query, long start, long queryLen, char *outKey, char *outOP, char *outVal){
    long pos = start;
    size_t outPos = 0;

    // if(query[pos]=='('){
        //     pos = find_obj_readSubQuery(obj,query,pos+1,queryLen,outKey,outOP,outVal);
        //     if(pos>=queryLen || query[pos]!=')' || pos<0){
        //         return pos<0 ? pos : -1; //error
        //     }
        //     pos++; //skip ')'
        //     continue;
        // }

        //get key token
        long startToken = pos;
        pos = find_obj_readToken(query,pos,queryLen,outKey);
        if(pos<0){ return pos;}
        if( (outKey[0]<'0'||outKey[0]>'9') && (outKey[0]<'a'||outKey[0]>'z') && (outKey[0]<'A'||outKey[0]>'Z') && outKey[0]!='_' && outKey[startToken]!='\'' && outKey[startToken]!='\"'){
            //token must start with letter or underscore or quote or number
            return -2; //invalid key
        }

        pos = find_obj_skipWhitespace(query,pos,queryLen);
        //get operator
        outOP[0] = query[pos];
        if(!(outOP[0]=='='||outOP[0]=='!'||outOP[0]=='<'||outOP[0]=='>')){
            return -5; //invalid operator
        }
        pos++;
        if(!find_obj_isWhitespace(query,pos,queryLen) && pos<queryLen){
            outOP[1] = query[pos];
            outOP[2]=0;
            pos++;
        }
        else{
            outOP[1]=0;
        }
        if(!find_obj_isWhitespace(query,pos,queryLen) || pos>=queryLen){
            return -5; //invalid operator
        }
        long opIndex = find_obj_getOperatorIndex(outOP);
        if(opIndex<0){
            return -5; //invalid operator
        }
        outOP[0] = '0'+ opIndex; //convert to single char for easier processing
        outOP[1]=0;

        pos = find_obj_skipWhitespace(query,pos,queryLen);

        //get value token
        startToken = pos;
        pos = find_obj_readToken(query,pos,queryLen,outVal);
        if(pos<0){ return pos;}
        if( (outVal[0]<'0'||outVal[0]>'9') && (outVal[0]<'a'||outVal[0]>'z') && (outVal[0]<'A'||outVal[0]>'Z') && outVal[0]!='_' && outVal[startToken]!='\'' && outVal[startToken]!='\"'){
            //token must start with letter or underscore or quote or number
            return -6; //invalid value
        }

    return pos;
}
bool find_obj_operateNumber(double left, char op, double right){
    switch(op){
        case '0': //==
            return (left == right);
        case '1': //!=
            return ( left != right);
        case '2': //<=
            return (left <= right);
        case '3': //>=
            return (left >= right);
        case '4': //<
            return (left < right);
        case '5': //>
            return (left > right);
        default:
            return false; //invalid operation
    }
}
bool find_obj_operateString(const char *left, char op, const char *right){
    switch(op){
        case '0': //==
            return (strcmp(left,right)==0);
        case '1': //!=
            return (strcmp(left,right)!=0);
        case '2': //<=
            return (strcmp(left,right)<=0);
        case '3': //>=
            return (strcmp(left,right)>=0);
        case '4': //<
            return (strcmp(left,right)<0);
        case '5': //>
            return (strcmp(left,right)>0);
        default:
            return false; //invalid operation for string
    }
}
bool find_obj_queryMatch(const char *parent_key/*used when comparing key when key=='__id'*/, _jsonobj* parent, char *key, char op, char *val){
    //caller is responsible for unlocking parent
    long type;
    _jsonobj *objPtr;
    if(key[0]=='_'&&key[1]=='_'&&key[2]=='i'&&key[3]=='d'&&key[4]==0){
        //special case for __id
        type = JSON_TEXT;
        return find_obj_operateString(parent_key,op,val);
    }
    else{
        type = jsonobj_ftables[parent->m_ftable]->Type();
        i64 keyLoc =0;
        if(type==JSON_OBJ){
            //find key in object
            keyLoc = ((jsonobj*)parent)->Find(key);
        }
        else if(type==JSON_ARRAY){
            //key must be an index
            long index = atoi(key);
            if(index<0|| (unsigned long)index>=((jsonarray*)parent)->m_size ){
                return false; //index out of range
            }
            keyLoc = ((jsonarray*)parent)->operator[](index&0xffffffff);
        }
        else {
            return false; //only object or array can be queried
        }
        if(!keyLoc){
            return false; //key not found
        }
        objPtr = (_jsonobj*)g_jsonMem->Lock(keyLoc,true);
        type = jsonobj_ftables[objPtr->m_ftable]->Type();
        bool result = false;
        if(type==JSON_TEXT){
            const char *strPtr = ((jsonstring*)objPtr)->m_str ? (char*)g_jsonMem->Lock(((jsonstring*)objPtr)->m_str,true) : g_find_obj_emptyString;
            result = find_obj_operateString(strPtr,op,val);
            if( ((jsonstring*)objPtr)->m_str ) g_jsonMem->Unlock( ((jsonstring*)objPtr)->m_str );
        }
        else if(type==JSON_NUMBER){
            double num = ((jsonnumber*)objPtr)->num;
            double numVal = atof(val);
            result = find_obj_operateNumber(num,op,numVal);
        }
        // else{
        //     //invalid type for comparison
        // }
        g_jsonMem->Unlock(keyLoc);
        return result;

    }
}
long find_obj_evaluateQuery(const char *parent_key, _jsonobj* parent, long start, char *query, long queryLen, bool *outResult);
long find_obj_evaluateOperand(const char *parent_key, _jsonobj* parent, long start, char *query, long queryLen, bool *outResult){
    long pos = start;
    char outKey[256];
    char outOP[4];
    char outVal[256];

    pos = find_obj_skipWhitespace(query,pos,queryLen);
    if(pos>=queryLen){
        //no more tokens
        return -10; //error empty operand
    }

    if(query[pos]=='('){
        //sub evaluate query
        pos++;
        pos = find_obj_evaluateQuery(parent_key,parent,pos, query,queryLen,outResult);
        if(pos<0) return pos; //error
        if(query[pos]!=')') return -13;//error missing ')'
        pos++; //skip ')'
    }
    else{
        pos = find_obj_readSubQuery(query,pos,queryLen,outKey,outOP,outVal);
        if(pos<0) return pos;
        *outResult = find_obj_queryMatch(parent_key,parent,outKey,outOP[0],outVal);
    }
    return pos;
}
long find_obj_evaluateQuery(const char *parent_key, _jsonobj* parent, long start, char *query, long queryLen, bool *outResult){
    //caller is responsible for unlocking parent
    long pos = start;
    bool result; // next operand of (OR, AND)

    //very first operand
    pos = find_obj_evaluateOperand(parent_key,parent,pos, query,queryLen,outResult);
    if(pos<0) {
        if(pos==-10){
            //empty query
            *outResult = true; //empty query matches all
            return start;
        }
        return pos;
    }

    while(pos<queryLen && query[pos]!=')'){
        pos = find_obj_skipWhitespace(query,pos,queryLen);
        if(pos>=queryLen){
            //no more tokens
            return pos;
        }

        //get operator
        if(find_obj_isOR(query,pos,queryLen)){
            //OR operator
            pos+=2;
            pos = find_obj_evaluateOperand(parent_key,parent,pos, query,queryLen,&result);
            if(pos<0) return pos;
            *outResult = *outResult || result;
        }
        else if(find_obj_isAND(query,pos,queryLen)){
            //AND operator
            pos+=2;
            pos = find_obj_evaluateOperand(parent_key,parent,pos, query,queryLen,&result);
            if(pos<0) return pos;
            *outResult = *outResult && result;
        }
        else{
            return -12; //invalid operator
        }
    }
    return pos;
}

i64 jsonobj::Query(const char *query, i64 p_startItr, _keypair *p_retPairs, unsigned long *p_numPairs){
    if(!m_tableLoc || !p_numPairs || *p_numPairs==0){
        return -1; //invalid parameters
    }
    unsigned long maxPairs = *p_numPairs;
    *p_numPairs = 0;
    i64 *table = (i64*)g_jsonMem->Lock(m_tableLoc,true);
    unsigned long i;
    long itrCount = 0;
    for (i = 0; i < m_tablesize; i++) {
        i64 itr = table[i];
        while (itr) {
            jsonkeypair* itrPtr = (jsonkeypair*)g_jsonMem->Lock(itr,true);
            i64 next = itrPtr->next;
            if(itrCount>=p_startItr){
                char *keyPtr = (char*)g_jsonMem->Lock(itrPtr->key,true);
                _jsonobj* childPtr = (_jsonobj*)g_jsonMem->Lock(itrPtr->val,true);
                bool match = false;
                long evalPos = find_obj_evaluateQuery(keyPtr,childPtr,0,(char*)query,strlen(query),&match);
                g_jsonMem->Unlock(itrPtr->val);
                g_jsonMem->Unlock(itrPtr->key);
                if(evalPos<0){
                    //error in query evaluation
                    g_jsonMem->Unlock(itr);
                    g_jsonMem->Unlock(m_tableLoc);
                    return evalPos; //propagate error
                }
                if(match ){
                    //store result
                    if(p_retPairs){
                        p_retPairs[*p_numPairs].key = itrPtr->key;
                        p_retPairs[*p_numPairs].val = itrPtr->val;
                    }
                    (*p_numPairs)++;
                    if(*p_numPairs>=maxPairs){
                        //reached max pairs
                        g_jsonMem->Unlock(itr);
                        g_jsonMem->Unlock(m_tableLoc);
                        return itrCount+1; //success return number of items iterated
                    }
                }
            }
            g_jsonMem->Unlock(itr);
            itr = next;
            itrCount++;
        }
    }
    g_jsonMem->Unlock(m_tableLoc);
    return itrCount; //success return number of items iterated
}
i64 jsonarray::Query(const char *query, i64 p_startItr, _keypair *p_retPairs, unsigned long *p_numPairs){
    if(!m_dataLoc || !p_numPairs || *p_numPairs==0){
        return -1; //invalid parameters
    }
    unsigned long maxPairs = *p_numPairs;
    *p_numPairs = 0;
    i64 *table = (i64*)g_jsonMem->Lock(m_dataLoc,true);
    unsigned long i;
    char keyBuf[64];
    for (i = (unsigned long)(p_startItr&0xffffffff); i < m_size; i++) {
        i64 itr = table[i];
        _jsonobj* childPtr = (_jsonobj*)g_jsonMem->Lock(itr,true);
        bool match = false;
        sprintf(keyBuf, "%lu", i);
        long evalPos = find_obj_evaluateQuery(keyBuf,childPtr,0,(char*)query,strlen(query),&match);
        g_jsonMem->Unlock(itr);
        if(evalPos<0){
            //error in query evaluation
            g_jsonMem->Unlock(m_dataLoc);
            return evalPos; //propagate error
        }
        if(match){
            //store result
            if(p_retPairs){
                p_retPairs[*p_numPairs].key = i; //index as key
                p_retPairs[*p_numPairs].val = itr;
            }
            (*p_numPairs)++;
            if(*p_numPairs>=maxPairs){
                //reached max pairs
                g_jsonMem->Unlock(m_dataLoc);
                return i+1; //success return number of items iterated
            }
        }
    }
    g_jsonMem->Unlock(m_dataLoc);
    return m_size; //success return number of items iterated
}
