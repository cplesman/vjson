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
long vmem::Init(){
	ifile_attribute info = m_fileInterface->attributes(m_dir.str());
	long err =0;
	if (info == ifile_attribute::IFILE_ERROR) {
		if (m_fileInterface->mkdir(m_dir.str()) < 0) {
			err = VMEM_ERROR_PATHIO;
		}
	}
	else if (info != ifile_attribute::IFILE_ISDIR) {
		err = VMEM_ERROR_PATHIO;
	}
	if(err<0) return err;

	vcache *genBlock;
	err = readFileBlock(&genBlock, 0);
	if(err<0) {
		if (err!=VMEM_ERROR_FILEIO) {
			return err; //not a file io error
		}
		err = openFileBlock(0, "w"); if(err<0) return err;
		err = writeBigEmptyBlock(VMEM_FILEBLOCKSIZE);
		m_fileInterface->close();
		if(err<0) return err;

		err = readFileBlock(&genBlock, 0);  if(err<0) return err;

		vblock b;
		m_size = VMEM_FILEBLOCKSIZE; //size of initial block and also location for next extension
		b.m_size = VMEM_FILEBLOCKSIZE;
		b.m_next = 0;
		i64 size;
		size = WriteGenBlock(genBlock->m_mem);
		if (size < 0) {
			return (long)size; //error
		}
		m_start = size + (i64)sizeof(m_start) + (i64)sizeof(m_size);
		b.m_size -= m_start;
		memcpy(genBlock->m_mem + size, &m_start, (i64)sizeof(m_start) + sizeof(m_size));
		size += (i64)sizeof(m_start) + sizeof(m_size);
		memcpy(genBlock->m_mem + size, &b, sizeof(vblock));
		genBlock->m_flags |= VMEM_CACHEFLAG_DIRTY;
	}
	i64 size = ReadGenBlock(genBlock->m_mem);
	if (size < 0) {
		return (long)size; //error
	}
	memcpy(&m_start, genBlock->m_mem + size, (i64)sizeof(m_start) + sizeof(m_size));
	isOpen = true;
	return err;
}
long vmem::Close() {
	return destroy();
}
long vmem::destroy() {
	if (!isOpen) {
		return 0;
	}
	vcache *genBlock;
	long err = readFileBlock(&genBlock, 0); if(err<0) return err;

	i64 size = WriteGenBlock(genBlock->m_mem);
	if (size < 0) {
		return (long)size;
	}
	memcpy(genBlock->m_mem+ size, &m_start, (i64)sizeof(m_start) + sizeof(m_size));

	int i;
	for (i = 0; i < VMEM_CACHETABLESIZE; i++) {
		vcache *citr;
		while ((citr = m_cache[i])) {
			vcache *next = citr->m_next;
			long err = freeCache(citr);
			if(err<0) return err;
			m_cache[i] = next;
		}
	}
#ifdef _DEBUG
	assert(m_cacheSize == 0);
#endif
	isOpen = false;
	return 0;
}

i64 vmem::Alloc(i64 p_size) {
	i64 loc;
	loc = allocBlock(p_size);
	if (loc == 0) {
		long err = extend(); if(err<0) return err;
		loc = allocBlock( p_size);
		if (loc == 0) {
			return VMEM_ERROR_OOM;
		}
		//else failed to extend memory (could be because we ran out of drive space)
	}
	return loc;
}
long vmem::Free(i64 p_loc) {
	return freeBlock(p_loc);
}
i64 vmem::SizeOf(i64 loc) {
	vcache *cache;
	i64 err = readFileBlock(&cache, loc);
	if(err<0) return err;

	vblock *b = (vblock*)(cache->m_mem + (loc&VMEM_FILEBLOCKMASK));
	return b->m_size - sizeof(i64);
}
void *vmem::Lock(i64 loc,bool p_readonly) {
	vcache *cache;
	long err = readFileBlock(&cache, loc); if(err<0) return 0;

	vblock *b = (vblock*)(cache->m_mem + (loc&VMEM_FILEBLOCKMASK));
	void *lockedMem = (void*)(((i64*)b) + 1);

	cache->m_lockCount++;
	if(!p_readonly) cache->m_flags |= VMEM_CACHEFLAG_DIRTY;

	return lockedMem;
}
long vmem::Unlock(i64 loc) {
	vcache *cache;
	long err = readFileBlock(&cache, loc); if(err<0) return err;
#ifdef _DEBUG
	assert(cache->m_lockCount > 0);
#endif
	cache->m_lockCount--;
	//was already set to DIRTY in Lock()
	return err;
}


i64 vmem::CalculateFree() {
	i64 size = 0;
	if (m_start <= 0) {
		return 0;
	}

	vblock *b = readBlock(m_start);
	if (b == 0) {
		return VMEM_ERROR_FILEIO;
	}
	size += VMEM_BLOCKSIZE(b);

	while (b->m_next) {
		b = readBlock( b->m_next);
		if (b == 0) {
			return VMEM_ERROR_FILEIO;
		}
		size += VMEM_BLOCKSIZE(b);
	}

	return size;
}
