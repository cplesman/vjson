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

void vmem::extend() {
	//first create a memory block to hold the heap queue
	//so that if we need to free memory to mov heap queue
	vblock b;
	b.m_next = 0;
	b.m_size = VMEM_FILEBLOCKSIZE | 1/*set to used initialy*/;

	i64 endBlockLoc = m_size;
	openFileBlock(endBlockLoc, "w");// open_mem(filename, "w");
	long long err;
	err = m_fileInterface->write(&b, sizeof(vblock));
	if (err != sizeof(vblock)) {
		m_fileInterface->close();
		throw vmem_exception("FILEIO");
	}
	writeBigEmptyBlock(VMEM_FILEBLOCKSIZE - sizeof(vblock));
	m_fileInterface->close();

	m_size += VMEM_FILEBLOCKSIZE;
	freeBlock(endBlockLoc); //place block back in free list
}


void vmem::openFileBlock(i64 p_blockLoc, const char *flags){
	m_pathBuffer = m_dir;
	i64 upper = p_blockLoc >> VMEM_FILEBLOCKSHIFT;
	char hex[16];
	vmem_binToHexString(hex, (unsigned char*)&upper, sizeof(i64) - (VMEM_FILEBLOCKSHIFT / 8));
	m_pathBuffer += hex;
	long err = m_fileInterface->open(m_pathBuffer.str(), flags);
	if (err<0) {
		throw vmem_exception("FILEIO");
	}
}

#define VMEM_INITCHUNKSIZE		(VMEM_FILEBLOCKSIZE>>8)
void vmem::writeBigEmptyBlock(i64 p_size){
	char *tmp = new char[VMEM_INITCHUNKSIZE];
	long long err;
	if (!tmp) {
		m_fileInterface->close();
		throw vmem_exception("OOM");
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
			m_fileInterface->close();
			throw vmem_exception("FILEIO");
		}
	}
	delete[] tmp;
}
void vmem::loadFileBlock(char *p_block, i64 p_blockLoc){
	openFileBlock(p_blockLoc, "rb");
	long long err = m_fileInterface->read(p_block, VMEM_FILEBLOCKSIZE);
	m_fileInterface->close();
	if (err != VMEM_FILEBLOCKSIZE) {
		throw new vmem_exception("FILEIO");
	}
}
void vmem::saveFileBlock(char *p_block, i64 p_blockLoc){
	openFileBlock(p_blockLoc, "wb");
	//i64 loc = p_blockLoc & VMEM_FILEBLOCKMASK;
	i64 err = m_fileInterface->write(p_block, VMEM_FILEBLOCKSIZE);
	m_fileInterface->close();
	if (err != VMEM_FILEBLOCKSIZE) {
		throw vmem_exception("FILEIO");
	}
}
void vmem::readFileBlock(vcache **p_retBlock, i64 p_blockLoc){
	long key = getCacheKey(p_blockLoc);
	i64 cloc = p_blockLoc & (~(VMEM_FILEBLOCKSIZE - 1));
	vcache *citr = m_cache[key];
	//vcache *prev;
	while (citr) {
		if (citr->m_loc == cloc) {
			if (citr->m_hits < 0x1000)
				citr->m_hits++;
			*p_retBlock = citr;
			return;
		}
		//prev = citr;
		citr = citr->m_next;
	}
	char *ba = new char[VMEM_FILEBLOCKSIZE];
	if (!ba) {
		m_fileInterface->close();
		throw vmem_exception("OOM");
	}
	try {
		loadFileBlock(ba, p_blockLoc);
	}
	catch (vmem_exception e) {
		delete[] ba;
		//file closed by loadFileBlock()
		throw e;
	}
	vcache *c = new vcache;
	if (!c) {
		delete[] ba;
		m_fileInterface->close();
		throw vmem_exception("OOM");
	}
	while (sizeof(vblock) + m_cacheSize > VMEM_MAXCACHESIZE) {
		try {
			removeLowestHitCache();
		}
		catch (vmem_exception e) {
			delete[] ba;
			//file closed by saveFileBlock in removeLowestHitCache()
			throw e;
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
}
