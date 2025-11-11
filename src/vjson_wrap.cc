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
    vjsonMM = new JsonMM(str);
    vjsonMM->Init();
    CHECK(napi_create_object(env, &js_obj)==napi_ok);
    CHECK(napi_wrap(env,js_obj,vjsonMM,vjson_wrap::freeMem,NULL, NULL) == napi_ok);
    return js_obj;
}
void vjson_wrap::freeMem(napi_env env, void* data, void* hint){         //called when object is garbage collected
    JsonMM* vjsonMM = (JsonMM*)data;
    vjsonMM->Close();
    delete ((JsonMM*)data);
}

napi_value vjson_wrap::create_obj(napi_env env, napi_callback_info info){ //allocate and wrap oject
    helper_check_and_extractObject();
    size_t argc = 3; napi_value argv[3]; JsonMM *vjsonMM; i64 parent;
    napi_valuetype expectedTypes[3] = {napi_object,napi_string,napi_object};
    napi_status err = helper_checkparams(env,info,argc, argv, expectedTypes);
    if(err!=napi_ok){
        napi_get_null(env, &js_obj);
        return js_obj;
    }
    char type[64]; size_t typeSize;
    CHECK(napi_get_value_string_utf8(env, argv[1],type,64-1,&typeSize)==napi_ok);
    i64 *objret = new i64;
    long lerr = JSON_ERROR_INVALIDDATA;
    switch(type[0]){
        default:
        case 'n': //number or null
            if(type[1]=='u'&&type[2]=='m'){ //null
                lerr = jsonnumber_Create(objret);
            }
            else { //number
                lerr = jsonnull_Create(objret);
            }
            break;
        case 'a': //array
            lerr = jsonarray_Create(objret); break;
        case 'o': //object
            lerr = jsonobj_Create(objret); break;
        case 'u': //undefined
            lerr = jsonundefined_Create(objret); break;
        case 's': //string
            lerr = jsonstring_Create(objret); break;
        case 'b': //boolean
            lerr = jsonboolean_Create(objret); break;
    }
    
    CHECK(napi_create_object(env, &js_obj)==napi_ok);
    CHECK(napi_wrap(env,js_obj,objret,vjson_wrap::free_obj,NULL, NULL) == napi_ok);
    return js_obj;
}

void vjson_wrap::free_obj/*or array*/(napi_env env, void* data, void* hint){          //called when object is garbage collected
    delete ((i64*)data); //no need to delete inside data as they will be saved in vmem
}

i64 GetObjectFromKeyPath(JsonMM *mem, char *keyPath){
    //keyPath is like "root/level1/level2/targetKey"
    //return location of targetKey object
    //if any key in the path does not exist, return 0
    return 0; //TODO: implement this
}
size_t CreateUniqueKeyForObject(JsonMM *mem, i64 parentObjLoc, char *outKey, size_t maxKeySize){
    //generate a unique key under parentObjLoc and write to outKey
    //assume outKey has enough space
    //simple example using timestamp, in real implementation ensure uniqueness
    time_t now = time(NULL);
    size_t size = snprintf(outKey, maxKeySize, "%lld", now); //simple example, use timestamp
    if(size<0) return 0;
    //check uniqueness
    jsonobj *parentObjPtr = (jsonobj*)mem->Lock(parentObjLoc);
    i64 foundPair = parentObjPtr->Find(outKey); //to check uniqueness in real implementation
    while(foundPair){
        //handle collision, in real implementation
        now += 1;
        size = snprintf(outKey, maxKeySize, "%lld", now); //simple example, use timestamp
        if(size<0) break;
        foundPair = parentObjPtr->Find(outKey); //to check uniqueness in real implementation
    }
    mem->Unlock(parentObjLoc);
    return size<0 ? 0 : size;
}

long append_jsonobj(JsonMM *mem, jsonobj* p_obj, napi_value node_obj){
    napi_value propertyNames;
    CHECK(napi_get_property_names(env, node_obj, &propertyNames) == napi_ok);
    uint32_t propCount;
    CHECK(napi_get_array_length(env, propertyNames, &propCount) == napi_ok);
    for (uint32_t i = 0; i < propCount; i++) {
        napi_value propName;
        CHECK(napi_get_element(env, propertyNames, i, &propName) == napi_ok);
        char propNameStr[256]; size_t propNameStrSize;
        CHECK(napi_get_value_string_utf8(env, propName, propNameStr, 256 - 1, &propNameStrSize) == napi_ok);
        napi_value propValue;
        CHECK(napi_get_property(env, node_obj, propName, &propValue) == napi_ok);
        //determine type of propValue and set in newObjPtr accordingly
        //for simplicity, only handling string properties here
        napi_valuetype propValueType;
        CHECK(napi_typeof(env, propValue, &propValueType) == napi_ok);
        switch (propValueType){
            case napi_string: {
                size_t valueStrSize;
                CHECK(napi_get_value_string_utf8(env, propValue, nullptr, 0, &valueStrSize) == napi_ok);
                i64 valueStr = 0;
                if(valueStrSize) {
                    valueStr = mem->Alloc(valueStrSize);
                    if(!valueStr) return JSON_ERROR_OUTOFMEMORY;
                    char *valueStrPtr = (char*)mem->Lock(valueStr);
                    CHECK(napi_get_value_string_utf8(env, propValue, valueStrPtr, 0, &valueStrSize) == napi_ok);
                    mem->Unlock(valueStr);
                }
                //create and append jsonstring object
                i64 strObjLoc= p_obj->AppendText(propNameStr, valueStr/*ok if null*/);
                if(!strObjLoc) return JSON_ERROR_OUTOFMEMORY;
                break;
            }
            case napi_object:{
                i64 obj = p_boj->AppendObj(propNameStr);
                if(!obj){
                    return JSON_ERROR_OUTOFMEMORY;
                }
                jsonobj *objPtr = (jsonobj*)mem->Lock(obj);
                napi_is_array()
                long err = append_jsonobj(mem,objPtr,propValue);
                if(err<0) return err;
                break;
            }
            case napi_number:{
                double dval;
                CHECK(napi_get_value_double(env, propValue, &dval) == napi_ok);
                i64 num = p_obj->AppendNumber(propNameStr,dval);
                if(!num) return JSON_ERROR_OUTOFMEMORY;
                break;
            }
            case napi_boolean:{
                bool bval;
                CHECK(napi_get_value_bool(env,propValue,&dval)==napi_ok);
                i64 b = p_obj->AppendBoolean(propNameStr,dval);
                if(!b) return JSON_ERROR_OUTOFMEMORY;
                break;
            }
            case napi_null:{
                bool bval;
                i64 b = p_obj->AppendNull(propNameStr);
                if(!b) return JSON_ERROR_OUTOFMEMORY;
                break;
            }
            case napi_undefined:{
                bool bval;
                i64 b = p_obj->AppendUndefined(propNameStr);
                if(!b) return JSON_ERROR_OUTOFMEMORY;
                break;
            }
        }
    }
}

napi_value vjson_wrap::append_obj(napi_env env, napi_callback_info info){
    //params: vjsonMM, keypath of parent object, ID/KEY of object to append (empty to create key), object to append
    napi_value js_obj;
    napi_value argv[4]; 
    JsonMM *mem; char objPath[256]; size_t objPathSize; char key[256]; size_t keySize;
    napi_valuetype expectedTypes[4] = { napi_object,napi_string,napi_string,napi_object };
    napi_status err = helper_checkparams(env, info, 4, argv, expectedTypes);
    if (err != napi_ok) {
        napi_get_null(env, &js_obj);
        return js_obj;
    }
    CHECK(napi_unwrap(env, argv[0], (void**)&mem)==napi_ok)
    char objPath[256]; size_t objPathSize;
    CHECK(napi_string_to_utf8(env, argv[1], objPath, 256 - 1, &objPathSize) == napi_ok);
    if(!objPathSize){
        napi_throw_error(env, "-3", "invalid object path");
        return NULL;
    }
    i64 parentObjLoc = GetObjectFromKeyPath(mem, objPath);
    if(!parentObjLoc){
        napi_throw_error(env, "-4", "parent object not found");
        return NULL;
    }
    char key[256]; size_t keySize;
    CHECK(napi_string_to_utf8(env, argv[2], key, 256 - 1, &keySize) == napi_ok);
    if(!keySize){
        //generate unique key
        keySize = CreateUniqueKeyForObject(mem, parentObjLoc, key, 256);
        if(!keySize){
            napi_throw_error(env, "-5", "failed to create unique key");
            return NULL;
        }
    }

    //append object
    g_jsonMem = mem; //set global memory manager for jsonobj_Create
    i64 newObjLoc;
    long err =jsonobj_Create(&newObjLoc);
    if(err<0){
        napi_throw_error(env, "-6", "failed to create new object");
        return NULL;
    }
    jsonobj *parentObjPtr = (jsonobj*)mem->Lock(parentObjLoc);
    err = parentObjPtr->AppendObj(key, newObjLoc);
    if(err<0){
        mem->Unlock(parentObjLoc);
        jsonobj_Delete(newObjLoc);
        napi_throw_error(env, "-7", "failed to append new object to parent");
        return NULL;
    }
    jsonobj *newObjPtr = (jsonobj*)mem->Lock(newObjLoc);

    

    CHECK(napi_create_string_utf8(env, key, keySize, &js_obj) == napi_ok);
    return js_obj;
}
napi_value vjson_wrap::divide(napi_env env, napi_callback_info info){
    num10* oDataB;
    helper_check_and_extractObject(2);
    CHECK(napi_unwrap(env, argv[1], (void**)&oDataB) == napi_ok);
    num10* result = new num10(oData->divide(*oDataB));
    CHECK(napi_create_object(env, &js_obj) == napi_ok);
    CHECK(napi_wrap(env, js_obj, result, vjson_wrap::free, NULL, NULL) == napi_ok);
    return js_obj;
}
napi_value vjson_wrap::add(napi_env env, napi_callback_info info){
    num10* oDataB;
    helper_check_and_extractObject(2);
    CHECK(napi_unwrap(env, argv[1], (void**)&oDataB) == napi_ok)
    num10 *result = new num10(oData[0] + (*oDataB));
    CHECK(napi_create_object(env, &js_obj) == napi_ok);
    CHECK(napi_wrap(env, js_obj, result, vjson_wrap::free, NULL, NULL) == napi_ok);
    return js_obj;
}
napi_value vjson_wrap::sub(napi_env env, napi_callback_info info){
    num10* oDataB;
    helper_check_and_extractObject(2);
    CHECK(napi_unwrap(env, argv[1], (void**)&oDataB) == napi_ok);
    num10* result = new num10(oData[0] - (*oDataB));
    CHECK(napi_create_object(env, &js_obj) == napi_ok);
    CHECK(napi_wrap(env, js_obj, result, vjson_wrap::free, NULL, NULL) == napi_ok);
    return js_obj;
}
napi_value vjson_wrap::toString(napi_env env, napi_callback_info info) {
    helper_check_and_extractObject(1);
    char str[256];
    oData->toString(str);
    CHECK(napi_create_string_utf8(env, str, NAPI_AUTO_LENGTH, &js_obj) == napi_ok);
    return js_obj;
}
napi_value vjson_wrap::toDouble(napi_env env, napi_callback_info info) {
	helper_check_and_extractObject(1);
	CHECK(napi_create_double(env, oData->toDouble(), &js_obj) == napi_ok);
	return js_obj;
}
napi_value vjson_wrap::floor(napi_env env, napi_callback_info info) {
	helper_check_and_extractObject(1);
	num10* result = new num10(oData->floor());
	CHECK(napi_create_object(env, &js_obj) == napi_ok);
	CHECK(napi_wrap(env, js_obj, result, vjson_wrap::free, NULL, NULL) == napi_ok);
	return js_obj;
}
napi_value vjson_wrap::clone(napi_env env, napi_callback_info info) {
	helper_check_and_extractObject(1);
	num10* result = new num10(oData[0]);
	CHECK(napi_create_object(env, &js_obj) == napi_ok);
	CHECK(napi_wrap(env, js_obj, result, vjson_wrap::free, NULL, NULL) == napi_ok);
	return js_obj;
}

napi_value vjson_wrap::isZero(napi_env env, napi_callback_info info) {
	helper_check_and_extractObject(1);
	CHECK(napi_create_int32(env, !oData->m_n ? 1 : 0, &js_obj) == napi_ok);
	return js_obj;
}
napi_value vjson_wrap::isNeg(napi_env env, napi_callback_info info) {
	helper_check_and_extractObject(1);
	CHECK(napi_create_int32(env, oData->isneg, &js_obj) == napi_ok);
	return js_obj;
}


NAPI_MODULE_INIT(/*env, exports*/) {
  // Declare the bindings this addon provides. The data created above is given
  // as the last initializer parameter, and will be given to the binding when it
  // is called.
  napi_property_descriptor bindings[] = {
    {"create", NULL, vjson_wrap::create, NULL, NULL, NULL, napi_enumerable, nullptr/*can put global data here*/},
    {"multiply", NULL, vjson_wrap::multiply, NULL, NULL, NULL, napi_enumerable, nullptr},
	{"divide", NULL, vjson_wrap::divide, NULL, NULL, NULL, napi_enumerable, nullptr},
    {"add", NULL, vjson_wrap::add, NULL, NULL, NULL, napi_enumerable, nullptr},
	{"sub", NULL, vjson_wrap::sub, NULL, NULL, NULL, napi_enumerable, nullptr},
	{"clone", NULL, vjson_wrap::clone, NULL, NULL, NULL, napi_enumerable, nullptr},
	{"floor", NULL, vjson_wrap::floor, NULL, NULL, NULL, napi_enumerable, nullptr},
    {"toString", NULL, vjson_wrap::toString, NULL, NULL, NULL, napi_enumerable, nullptr},
    {"toDouble", NULL, vjson_wrap::toDouble, NULL, NULL, NULL, napi_enumerable, nullptr},
    {"isZero", NULL, vjson_wrap::isZero, NULL, NULL, NULL, napi_enumerable, nullptr},
    {"isNeg", NULL, vjson_wrap::isNeg, NULL, NULL, NULL, napi_enumerable, nullptr},
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
