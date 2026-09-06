#pragma once

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/math/math.h"
#include "kernel/libcrt/def.h"
#include "compile/toolchain/gnu-efi-build/x86_64/include/efi/efi.h"
#include "compile/toolchain/gnu-efi-build/x86_64/include/efi/efilib.h"

#define PagingTableLength				(512)
#define DIRECT_MAP_BASE					0xFFFF800000000000ULL
#define PHYS_TO_VIRT(phys) ((void *)((uint64_t)(phys) + DIRECT_MAP_BASE))
#define SearchPageTableAddress(Table, Address)		\
	{for(cc = 0; cc < PagingTableLength; ++cc){if(!((Table)[cc].Present) || ((Table)[cc].PhysicalAddress == Address)){break;}}}

// Page Map Level 5 Entry (PML5E)
// Points to a PML4 table in 57-bit virtual addressing mode
typedef struct{
	//	1 = Present in memory
    uint64_t Present				: 1;  
	//	0 = Read-only, 1 = Read/Write
    uint64_t ReadWrite				: 1;  
	//	0 = Supervisor (Kernel), 1 = User
    uint64_t UserSupervisor			: 1;  
	//	Cache write-through policy
    uint64_t PageLevelWriteThrough	: 1; 
	//	1 = Disable caching for table
    uint64_t PageLevelCacheDisable	: 1; 
	//	Set by CPU when traversed
    uint64_t Accessed				: 1;  
	//	Free for OS use
	//! Custom Flag
    uint64_t UsedPage				: 1;   
	//	Must be 0
    uint64_t 						: 1;  
	//	Free for OS use
    uint64_t Available2				: 4;  
	//	Physical address of PML4 (4KB aligned)
    uint64_t PhysicalAddress		: 40; 
	//	Free for OS use
    uint64_t Available3				: 11; 
	//	NX Bit (1 = Disallow code execution)
    uint64_t ExecuteDisable			: 1;  
}__packed PML5Entry;

// Page Map Level 4 Entry (PML4E)
// Points to a Page Directory Pointer Table (PDPT)
typedef struct{
	//	1 = Present in memory
    uint64_t Present				: 1;  
	//	0 = Read-only, 1 = Read/Write
    uint64_t ReadWrite				: 1;  
	//	0 = Supervisor (Kernel), 1 = User
    uint64_t UserSupervisor			: 1;  
	//	Cache write-through policy
    uint64_t PageLevelWriteThrough	: 1;  
	//	1 = Disable caching for table
    uint64_t PageLevelCacheDisable	: 1;  
	//	Set by CPU when traversed
    uint64_t Accessed				: 1;  
	//	Free for OS use
	//! Custom Flag
    uint64_t UsedPage				: 1;  
	//	Must be 0
    uint64_t Reserved1				: 1;  
	//	Free for OS use
    uint64_t Available2				: 4;  
	//	Physical address of PDPT (4KB aligned)
    uint64_t PhysicalAddress		: 40; 
	//	Free for OS use
    uint64_t Available3				: 11; 
	//	NX Bit
    uint64_t ExecuteDisable			: 1;  
}__packed PML4Entry;

typedef struct{
	//	1 = Present
    uint64_t Present				: 1;  
	//	0 = Read-only, 1 = Read/Write
    uint64_t ReadWrite				: 1;  
	//	0 = Supervisor, 1 = User
    uint64_t UserSupervisor			: 1;  
	//	Cache write-through
    uint64_t PageLevelWriteThrough	: 1;  
	//	Cache disable
    uint64_t PageLevelCacheDisable	: 1;  
	//	Accessed flag
    uint64_t Accessed				: 1;  
	//	Free for OS use
	//! Custom Flag
    uint64_t UsedPage				: 1;  
	//	MUST BE 0 (Points to Page Directory)
    uint64_t PageSize				: 1;  
	//	Free for OS use
    uint64_t Available2				: 4;  
	//	Physical address of PD (4KB aligned)
    uint64_t PhysicalAddress		: 40; 
	//	Free for OS use
    uint64_t Available3				: 11; 
	//	NX Bit
    uint64_t ExecuteDisable			: 1;  
}__packed PDPTEntryDirectory;
typedef struct{
	//	1 = Present
    uint64_t Present				: 1;  
	//	0 = Read-only, 1 = Read/Write
    uint64_t ReadWrite				: 1;  
	//	0 = Supervisor, 1 = User
    uint64_t UserSupervisor			: 1;  
	//	Cache write-through
    uint64_t PageLevelWriteThrough	: 1;  
	//	Cache disable
    uint64_t PageLevelCacheDisable	: 1;  
	//	Accessed flag
    uint64_t Accessed				: 1;  
	//	1 = Written to
    uint64_t Dirty					: 1;  
	//	MUST BE 1 (1 GB Large Page)
    uint64_t PageSize				: 1;  
	//	1 = Global page (prevents TLB flush)
    uint64_t Global					: 1;  
	//	Free for OS use
	//! Custom Flag
	uint64_t UsedPage				: 1;
    uint64_t Available1				: 2;  
	//	PAT bit for 1GB pages
    uint64_t PageAttributeTable		: 1;  
	//	Reserved (Must be 0)
    uint64_t 						: 17; 
	//	Physical base address (1GB aligned)
    uint64_t PhysicalAddress		: 22; 
	//	Free for OS use
    uint64_t Available2				: 7;  
	//	Memory Protection Keys (PKRU)
    uint64_t ProtectionKey			: 4;  
	//	NX Bit
    uint64_t ExecuteDisable			: 1;  
}__packed PDPTEntry1GB;

typedef struct{
	//	1 = Present
    uint64_t Present				: 1;  
	//	0 = Read-only, 1 = Read/Write
    uint64_t ReadWrite				: 1;  
	//	0 = Supervisor, 1 = User
    uint64_t UserSupervisor			: 1;  
	//	Cache write-through
    uint64_t PageLevelWriteThrough	: 1;  
	//	Cache disable
    uint64_t PageLevelCacheDisable	: 1;  
	//	Accessed flag
    uint64_t Accessed				: 1;  
	//	Free for OS use
	//! Custom Flag
    uint64_t UsedPage				: 1;  
	//	MUST BE 0 (Points to Page Table)
    uint64_t PageSize				: 1;  
	//	Free for OS use
    uint64_t Available2				: 4;  
	//	Physical address of PT (4KB aligned)
    uint64_t PhysicalAddress		: 40; 
	//	Free for OS use
    uint64_t Available3				: 11; 
	//	NX Bit
    uint64_t ExecuteDisable			: 1;  
}__packed PageDirectoryEntry;
typedef struct{
	//	1 = Present
    uint64_t Present				: 1;  
	//	0 = Read-only, 1 = Read/Write
    uint64_t ReadWrite				: 1;  
	//	0 = Supervisor, 1 = User
    uint64_t UserSupervisor			: 1;  
	//	Cache write-through
    uint64_t PageLevelWriteThrough	: 1;  
	//	Cache disable
    uint64_t PageLevelCacheDisable	: 1;  
	//	Accessed flag
    uint64_t Accessed				: 1;  
	//	1 = Written to
    uint64_t Dirty					: 1;  
	//	MUST BE 1 (2 MB Large Page)
    uint64_t PageSize				: 1;  
	//	1 = Global page
    uint64_t Global					: 1;  
	//	Free for OS use
	//! Custom Flag
	uint64_t UsedPage				: 1;
    uint64_t Available1				: 2;  
	//	PAT bit for 2MB pages
    uint64_t PageAttributeTable		: 1;  
	//	Reserved (Must be 0)
    uint64_t 						: 8;  
	//	Physical base address (2MB aligned)
    uint64_t PhysicalAddress		: 31; 
	//	Free for OS use
    uint64_t Available2				: 7;  
	//	Memory Protection Keys
    uint64_t ProtectionKey			: 4;  
	//	NX Bit
    uint64_t ExecuteDisable			: 1;  
}__packed PageDirectoryEntry2MB;

// Page Table Entry (PTE)
// Final translation entry mapping a standard 4 KB physical frame
typedef struct{
	//	1 = Present
    uint64_t Present				: 1;  
	//	0 = Read-only, 1 = Read/Write
    uint64_t ReadWrite				: 1;  
	//	0 = Supervisor, 1 = User
    uint64_t UserSupervisor			: 1;  
	//	Cache write-through
    uint64_t PageLevelWriteThrough	: 1;  
	//	Cache disable
    uint64_t PageLevelCacheDisable	: 1;  
	//	1 = Read/written to by CPU
    uint64_t Accessed				: 1;  
	//	1 = Written to by CPU
    uint64_t Dirty					: 1;  
	//	PAT bit for 4KB pages
    uint64_t PageAttributeTable		: 1;  
	//	1 = Global page
    uint64_t Global					: 1;  
	//	Free for OS use
	//! Custom Flag
	uint64_t UsedPage				: 1;
    uint64_t Available1				: 2;  
	//	Physical frame address (4KB aligned)
    uint64_t PhysicalAddress		: 40; 
	//	Free for OS use
    uint64_t Available2				: 7;  
	//	Protection Keys
    uint64_t ProtectionKey			: 4;  
	//	NX Bit
    uint64_t ExecuteDisable			: 1;  
}__packed PageTableEntry4KB;

// Table containers wrapping 512 entries per 4KB table page
typedef __align(4096) PML5Entry PML5Table[PagingTableLength];
typedef __align(4096) PML4Entry PML4Table[PagingTableLength];
typedef union{
	PDPTEntryDirectory		directories[PagingTableLength];
	PDPTEntry1GB			pages1GB[PagingTableLength];
}__align(4096) PDPTTable;
typedef union{
	PageDirectoryEntry	tables[PagingTableLength];
	PageDirectoryEntry2MB	pages2MB[PagingTableLength];
}__align(4096) PageDirectory;
typedef __align(4096) PageTableEntry4KB PageTable[PagingTableLength];

#define VirtualAddressAToU64(VA)((uint64_t)(((int64_t)( \
        ((uint64_t)(VA).Offset)		|					\
        (((uint64_t)(VA).L1) << 12)	|					\
        (((uint64_t)(VA).L2) << 21)	|					\
        (((uint64_t)(VA).L3) << 30)	|					\
        (((uint64_t)(VA).L4) << 39)	|					\
        (((uint64_t)(VA).L5) << 48)) << 7) >> 7))
typedef struct{
	uint64_t	Offset	: 12;
	uint64_t	L1		: 9;
	uint64_t	L2		: 9;
	uint64_t	L3		: 9;
	uint64_t	L4		: 9;
	uint64_t	L5		: 9;
	uint64_t			: 7;
}__packed VirtualAddress;

typedef struct{
	uint64_t	L1			: 9, 
				EnableL1	: 1;

	uint64_t	L2			: 9, 
				EnableL2	: 1;

	uint64_t	L3			: 9, 
				EnableL3	: 1;

	uint64_t	L4			: 9, 
				EnableL4	: 1;

	uint64_t	L5			: 9, 
				EnableL5	: 1;
	uint64_t				: 14;
}/*__packed*/ PageCoordinate;


#define PAGE_SIZE3					nGB(1)
#define PAGE_SIZE2					nMB(2)
#define PAGE_SIZE					nKB(4)
#define LowestLocal					0

enumdef(uint8_t, PageAllocationType){PAFNoFree = 0x1, PAFNoTouch, PAFCachable = 0x2, PAFUsed = 0x4};

enumdef(uint32_t, PageAllocationFlags){
	ReadOnly = 0x0, ReadWritable = 0x1, 
	UserMode = 0x2, SupervisorMode = 0x4, 
	PageLevelWriteThroughEnable = 0x8, 
	PageLevelCacheEnable = 0x10, 
	PageAttributeTableEnable = 0x20, 
	GlobalEnable = 0x40, ExecuteEnable = 0x80
};

//	Each Bit Corresponds to 32 bytes
//	.5% of each Gigabyte is used for the PageAllocationTable
typedef struct{
	GenericReference	Parent;
	PageAllocationType	Type;
	//	Have a uint16_t worth of Padding
	PageCoordinate		Coordinate;
	//	Determines the N Entries till the Tip of the Chain.
	uint16_t			Local;
}PageAllocationState;

LibAPI uint32_t GetMaxPhysicalAddressBits(void);

LibAPI void InvalidatePage(void *Virtual);
LibAPI void InvalidatePages(void *Virtual, uint64_t Bytes);

LibAPI void *__sysvabi MapPTVirtual(void *Physical);
LibAPI void *MapPhysical(void *Virtual);
LibAPI void *WalkPageTreeByVirtual(void *Virtual, uint32_t *Level);
LibAPI bool RepointVirtualAddress(void *Virtual, void *NewPhysical);
LibAPI void *WalkPageTreeByPhysical(void *Physical, uint32_t *Level);
LibAPI void *WalkPageTree(uint32_t *Level, PageCoordinate Coordinate);

LibAPI extern bool __sysvabi IsLA57Supported(void);
LibAPI extern void __sysvabi EnableLA57(void);
LibAPI extern bool __sysvabi IsLA57Enabled(void);
LibAPI extern void *__sysvabi GetPageTable(bool *Level5);
LibAPI extern uint32_t __sysvabi LoadPageTree(void *PagingTree, void(*PTR)(void), uint64_t DataSegment, uint64_t CodeSegment, bool64_t Level5);
LibAPI uint64_t TotalMemorySize(void);
LibAPI bool MemoryIsMapped(void *Ptr);
LibAPI uint64_t TotalMappedMemory(void *ptr);

LibAPI void FreePages(void *Virtual);
LibAPI void *MapVirtual(void *Physical);
LibAPI bool InitialiseAllocationState(void *D_, uint64_t N, uint64_t TotalMemory);
LibAPI void *AllocatePages(void *Base, uint64_t NBytes, PageAllocationFlags Flags, uint8_t ProtectionKey);
LibAPI void *AllocatePagesFromRange(void *Base, uint64_t Limit, uint64_t NBytes, PageAllocationFlags Flags, uint8_t ProtectionKey);
LibAPI void *AllocateAlignedPagesFromRange(void *Base, uint64_t Limit, uint64_t NBytes, PageAllocationFlags Flags, uint8_t ProtectionKey, uint32_t Align);