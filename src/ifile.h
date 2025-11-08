#pragma once

enum ifile_attribute {
	IFILE_ERROR = -1,
	IFILE_ISDIR = 0x4000,
	IFILE_ISFILE = 0
};
class ifile {
public:
	virtual long open(const char *p_path, const char *p_options) = 0;
	virtual long long read(void *p_buf, long long p_size) = 0;
	virtual long long write(const void *p_buf, long long p_size) = 0;
	virtual long long size() = 0;
	virtual void close() = 0;

	virtual ifile_attribute attributes(const char *p_path) = 0;
	virtual long mkdir(const char *p_path) = 0;

	virtual ~ifile() {  }
};

