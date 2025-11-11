#include <stdio.h>
#include <stdlib.h>
#include "vjson/json.h"

#include <node_api.h>

#define CHECK(expr) \
  { \
    if ((expr) == 0) { \
      fprintf(stderr, "%s:%d: failed assertion `%s'\n", __FILE__, __LINE__, #expr); \
      fflush(stderr); \
      abort(); \
    } \
  }

#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

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

class vjson_wrap{
public:
    static napi_value init(napi_env env, napi_callback_info info); //allocate and wrap oject
    static void freeMem(napi_env env, void* data, void* hint);          //called when object is garbage collected

    static napi_value flush(napi_env env, napi_callback_info info);

    static napi_value create_obj(napi_env env, napi_callback_info info);
    static void free_obj/*or array*/(napi_env env, void* data, void* hint);          //called when object is garbage collected

    static napi_value append_obj(napi_env env, napi_callback_info info);
    static napi_value delete_obj(napi_env env, napi_callback_info info);
    static napi_value update_obj(napi_env env, napi_callback_info info);
    static napi_value read_obj(napi_env env, napi_callback_info info);

    static napi_value append_array(napi_env env, napi_callback_info info);
    static napi_value delete_array(napi_env env, napi_callback_info info);
    static napi_value update_array(napi_env env, napi_callback_info info);
    static napi_value read_array(napi_env env, napi_callback_info info);
};
