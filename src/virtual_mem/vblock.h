#pragma once

typedef long long i64;

#define ARCH_BOUNDARY_MASK (~(sizeof(i64)-1))
#define ARCH_BOUNDARY_LESS1 ((sizeof(i64)-1))

#define VMEM_BLOCKSIZE(b) ((b)->m_size&0xfffffffffffffff8)
#define VMEM_BLOCKSETFREE(b) ((b)->m_size&=~((i64)0x1))
#define VMEM_BLOCKISFREE(b) (!((b)->m_size&0x1))

class vblock {
public:
	i64 m_size, m_next;
};
