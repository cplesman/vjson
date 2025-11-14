#include "vjson/json.h"
#include <node_api.h>

#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>

class fileio : public ifile {
	FILE* f;
public:
	fileio():ifile() { f = 0; }
	long open(const char* p_path, const char* p_options) {
		f = fopen(p_path, p_options);
		if (!f) {
			return -1;
		}
		return 0;
	}
	long long read(void* p_buf, long long p_size) {
		return fread(p_buf, (size_t)1, (size_t)p_size, f);
	}
	long long write(const void* p_buf, long long p_size) {
		return fwrite(p_buf, (size_t)1, (size_t)p_size, f);
	}
	long long size() {
		fseek(f, 0, SEEK_END);
		return (long long)ftell(f);
	}
	void close() {
		fclose(f);
	}
	ifile_attribute attributes(const char* p_path) {
		struct stat s;
		if (stat(p_path, &s) == 0) {
			if ((s.st_mode & S_IFDIR)) {
				return ifile_attribute::IFILE_ISDIR; //not a directory
			}
			else {
				return ifile_attribute::IFILE_ISFILE;
			}
		}
		return ifile_attribute::IFILE_ERROR;
	}
	long mkdir(const char* p_path) {
#ifdef _WIN32
		if (_mkdir(p_path) == -1) {
			return -10;
		}
#else
		if (::mkdir(p_path, 0755) == -1) {
			return -10;
		}
#endif
		return 0;
	}
};


class JsonMM : public vmem{
public:
	i64 m_globalObjLoc;

	JsonMM(const char *dbPath) :vmem(dbPath, new fileio()) {}
	~JsonMM() {}

	i64 ReadGenBlock(const void* fromMem) override {
		//virtually overload these functions to write/read initial settings
		m_globalObjLoc = *((i64*)fromMem);
		return sizeof(i64);
	}
	i64 WriteGenBlock(void* toMem) override {
		//virtually overload these functions to write/read initial settings
		*((i64*)toMem) = m_globalObjLoc;
		return sizeof(i64);
	}
};


napi_status helper_checkparams(napi_env env, napi_callback_info info, size_t argc/*expected*/, napi_value *argv, napi_valuetype *types){
    size_t expected = argc;
    //argc will return more even if expected is less
    napi_get_cb_info(env,info, &argc, argv, NULL,NULL/*((void**)&addon_data)*/);
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

void freeMem(napi_env env, void* data, void* hint);
napi_value initMem(napi_env env, napi_callback_info info){ //allocate and wrap oject
    napi_value js_obj;
    napi_get_null(env, &js_obj);
    return js_obj;
    size_t argc = 1; napi_value argv[1]; JsonMM *mem;
    napi_valuetype expectedTypes[1] = {napi_string};
    napi_status err = helper_checkparams(env,info,argc, argv, expectedTypes);
    if(err!=napi_ok){
        napi_get_null(env, &js_obj);
        return js_obj;
    }
    char str[512]; size_t strSize;
    napi_get_value_string_utf8(env, argv[0],str,512-1,&strSize);
    try{
        mem = new JsonMM(str);
        mem->Init();
        napi_create_object(env, &js_obj);
        napi_wrap(env,js_obj,mem,freeMem,NULL, NULL);
    }
    catch(vmem_exception e){
        napi_create_string_utf8(env,"failed to init", 14, &js_obj);
    }
    return js_obj;
}
void freeMem(napi_env env, void* data, void* hint){         //called when object is garbage collected
    JsonMM* vjsonMM = (JsonMM*)data;
    vjsonMM->Close();
    delete ((vmem*)data);
}


napi_value create(napi_env env, napi_callback_info info){ 
    size_t argc = 2; napi_value argv[2]; JsonMM *mem; napi_value js_obj;
    napi_valuetype expectedTypes[2] = {napi_object,napi_number};
    napi_status err = helper_checkparams(env,info,argc, argv, expectedTypes);
    if(err!=napi_ok){
        napi_get_null(env, &js_obj);
        return js_obj;
    }
    napi_unwrap(env, argv[0],(void**)&mem);
    int64_t size;
    napi_get_value_int64(env, argv[1], &size);

    i64 memloc = mem->Alloc(size);
    napi_create_int64(env, memloc, &js_obj);

    return js_obj;
}
napi_value free(napi_env env, napi_callback_info info){ 
    size_t argc = 2; napi_value argv[2]; JsonMM *mem; napi_value js_obj;
    napi_valuetype expectedTypes[2] = {napi_object,napi_number};
    napi_status err = helper_checkparams(env,info,argc, argv, expectedTypes);
    if(err!=napi_ok){
        napi_get_null(env, &js_obj);
        return js_obj;
    }
    napi_unwrap(env, argv[0],(void**)&mem);
    int64_t memloc;
    napi_get_value_int64(env, argv[1], &memloc);

    mem->Free(memloc);

    napi_get_boolean(env,true,&js_obj);
    return js_obj;
}

napi_value CalculateFree(napi_env env, napi_callback_info info){ 
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
    {"init", NULL, initMem, NULL, NULL, NULL, napi_enumerable, nullptr/*can put global data here*/},
    {"calculateFree", NULL, CalculateFree, NULL, NULL, NULL, napi_enumerable, nullptr/*can put global data here*/},

    {"create", NULL, create, NULL, NULL, NULL, napi_enumerable, nullptr/*can put global data here*/},
    {"free", NULL, free, NULL, NULL, NULL, napi_enumerable, nullptr/*can put global data here*/},
  };

  // Expose the bindings declared above to JavaScript.
  napi_define_properties(env,
                                exports,
                                sizeof(bindings) / sizeof(bindings[0]),
                                bindings);

  // Return the `exports` object provided. It now has two new properties, which
  // are the functions we wish to expose to JavaScript.
  return exports;
}
