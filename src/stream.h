#pragma once
#pragma warning(disable: 26495)

#define STREAM_BUFFERSIZE		512

#define STREAM_ERROR_CLOSED		-1 //OR eof
#define STREAM_ERROR_OTHER		-2
#define STREAM_ERROR_OVERFLOW	-4
#define STREAM_ERROR_BUFFERFULL	-8 //matched with OUTORMEMORY

#define STREAM_SEEK_BEGIN		0
//#define STREAM_SEEK_END			1
#define STREAM_SEEK_CUR			2

class stream {
public:
	long GetBytes(char *p_buffer, long numbytes);
	long PeekBytes(long p_offset, char *p_bytes, long numbytes);
	long MakeRoom(long numbytes); //insure buffer is atleast numbytes, may reach eof (returns numbytes avail), max is BUFFERSIZE
	long PutBytes(const char *p_buffer, long numbytes);
	long Seek(char poscode, long offset);
	long Tell();
	long Init(void *id/*can be ptr to file, socket etc*/);
	void *m_streamid;
	char m_buffer[STREAM_BUFFERSIZE];
	unsigned short m_zerofornulltermination; //in case of text streams
	unsigned short m_bufferpos;
	unsigned int m_bufferendpos;
	long m_streamloc;

	unsigned long BytesLeftInBuffer() { return m_bufferendpos - m_bufferpos; }

	virtual long getBytes(char *p_buffer, long numbytes) = 0;
	virtual long putBytes(const char *p_buffer, long numbytes) = 0;
	virtual long seek(long offset) { return 0; }
	virtual long tell() { return m_streamloc; }
};


