#include "text_stream.h"

long text_stream::getBytes(char *p_buffer, long  p_numbytes) { //gets the next (upto)256 bytes, returns number of bytes read, or negative for error
	int i;
	for (i = 0; i < p_numbytes; i++) {
		p_buffer[i] = (((char*)m_streamid) + itr)[i];
	}
	itr += p_numbytes;
	return p_numbytes;
}
long text_stream::putBytes(const char *p_buffer, long  p_numbytes) { //gets the next (upto)256 bytes, returns number of bytes read, or negative for error
	return 0;
}

