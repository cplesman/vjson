#include "virtual_mem.h"
#include <memory.h>

unsigned char vmem_binCHToHex(unsigned char b) {
	if (b < 10)
	{
		return '0' + b;
	}
	return 'A' - 10 + b;
}
void vmem_binToHexString(char *result, const unsigned char *bin, int size) {
	int i;
	int len = size;
	for (i = len - 1; i >= 0; i--) {
		result[i * 2] = (char)vmem_binCHToHex(bin[i] >> 4);
		result[i * 2 + 1] = (char)vmem_binCHToHex(bin[i] & 0xf);
	}
	result[len * 2] = 0;
}

long vmem::extend() {
	//first create a memory block to hold the heap queue
	//so that if we need to free memory to mov heap queue
	vblock b;
	b.m_next = 0;
	b.m_size = VMEM_FILEBLOCKSIZE | 1/*set to used initialy*/;

	i64 endBlockLoc = m_size;
	long long err = openFileBlock(endBlockLoc, "w");// open_mem(filename, "w");
	if(err<0) return (long)err;
	err = m_fileInterface->write(&b, sizeof(vblock));
	if (err != sizeof(vblock)) {
		m_fileInterface->close();
		return err >=0  ?  VMEM_ERROR_FILEIO : (long)err;
	}
	err = writeBigEmptyBlock(VMEM_FILEBLOCKSIZE - sizeof(vblock)); //doesn't close on error
	m_fileInterface->close();
	if(err<0) return err; //return after closing file

	m_size += VMEM_FILEBLOCKSIZE;
	return freeBlock(endBlockLoc); //place block back in free list
}


long vmem::openFileBlock(i64 p_blockLoc, const char *flags){
	m_pathBuffer = m_dir;
	i64 upper = p_blockLoc >> VMEM_FILEBLOCKSHIFT;
	char hex[16];
	vmem_binToHexString(hex, (unsigned char*)&upper, sizeof(i64) - (VMEM_FILEBLOCKSHIFT / 8));
	m_pathBuffer += hex;
	long err = m_fileInterface->open(m_pathBuffer.str(), flags);
	return err <0 ? VMEM_ERROR_FILEIO : err;
}

#define VMEM_INITCHUNKSIZE		(VMEM_FILEBLOCKSIZE>>8)
long vmem::writeBigEmptyBlock(i64 p_size){
	//assume file is open already
	char *tmp = new char[VMEM_INITCHUNKSIZE];
	long long err;
	if (!tmp) {
		//m_fileInterface->close();
		return VMEM_ERROR_OOM;
	}
	memset(tmp, 0xef, VMEM_INITCHUNKSIZE);
	i64 i = 0;
	for (; i < p_size; i += VMEM_INITCHUNKSIZE) {
		i64 size = VMEM_INITCHUNKSIZE;
		if (i + VMEM_INITCHUNKSIZE > p_size) {
			size = p_size - i;
		}
		err = m_fileInterface->write(tmp, size);
		if (err != size) {
			delete [] tmp;
			//m_fileInterface->close();
			return VMEM_ERROR_FILEIO;
		}
	}
	delete[] tmp;
	return 0;
}
long vmem::loadFileBlock(char *p_block, i64 p_blockLoc){
	long e = openFileBlock(p_blockLoc, "rb"); if(e<0) return e;
	long long err = m_fileInterface->read(p_block, VMEM_FILEBLOCKSIZE);
	m_fileInterface->close();
	return err<0 ? VMEM_ERROR_FILEIO : err;
}
long vmem::saveFileBlock(char *p_block, i64 p_blockLoc){
	long e = openFileBlock(p_blockLoc, "wb");
	if(e<0) return e;
	//i64 loc = p_blockLoc & VMEM_FILEBLOCKMASK;
	i64 err = m_fileInterface->write(p_block, VMEM_FILEBLOCKSIZE);
	m_fileInterface->close();
	if (err != VMEM_FILEBLOCKSIZE) {
		return VMEM_ERROR_FILEIO;
	}
	return 0;
}
long vmem::readFileBlock(vcache **p_retBlock, i64 p_blockLoc){
	long key = getCacheKey(p_blockLoc);
	i64 cloc = p_blockLoc & (~(VMEM_FILEBLOCKSIZE - 1));
	vcache *citr = m_cache[key];
	//vcache *prev;
	while (citr) {
		if (citr->m_loc == cloc) {
			if (citr->m_hits < 0x1000)
				citr->m_hits++;
			*p_retBlock = citr;
			return 0; //no error
		}
		//prev = citr;
		citr = citr->m_next;
	}
	char *ba = new char[VMEM_FILEBLOCKSIZE];
	if (!ba) {
		return VMEM_ERROR_OOM;
	}
	long err = loadFileBlock(ba, p_blockLoc);
	if(err<0) {
		delete[] ba;
		//file closed by loadFileBlock()
		return err;
	}
	vcache *c = new vcache;
	if (!c) {
		delete[] ba;
		return VMEM_ERROR_OOM;
	}
	while (sizeof(vblock) + m_cacheSize > VMEM_MAXCACHESIZE) {
		err = removeLowestHitCache();
		if(err<0) {
			delete[] ba;
			//file closed by saveFileBlock in removeLowestHitCache()
			return err;
		}
	}
	c->m_next = m_cache[key];
	c->m_hits = 1;
	c->m_loc = cloc;
	c->m_mem = (ba);
	c->m_lockCount = 0;
	c->m_flags = 0;
	m_cache[key] = c;
	m_cacheSize += VMEM_FILEBLOCKSIZE + sizeof(vcache);
	*p_retBlock = c;

	return 0;
}
