#include "vjson_wrap.h"

//creates and appends
i64 createChild(napi_env env, JsonMM *mem, napi_value elementValue){
    i64 retObj = 0;
    napi_valuetype elementValueType;
    CHECK(napi_typeof(env, elementValue, &elementValueType) == napi_ok);
    switch (elementValueType){
        case napi_string: {
            size_t valueStrSize;
            CHECK(napi_get_value_string_utf8(env, elementValue, nullptr, 0, &valueStrSize) == napi_ok);
            i64 valueStr = 0;
            if(valueStrSize) {
                valueStr = mem->Alloc(valueStrSize);
                if(!valueStr) return JSON_ERROR_OUTOFMEMORY;
                char *valueStrPtr = (char*)mem->Lock(valueStr);
                CHECK(napi_get_value_string_utf8(env, elementValue, valueStrPtr, 0, &valueStrSize) == napi_ok);
                mem->Unlock(valueStr);
            }
            //create jsonstring object
            long err = jsonstring_Create(&retObj); if(err<0) {
                if(valueStr) {mem->Free(valueStr);}
                return JSON_ERROR_OUTOFMEMORY;
            }
            jsonstring* sstr = (jsonstring*)g_jsonMem->Lock(retObj);
            err = sstr->SetString(valueStr);
            g_jsonMem->Unlock(retObj);
            if(err<0){ jsonstring_Delete(retObj); return err; }
            break;
        }
        case napi_object:{
            bool isarray;
            napi_is_array(env, elementValue, &isarray);
            long err;
            if(isarray){
                err = (long)(jsonarray_Create(&retObj));
                if(err>=0){
                    jsonarray *objPtr = (jsonarray*)mem->Lock(retObj);
                    err = append_toarray(env,mem,objPtr,elementValue);
                    mem->Unlock(retObj);
                }
            }
            else{
                err = (long)(jsonobj_Create(&retObj));
                if(err>=0){
                    jsonobj *objPtr = (jsonobj*)mem->Lock(retObj);
                    err = append_toobj(env, mem,objPtr,elementValue);
                    mem->Unlock(retObj);
                }
            }
            if(err<0) return err;
            break;
        }
        case napi_number:{
            double dval; CHECK(napi_get_value_double(env, elementValue, &dval) == napi_ok);
            long err = jsonnumber_Create(&retObj); if(err<0) { return err; }
            jsonnumber* numPtr = (jsonnumber*)g_jsonMem->Lock(retObj);
            numPtr->num = dval;
            g_jsonMem->Unlock(retObj);
            break;
        }
        case napi_boolean:{
            bool bval; CHECK(napi_get_value_bool(env, elementValue, &bval) == napi_ok);
            long err = jsonboolean_Create(&retObj); if(err<0) { return err; }
            jsonboolean* numPtr = (jsonboolean*)g_jsonMem->Lock(retObj);
            numPtr->b = bval;
            g_jsonMem->Unlock(retObj);
            break;
        }
        case napi_null:{
            long err = jsonnull_Create(&retObj); if(err<0) { return err; }
            break;
        }
        case napi_undefined:{
            long err = jsonundefined_Create(&retObj); if(err<0) { return err; }
            break;
        }
    }    
    return retObj;
}

long append_toarray(napi_env env, JsonMM *mem, jsonarray* p_obj, napi_value node_obj){
    uint32_t elementCount;
    CHECK(napi_get_array_length(env, node_obj, &elementCount) == napi_ok);
    for (uint32_t i = 0; i < elementCount; i++) {
        napi_value elementValue;
        CHECK(napi_get_element(env, node_obj, i, &elementValue) == napi_ok);
        //determine type of propValue and set in newObjPtr accordingly
        //for simplicity, only handling string properties here
        i64 objI = createChild(env,mem,elementValue);
        if(objI<0) return objI;

        long err = p_obj->AppendObj(objI);
        if(err<0){
            _jsonobj* objPtr = (_jsonobj*)mem->Lock(objI);
            jsonobj_ftables[objPtr->m_ftable]->Delete(objI);
            mem->Unlock(objI);
        }
    }
    return 0;
}

long append_toobj(napi_env env, JsonMM *mem, jsonobj* p_obj, napi_value node_obj, bool overwrite){
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
        i64 objI = createChild(env,mem,propValue);
        if(objI<0) return objI;

        long err =p_obj->AppendObj(propNameStr, objI);
        if(err<0){
            _jsonobj* objPtr = (_jsonobj*)mem->Lock(objI);
            jsonobj_ftables[objPtr->m_ftable]->Delete(objI);
            mem->Unlock(objI);
            return err;
        }
    }
    return 0;
}

napi_value vjson_wrap::append_obj(napi_env env, napi_callback_info info){
    //params: vjsonMM, keypath of parent object, ID/KEY of object to append (empty to create key), object to append
    napi_value js_obj;
    napi_value argv[4];
    JsonMM *mem; char key[256]; size_t keySize;
    napi_valuetype expectedTypes[4] = { napi_object,napi_string,napi_string,napi_object };
    size_t numArgs;
    CHECK(napi_get_cb_info(env,info, &numArgs, argv, NULL,NULL/*((void**)&addon_data)*/) == napi_ok);
    bool argerr=false;
    if(numArgs>2) { //appendToArray(vjsonMM,keypath of parent, object to append)
        napi_valuetype argType; 
        napi_typeof(env, argv[0], &argType); if(argType!=napi_object) {argerr=true;}
        napi_typeof(env, argv[1], &argType); if(argType!=napi_string) {argerr=true;}
        if(numArgs==4) {napi_typeof(env, argv[2], &argType); if(argType!=napi_string) {argerr=true;}}
        else if(numArgs!=3) argerr=true;
    }
    else argerr=true;
    if (argerr) {
        napi_throw_error(env, "-2", "invalid arguments");
        napi_get_null(env, &js_obj); return js_obj;
    }

    CHECK(napi_unwrap(env, argv[0], (void**)&mem)==napi_ok)
    g_jsonMem = mem; //set global memory manager for jsonobj_Create
    char objPath[2048]; size_t objPathSize;
    CHECK(napi_get_value_string_utf8(env, argv[1], objPath, 2048 - 1, &objPathSize) == napi_ok);
    if(!objPathSize || objPathSize==2048-1/*maxed out*/){
        napi_throw_error(env, "-3", "invalid object path");
        napi_get_null(env, &js_obj); return js_obj;
    }
    i64 parentObjLoc = GetObjectFromKeyPath(mem, objPath,objPathSize);
    if(!parentObjLoc){
        napi_throw_error(env, "-4", "parent object not found");
        napi_get_null(env, &js_obj); return js_obj;
    }
    _jsonobj* parentObjPtr = (_jsonobj*)mem->Lock(parentObjLoc);
    long type = jsonobj_ftables[parentObjPtr->m_ftable]->Type();
    mem->Unlock(parentObjLoc);
    if(type==JSON_ARRAY) {
        if( numArgs!=3){
            napi_throw_error(env, "-5", "parent object is array and expecting 3 arguments");
            napi_get_null(env, &js_obj); return js_obj;
        }
    }
    else if(type==JSON_OBJ){
        if(numArgs!=4){
            napi_throw_error(env, "-6", "parent object is object and expecting 4 arguments");
            napi_get_null(env, &js_obj); return js_obj;
        }
    }
    else{
        napi_throw_error(env, "-7", "parent object is not object or array");
        napi_get_null(env, &js_obj); return js_obj;
    }

        //append object
    i64 newObjLoc = createChild(env,mem,argv[3]/*object to append*/);
    if(newObjLoc<0){
        napi_throw_error(env, "-8", "failed to create new object");
        napi_get_null(env, &js_obj); return js_obj;
    }

    parentObjPtr = (_jsonobj*)mem->Lock(parentObjLoc);
    long err;
    if(type==JSON_ARRAY) {
        err =((jsonarray*)parentObjPtr)->AppendObj(newObjLoc);
    }
    else {
        CHECK(napi_get_value_string_utf8(env, argv[2], key, 256 - 1, &keySize) == napi_ok);
        if(!keySize || !key[0]){
            //generate unique key
            keySize = CreateUniqueKeyForObject(mem, parentObjLoc, key, 256);
            if(!keySize){
                err= JSON_ERROR_OUTOFMEMORY;
            }
        }
        err = ((jsonobj*)parentObjPtr)->AppendObj(key,newObjLoc);
    }
    mem->Unlock(parentObjLoc);
    if(err<0){
        _jsonobj *objPtr= (_jsonobj*)mem->Lock(newObjLoc);
        long t = objPtr->m_ftable;
        mem->Unlock(newObjLoc);
        jsonobj_ftables[t]->Delete(newObjLoc);
        napi_throw_error(env, "-10", "failed to add new object");
        napi_get_null(env, &js_obj); return js_obj;
    }
    
    if(type==JSON_OBJ){
        CHECK(napi_create_string_utf8(env, key, keySize, &js_obj) == napi_ok);
        return js_obj; //send key back
    }
    napi_create_int32(env, err, &js_obj); 
    return js_obj; //send index of new element added
}

napi_value vjson_wrap::update_obj(napi_env env, napi_callback_info info){
    //params: vjsonMM, keypath of parent object, ID/KEY of object to append (empty to create key), object to update
    napi_value js_obj;
    napi_value argv[4];
    JsonMM *mem; char key[256]; size_t keySize;
    napi_valuetype expectedTypes[4] = { napi_object,napi_string,napi_string,napi_object };
    size_t numArgs;
    CHECK(napi_get_cb_info(env,info, &numArgs, argv, NULL,NULL/*((void**)&addon_data)*/) == napi_ok);
    bool argerr=false;
    napi_valuetype argType; 
    if(numArgs==4) { //appendToArray(vjsonMM,keypath of parent, object to append)
        napi_typeof(env, argv[0], &argType); if(argType!=napi_object) {argerr=true;}
        napi_typeof(env, argv[1], &argType); if(argType!=napi_string) {argerr=true;}
        napi_typeof(env, argv[2], &argType); if(argType!=napi_string||argType!=napi_number) {argerr=true;}
    }
    if (argerr) {
        napi_throw_error(env, "-2", "invalid arguments");
        napi_get_null(env, &js_obj); return js_obj;
    }

    CHECK(napi_unwrap(env, argv[0], (void**)&mem)==napi_ok)
    g_jsonMem = mem; //set global memory manager for jsonobj_Create
    char objPath[2048]; size_t objPathSize;
    CHECK(napi_get_value_string_utf8(env, argv[1], objPath, 2048 - 1, &objPathSize) == napi_ok);
    if(!objPathSize || objPathSize==2048-1/*maxed out*/){
        napi_throw_error(env, "-3", "invalid object path");
        napi_get_null(env, &js_obj); return js_obj;
    }
    i64 parentObjLoc = GetObjectFromKeyPath(mem, objPath,objPathSize);
    if(!parentObjLoc){
        napi_throw_error(env, "-4", "parent object not found");
        napi_get_null(env, &js_obj); return js_obj;
    }
    _jsonobj* parentObjPtr = (_jsonobj*)mem->Lock(parentObjLoc);
    long type = jsonobj_ftables[parentObjPtr->m_ftable]->Type();
    mem->Unlock(parentObjLoc);
    if(type==JSON_ARRAY) {
        if( argType!=napi_number){
            napi_throw_error(env, "-5", "parent object is array and expecting index");
            napi_get_null(env, &js_obj); return js_obj;
        }

    }
    else if(type==JSON_OBJ){
        if(argType!=napi_string){
            napi_throw_error(env, "-6", "parent object is object and expecting key");
            napi_get_null(env, &js_obj); return js_obj;
        }
    }
    else{
        napi_throw_error(env, "-7", "parent object is not object or array");
        napi_get_null(env, &js_obj); return js_obj;
    }

    parentObjPtr = (_jsonobj*)mem->Lock(parentObjLoc);
    int64_t index;
    if(type==JSON_ARRAY) {
        napi_get_value_int64(env,argv[2],&index);
        if(index<0||((jsonarray*)parentObjPtr)->m_size<=index){
            mem->Unlock(parentObjLoc);
            napi_throw_error(env, "-8", "index out of range");
            napi_get_null(env, &js_obj); return js_obj;
        }
    }
    else {
        CHECK(napi_get_value_string_utf8(env, argv[2], key, 256 - 1, &keySize) == napi_ok);
        if(!keySize || !key[0]  ){
            mem->Unlock(parentObjLoc);
            napi_throw_error(env, "-9", "invalid key");
            napi_get_null(env, &js_obj); return js_obj;
        }
    }

    //append or update object
    i64 newObjLoc = createChild(env,mem,argv[3]/*object to append*/);
    if(newObjLoc<0){
        mem->Unlock(parentObjLoc);
        napi_throw_error(env, "-8", "failed to create new object");
        napi_get_null(env, &js_obj); return js_obj;
    }

    if(type==JSON_ARRAY) {
        ((jsonarray*)parentObjPtr)->Replace(index&0xffffffff,newObjLoc);
    }
    else {
        ((jsonobj*)parentObjPtr)->Replace(key,newObjLoc);
    }

    mem->Unlock(parentObjLoc);
    
    napi_get_boolean(env,true,&js_obj);
    return js_obj;
}


napi_value vjson_wrap::delete_obj(napi_env env, napi_callback_info info){
    //params: vjsonMM, keypath of parent object, ID/KEY of object to append (empty to create key), object to delete
    napi_value js_obj;
    napi_value argv[4];
    JsonMM *mem; char key[256]; size_t keySize;
    napi_valuetype expectedTypes[4] = { napi_object,napi_string,napi_string,napi_object };
    size_t numArgs;
    CHECK(napi_get_cb_info(env,info, &numArgs, argv, NULL,NULL/*((void**)&addon_data)*/) == napi_ok);
    bool argerr=false;
    napi_valuetype argType; 
    if(numArgs==4) { //appendToArray(vjsonMM,keypath of parent, object to append)
        napi_typeof(env, argv[0], &argType); if(argType!=napi_object) {argerr=true;}
        napi_typeof(env, argv[1], &argType); if(argType!=napi_string) {argerr=true;}
        napi_typeof(env, argv[2], &argType); if(argType!=napi_string) {argerr=true;}
    }
    if (argerr) {
        napi_throw_error(env, "-2", "invalid arguments");
        napi_get_null(env, &js_obj); return js_obj;
    }

    CHECK(napi_unwrap(env, argv[0], (void**)&mem)==napi_ok)
    g_jsonMem = mem; //set global memory manager for jsonobj_Create
    char objPath[2048]; size_t objPathSize;
    CHECK(napi_get_value_string_utf8(env, argv[1], objPath, 2048 - 1, &objPathSize) == napi_ok);
    if(!objPathSize || objPathSize==2048-1/*maxed out*/){
        napi_throw_error(env, "-3", "invalid object path");
        napi_get_null(env, &js_obj); return js_obj;
    }
    i64 parentObjLoc = GetObjectFromKeyPath(mem, objPath,objPathSize);
    if(!parentObjLoc){
        napi_throw_error(env, "-4", "parent object not found");
        napi_get_null(env, &js_obj); return js_obj;
    }
    _jsonobj* parentObjPtr = (_jsonobj*)mem->Lock(parentObjLoc);
    long type = jsonobj_ftables[parentObjPtr->m_ftable]->Type();
    mem->Unlock(parentObjLoc);
    if(type!=JSON_OBJ){
        napi_throw_error(env, "-7", "parent object is not object");
        napi_get_null(env, &js_obj); return js_obj;
    }

    parentObjPtr = (_jsonobj*)mem->Lock(parentObjLoc);
    CHECK(napi_get_value_string_utf8(env, argv[2], key, 256 - 1, &keySize) == napi_ok);
    if(!keySize || !key[0]  ){
        mem->Unlock(parentObjLoc);
        napi_throw_error(env, "-9", "invalid key");
        napi_get_null(env, &js_obj); return js_obj;
    }

    ((jsonobj*)parentObjPtr)->Delete(key);

    mem->Unlock(parentObjLoc);
    
    napi_get_boolean(env,true,&js_obj);
    return js_obj;
}

