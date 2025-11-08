#include "virtual_mem.h"

vblock *vmem::readBlock(i64 p_loc) {
	vcache *c;
	readFileBlock(&c, p_loc);
	return (vblock*)(c->m_mem + (p_loc&VMEM_FILEBLOCKMASK));
}
void vmem::freeBlock(i64 p_loc) {
	//caller must make sure block is not in use
	vcache *c;
	readFileBlock(&c, p_loc);
	vblock *b = (vblock*)(c->m_mem + (p_loc&VMEM_FILEBLOCKMASK));
	VMEM_BLOCKSETFREE(b);
	c->m_flags |= VMEM_CACHEFLAG_DIRTY;

	i64 heapBlockSize = b->m_size; //save for compare
	i64 prevLoc = 0;
	i64 itrLoc = m_start;
	if (itrLoc) {
		b = readBlock(itrLoc);
		while (heapBlockSize > b->m_size) {
			prevLoc = itrLoc;
			itrLoc = b->m_next;
			if (b->m_next) {
				b = readBlock(itrLoc);
			}
			else {
				break; //reached end
			}
		}
	}

	//cache may have changed so we need to reload block
	readFileBlock(&c, p_loc);
	b = (vblock*)(c->m_mem + (p_loc&VMEM_FILEBLOCKMASK));
	c->m_flags |= VMEM_CACHEFLAG_DIRTY;
	if (!prevLoc) {
		b->m_next = m_start;
		m_start = p_loc;
	}
	else {
		b->m_next = itrLoc/*maybe null or zero*/;
		readFileBlock(&c, prevLoc);
		vblock *b = (vblock*)(c->m_mem + (prevLoc&VMEM_FILEBLOCKMASK));
		b->m_next = p_loc;
		c->m_flags |= VMEM_CACHEFLAG_DIRTY;
	}
}

i64 vmem::allocBlock(i64 pSize) {
	pSize += sizeof(i64)/*i64 size*/ + ARCH_BOUNDARY_LESS1;
	pSize &= ARCH_BOUNDARY_MASK;// arch byte boundary
	i64 prevLoc = 0;
	i64 itrLoc = m_start;
	vblock *itr = 0;
	vcache *itrCache = 0;
	if (itrLoc) {
		readFileBlock(&itrCache, itrLoc);
		itr = (vblock*)(itrCache->m_mem + (itrLoc&VMEM_FILEBLOCKMASK));
		while (pSize > itr->m_size) {
			prevLoc = itrLoc;
			itrLoc = itr->m_next;
			if (itr->m_next) {
				readFileBlock(&itrCache, itrLoc);
				itr = (vblock*)(itrCache->m_mem + (itrLoc&VMEM_FILEBLOCKMASK));
			}
			else {
				break;
			}
		}
	}

	if (!itrLoc) {
		return 0; //out of memory/no block available
	}

	i64 next = itr->m_next;
	if (!prevLoc) {
		m_start = itr->m_next;
	}
	else {
		readFileBlock(&itrCache, prevLoc);
		itr = (vblock*)(itrCache->m_mem + (prevLoc&VMEM_FILEBLOCKMASK));
		itr->m_next /*prev->m_next*/ = next;
		itrCache->m_flags |= VMEM_CACHEFLAG_DIRTY;
		//re load itrLoc cause it may have changed
		readFileBlock(&itrCache, itrLoc);
		itr = (vblock*)(itrCache->m_mem + (itrLoc&VMEM_FILEBLOCKMASK));
	}

	itr->m_size |= 1; //set to used so free doesnt combine blocks again (TODO: impliment combining blocks)
	itrCache->m_flags |= VMEM_CACHEFLAG_DIRTY;

	if (itr->m_size > pSize + (i64)sizeof(vblock) /*some data is included in memblock since mNext is not used in used blocks*/) {
		//room to split
		itr->m_size -= pSize;
		i64 bSize = VMEM_BLOCKSIZE(itr);// itr->m_size;
		i64 nB = itrLoc + bSize;
		readFileBlock(&itrCache, nB);
		itr = (vblock*)(itrCache->m_mem + (nB&VMEM_FILEBLOCKMASK));
		itr->m_next = 0;
		itr->m_size = pSize | 1; //pSize was already adjusted to inlclude m_size, this also sets block as used
		itrCache->m_flags |= VMEM_CACHEFLAG_DIRTY;

		freeBlock(itrLoc); //place block back in free list

		itrLoc = nB;
		//err = vmem_ReadBlock(&itrCache, p_heap, nB); //load newly created block
		//if (err < 0) {
		//	return err;
		//}
		//itr = itrCache->m_mem;
	}

	return itrLoc;
}
