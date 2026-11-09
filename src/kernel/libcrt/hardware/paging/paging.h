#pragma once

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/math/math.h"
#include "kernel/libcrt/def.h"
#include "compile/toolchain/gnu-efi-build/x86_64/include/efi/efi.h"
// #include "compile/toolchain/gnu-efi-build/x86_64/include/efi/efilib.h"

#define PagingTableLength	(512ULL)
#define PagingTableMax		(511ULL)
#define DIRECT_MAP_BASE		0xFFFF800000000000ULL
// #define PHYS_TO_VIRT(phys) ((void *)((uint64_t)(phys) + DIRECT_MAP_BASE))
#define SearchPageTableAddress(Table, Address)		\
	{for(cc = 0; cc < PagingTableLength; ++cc){if(!((Table)[cc].Present) || ((Table)[cc].PhysicalAddress == Address)){break;}}}

// Page Map Level 5 Entry (PML5E)
// Points to a PML4 table in 57-bit virtual addressing mode
typedef volatile struct{
	//	1 = Present in memory
	//*	The Page is Allocated and Usable
	uint64_t Present				: 1;  
	//	0 = Read-only, 1 = Read/Write
	uint64_t ReadWrite				: 1;  
	//	0 = Supervisor (Kernel), 1 = User
	uint64_t UserMode			: 1;  
	//	Cache write-through policy
	uint64_t PageLevelWriteThrough	: 1; 
	//	1 = Disable caching for table
	uint64_t PageLevelCacheDisable	: 1; 
	//	Set by CPU when traversed
	uint64_t Accessed				: 1;  
	//	Free for OS use
	//! Custom Flag
	//*	The Page is Mapped to Physical Memory
	uint64_t PageInMemory			: 1;
	//	Must be 0
	uint64_t Reserved1				: 1;  
	//	Free for OS use
	uint64_t Available2				: 4;  
	//	Physical address of PML4 (4KB aligned)
	uint64_t PhysicalAddress		: 51; 
	//	Free for OS use
	//	NX Bit (1 = Disallow code execution)
	uint64_t ExecuteDisable			: 1;  
}__packed PML5Entry;

// Page Map Level 4 Entry (PML4E)
// Points to a Page Directory Pointer Table (PDPT)
typedef volatile struct{
	//	1 = Present in memory
	//*	The Page is Allocated and Usable
	uint64_t Present				: 1;  
	//	0 = Read-only, 1 = Read/Write
	uint64_t ReadWrite				: 1;  
	//	0 = Supervisor (Kernel), 1 = User
	uint64_t UserMode			: 1;  
	//	Cache write-through policy
	uint64_t PageLevelWriteThrough	: 1;  
	//	1 = Disable caching for table
	uint64_t PageLevelCacheDisable	: 1;  
	//	Set by CPU when traversed
	uint64_t Accessed				: 1;  
	//	Free for OS use
	//! Custom Flag
	//*	The Page is Mapped to Physical Memory
	uint64_t PageInMemory			: 1;  
	//	Must be 0
	uint64_t Reserved1				: 1;  
	//	Free for OS use
	uint64_t Available2				: 4;  
	//	Physical address of PDPT (4KB aligned)
	uint64_t PhysicalAddress		: 51; 
	//	Free for OS use
	//	NX Bit
	uint64_t ExecuteDisable			: 1;  
}__packed PML4Entry;

typedef volatile struct{
	//	1 = Present
	//*	The Page is Allocated and Usable
	uint64_t Present				: 1;  
	//	0 = Read-only, 1 = Read/Write
	uint64_t ReadWrite				: 1;  
	//	0 = Supervisor, 1 = User
	uint64_t UserMode			: 1;  
	//	Cache write-through
	uint64_t PageLevelWriteThrough	: 1;  
	//	Cache disable
	uint64_t PageLevelCacheDisable	: 1;  
	//	Accessed flag
	uint64_t Accessed				: 1;  
	//	Free for OS use
	//! Custom Flag
	//*	The Page is Mapped to Physical Memory
    uint64_t PageInMemory			: 1;  
	//	MUST BE 0 (Points to Page Directory)
	uint64_t PageSize				: 1;  
	//	Free for OS use
	uint64_t Reserved2				: 4;  
	//	Physical address of PD (4KB aligned)
	uint64_t PhysicalAddress		: 51;
	//	NX Bit
	uint64_t ExecuteDisable			: 1;  
}__packed PDPTEntryDirectory;
typedef volatile struct{
	//	1 = Present
	//*	The Page is Allocated and Usable
	uint64_t Present				: 1;  
	//	0 = Read-only, 1 = Read/Write
	uint64_t ReadWrite				: 1;  
	//	0 = Supervisor, 1 = User
	uint64_t UserMode				: 1;  
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
	//*	The Page is Mapped to Physical Memory
    uint64_t PageInMemory			: 1;
	uint64_t Available1				: 2;  
	//	PAT bit for 1GB pages
	uint64_t PageAttributeTable		: 1;  
	//	Reserved (Must be 0)
	uint64_t Reserved1				: 17; 
	//	Physical base address (1GB aligned)
	uint64_t PhysicalAddress		: 29; 
	//	Free for OS use
	//	Memory Protection Keys (PKRU)
	uint64_t ProtectionKey			: 4;  
	//	NX Bit
	uint64_t ExecuteDisable			: 1;  
}__packed PDPTEntry1GB;

typedef volatile struct{
	//	1 = Present
	//*	The Page is Allocated and Usable
	uint64_t Present				: 1;  
	//	0 = Read-only, 1 = Read/Write
	uint64_t ReadWrite				: 1;  
	//	0 = Supervisor, 1 = User
	uint64_t UserMode			: 1;  
	//	Cache write-through
	uint64_t PageLevelWriteThrough	: 1;  
	//	Cache disable
	uint64_t PageLevelCacheDisable	: 1;  
	//	Accessed flag
	uint64_t Accessed				: 1;  
	//	Free for OS use
	//! Custom Flag
	//*	The Page is Mapped to Physical Memory
    uint64_t PageInMemory			: 1;  
	//	MUST BE 0 (Points to Page Table)
	uint64_t PageSize				: 1;  
	//	Free for OS use
	uint64_t Available2				: 4;  
	//	Physical address of PT (4KB aligned)
	uint64_t PhysicalAddress		: 40; 
	uint64_t Available3				: 11;
	//	Free for OS use
	//	NX Bit
	uint64_t ExecuteDisable			: 1;  
}__packed PageDirectoryEntry;
typedef volatile struct{
	//	1 = Present
	//*	The Page is Allocated and Usable
	uint64_t Present				: 1;  
	//	0 = Read-only, 1 = Read/Write
	uint64_t ReadWrite				: 1;  
	//	0 = Supervisor, 1 = User
	uint64_t UserMode				: 1;  
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
	//*	The Page is Mapped to Physical Memory
    uint64_t PageInMemory			: 1;
	uint64_t Available1				: 2;  
	//	PAT bit for 2MB pages
	uint64_t PageAttributeTable		: 1;  
	//	Reserved (Must be 0)
	uint64_t Reserved1				: 8;  
	//	Physical base address (2MB aligned)
	uint64_t PhysicalAddress		: 38; 
	//	Free for OS use 
	//	Memory Protection Keys
	uint64_t ProtectionKey			: 4;  
	//	NX Bit
	uint64_t ExecuteDisable			: 1;  
}__packed PageDirectoryEntry2MB;

// Page Table Entry (PTE)
// Final translation entry mapping a standard 4 KB physical frame
typedef volatile struct{
	//	1 = Present
	//*	The Page is Allocated and Usable
	uint64_t Present				: 1;  
	//	0 = Read-only, 1 = Read/Write
	uint64_t ReadWrite				: 1;  
	//	0 = Supervisor, 1 = User
	uint64_t UserMode			: 1;  
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
	//*	The Page is Mapped to Physical Memory
	uint64_t PageInMemory			: 1;
	uint64_t Available1				: 2;  
	//	Physical frame address (4KB aligned)
	uint64_t PhysicalAddress		: 40;
	uint64_t Available2				: 7; 
	//	Free for OS use 
	//	Protection Keys
	uint64_t ProtectionKey			: 4;  
	//	NX Bit
	uint64_t ExecuteDisable			: 1;  
}__packed PageTableEntry4KB;

#define InvalidPointer	((void *)UINT64_MAX)

// Table containers wrapping 512 entries per 4KB table page
typedef volatile __align(4096) PML5Entry PML5Table[PagingTableLength];
typedef volatile __align(4096) PML4Entry PML4Table[PagingTableLength];
typedef volatile union{
	PDPTEntryDirectory		directories[PagingTableLength];
	PDPTEntry1GB			pages1GB[PagingTableLength];
}__align(4096) PDPTTable;
typedef volatile union{
	PageDirectoryEntry	tables[PagingTableLength];
	PageDirectoryEntry2MB	pages2MB[PagingTableLength];
}__align(4096) PageDirectory;
typedef __align(4096) PageTableEntry4KB PageTable[PagingTableLength];

#define VirtualAddressToU64(VA)	(((uint64_t)(((int64_t)( \
	((uint64_t)(VA).Offset)		|					\
		(((uint64_t)(VA).L1) << 12)	|					\
		(((uint64_t)(VA).L2) << 21)	|					\
		(((uint64_t)(VA).L3) << 30)	|					\
		(((uint64_t)(VA).L4) << 39)	|					\
		(((uint64_t)(VA).L5) << 48)) << 7) >> 7)) & BitMask(GetMaxPhysicalAddressBits()))
#define U64ToVirtualAddress(U64)(VirtualAddress){.L0 = 0x00, .Offset = (U64 % PAGE_SIZE) & BitMask(12), .L1 = U64 & PAGE_MASK,	\
	.L2 = (U64 >> PAGE_SHIFT) & PAGE_MASK, .L3 = (U64 >> PAGE_SHIFT2) & PAGE_MASK, .L4 = (U64 >> PAGE_SHIFT3) & PAGE_MASK, .L5 = (U64 >> PAGE_SHIFT4) & PAGE_MASK}
#define InitVirtualAddress(PC)	(VirtualAddress){.L0 = 0, .L1 = ((PC).EnableL1? (PC).L1: 0), .L2 = ((PC).EnableL2? (PC).L2: 0),	\
	.L3 = ((PC).EnableL3? (PC).L3: 0), .L4 = ((PC).EnableL4? (PC).L4: 0), .L5 = ((PC).EnableL5? (PC).L5: 0)}
		typedef struct{
	uint64_t	Offset	: 12;
	uint64_t	L1		: 9;
	uint64_t	L2		: 9;
	uint64_t	L3		: 9;
	uint64_t	L4		: 9;
	uint64_t	L5		: 9;
	//	Alias for 0/1 Bits
	uint64_t	L0		: 7;
}__packed VirtualAddress;
static const VirtualAddress VirtA_MAX = {.L1 = 0x1FF, .L2 = 0x1FF, .L3 = 0x1FF, .L4 = 0x1FF, .L5 = 0x1FF, .L0 = 0x7F, .Offset = 0xFFF};

#define InitPageCoordinate(VA)	(PageCoordinate){	\
	.EnableL1 = true, .EnableL2 = true, .EnableL3 = true, .EnableL4 = true, .EnableL5 = IsLA57Enabled(), 	\
	.L1 = (VA).L1, .L2 = (VA).L2, .L3 = (VA).L3, .L4 = (VA).L4, .L5 = (VA).L5}
typedef struct{
	uint64_t	L1			: 9,	EnableL1	: 1;
	uint64_t	L2			: 9,	EnableL2	: 1;
	uint64_t	L3			: 9,	EnableL3	: 1;
	uint64_t	L4			: 9,	EnableL4	: 1;
	uint64_t	L5			: 9,	EnableL5	: 1;
	uint64_t									: 0;
}/*__packed*/ PageCoordinate;
#define PageCLCheck(PC1, PC2, L)	((((PC1).EnableL##L + (PC2).EnableL##L) == 2)? ((PC1).L##L == (PC1).L##L): true)
#define PageCEq(PC1, PC2)	if(PageCLCheck(PC1, PC2, 1) && PageCLCheck(PC1, PC2, 2) && PageCLCheck(PC1, PC2, 3) && PageCLCheck(PC1, PC2, 4) && PageCLCheck(PC1, PC2, 5))
#define PageC_MAX (PageCoordinate){.EnableL1 = true, .EnableL2 = true, .EnableL3 = true, .EnableL4 = true,	\
	.EnableL5 = IsLA57Enabled(), .L1 = 0x1FF, .L2 = 0x1FF, .L3 = 0x1FF, .L4 = 0x1FF, .L5 = 0x1FF}
void AddPageCoordinate(PageCoordinate *pc, uint64_t count);


#define PAGE_MASK			BitMask(9)

#define PAGE_SIZE3			nGB(1)
#define PAGE_SIZE2			nMB(2)
#define PAGE_SIZE			nKB(4)

#define PAGE_SHIFT			(12)
#define PAGE_SHIFT2			(21)
#define PAGE_SHIFT3			(30)
#define PAGE_SHIFT4			(39)
#define PAGE_SHIFT5			(48)
#define KernelBinarySpace	(0x80000010ULL)

enumdef(uint32_t, PageAllocationType){
	PATUnfreeableRegion = 0x1, PATUntouchableRegion, PATUsedRegion = 0x4, 
	//	The entry describes a Non-Physical Region
	PATSubDescriptorMapping = 0x8, //	PATPageTableMapping = 0x10, 
	PATNonPhysicalDescriptor = 0x20
};

enumdef(uint32_t, PageAllocationFlags){
	ReadOnly = 0x0, ReadWritable = 0x1, 
	UserMode = 0x2, SupervisorMode = 0x4, 
	PageLevelWriteThroughEnable = 0x8, 
	PageLevelCacheEnable = 0x10, 
	PageAttributeTableEnable = 0x20, 
	GlobalEnable = 0x40, ExecuteEnable = 0x80
};

enumdef(uint32_t, UATBitMask){
	//	A proceeding argument of false, will set with the | operation, 
	//	whilst a proceeding argument of true will set with the & operation.
	UATPageAllocationType = 0x01, 
	UATPageCoordinate = 0x02, 
	UATValidBit = 0x04, UATVirtualNeighbour = 0x08, 
	//	Stub for Update Everything.
	UATMaximumValue = UINT32_MAX
};

//	Each Bit Corresponds to 32 bytes
//	.5% of each Gigabyte is used for the PageAllocationTable
//	.5% of all Page Levels are used to map each block.

#define InvalidSubDescriptorIndex	(UINT32_MAX)
typedef volatile struct{
	//	The Physical Base Address for the Range.
	//!	Page Aligned.
	uint64_t			Physical;
	//	The Index of the SubDescriptor array that maps allocations within the range.
	//!	If Greater that ((InternalAllocationState *)->TotalSubDescriptors / (InternalAllocationState *)->TotalSubDescriptorsPools), then is the EndOfAllocation.
	uint32_t			SubDescriptorIndex;
	//	This is the NoPages that are Mapped
	uint32_t			PageLimit;
	//	The type for the Memory, Attained from GetMemoryMap->(EFI_MEMORY_DESCRIPTOR *)->Type.
	EFI_MEMORY_TYPE		EfiMemoryType;
}MemoryRangeDescriptor;
//	Describes an Allocation for a Descriptor
//		PhysicalAddress:	WalkEmulatedPageTree  ->  (Struct *)->PhysicalAddress
//		ElapsedBytes:		WalkEmulatedPageTree  ->  (uint32_t Level) eq 3? PAGE_SIZE3: (eq 2? PAGE_SIZE2: (eq 1? PAGE_SIZE: 0))
//!	This struct is intentionally clueless and slow to maneuver around in order to dispel Page Allcoations that I may inadvertedly use.
typedef volatile struct{
	PageCoordinate		PC;
	PageAllocationType	AllocationType;
	//	Is this the Final Allocation within the Main Descriptor
	uint16_t			EndOfList		: 1;
	//	Is the Allocation still Valid for the Current Page Table State
	uint16_t			Valid			: 1;
	//	The next SubDescriptor that is 'Virtual' Contiguous, As in Contiguous on the Page Table.
	//	If Greater that (InternalAllocationState *)->TotalSubDescriptors, then is the EndOfAllocation.
	uint32_t			VirtualNeighbour;
}MemoryRangeSubDescriptor;

typedef struct{
	void					*PhysicalBase;
	uint64_t				PhysicalLimit;
	PageAllocationType		Type;
}__packed SimpleMemoryRangeDescriptor_t;

#define SetPagePhysicalAddress(Page, Address)	{(Page).PhysicalAddress = ((uint64_t)(Address) & ((1ULL << (GetMaxPhysicalAddressBits() + 1)) - 1));}
#define QuickPageWalk(PageCoord, PAGEVarName)	void *PAGEVarName = NULL;	uint32_t PAGEVarName##Lvl = 0x00;	{	\
	bool _L5##PAGEVarName##_PT = IsLA57Enabled();	PAGEVarName##Lvl = (_L5##PAGEVarName##_PT? 5: 4);\
	void *PAGEVarName##_PT = GetPageTable(&_L5##PAGEVarName##_PT, _StoredPageTableMode, NULL);	\
	PAGEVarName = WalkEmulatedPageTree(PageCoord, PAGEVarName##_PT, &PAGEVarName##Lvl);	\
}	if(Page != InvalidPointer)	\

/*
	A return address of UINT64_MAX refers to an Invalid Address
	A return Size of 0 refers to an Invalid Size.
	A return Size of UINT64_MAXC refers to an Unknown or Uncomputed Size.
*/

// LibAPI void *GeneratePageTable(SimpleMemoryRangeDescriptor_t *Descriptors, uint32_t N);

LibAPI bool _1GPageSupport(void);
LibAPI uint32_t GetMaxPhysicalAddressBits(void);
uint64_t NormaliseAddress(uint64_t Address, bool Low);

// LibAPI bool EnableInputAddress(bool toggle, bool w);

LibAPI void InvalidatePage(void *Virtual);
LibAPI void InvalidatePages(void *Virtual, uint64_t Bytes);

// LibAPI void *__sysvabi MapPTVirtual(void *Physical);
LibAPI void *MapVirtual(void *Physical);
LibAPI void *MapPhysical(void *Virtual);
LibAPI void *WalkPageTreeByVirtual(void *Virtual, uint32_t *Level);
LibAPI uint64_t RepointVirtualAddress(void *Virtual, void *NewPhysical);

LibAPI void *WalkPageTreeByPhysical(void *Physical, uint32_t *Level);
LibAPI void *WalkPageTree(uint32_t *Level, PageCoordinate Coordinate);

LibAPI extern void __sysvabi EnableLA57(void);
LibAPI extern bool __sysvabi IsLA57Enabled(void);
LibAPI extern bool __sysvabi IsLA57Supported();

enumdef(uint32_t, GetPageTableMode){_1To1MappingMode = 0x00, _StoredPageTableMode = 0x01, _UpdatePageTableMode = 0x01, _CPUStall = 0x02};
LibAPI extern void *__sysvabi GetPageTable(bool *Level5, GetPageTableMode Mode, void *OptionalPageTablePhysical);
LibAPI extern uint32_t __sysvabi LoadPageTree(void *PagingTree, uint64_t DataSegment, uint64_t CodeSegment, bool64_t Level5);

LibAPI uint64_t TotalMemorySize(void);
LibAPI uint64_t TotalMappedMemory(void *ptr);
LibAPI bool MemoryIsMapped(void *ptr);

LibAPI void FreePages(void *Virtual);
LibAPI void *AllocNoUpdate(uint64_t Pages, PageAllocationFlags Flags, uint8_t ProtectionKey);
LibAPI void *WalkEmulatedPageTree(PageCoordinate PC, void *PT, uint32_t *Lvl);
LibAPI void *AllocatePages(void *Base, uint64_t NBytes, PageAllocationFlags Flags, uint8_t ProtectionKey);
LibAPI bool InitialiseAllocationState(void *D_, const uint64_t StructSize, const uint64_t N, const uint64_t TotalMemory);