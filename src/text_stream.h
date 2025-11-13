#pragma once
#include "stream.h"

class text_stream :public stream {
	long itr;
public:
	text_stream(const char *text/*storage memory*/) {
		Init(text);
	}
	void Init(const char *text/*storage memory*/) {
		itr = 0;
		stream::Init((void*)text);
	}
	
	long getBytes(char *p_buffer, long p_numbytes);
	long putBytes(const char *p_buffer, long p_numbytes);
};