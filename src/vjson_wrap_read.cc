#include "vjson_wrap.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>

using namespace std;

char g_read_obj_emptyString[] = "";

napi_value read_obj(napi_env env, i64 objLoc, long depth=1){
    napi_value js_obj;
    _jsonobj* objPtr = (_jsonobj*)g_jsonMem->Lock(objLoc,true);
    long type = jsonobj_ftables[objPtr->m_ftable]->Type();
    switch(type){
        case JSON_ARRAY:
            napi_create_array(env, &js_obj); //assume no errors
            if(((jsonarray*)objPtr)->m_dataLoc && depth>0){
                i64*data = (i64*)g_jsonMem->Lock(((jsonarray*)objPtr)->m_dataLoc,true);
                for(unsigned i=0;i<((jsonarray*)objPtr)->m_size;i++){
                    napi_set_element(env, js_obj,i,read_obj(env,data[i],depth-1));
                }
                g_jsonMem->Unlock(((jsonarray*)objPtr)->m_dataLoc);
            }
            break;
        case JSON_OBJ:
            napi_create_object(env,&js_obj);
            if(((jsonobj*)objPtr)->m_tableLoc && depth>0){
                i64 *table = (i64*)g_jsonMem->Lock(((jsonobj*)objPtr)->m_tableLoc,true);
                unsigned long i;
                for (i = 0; i < ((jsonobj*)objPtr)->m_tablesize; i++) {
                    i64 itr = table[i];
                    while (itr) {
                        jsonkeypair* itrPtr = (jsonkeypair*)g_jsonMem->Lock(itr,true);
                        i64 next = itrPtr->next;
                        char *keyPtr = (char*)g_jsonMem->Lock(itrPtr->key,true);
                        napi_set_named_property(env,js_obj, keyPtr, read_obj(env,itrPtr->val,depth-1));                        
                        g_jsonMem->Unlock(itrPtr->key);
                        g_jsonMem->Unlock(itr);
                        itr = next;
                    }
                }
                g_jsonMem->Unlock(((jsonobj*)objPtr)->m_tableLoc);
            }
            break;
        case JSON_NULL:
        default:
            napi_get_null(env,&js_obj);
            break;
        case JSON_UNDEFINED:
            napi_get_undefined(env,&js_obj);
            break;
        case JSON_TEXT:{
            const char *strPtr = ((jsonstring*)objPtr)->m_str ? ((char*)g_jsonMem->Lock(((jsonstring*)objPtr)->m_str,true)) : g_read_obj_emptyString;
            napi_create_string_utf8(env, strPtr, NAPI_AUTO_LENGTH, &js_obj);
            if( ((jsonstring*)objPtr)->m_str ) g_jsonMem->Unlock( ((jsonstring*)objPtr)->m_str );
            break;
        }
        case JSON_NUMBER:
            napi_create_double(env, ((jsonnumber*)objPtr)->num, &js_obj);
            break;
        case JSON_BOOLEAN:
            napi_get_boolean(env, ((jsonboolean*)objPtr)->b, &js_obj);
            break;
    }

    g_jsonMem->Unlock(objLoc);
    return js_obj;
}

napi_value vjson_wrap::read_obj(napi_env env, napi_callback_info info){
    //params: vjsonMM, keypath of object
    napi_value js_obj;
    napi_value argv[3];
    JsonMM *mem;
    napi_valuetype expectedTypes[3] = { napi_object,napi_string,napi_number };
    napi_status err = helper_checkparams(env, info, 3, argv, expectedTypes);
    if (err != napi_ok) {
        napi_get_null(env, &js_obj);
        return js_obj;
    }
    CHECK(napi_unwrap(env, argv[0], (void**)&mem)==napi_ok)
    g_jsonMem = mem; //set global memory manager for jsonobj_Create
    char objPath[2048]; size_t objPathSize;
    CHECK(napi_get_value_string_utf8(env, argv[1], objPath, 2048 - 1, &objPathSize) == napi_ok);
    if(!objPathSize || objPathSize==2048-1/*maxed out*/){
        napi_throw_error(env, "-3", "invalid object path");
        napi_get_null(env, &js_obj); return js_obj;
    }
    int64_t depth;
    CHECK(napi_get_value_int64(env, argv[2], (int64_t*)&depth)==napi_ok);
    if(depth<1||depth>0x7FFFFFFF){
        napi_throw_error(env, "-5", "invalid depth");
        napi_get_null(env, &js_obj); return js_obj;
    }
    i64 objLoc = GetObjectFromKeyPath(mem, objPath,objPathSize);
    if(objLoc<0){
        napi_throw_error(env, "-4", "object not found");
        napi_get_null(env, &js_obj); return js_obj;
    }

    js_obj = ::read_obj(env, objLoc,(long)depth);
    return js_obj; //send object read
}

napi_value find_obj(napi_env env, i64 collectionLoc, char *query, long queryLen, long depth=1){
    unsigned long i;
    napi_value js_obj;
    _jsonobj* objPtr = (_jsonobj*)g_jsonMem->Lock(collectionLoc,true);
    long type = jsonobj_ftables[objPtr->m_ftable]->Type();
    _keypair retPairs[2048];
    unsigned long numPairs = 2048;
    char childKey[64];
    i64 itr = 0;
    switch(type){
        case JSON_ARRAY:
            napi_create_object(env,&js_obj); //use object to store matched items with index as key
            while(numPairs==2048){
                itr = ((jsonarray*)objPtr)->Query(query,itr,retPairs,&numPairs/*=2048*/);
                if(itr<0){
                    //error in query evaluation
                    napi_throw_error(env, "-6", "error in query evaluation");
                    g_jsonMem->Unlock(collectionLoc);
                    napi_get_null(env,&js_obj);
                    return js_obj;
                }
                for(i=0;i<numPairs;i++){
                    sprintf(childKey, "%lu", (unsigned long)retPairs[i].key);
                    napi_set_named_property(env, js_obj, childKey, read_obj(env,retPairs[i].val,depth-1));
                }
            }
            break;
        case JSON_OBJ:
            napi_create_object(env,&js_obj);
            while(numPairs==2048){
                itr = ((jsonobj*)objPtr)->Query(query,itr,retPairs,&numPairs/*=2048*/);
                if(itr<0){
                    //error in query evaluation
                    napi_throw_error(env, "-6", "error in query evaluation");
                    g_jsonMem->Unlock(collectionLoc);
                    napi_get_null(env,&js_obj);
                    return js_obj;
                }
                for(i=0;i<numPairs;i++){
                    char *keyPtr = (char*)g_jsonMem->Lock(retPairs[i].key,true);
                    napi_set_named_property(env, js_obj, keyPtr, read_obj(env,retPairs[i].val,depth-1));
                    g_jsonMem->Unlock(retPairs[i].key);
                }
            }
            break;
        default:
            napi_throw_error(env, "-16", "invalid collection type, must be object or array");
            napi_get_null(env,&js_obj);
            break;
    }

    g_jsonMem->Unlock(collectionLoc);
    return js_obj;
}
napi_value vjson_wrap::find_obj(napi_env env, napi_callback_info info){
    //params: vjsonMM, keypath of collection, query string, depth (children levels to read)
    napi_value js_obj;
    napi_value argv[4];
    JsonMM *mem;
    napi_valuetype expectedTypes[4] = { napi_object,napi_string,napi_string,napi_number };
    napi_status err = helper_checkparams(env, info, 4, argv, expectedTypes);
    if (err != napi_ok) {
        napi_get_null(env, &js_obj);
        return js_obj;
    }
    CHECK(napi_unwrap(env, argv[0], (void**)&mem)==napi_ok)
    g_jsonMem = mem; //set global memory manager for jsonobj_Create
    char objPath[2048]; size_t objPathSize;
    CHECK(napi_get_value_string_utf8(env, argv[1], objPath, 2048 - 1, &objPathSize) == napi_ok);
    if(!objPathSize || objPathSize>=2048-1/*maxed out*/){
        napi_throw_error(env, "-3", "invalid object path");
        napi_get_null(env, &js_obj); return js_obj;
    }
    char query[4096]; size_t querySize;
    CHECK(napi_get_value_string_utf8(env, argv[2], query, 4096 - 1, &querySize) == napi_ok);
    if(!querySize || querySize>=4096-1/*maxed out*/){
        napi_throw_error(env, "-13", "invalid query string");
        napi_get_null(env, &js_obj); return js_obj;
    }
    int64_t depth;
    CHECK(napi_get_value_int64(env, argv[3], (int64_t*)&depth)==napi_ok);
    if(depth<1||depth>0x7FFFFFFF){
        napi_throw_error(env, "-5", "invalid depth");
        napi_get_null(env, &js_obj); return js_obj;
    }
    i64 objLoc = GetObjectFromKeyPath(mem, objPath,objPathSize);
    if(objLoc<0){
        napi_throw_error(env, "-4", "object not found");
        napi_get_null(env, &js_obj); return js_obj;
    }

    js_obj = ::find_obj(env, objLoc,query, querySize,(long)depth);
    return js_obj; //send object read
}
