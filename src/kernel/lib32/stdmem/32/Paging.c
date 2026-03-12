#include "stdmem.h"

// Coalesced from https://wiki.osdev.org/Paging

typedef struct PageDirEntry4MB{
	uint32_t ADDRHIGH		: 9;
	uint32_t RSVD			: 1;
	uint32_t ADDRLOW		: 7;
	// If PAT is supported, then PAT along with PCD and PWT shall indicate the memory caching type. 
	// Otherwise, it is reserved and must be set to 0.
	uint32_t PAT			: 1;
	// Available for Use
	uint32_t AVL			: 2;
	// Tells the processor not to invalidate the TLB entry corresponding to the page upon a MOV to CR3 instruction. 
	// Bit 7 (PGE) in CR4 must be set to enable global pages.
	uint32_t Global			: 1;
	// Stores the page size for that specific entry. 
	// If the bit is set, then the PDE maps to a page that is 4 MiB in size. 
	// Otherwise, it maps to a 4 KiB page table. Please note that 4-MiB pages require PSE to be enabled.
	uint32_t PageSize		: 1;
	// Is used to determine whether a page has been written to.
	uint32_t Dirty			: 1;
	// is used to discover whether a PDE or PTE was read during virtual address translation. 
	// If it has, then the bit is set, otherwise, it is not. 
	// Note that, this bit will not be cleared by the CPU, so that burden falls on the OS (if it needs this bit at all).
	uint32_t Accessed		: 1;
	// The 'Cache Disable' bit. 
	// If the bit is set, the page will not be cached. Otherwise, it will be.
	uint32_t CacheDisable	: 1;
	// Controls Write-Through' abilities of the page. 
	// If the bit is set, write-through caching is enabled. 
	// If not, then write-back is enabled instead.
	uint32_t WriteThrough	: 1;
	// the 'User/Supervisor' bit, controls access to the page based on privilege level. 
	// If the bit is set, then the page may be accessed by all; 
	// if the bit is not set, however, only the supervisor can access it. 
	// For a page directory entry, the user bit controls access to all the pages referenced by the page directory entry. 
	// Therefore if you wish to make a page a user page, you must set the user bit in the relevant page directory entry as well as the page table entry.
	uint32_t User_Supervisor: 1;
	// 'Read/Write' permissions flag. 
	// If the bit is set, the page is read/write. 
	// Otherwise when it is not set, the page is read-only. 
	// The WP bit in CR0 determines if this is only applied to userland, always giving the kernel write access (the default) or both userland and the kernel (see Intel Manuals 3A 2-20). 
	// The R/W bit of the parent tables is also checked: if any are 0, the page is treated as read-only.
	uint32_t Read_rite		: 1;
	// If the bit is set, the page is actually in physical memory at the moment. 
	// For example, when a page is swapped out, it is not in physical memory and therefore not 'Present'. 
	// If a page is called, but not present, a page fault will occur, and the OS should handle it.
	uint32_t Present		: 1;
}PACKEDSTRUCT PageDirEntry4MB;

typedef struct PageDirEntry4KB{
	uint32_t ADDRHIGH		: 19;
	// Available for Use
	uint32_t AVL			: 3;
	// Stores the page size for that specific entry. 
	// If the bit is set, then the PDE maps to a page that is 4 MiB in size. 
	// Otherwise, it maps to a 4 KiB page table. Please note that 4-MiB pages require PSE to be enabled.
	uint32_t PageSize		: 1;
	// Is used to discover whether a PDE or PTE was read during virtual address translation. 
	// If it has, then the bit is set, otherwise, it is not. 
	// Note that, this bit will not be cleared by the CPU, so that burden falls on the OS (if it needs this bit at all).
	uint32_t Accessed		: 1;
	// The 'Cache Disable' bit. 
	// If the bit is set, the page will not be cached. Otherwise, it will be.
	uint32_t CacheDisable	: 1;
	// Controls Write-Through' abilities of the page. 
	// If the bit is set, write-through caching is enabled. 
	// If not, then write-back is enabled instead.
	uint32_t WriteThrough	: 1;
	// the 'User/Supervisor' bit, controls access to the page based on privilege level. 
	// If the bit is set, then the page may be accessed by all; 
	// if the bit is not set, however, only the supervisor can access it. 
	// For a page directory entry, the user bit controls access to all the pages referenced by the page directory entry. 
	// Therefore if you wish to make a page a user page, you must set the user bit in the relevant page directory entry as well as the page table entry.
	uint32_t User_Supervisor: 1;
	// 'Read/Write' permissions flag. 
	// If the bit is set, the page is read/write. 
	// Otherwise when it is not set, the page is read-only. 
	// The WP bit in CR0 determines if this is only applied to userland, always giving the kernel write access (the default) or both userland and the kernel (see Intel Manuals 3A 2-20). 
	// The R/W bit of the parent tables is also checked: if any are 0, the page is treated as read-only.
	uint32_t ReadWrite		: 1;
	// If the bit is set, the page is actually in physical memory at the moment. 
	// For example, when a page is swapped out, it is not in physical memory and therefore not 'Present'. 
	// If a page is called, but not present, a page fault will occur, and the OS should handle it.
	uint32_t Present		: 1;
}PACKEDSTRUCT PageDirEntry4KB;

typedef struct PageTableEntry{
	uint32_t ADDRHIGH			: 19;
	// Availdable for Use
	uint32_t AVL				: 2;
	// Tells the processor not to invalidate the TLB entry corresponding to the page upon a MOV to CR3 instruction. 
	// Bit 7 (PGE) in CR4 must be set to enable global pages.
	uint32_t Global				: 1;
	
	uint32_t PageAttributeTable	: 1;
	// Is used to determine whether a page has been written to.
	uint32_t Dirty				: 1;
	// Is used to discover whether a PDE or PTE was read during virtual address translation. 
	// If it has, then the bit is set, otherwise, it is not. 
	// Note that, this bit will not be cleared by the CPU, so that burden falls on the OS (if it needs this bit at all).
	uint32_t Accessed			: 1;
	// The 'Cache Disable' bit. 
	// If the bit is set, the page will not be cached. Otherwise, it will be.
	uint32_t CacheDisable		: 1;
	// Controls Write-Through' abilities of the page. 
	// If the bit is set, write-through caching is enabled. 
	// If not, then write-back is enabled instead.
	uint32_t WriteThrough		: 1;
	// the 'User/Supervisor' bit, controls access to the page based on privilege level. 
	// If the bit is set, then the page may be accessed by all; 
	// if the bit is not set, however, only the supervisor can access it. 
	// For a page directory entry, the user bit controls access to all the pages referenced by the page directory entry. 
	// Therefore if you wish to make a page a user page, you must set the user bit in the relevant page directory entry as well as the page table entry.
	uint32_t User_Supervisor	: 1;
	// 'Read/Write' permissions flag. 
	// If the bit is set, the page is read/write. 
	// Otherwise when it is not set, the page is read-only. 
	// The WP bit in CR0 determines if this is only applied to userland, always giving the kernel write access (the default) or both userland and the kernel (see Intel Manuals 3A 2-20). 
	// The R/W bit of the parent tables is also checked: if any are 0, the page is treated as read-only.
	uint32_t ReadWrite			: 1;
	// If the bit is set, the page is actually in physical memory at the moment. 
	// For example, when a page is swapped out, it is not in physical memory and therefore not 'Present'. 
	// If a page is called, but not present, a page fault will occur, and the OS should handle it.
	uint32_t Present			: 1;
}PACKEDSTRUCT PageTableEntry;
