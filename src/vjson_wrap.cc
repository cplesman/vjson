#include "vjson_wrap.h"

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

#define helper_check_and_extractObject(num)  \
    napi_value argv[3]; JsonMM *mem; napi_value js_obj; \
    napi_valuetype expectedTypes[3] = { napi_object,napi_string,napi_object }; \
    napi_status err = helper_checkparams(env, info, num, argv, expectedTypes); \
    if (err != napi_ok) { \
        napi_get_null(env, &js_obj); \
        return js_obj; \
    } \
    CHECK(napi_unwrap(env, argv[0], (void**)&mem)==napi_ok)


napi_value vjson_wrap::init(napi_env env, napi_callback_info info){ //allocate and wrap oject
    size_t argc = 1; napi_value argv[1]; JsonMM *vjsonMM; napi_value js_obj;
    napi_valuetype expectedTypes[1] = {napi_string};
    napi_status err = helper_checkparams(env,info,argc, argv, expectedTypes);
    if(err!=napi_ok){
        napi_get_null(env, &js_obj);
        return js_obj;
    }
    char str[256]; size_t strSize;
    CHECK(napi_get_value_string_utf8(env, argv[0],str,256-1,&strSize)==napi_ok);
    vjsonMM = new JsonMM(str);
    vjsonMM->Init();
    CHECK(napi_create_object(env, &js_obj)==napi_ok);
    CHECK(napi_wrap(env,js_obj,vjsonMM,vjson_wrap::free,NULL, NULL) == napi_ok);
    return js_obj;
}
void vjson_wrap::freeMem(napi_env env, void* data, void* hint){         //called when object is garbage collected
    JsonMM* vjsonMM = (JsonMM*)data;
    vjsonMM->Close();
    delete ((JsonMM*)data);
}

void vjson_wrap::freeObj/*or array*/(napi_env env, void* data, void* hint){          //called when object is garbage collected
    delete ((i64*)data); //no need to delete inside data as they will be saved in vmem
}

i64 GetObjectFromKeyPath(JsonMM *mem, char *keyPath){
    //keyPath is like "root/level1/level2/targetKey"
    //return location of targetKey object
    //if any key in the path does not exist, return 0
    return 0; //TODO: implement this
}

napi_value vjson_wrap::append_obj(napi_env env, napi_callback_info info){
    //params: vjsonMM, keypath of parent object, ID/KEY of object to append (empty to create key), object to append
    napi_value js_obj;
    helper_check_and_extractObject(3); //unwrap vjsonMM
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
        
    }
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
