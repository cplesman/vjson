#include "virtual_mem.h"

void vmem::freeCache(vcache *c) {
	if (c->m_flags&VMEM_CACHEFLAG_DIRTY) {
		saveFileBlock(c->m_mem, c->m_loc);
	}
	m_cacheSize -= VMEM_FILEBLOCKSIZE + sizeof(vcache);
	delete [] c->m_mem;
	delete c;
}
void vmem::flushCache(vcache *c) {
	if (c->m_flags&VMEM_CACHEFLAG_DIRTY) {
		saveFileBlock(c->m_mem, c->m_loc);
		c->m_flags &= ~VMEM_CACHEFLAG_DIRTY;
	}
}

void vmem::removeLowestHitCache() {
	long i;
	vcache **lowest = 0;
	for (i = 0; i < VMEM_CACHETABLESIZE; i++) {
		vcache *itr = m_cache[i];
		vcache *prevItr = 0;
		while (itr) {
			if (itr->m_lockCount == 0) {
				if ((!lowest || (itr->m_hits < (*lowest)->m_hits))) {
					if (prevItr) lowest = &prevItr->m_next;
					else lowest = &m_cache[i];
				}
				if (itr->m_hits) {
					itr->m_hits--; //decrement all hits
				}
			}
			prevItr = itr;
			itr = itr->m_next;
		}
	}
	if (lowest) {
		vcache *c = *lowest;
		*lowest = (*lowest)->m_next;
		freeCache( c);
	}
	//else cahce is all in use
}


