#pragma once

#include "vblock.h"

#define VMEM_CACHETABLESIZE			0x100
#define VMEM_CACHETABLEMASK			(VMEM_CACHETABLESIZE-1)
#define VMEM_MAXCACHESIZE			2000000000 //2 gb
#define VMEM_CACHEFLAG_NOTLOADED	0x1
#define VMEM_CACHEFLAG_DIRTY		0x2

class vcache {
public:
	i64 m_loc;
	char *m_mem;
	short m_hits;
	short m_flags;
	long m_lockCount;
	vcache *m_next;
};
