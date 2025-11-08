#include "stream.h"

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

long stream::Init(void *p_id){
	m_buffer[0] = 0;
	m_bufferpos = 0;
	m_bufferendpos = 0;
	m_zerofornulltermination = 0;
	m_streamid = p_id;
	m_streamloc = 0;
	return 0;
}
long stream::PutBytes(const char *p_bytes, long p_numbytes) {
	long len = putBytes(p_bytes, p_numbytes);
	if (len > 0) {
		m_streamloc += len;
		m_bufferpos = 0;
		m_bufferendpos = 0;
	}
	return len;
}
long stream::PeekBytes(long p_offset, char *p_bytes, long p_numbytes) {
	long needed = p_offset + p_numbytes;
	long err = MakeRoom(needed);
	if (err <= 0) {
		return err;
	}
	memcpy(p_bytes, m_buffer + m_bufferpos + p_offset, p_numbytes);
	return p_numbytes;
}
long stream::MakeRoom(long p_numbytes) {
	long leftOverBytes = m_bufferendpos - m_bufferpos;
	long needed = p_numbytes;
	if (leftOverBytes < needed) {
		if (needed > STREAM_BUFFERSIZE) {
			return STREAM_ERROR_BUFFERFULL;
		}
		//TODO: check if works ( hopefully it doesn't invalidate buffer )
		memcpy(m_buffer, m_buffer + m_bufferpos, leftOverBytes);
		m_bufferendpos = (short)leftOverBytes;
		m_bufferpos = 0;
		int iResult = getBytes(m_buffer + leftOverBytes, STREAM_BUFFERSIZE - leftOverBytes);
		if (iResult < 0) {
			return iResult;
		}
		m_streamloc += iResult;
		m_bufferendpos += iResult;
		m_buffer[m_bufferendpos] = 0; //null terminate, ok if its 1 byte past end of buffer
		return iResult;
	}
	return p_numbytes;
}
long stream::GetBytes(char *p_bytes, long p_numbytes) {
	int leftOverBytes = m_bufferendpos - m_bufferpos;
	int i = 0;
	if (leftOverBytes) {
		if (leftOverBytes > p_numbytes) {
			leftOverBytes = p_numbytes;
		}
		memcpy(p_bytes, m_buffer + m_bufferpos, leftOverBytes);
		m_bufferpos += leftOverBytes;
		i += leftOverBytes;
	}
	while (i < p_numbytes) {
		m_bufferpos = 0;
		m_bufferendpos = 0;
		int iResult = getBytes(m_buffer, sizeof(m_buffer));
		if (iResult < 0) { 
			return iResult;
		}
		m_buffer[iResult] = 0; //null terminate, ok if its 1 byte past buffer
		m_bufferendpos = iResult;
		m_streamloc += iResult;
		if (iResult == 0) {
			return i;  //no bytes are read, could be end of stream
		}
		leftOverBytes = iResult > (p_numbytes - i) ? (p_numbytes - i) : iResult;
		memcpy(p_bytes + i, m_buffer + m_bufferpos, leftOverBytes);
		m_bufferpos += leftOverBytes;
		i += leftOverBytes;
	}
	return i;
}

long stream::Seek(char poscode, long offset) {
	long loc = offset;
	if(poscode=STREAM_SEEK_CUR){ //current
		loc = m_streamloc - m_bufferendpos + m_bufferpos + offset;
	}
	if (loc < 0) {
		return STREAM_ERROR_OVERFLOW;
	}

	if (loc < m_streamloc - (long)m_bufferendpos || loc > m_streamloc) {
		long err = seek(loc);
		if (err < 0) {
			return err; //maybe error on unseekable streams
		}
		m_streamloc = loc;
		m_bufferendpos = m_bufferpos = 0; //forces getBytes
		return 0; //no error
	}

	//within buffer
	m_bufferpos = (m_streamloc - loc)&0xffff;
	return 0;
}

long stream::Tell() {
	return m_streamloc - m_bufferendpos + m_bufferpos;
}

