// test_json.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#ifdef _WIN32
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "../src/vjson/json.h"

#include "../src/stream.h"
#include "../src/text_stream.h"
#include <cassert>

#include <stdio.h>
#include <stdlib.h>
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

	JsonMM() :vmem("json_vmem", new fileio()) {}
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

class printstream : public stream {
public:
    long getBytes(char* p_buffer, long numbytes) override {
        if(numbytes) {
            for(long i=0;i<numbytes;i++) {
                p_buffer[i] = ' ';
            }
        }
        return numbytes;
    }
    long putBytes(const char* p_buffer, long numbytes) override {
        long bi = 0;
        char tmp[32];
        while(bi<numbytes  ){
            long i=0;
            while(bi<numbytes && i<31){
                tmp[i++] = p_buffer[bi++];
            }
            tmp[i] = 0;
            printf("%s", tmp);
        }
        return bi;
    }
};

char jsontext[] =
"{\r\n"
"  'name': 'Test Object',\r\n"
"  'value': 123.456,\r\n"
"  'active': true,\r\n"
"  'active2': false,\r\n"
"  'active4': true,\r\n"
"  'active3': true,\r\n"
"  'anullvalue':null,\r\n"
"  'anundefinedvalue':undefined,\r\n"
"  'items': [\r\n"
"    'Item 0',{\r\n"
"       'name': 'Sub Object', 'value': 789, \r\n"
"       'name2': 'Sub Object2', 'value2': 789, \r\n"
"     },\r\n"
"    'Item 2',\r\n"
"    'Item 3',\r\n"
"    'Item 4'\r\n"
"  ]\r\n"
"}";

int main(){
	g_jsonMem = new JsonMM();

    text_stream ts(jsontext);
    printstream ps;

    g_jsonMem->Init();

	printf("free %lld bytes\n", g_jsonMem->CalculateFree());

    i64 jsonobj_root = ((JsonMM*)g_jsonMem)->m_globalObjLoc;
	if( jsonobj_root==0){
		int err = jsonobj::Create(&jsonobj_root);
		if(err<0){
			printf("Failed to create json object\n");
			return -1;
		}
		err = jsonobj::Load(jsonobj_root, &ts);
		if(err<0) printf("failed to load\r\n");
		((JsonMM*)g_jsonMem)->m_globalObjLoc = jsonobj_root;
	}

    ps.Init(0);
    JSON_send(&ps,jsonobj_root, 2);

    printf("\r\nobject at key 'items' = \n");
	_jsonobj* rootPtr = (_jsonobj*)g_jsonMem->Lock(jsonobj_root);
	i64 itemsLoc = ((jsonobj*)rootPtr)->Find("items");
	JSON_send(&ps, itemsLoc, 2);
	g_jsonMem->Unlock(jsonobj_root);

    // jsonobj_Delete(jsonobj_root);
	// ((JsonMM*)g_jsonMem)->m_globalObjLoc = 0;

#ifdef _WIN32
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtDumpMemoryLeaks();
#endif
	printf("\r\nfree %lld bytes\n", g_jsonMem->CalculateFree());

	g_jsonMem->Close();
	delete g_jsonMem;

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
