#pragma once

#include "vcache.h"
#include "../ifile.h"
#include "../cstring.h"

#define VMEM_FILEBLOCKSHIFT		((i64)26)  //64mb not for holding videos
#define VMEM_FILEBLOCKSIZE		((i64)1<<VMEM_FILEBLOCKSHIFT)
#define VMEM_FILEBLOCKMASK		((VMEM_FILEBLOCKSIZE-1)&(ARCH_BOUNDARY_MASK))

#define VMEM_ERROR_FILEIO		-0XA
#define VMEM_ERROR_OOM  		-0XB
#define VMEM_ERROR_PATHIO  		-0XC

class vmem {
	//m_start and m_size must be together, m_start first
protected:
	i64 m_start;
	i64 m_size;

	bool isOpen;

	cstring m_dir;
	cstring m_pathBuffer;
	i64 m_cacheSize;
	long m_lastError;
	vcache* m_cache[VMEM_CACHETABLESIZE];

	ifile *m_fileInterface;

	i64 allocBlock(i64 p_size);
	long freeBlock(i64 p_blockLoc);
	vblock *readBlock(i64 p_blockLoc);

	long extend();

	long openFileBlock(i64 m_blockLoc, const char *flags);
	long writeBigEmptyBlock(i64 p_size);
	long loadFileBlock(char *p_block, i64 p_blockLoc);
	long saveFileBlock(char *p_block, i64 p_blockLoc);
	long readFileBlock(vcache **p_retBlock, i64 p_blockLoc);

	long getCacheKey(i64 p_loc) {
		return ((p_loc >> VMEM_FILEBLOCKSHIFT)&(VMEM_CACHETABLEMASK));
	}
	long freeCache(vcache *c);
	long flushCache(vcache *c);
	long removeLowestHitCache();
	long destroy();
public:
	vmem(const char *path, ifile*fileInterface/*vmem will be responsible for deletion*/);
	~vmem();

	//write and read returns number of bytes read/written, negative on error
	//virtually overload these functions to write/read initial settings
	virtual i64 WriteGenBlock(void *toMem);
	virtual i64 ReadGenBlock(const void *fromMem);

	long Init();
	long Close();

	long Flush();

	i64 Alloc(i64 p_size);
	long Free(i64 p_loc);
	void *Lock(i64 p_loc, bool p_readonly=false);
	long Unlock(i64 p_loc);

	i64 CalculateFree();

	i64 SizeOf(i64 p_loc); //same speed as Lock()
	i64 SizeOf(void *p_lockedPtr) {
		i64 size = ((*(((i64*)p_lockedPtr) - 1))&ARCH_BOUNDARY_MASK) - sizeof(i64);
		return size;
	}
};

class cachelock {
	i64 loc;
	vmem *db;
public:
	cachelock(void **data, vmem *db, i64 ploc) {
		loc = ploc;
		this->db = db;
		*data = db->Lock(ploc);
	}
	~cachelock() {
		db->Unlock(loc);
	}
};

