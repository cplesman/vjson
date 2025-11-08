#include "virtual_mem.h"
#ifdef _DEBUG
#include <assert.h>
#endif
#include <string.h>

vmem::vmem(const char *path, ifile*fileInterface) {
	m_fileInterface = fileInterface;
	m_dir = path;
	m_dir += "/";

	m_cacheSize = 0;
	long i = 0;
	for (; i < VMEM_CACHETABLESIZE; i++) {
		m_cache[i] = 0;
	}
}
vmem::~vmem() {
	destroy();
	m_dir.Destroy();
	m_pathBuffer.Destroy();
	delete m_fileInterface;
}

i64 vmem::WriteGenBlock(void *toMem) {
	//virtually overload these functions to write/read initial settings
	return 0;
}
i64 vmem::ReadGenBlock(const void*fromMem) {
	//virtually overload these functions to write/read initial settings
	return 0;
}
void vmem::Init(){
	ifile_attribute info = m_fileInterface->attributes(m_dir.str());
	if (info == ifile_attribute::IFILE_ERROR) {
		if (m_fileInterface->mkdir(m_dir.str()) < 0) {
			throw vmem_exception("PATHIO");
		}
	}
	else if (info != ifile_attribute::IFILE_ISDIR) {
		throw vmem_exception("PATHIO");
	}

	vcache *genBlock;
	try {
		readFileBlock(&genBlock, 0);
	}
	catch (vmem_exception e) {
		if (strcmp(e.Msg(), "FILEIO")) {
			throw e; //not a file io error
		}
		openFileBlock(0, "w");
		writeBigEmptyBlock(VMEM_FILEBLOCKSIZE);
		m_fileInterface->close();

		readFileBlock(&genBlock, 0);

		vblock b;
		m_size = VMEM_FILEBLOCKSIZE; //size of initial block and also location for next extension
		b.m_size = VMEM_FILEBLOCKSIZE;
		b.m_next = 0;
		i64 size;
		size = WriteGenBlock(genBlock->m_mem);
		if (size < 0) {
			throw vmem_exception("FILEIO");
		}
		m_start = size + (i64)sizeof(m_start) + (i64)sizeof(m_size);
		b.m_size -= m_start;
		memcpy(genBlock->m_mem + size, &m_start, (i64)sizeof(m_start) + sizeof(m_size));
		size += (i64)sizeof(m_start) + sizeof(m_size);
		memcpy(genBlock->m_mem + size, &b, sizeof(vblock));
	}
	i64 size = ReadGenBlock(genBlock->m_mem);
	if (size < 0) {
		throw vmem_exception("FILEIO");
	}
	memcpy(&m_start, genBlock->m_mem + size, (i64)sizeof(m_start) + sizeof(m_size));
	isOpen = true;
}
void vmem::Close() {
	destroy();
}
void vmem::destroy() {
	if (!isOpen) {
		return;
	}
	vcache *genBlock;
	readFileBlock(&genBlock, 0);

	i64 size = WriteGenBlock(genBlock->m_mem);
	if (size < 0) {
		throw vmem_exception("FILEIO");
	}
	memcpy(genBlock->m_mem+ size, &m_start, (i64)sizeof(m_start) + sizeof(m_size));

	int i;
	for (i = 0; i < VMEM_CACHETABLESIZE; i++) {
		vcache *citr;
		while ((citr = m_cache[i])) {
			vcache *next = citr->m_next;
			freeCache(citr);
			m_cache[i] = next;
		}
	}
#ifdef _DEBUG
	assert(m_cacheSize == 0);
#endif
	isOpen = false;
}

i64 vmem::Alloc(i64 p_size) {
	i64 loc;
	loc = allocBlock(p_size);
	if (loc == 0) {
		extend();
		loc = allocBlock( p_size);
		if (loc == 0) {
			throw vmem_exception("OOHD"); //memory is larger than a FILEBLOCKSIZE? ran out of harddrive space?
		}
		//else failed to extend memory (could be because we ran out of drive space)
	}
	return loc;
}
void vmem::Free(i64 p_loc) {
	freeBlock(p_loc);
}
i64 vmem::SizeOf(i64 loc) {
	vcache *cache;
	readFileBlock(&cache, loc);

	vblock *b = (vblock*)(cache->m_mem + (loc&VMEM_FILEBLOCKMASK));
	return b->m_size - sizeof(i64);
}
void *vmem::Lock(i64 loc) {
	vcache *cache;
	readFileBlock(&cache, loc);

	vblock *b = (vblock*)(cache->m_mem + (loc&VMEM_FILEBLOCKMASK));
	void *lockedMem = (void*)(((i64*)b) + 1);

	cache->m_lockCount++;
	cache->m_flags |= VMEM_CACHEFLAG_DIRTY;

	return lockedMem;
}
void vmem::Unlock(i64 loc) {
	vcache *cache;
	readFileBlock(&cache, loc);
#ifdef _DEBUG
	assert(cache->m_lockCount > 0);
#endif
	cache->m_lockCount--;
	//was already set to DIRTY in Lock()
}


i64 vmem::CalculateFree() {
	i64 size = 0;
	if (m_start <= 0) {
		return 0;
	}

	vblock *b = readBlock(m_start);
	if (b == 0) {
		return -1;
	}
	size += VMEM_BLOCKSIZE(b);

	while (b->m_next) {
		b = readBlock( b->m_next);
		if (b == 0) {
			return -1;
		}
		size += VMEM_BLOCKSIZE(b);
	}

	return size;
}
