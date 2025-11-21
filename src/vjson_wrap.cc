#include "vjson_wrap.h"

#include <time.h>

napi_status helper_checkparams(napi_env env, napi_callback_info info, size_t argc/*expected*/, napi_value *argv, napi_valuetype *types){
    size_t expected = argc;
    //argc will return more even if expected is less
    CHECK(napi_get_cb_info(env,info, &argc, argv, NULL,NULL/*((void**)&addon_data)*/) == napi_ok);
    if(argc!=expected){
        napi_throw_error(env, "-1", "invalid number of parameters");
        goto helper_checkparams_error;
    }
    for(size_t i=0;i<expected;i++){
        napi_valuetype argType;
        napi_typeof(env, argv[i], &argType);
        if( argType!=types[i] ){
            napi_throw_type_error(env, "-2", "invalid argument type");
            goto helper_checkparams_error;
        }
    }
    return napi_ok;
    helper_checkparams_error:
        return napi_status::napi_invalid_arg;
}

#define helper_check_and_extractObject()  \
    napi_value argv[3]; JsonMM *mem; i64 *parent; napi_value js_obj; \
    napi_valuetype expectedTypes[3] = { napi_object,napi_object,napi_string }; \
    napi_status err = helper_checkparams(env, info, 3, argv, expectedTypes); \
    if (err != napi_ok) { \
        napi_get_null(env, &js_obj); \
        return js_obj; \
    } \
    CHECK(napi_unwrap(env, argv[0], (void**)&mem)==napi_ok) \
    CHECK(napi_unwrap(env, argv[1], (void**)&parent)==napi_ok) 


napi_value vjson_wrap::init(napi_env env, napi_callback_info info){ //allocate and wrap oject
    size_t argc = 1; napi_value argv[1]; JsonMM *vjsonMM; napi_value js_obj;
    napi_valuetype expectedTypes[1] = {napi_string};
    napi_status err = helper_checkparams(env,info,argc, argv, expectedTypes);
    if(err!=napi_ok){
        napi_get_null(env, &js_obj);
        return js_obj;
    }
    char str[512]; size_t strSize;
    CHECK(napi_get_value_string_utf8(env, argv[0],str,512-1,&strSize)==napi_ok);
    g_jsonMem = vjsonMM = new JsonMM(str);
    long e = vjsonMM->Init();
    if(e>=0 && vjsonMM->m_root<=0){
        i64 root;
        e = jsonobj::Create(&root);
        if(e>=0){
            vjsonMM->m_root = root;
        }
    }
    if(e<0) {
        delete vjsonMM;
        napi_throw_error(env, "-10", "failed to initialize vjson");
        napi_get_null(env,&js_obj);
        return js_obj;
    }

    CHECK(napi_create_object(env, &js_obj)==napi_ok);
    CHECK(napi_wrap(env,js_obj,vjsonMM,vjson_wrap::freeMem,NULL, NULL) == napi_ok);
    return js_obj;
}
void vjson_wrap::freeMem(napi_env env, void* data, void* hint){         //called when object is garbage collected
    JsonMM* vjsonMM = (JsonMM*)data;
    vjsonMM->Close();
    g_jsonMem = 0;
    delete ((JsonMM*)data);
}

// napi_value vjson_wrap::create_obj(napi_env env, napi_callback_info info){ //allocate and wrap oject
//     helper_check_and_extractObject();
//     size_t argc = 3; napi_value argv[3]; JsonMM *vjsonMM; i64 parent;
//     napi_valuetype expectedTypes[3] = {napi_object,napi_string,napi_object};
//     napi_status err = helper_checkparams(env,info,argc, argv, expectedTypes);
//     if(err!=napi_ok){
//         napi_get_null(env, &js_obj);
//         return js_obj;
//     }
//     char type[64]; size_t typeSize;
//     CHECK(napi_get_value_string_utf8(env, argv[1],type,64-1,&typeSize)==napi_ok);
//     i64 *objret = new i64;
//     long lerr = JSON_ERROR_INVALIDDATA;
//     switch(type[0]){
//         default:
//         case 'n': //number or null
//             if(type[1]=='u'&&type[2]=='m'){ //null
//                 lerr = jsonnumber_Create(objret);
//             }
//             else { //number
//                 lerr = jsonnull_Create(objret);
//             }
//             break;
//         case 'a': //array
//             lerr = jsonarray_Create(objret); break;
//         case 'o': //object
//             lerr = jsonobj_Create(objret); break;
//         case 'u': //undefined
//             lerr = jsonundefined_Create(objret); break;
//         case 's': //string
//             lerr = jsonstring_Create(objret); break;
//         case 'b': //boolean
//             lerr = jsonboolean_Create(objret); break;
//     }
    
//     CHECK(napi_create_object(env, &js_obj)==napi_ok);
//     CHECK(napi_wrap(env,js_obj,objret,vjson_wrap::free_obj,NULL, NULL) == napi_ok);
//     return js_obj;
// }

// void vjson_wrap::free_obj/*or array*/(napi_env env, void* data, void* hint){          //called when object is garbage collected
//     delete ((i64*)data); //no need to delete inside data as they will be saved in vmem
// }

i64 GetObjectFromKeyPath(JsonMM *mem, char *keyPath,unsigned long keyPathLen){
    //keyPath is like "root/level1/level2/targetKey"
    //return location of targetKey object
    //if any key in the path does not exist, return 0
    char key[256];
    if(!mem->m_root) {return JSON_ERROR_INVALIDDATA;}
    i64 curObjLoc = mem->m_root;
    if(!keyPathLen||!keyPath[0]) return mem->m_root; //return ROOT
    jsonobj *obj = (jsonobj*)mem->Lock(mem->m_root);
    long curType = jsonobj_ftables[obj->m_ftable]->Type();
    if(curType!=JSON_ARRAY&&curType!=JSON_OBJ) {mem->Unlock(curObjLoc); return JSON_ERROR_INVALIDDATA;}
    //keyPath should be atleast 1024 bytes in length
    unsigned long i=0; unsigned long pi=0;
    while(pi<keyPathLen&&keyPath[pi]=='/') pi++;
    while(pi<keyPathLen){
        key[i] = keyPath[pi];
        i++; pi++;
        if(pi==keyPathLen || !keyPath[pi] || keyPath[pi]=='/'){
            //if(!i) break; //reached end
            key[i] = 0;
            if(curType==JSON_ARRAY&& (key[0]<'0'||key[0]>'9')) {mem->Unlock(curObjLoc); return JSON_ERROR_INVALIDDATA;} //expecting integer index
            i64 nexti = curType==JSON_ARRAY ? ((*((jsonarray*)obj))[(atoi(key))]): obj->Find(key);
            if(!nexti || (pi<keyPathLen-1&&keyPath[pi+1]=='/'/*//*/)) {mem->Unlock(curObjLoc); return JSON_ERROR_INVALIDDATA;}
            mem->Unlock(curObjLoc);
            curObjLoc = nexti; i=0;
            obj = (jsonobj*)mem->Lock(curObjLoc);
            curType = jsonobj_ftables[obj->m_ftable]->Type();
            if(curType!=JSON_ARRAY&&curType!=JSON_OBJ) {mem->Unlock(curObjLoc); return JSON_ERROR_INVALIDDATA;}
            while(pi<keyPathLen&&keyPath[pi]=='/') pi++;
        }
    }
    mem->Unlock(curObjLoc);
    return curObjLoc;
}
size_t CreateUniqueKeyForObject(JsonMM *mem, i64 parentObjLoc, char *outKey, size_t maxKeySize){
    //generate a unique key under parentObjLoc and write to outKey
    //assume outKey has enough space
    //simple example using timestamp, in real implementation ensure uniqueness
    time_t now = time(NULL);
    size_t size = snprintf(outKey, maxKeySize, "%ld", now); //simple example, use timestamp
    if(size<5) return 0;
    //check uniqueness
    jsonobj *parentObjPtr = (jsonobj*)mem->Lock(parentObjLoc);
    i64 foundPair = parentObjPtr->Find(outKey); //to check uniqueness in real implementation
    while(foundPair){
        //handle collision, in real implementation
        now += 1;
        size = snprintf(outKey, maxKeySize, "%ld", now); //simple example, use timestamp
        if(size<5) break;
        foundPair = parentObjPtr->Find(outKey); //to check uniqueness in real implementation
    }
    mem->Unlock(parentObjLoc);
    return size<5 ? 0 : size;
}

napi_value vjson_wrap::flush(napi_env env, napi_callback_info info){
    napi_value js_obj;
    napi_value argv[1];
    JsonMM *mem;
    napi_valuetype expectedTypes[1] = { napi_object };
    helper_checkparams(env,info,1,argv,expectedTypes);

    CHECK(napi_unwrap(env, argv[0], (void**)&mem)==napi_ok)
    mem->Flush();

    napi_get_boolean(env,true,&js_obj);
    return js_obj;
}

napi_value vjson_wrap::calculateFree(napi_env env, napi_callback_info info){ 
    size_t argc = 1; napi_value argv[1]; JsonMM *mem; napi_value js_obj;
    napi_valuetype expectedTypes[1] = {napi_object};
    napi_status err = helper_checkparams(env,info,argc, argv, expectedTypes);
    if(err!=napi_ok){
        napi_get_null(env, &js_obj);
        return js_obj;
    }
    napi_unwrap(env, argv[0],(void**)&mem);

    i64 numfree = mem->CalculateFree();
    napi_create_int64(env, numfree, &js_obj);

    return js_obj;
}

NAPI_MODULE_INIT(/*env, exports*/) {
  // Declare the bindings this addon provides. The data created above is given
  // as the last initializer parameter, and will be given to the binding when it
  // is called.
  napi_property_descriptor bindings[] = {
    {"init", NULL, vjson_wrap::init, NULL, NULL, NULL, napi_enumerable, nullptr/*can put global data here*/},
    {"flush", NULL, vjson_wrap::flush, NULL, NULL, NULL, napi_enumerable, nullptr},
 
    {"calculateFree", NULL, vjson_wrap::calculateFree, NULL, NULL, NULL, napi_enumerable, nullptr},
	
    {"read", NULL, vjson_wrap::read_obj, NULL, NULL, NULL, napi_enumerable, nullptr},
    {"append", NULL, vjson_wrap::append_obj, NULL, NULL, NULL, napi_enumerable, nullptr},
    {"update", NULL, vjson_wrap::update_obj, NULL, NULL, NULL, napi_enumerable, nullptr},
    {"delete", NULL, vjson_wrap::delete_obj, NULL, NULL, NULL, napi_enumerable, nullptr}
  };

  // Expose the bindings declared above to JavaScript.
  CHECK(napi_define_properties(env,
                                exports,
                                sizeof(bindings) / sizeof(bindings[0]),
                                bindings) == napi_ok);

  // Return the `exports` object provided. It now has two new properties, which
  // are the functions we wish to expose to JavaScript.
  return exports;
}
