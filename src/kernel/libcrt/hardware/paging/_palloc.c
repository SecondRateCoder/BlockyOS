#include "paging.h"
#include "kernel/libcrt/memory/memory.h"
#include "kernel/libcrt/SimpleOutput/SimpleOutput.h"

//	Non-Usable Used Persistent Memory.
static const EFI_MEMORY_TYPE TypeMask0[] = {EfiRuntimeServicesCode, EfiRuntimeServicesData, EfiPalCode, EfiACPIMemoryNVS, EfiUnacceptedMemoryType, EfiUnusableMemory};
//	Usable Persistent Memory
static const EFI_MEMORY_TYPE TypeMask1[] = {EfiConventionalMemory, EfiBootServicesCode, EfiBootServicesData, EfiLoaderCode, EfiPersistentMemory};
//	Used Persistent Memory
static const EFI_MEMORY_TYPE TypeMask2[] = {EfiLoaderData, KernelBinarySpace, EfiACPIReclaimMemory};
//	Non-Physical Memory.
static const EFI_MEMORY_TYPE TypeMask3[] = {EfiReservedMemoryType, EfiMemoryMappedIOPortSpace, EfiMemoryMappedIO, EfiUnacceptedMemoryType, EfiUnusableMemory};
//	Submapping that encapsulates UnTouchable Memory
static const EFI_MEMORY_TYPE TypeMask4[] = {EfiUnacceptedMemoryType, EfiUnusableMemory};
static const uint64_t		AttributeMask = EFI_MEMORY_WP | EFI_MEMORY_RP | EFI_MEMORY_RO | EFI_MEMORY_NV | EFI_MEMORY_RUNTIME;
static const PageAllocationType	AllocationMask = (PATUnfreeableRegion | PATUntouchableRegion | PATUsedRegion | PATSubDescriptorMapping | PATNonPhysicalDescriptor);

#define SubDescriptorRoundup	(32)
#pragma region Structs, Macros and Helpers
typedef volatile struct{
	//	This describes the Physical(in Pages)->Descriptor Index.
	uint16_t				*PhysicalDescriptorIndexes;
	//	This describes the Various Memory Ranges that apply to various Memory Types. 
	MemoryRangeDescriptor	*Descriptors;
	//	A uniform Buffer that describes all the Sub
	MemoryRangeSubDescriptor**SubDescriptors;
	uint64_t				TotalDescriptors, 
							TotalSubDescriptorPools, 
	//	All Pools will have roughly the same number of bytes made available to it.
							TotalSubDescriptors, 
							CurrentSubDescriptorPool, 
							MemoryMaxPages;
	uint32_t				LastAllocation;
	bool					Ready;
	//	Describes the Upper Echelons of Page Allocations: the first virtual address above the duplicate region.
	PageCoordinate			DuplicatePageTableBase;
}InternalAllocationState;
SimpleMemoryRangeDescriptor_t *QueryFreeMemory(void *Physical, uint64_t Pages, uint64_t *Count, const EFI_MEMORY_TYPE *TypeMask, uint64_t TypeMaskLength);
uint64_t UpdateAllocationTableByPhysical(uint64_t Physical, UATBitMask _BitMask, ...);
bool AppendSingleAllocationTableEntry(uint64_t Physical, UATBitMask _BitMask, ...);
void PrintPageTable(PageCoordinate PC);

static volatile InternalAllocationState *GetIState(){
	static __linkersection(.bss) InternalAllocationState __internal = {0};
	return &__internal;
}

#define GetEfiMemoryDescriptor(Desc, Ptr, N, StructSize)	EFI_MEMORY_DESCRIPTOR *Desc = (EFI_MEMORY_DESCRIPTOR *)((void *)(Ptr) + ((N) * (StructSize)))
#define QueryPhysicalAddress(Adr, IState)			\
	if((IState) && (IState)->Ready && (IState)->MemoryMaxPages && ((IState)->MemoryMaxPages > ((uint64_t)(Adr) / PAGE_SIZE)))
#define GetDescriptor(Descriptor, Adr, IState)	MemoryRangeDescriptor *Descriptor = NULL;	QueryPhysicalAddress(Adr, IState){	\
	Descriptor = (IState)->Descriptors + (IState)->PhysicalDescriptorIndexes[(uint64_t)(Adr) / PAGE_SIZE];}	\
	if(Descriptor)

#define _GetPageCoordinateElapsedPages(PC)	(_GetPageCoordinateElapsedBytes(PC) / PAGE_SIZE)
static uint64_t _GetPageCoordinateElapsedBytes(PageCoordinate PC){
	QuickPageWalk(PC, Page){
		switch(PageLvl){
			case 3:		{if(((PDPTEntryDirectory *)Page)->PageSize){return PAGE_SIZE3;}else{return UINT64_MAX;}}
			case 2:		{if(((PageDirectoryEntry *)Page)->PageSize){return PAGE_SIZE2;}else{return UINT64_MAX;}}
			case 1:		{return PAGE_SIZE;}
		}
	}
	return 0;
}
static uint64_t _GetPageCoordinatePhysicalAddress(PageCoordinate PC){
	QuickPageWalk(PC, Page){
		switch(PageLvl){
			case 3:		{return ((PDPTEntryDirectory *)Page)->PageSize? ((uint64_t)((PDPTEntry1GB *)Page)->PhysicalAddress << PAGE_SHIFT3): ((uint64_t)((PDPTEntryDirectory *)Page)->PhysicalAddress << PAGE_SHIFT);}
			case 2:		{return ((PageDirectoryEntry *)Page)->PageSize? ((uint64_t)((PageDirectoryEntry2MB *)Page)->PhysicalAddress << PAGE_SHIFT2): ((uint64_t)((PageDirectoryEntry *)Page)->PhysicalAddress << PAGE_SHIFT);}
			case 1:		{return (((PageTableEntry4KB *)Page)->PhysicalAddress << PAGE_SHIFT);}
		}
	}
	return UINT64_MAX;
}

//	Faster for Both Details
static void _GetPageCoordinateDetails(PageCoordinate PC, uint64_t *Bytes, uint64_t *Physical){
	(*Physical) = UINT64_MAX;
	(*Bytes) = 0;
	QuickPageWalk(PC, Page){
		switch(PageLvl){
			case 3:		{
				(*Physical) = ((PDPTEntryDirectory *)Page)->PageSize? ((uint64_t)((PDPTEntry1GB *)Page)->PhysicalAddress << PAGE_SHIFT3): ((uint64_t)((PDPTEntryDirectory *)Page)->PhysicalAddress << PAGE_SHIFT);
				(*Bytes) = ((PDPTEntryDirectory *)Page)->PageSize? PAGE_SIZE3: UINT64_MAX;		return;}
			case 2:		{
				(*Physical) = ((PageDirectoryEntry *)Page)->PageSize? ((uint64_t)((PageDirectoryEntry2MB *)Page)->PhysicalAddress << PAGE_SHIFT2): ((uint64_t)((PageDirectoryEntry *)Page)->PhysicalAddress << PAGE_SHIFT);
				(*Bytes) = ((PageDirectoryEntry *)Page)->PageSize? PAGE_SIZE2: UINT64_MAX;		return;}
			case 1:		{(*Physical) = ((uint64_t)((PageTableEntry4KB *)Page)->PhysicalAddress << PAGE_SHIFT);	(*Bytes) = PAGE_SIZE;	return;}
		}
	}
	return;
}

static MemoryRangeSubDescriptor *_GetSubDescriptorByPhysical(void *Phys, volatile InternalAllocationState *IState, uint64_t *Offset){
	Phys = (void *)NormaliseAddress(((uint64_t)Phys & ~BitMask(12)), true);
	GetDescriptor(D, Phys, IState){
		DebugPrintf("\n\t0x%0z{0x%0z\t%z\t%l}", (uint64_t)Phys, D->Physical, D->PageLimit, D->SubDescriptorIndex);
		MemoryRangeSubDescriptor *SD = IState->SubDescriptors[IState->CurrentSubDescriptorPool] + D->SubDescriptorIndex;
		if(Offset){(*Offset) = 0;}
		while(true){
			if(SD->Valid){
				uint64_t CurrPhys = 0x00, Elapsed = 0x00;
				_GetPageCoordinateDetails(SD->PC, &Elapsed, &CurrPhys);
				if(!((Elapsed == 0) || (Elapsed == UINT64_MAX) || (CurrPhys == UINT64_MAX))){
					if(((uint64_t)Phys >= CurrPhys) && ((uint64_t)Phys < (CurrPhys + Elapsed))){return SD;}
					if(SD->EndOfList){break;}
					if(Offset){(*Offset) += _GetPageCoordinateElapsedBytes(SD->PC);}
				}
			}
			if(SD->EndOfList){break;}
			SD++;
		}
	}
	DebugPrintf("\nNo Mapped Subdescriptor");
	return NULL;
}
static MemoryRangeSubDescriptor *_GetSubDescriptorByVirtual(void *Virt, volatile InternalAllocationState *IState, uint64_t *Offset){
	Virt = (void *)NormaliseAddress(((uint64_t)Virt & ~(uint64_t)(PAGE_SIZE - 1)), true);
	VirtualAddress VA = U64ToVirtualAddress((uint64_t)Virt);
	PageCoordinate PC = InitPageCoordinate(VA);
	uint64_t Phys = _GetPageCoordinatePhysicalAddress(PC);
	if(Phys != UINT64_MAX){return _GetSubDescriptorByPhysical((void *)Phys, IState, Offset);}
	DebugPrintf("\nNo Mapped Subdescriptor");
	return NULL;
}
static uint64_t _GetSubDescriptorCount(MemoryRangeDescriptor *D){
	InternalAllocationState *IState = GetIState();
	if(D->SubDescriptorIndex > (IState->TotalSubDescriptors / IState->TotalSubDescriptorPools)){return 0;}
	MemoryRangeSubDescriptor *SD = IState->SubDescriptors[IState->CurrentSubDescriptorPool] + D->SubDescriptorIndex;
	uint64_t Cnt = 0;
	while(!SD[Cnt].EndOfList){Cnt++;}
	return (Cnt + 1);
}

void AddPageCoordinate(PageCoordinate *pc, uint64_t bytes){
    // Convert bytes to 4KB pages (since L1 tracks 4KB units)
    // Ensure bytes are page-aligned or handle remainder if needed
    if(!bytes){return;}
    // Accumulate into L1 with standard carry propagation
    uint64_t total = pc->L1 + (bytes / PAGE_SIZE);
    pc->L1 = total % PagingTableLength;
    uint64_t carry = total / PagingTableLength;
    // Propagate carry up the hierarchy
    if(carry){
        total = pc->L2 + carry;
        pc->L2 = total % PagingTableLength;
        carry = total / PagingTableLength;
    }
    if(carry){
        total = pc->L3 + carry;
        pc->L3 = total % PagingTableLength;
        carry = total / PagingTableLength;
    }
	if(carry){
        total = pc->L4 + carry;
        pc->L4 = total % PagingTableLength;
        carry = total / PagingTableLength;
    }
    if(carry){
        total = pc->L5 + carry;
        pc->L5 = total % PagingTableLength;
        carry = total / PagingTableLength;
    }
}

uint64_t NormaliseAddress(uint64_t Address, bool Low){
	volatile InternalAllocationState *IS = GetIState();
	VirtualAddress VA = InitVirtualAddress(IS->DuplicatePageTableBase);
	const uint64_t Base = VirtualAddressToU64(VA);
	//	Was `Address > Base` / `Address < Base`: the boundary address itself was never converted.
	if(Low){return (Address >= Base)? (Address - Base): Address;}
	return (Address < Base)? (Address + Base): Address;
}
#pragma region Page Gobbledeegook
void *MapVirtual(void *Physical){
	InternalAllocationState *IState = GetIState();
	MemoryRangeSubDescriptor *SD = _GetSubDescriptorByPhysical(Physical, IState, NULL);
	if(!SD){return InvalidPointer;}
	//	SD->PC is the virtual coordinate that maps this sub-descriptor; Base is the physical address its entry holds.
	uint64_t Bytes = 0, Base = 0;
	_GetPageCoordinateDetails(SD->PC, &Bytes, &Base);
	if((Bytes == 0) || (Bytes == UINT64_MAX) || (Base == UINT64_MAX)){return InvalidPointer;}
	//	The sub-descriptor search works on a page-aligned address; put the offset back.
	const uint64_t Phys = NormaliseAddress((uint64_t)Physical, true);
	VirtualAddress VA = InitVirtualAddress(SD->PC);
	return (void *)(VirtualAddressToU64(VA) + (Phys - Base));
}
void *MapPhysical(void *Virtual){
	const uint64_t V = (uint64_t)Virtual;
	VirtualAddress VA = U64ToVirtualAddress(V);
	PageCoordinate PC = InitPageCoordinate(VA);
	QuickPageWalk(PC, Page){
		switch(PageLvl){
			case 3:	{PDPTEntry1GB *E = Page;			if(!E->Present || !E->PageSize){return InvalidPointer;}	return (void *)(((uint64_t)E->PhysicalAddress << PAGE_SHIFT3) + (V & (PAGE_SIZE3 - 1)));}
			case 2:	{PageDirectoryEntry2MB *E = Page;	if(!E->Present || !E->PageSize){return InvalidPointer;}	return (void *)(((uint64_t)E->PhysicalAddress << PAGE_SHIFT2) + (V & (PAGE_SIZE2 - 1)));}
			case 1:	{PageTableEntry4KB *E = Page;		if(!E->Present){return InvalidPointer;}					return (void *)(((uint64_t)E->PhysicalAddress << PAGE_SHIFT) + (V & (PAGE_SIZE - 1)));}
		}
	}
	return InvalidPointer;
}

//	Returns Entry
void *WalkEmulatedPageTree(PageCoordinate PC, void *PT, uint32_t *Lvl){
	const uint32_t Idx[6] = {0, PC.L1, PC.L2, PC.L3, PC.L4, PC.L5};
	const bool En[6] = {false, PC.EnableL1, PC.EnableL2, PC.EnableL3, PC.EnableL4, PC.EnableL5};
	volatile uint64_t *T = (volatile uint64_t *)PT;
	uint32_t L = *Lvl;
	if((L < 1) || (L > 5)){return NULL;}

	for(; L; --L){
		if(!En[L]){break;}
		//	Page Table Entry
		volatile uint64_t *E = T + Idx[L];
		// if(!(*E)){*Lvl = L;	return NULL;}
		//	Bit 0 clear but entry non-zero: skeleton leaf. Bit 7 is PS at levels 3 and 2 (at level 1 it is PAT).
		const bool Present = ((*E) & 0x1) || (L == 1), PageSizeEnabled = (L==2) || (L==3)? ((*E >> 7) & 0x1): 0x00;
		if(!Present){break;}
		if(Present & PageSizeEnabled){*Lvl = L;	return (void *)E;}
		switch(L){
			case 5: {T = (uint64_t *)((uint64_t)((PML5Entry *)E)->PhysicalAddress << PAGE_SHIFT);			break;}
			case 4: {T = (uint64_t *)((uint64_t)((PML4Entry *)E)->PhysicalAddress << PAGE_SHIFT);			break;}
			case 3: {T = (uint64_t *)((uint64_t)((PDPTEntryDirectory *)E)->PhysicalAddress << PAGE_SHIFT);	break;}
			case 2: {T = (uint64_t *)((uint64_t)((PageDirectoryEntry *)E)->PhysicalAddress << PAGE_SHIFT);	break;}
		}
		if(L == 1){break;}
	}
	*Lvl = L;
	const uint64_t Shift = (L == 5? PAGE_SHIFT5: (L == 4? PAGE_SHIFT4: (L == 3? PAGE_SHIFT3: (L == 2? PAGE_SHIFT2: PAGE_SHIFT))));
	VirtualAddress va = InitVirtualAddress(PC);
	return (void *)(T + ((VirtualAddressToU64(va) >> Shift) & PAGE_MASK));
}

#define PageInMemoryFlagUsetBit	(((uint32_t)UINT16_MAX << 16) | ReadWritable)
#define PresentBitUsetBit			(((uint32_t)UINT16_MAX << 16) | SupervisorMode)
bool UpdatePageTable(void *Virtual, uint64_t NBytes, PageAllocationFlags Flags, uint8_t ProtectionKey){
	bool L5Enabled = IsLA57Enabled();
	void *PageTable = GetPageTable(&L5Enabled, _StoredPageTableMode, NULL);
	//	XD is a reserved bit (fault) unless EFER.NXE is set.
	uint32_t EferLow, EferHigh;
	__asm__ volatile("rdmsr" : "=a"(EferLow), "=d"(EferHigh) : "c"(0xC0000080U));
	const bool NX = (EferLow >> 11) & 1;
	//	Cover whole pages: an unaligned start must not shorten the range (only NBytes used to be rounded).
	{const uint64_t Lead = (uint64_t)Virtual & (PAGE_SIZE - 1);	Virtual = (void *)((uint64_t)Virtual - Lead);	NBytes = __roundup(NBytes + Lead, PAGE_SIZE);}
	while(NBytes){
		//	Level in: top level. Level out: the level the walk stopped at (3: 1GB, 2: 2MB, 1: 4KB).
		uint32_t Level = IsLA57Enabled()? 5: 4;
		VirtualAddress VA = U64ToVirtualAddress((uint64_t)Virtual);
		PageCoordinate PC = InitPageCoordinate(VA);
		void *PT = WalkEmulatedPageTree(PC, PageTable, &Level);
		//	WalkEmulatedPageTree returns the slot where it stopped EVEN WHEN THAT SLOT IS EMPTY, so `!PT` never fires.
		//	Writing flags into an empty slot creates a present mapping of physical 0. Only a non-zero leaf may be updated:
		//	levels 3 and 2 must carry PS (otherwise it is a missing or non-present table pointer), level 1 must be non-zero,
		//	and levels 5 and 4 are never leaves.
		const uint64_t Raw = PT? *(volatile uint64_t *)PT: 0;
		if(!Raw || (Level > 3) || ((Level > 1) && !(Raw & (1ULL << 7)))){DebugPrintf("\nFailed to get PT");	return false;}
		//	Step to the end of THIS leaf (it can be larger than the request, or the request can start inside it).
		const uint64_t Span = 1ULL << (PAGE_SHIFT + (9 * (Level - 1)));
		uint64_t Step = Span - ((uint64_t)Virtual & (Span - 1));
		if(Step > NBytes){Step = NBytes;}
		const bool XD = (!__check(Flags, ExecuteEnable) && NX);
		switch(Level){
			case 3: {
				PDPTEntry1GB *L3 = (PDPTEntry1GB *)PT;
				L3->PageInMemory = !__check(Flags, PageInMemoryFlagUsetBit);
				L3->Present = !__check(Flags, PresentBitUsetBit);
				L3->ExecuteDisable = XD;
				L3->PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable);
				L3->PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable);
				L3->ReadWrite = __check(Flags, ReadWritable);
				L3->UserMode = __check(Flags, UserMode);
				L3->ProtectionKey = ProtectionKey;
				break;
			} case 2: {
				PageDirectoryEntry2MB *L2 = (PageDirectoryEntry2MB *)PT;
				L2->PageInMemory = !__check(Flags, PageInMemoryFlagUsetBit);
				L2->Present = !__check(Flags, PresentBitUsetBit);
				L2->ExecuteDisable = XD;
				L2->PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable);
				L2->PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable);
				L2->ReadWrite = __check(Flags, ReadWritable);
				L2->UserMode = __check(Flags, UserMode);
				L2->ProtectionKey = ProtectionKey;
				break;
			} default: {
				PageTableEntry4KB *L1 = (PageTableEntry4KB *)PT;
				L1->PageInMemory = !__check(Flags, PageInMemoryFlagUsetBit);
				L1->Present = !__check(Flags, PresentBitUsetBit);
				L1->ExecuteDisable = XD;
				L1->PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable);
				L1->PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable);
				L1->ReadWrite = __check(Flags, ReadWritable);
				L1->UserMode = __check(Flags, UserMode);
				L1->ProtectionKey = ProtectionKey;
				break;
			}
		}
		InvalidatePage(Virtual);
		Virtual += Step;
		NBytes -= Step;
	}
	return true;
}
uint64_t RepointVirtualAddress(void *Virtual, void *NewPhysical){
	VirtualAddress VA = U64ToVirtualAddress((uint64_t)Virtual);
	PageCoordinate PC = InitPageCoordinate(VA);
	const uint64_t P = (uint64_t)NewPhysical;
	QuickPageWalk(PC, Page){
		switch(PageLvl){
			case 5:	{PML5Entry *E = Page;	SetPagePhysicalAddress(*E, P >> PAGE_SHIFT);	InvalidatePage(Virtual);	return UINT64_MAX;}
			case 4: {PML4Entry *E = Page;	SetPagePhysicalAddress(*E, P >> PAGE_SHIFT);	InvalidatePage(Virtual);	return UINT64_MAX;}
			case 3: {
				if(((PDPTEntryDirectory *)Page)->PageSize){
					if(P & (PAGE_SIZE3 - 1)){return UINT64_MIN;}
					SetPagePhysicalAddress(*(PDPTEntry1GB *)Page, P >> PAGE_SHIFT3);	InvalidatePage(Virtual);	return PAGE_SIZE3;
				}
				SetPagePhysicalAddress(*(PDPTEntryDirectory *)Page, P >> PAGE_SHIFT);	InvalidatePage(Virtual);	return UINT64_MAX;
			} case 2: {
				if(((PageDirectoryEntry *)Page)->PageSize){
					if(P & (PAGE_SIZE2 - 1)){return UINT64_MIN;}
					SetPagePhysicalAddress(*(PageDirectoryEntry2MB *)Page, P >> PAGE_SHIFT2);	InvalidatePage(Virtual);	return PAGE_SIZE2;
				}
				SetPagePhysicalAddress(*(PageDirectoryEntry *)Page, P >> PAGE_SHIFT);	InvalidatePage(Virtual);	return UINT64_MAX;
			} case 1: {
				if(P & (PAGE_SIZE - 1)){return UINT64_MIN;}
				PageTableEntry4KB *E = Page;	SetPagePhysicalAddress(*E, P >> PAGE_SHIFT);	InvalidatePage(Virtual);	return PAGE_SIZE;
			}
		}
	}
	return UINT64_MIN;
}


#define QuickPageTableAlloc(TotalBytes, PC)	PageCoordinate PC = {0};	{\
	uint64_t TotalBytes##Rem = Total, TotalBytes##_1GB = TotalBytes##Rem / PAGE_SIZE3;	TotalBytes##Rem %= PAGE_SIZE3;	\
	if(TotalBytes##Rem >= PAGE_SIZE3 - 8*PAGE_SIZE2){TotalBytes##_1GB++;	TotalBytes##Rem = 0;}	\
	uint64_t TotalBytes##_2MB = TotalBytes##Rem / PAGE_SIZE2;				TotalBytes##Rem %= PAGE_SIZE2;	\
	if(TotalBytes##Rem >= PAGE_SIZE2 - 8*PAGE_SIZE){TotalBytes##_2MB++;	TotalBytes##Rem = 0;}	\
	uint64_t TotalBytes##_4KB = __roundup(TotalBytes##Rem, PAGE_SIZE) / PAGE_SIZE;	\
	DebugPrintf("\nAllocating\t%z\t%z\t%z", TotalBytes##_1GB, TotalBytes##_2MB, TotalBytes##_4KB);	\
	PC = PageTableAlloc(TotalBytes##_1GB, TotalBytes##_2MB, TotalBytes##_4KB);	}
#define BufferPageTableAlloc(Count, Bfr, PC)	PageCoordinate PC = {0};	{uint64_t _##Count##_1GB = 0, _##Count##_2MB = 0, _##Count##_4KB = 0;	for(uint64_t cc = 0; cc < Count; ++cc){	\
		uint64_t _##Count##Rem = Bfr[cc].PhysicalLimit - (uint64_t)Bfr[cc].PhysicalBase; _##Count##_1GB += _##Count##Rem / PAGE_SIZE3;	_##Count##Rem %= PAGE_SIZE3;	\
		if(_##Count##Rem >= PAGE_SIZE3 - 8*PAGE_SIZE2){_##Count##_1GB++;	_##Count##Rem = 0;}	\
		_##Count##_2MB += _##Count##Rem / PAGE_SIZE2;				_##Count##Rem %= PAGE_SIZE2;	\
		if(_##Count##Rem >= PAGE_SIZE2 - 8*PAGE_SIZE){_##Count##_2MB++;	_##Count##Rem = 0;}	\
		_##Count##_4KB += __roundup(_##Count##Rem, PAGE_SIZE) / PAGE_SIZE;	\
	}	PC = PageTableAlloc(_##Count##_1GB, _##Count##_2MB, _##Count##_4KB);}
//*	This function allocates Addresses on the Page Table.
PageCoordinate PageTableAlloc(uint64_t _1GB, uint64_t _2MB, uint64_t _4KB){
	//	Allocate Memory based off of the
	InternalAllocationState *IS = GetIState();
	PageCoordinate PC = IS->DuplicatePageTableBase;
	bool Usable = false;
	DebugPrintf("\nAllcoating %z GBs, %z MBs, %z KBs", _1GB, _2MB, _4KB);
	while(true){
		Usable = false;
		PrintPageTable(PC);
		QuickPageWalk(PC, Page){
			uint64_t __1GB = _1GB, __2MB = _2MB, __4KB = _4KB;
			//	Since Flag Bits are the same bits, we'll simply Cast to PageTableEntry4KB
			PageTableEntry4KB *Tmp = Page;
			switch(PageLvl){
				case 5: {
					PML5Entry *E = Page;
					uint64_t Count = 0;
					SimpleMemoryRangeDescriptor_t *Bfr = QueryFreeMemory(InvalidPointer, 2, &Count, TypeMask1, sizeof(TypeMask1));
					if(!Bfr || !Count){DebugPrintf("\nFailed to discover Free Memory");	__chalt(true);}
					for(uint64_t cc = 0; cc < Count; ++cc){
						if(!UpdateAllocationTableByPhysical((uint64_t)Bfr[cc].PhysicalBase, UATValidBit | UATPageAllocationType, false, PATUsedRegion, true)){
							if(!AppendSingleAllocationTableEntry((uint64_t)Bfr[cc].PhysicalBase, UATValidBit | UATPageAllocationType, PATUsedRegion, true)){
								DebugPrintf("\nFailed to Append SubDescriptor");	__chalt(true);}}}
					*E = (PML5Entry){.Accessed = false, .ExecuteDisable = false, .PageInMemory = true, .PageLevelCacheDisable = false, 
						.PageLevelWriteThrough = false, .PhysicalAddress = 0, .Present = true, .ReadWrite = true, .UserMode = false};
					SetPagePhysicalAddress(*E, (uint64_t)Bfr[0].PhysicalBase >> PAGE_SHIFT);
					void *Phys = ((Bfr[0].PhysicalLimit - (uint64_t)Bfr[0].PhysicalBase) < (PAGE_SIZE * 2) && (Count > 1))? Bfr[1].PhysicalBase: (Bfr[0].PhysicalBase + PAGE_SIZE);
					PML4Entry *E_ = Bfr[0].PhysicalBase;	*E_ = (PML4Entry){.Accessed = false, .ExecuteDisable = false, .PageInMemory = true, .PageLevelCacheDisable = false, 
						.PageLevelWriteThrough = false, .PhysicalAddress = 0, .Present = true, .ReadWrite = true, .UserMode = false};
					SetPagePhysicalAddress(*E_, (uint64_t)Phys >> PAGE_SHIFT);
					continue;
				} case 4: {
					PML4Entry *E = Page;
					uint64_t Count = 0;
					SimpleMemoryRangeDescriptor_t *Bfr = QueryFreeMemory(InvalidPointer, 1, &Count, TypeMask1, sizeof(TypeMask1));
					if(!Bfr || !Count){DebugPrintf("\nFailed to discover Free Memory");	__chalt(true);}
					for(uint64_t cc = 0; cc < Count; ++cc){
						if(!UpdateAllocationTableByPhysical((uint64_t)Bfr[cc].PhysicalBase, UATValidBit | UATPageAllocationType, false, PATUsedRegion, true)){
							if(!AppendSingleAllocationTableEntry((uint64_t)Bfr[cc].PhysicalBase, UATValidBit | UATPageAllocationType, PATUsedRegion, true)){
								DebugPrintf("\nFailed to Append SubDescriptor");	__chalt(true);}}}
					*E = (PML4Entry){.Accessed = false, .ExecuteDisable = false, .PageInMemory = true, .PageLevelCacheDisable = false, 
						.PageLevelWriteThrough = false, .PhysicalAddress = 0, .Present = true, .ReadWrite = true, .UserMode = false};
					SetPagePhysicalAddress(*E, (uint64_t)Bfr[0].PhysicalBase >> PAGE_SHIFT);
					continue;
				}
			}
			if(!Tmp->Present){Usable = true;}
			//	Possibly Usable
			if(Usable){
				DebugPrintf("\t\nUsable Page");
				PageCoordinate PCTmp = PC;
				//	Query whether contiguous Pages are also Non-Present.
				while((__1GB || __2MB || __4KB) && Usable){
					QuickPageWalk(PCTmp, _Page){
						//	Similarly, Since Flag Bits are the same bits, we'll simply Cast to PageTableEntry4KB
						Tmp = _Page;
						if(Tmp->Present || (PageLvl > 3) || (PageLvl < 1)){Usable = false;	break;}
						switch(PageLvl){
							case 3: {if(__1GB){__1GB--;}	break;}
							case 2: {if(__2MB){__2MB--;}	break;}
							case 1: {if(__4KB){__4KB--;}	break;}
						}
						AddPageCoordinate(&PCTmp, _PageLvl == 3? PAGE_SIZE3: (_PageLvl == 2? PAGE_SIZE2: PAGE_SIZE));
					}else{break;}
				}
				if(Usable){return PC;}
			}
			//	If at this stage then the contiguous entries were non-usable.
			AddPageCoordinate(&PC, PageLvl == 3? PAGE_SIZE3: (PageLvl == 2? PAGE_SIZE2: PAGE_SIZE));
		}else{
			DebugPrintf("\nInvalid Page");
			//	PC should be at the Premise of an Invalid Page.
			//	Base should thus point to our next Invalid Entry.
			PageCoordinate Base = (PageCoordinate)PC;{
				bool Exit = false;
				//	We need to walk in reverse in order to construct a usable Subset of the Page Table.
				//	We first need to make sure we aren't encroaching on Mapped Pages.
				while(!Exit){
					QuickPageWalk(Base, Page){
						switch(PageLvl){
							case 1: {Base.EnableL1 = false;	Base.L1 = 0;	Base.L2++;	break;}
							case 2: {Base.EnableL2 = false;	Base.L2 = 0;	Base.L3++;	break;}
							case 3: {Base.EnableL3 = false;	Base.L3 = 0;	Base.L4++;	break;}
							case 4: {Base.EnableL4 = false;	Base.L4 = 0;	Base.L5++;	break;}
							default:{Exit = true;	break;}
						}
					}else{break;}
				}	PC = Base;
			}
			if(Base.EnableL5 && IsLA57Enabled()){
				QuickPageWalk(Base, Page){
					//	We need to allocate additional Memory.
					uint64_t Count = 0;
					SimpleMemoryRangeDescriptor_t *Bfr = QueryFreeMemory(InvalidPointer, 1, &Count, TypeMask1, sizeof(TypeMask1));
					//	Count should be equal to 1, but we dont really care if it's greater than one.
					if(!Bfr || !Count){DebugPrintf("\nFailed to discover Free Memory");	__chalt(true);}
					Base.EnableL4 = true;
						if(!UpdateAllocationTableByPhysical((uint64_t)Bfr[0].PhysicalBase, UATValidBit | UATPageAllocationType, false, PATUsedRegion, true)){
							if(!AppendSingleAllocationTableEntry((uint64_t)Bfr[0].PhysicalBase, UATValidBit | UATPageAllocationType, PATUsedRegion, true)){
								DebugPrintf("\nFailed to Append SubDescriptor");	__chalt(true);}}
					PML5Entry *L5 = Page;	*L5 = (PML5Entry){
						.Accessed = false, .ExecuteDisable = false, .PageInMemory = true, .PageLevelCacheDisable = false, 
						.PageLevelWriteThrough = false, .PhysicalAddress = 0x00, .Present = true, .ReadWrite = true, .UserMode = false};
					SetPagePhysicalAddress(*L5, (uint64_t)Bfr[0].PhysicalBase >> PAGE_SHIFT);
				}
			}
			QuickPageWalk(Base, Page){
				//	We need to allocate additional Memory.
				uint64_t Count = 0;
				SimpleMemoryRangeDescriptor_t *Bfr = QueryFreeMemory(InvalidPointer, 1, &Count, TypeMask1, sizeof(TypeMask1));
				//	Count should be equal to 1, but we dont really care if it's greater than one.
				if(!Bfr || !Count){DebugPrintf("\nFailed to discover Free Memory");	__chalt(true);}
				Base.EnableL3 = true;
						if(!UpdateAllocationTableByPhysical((uint64_t)Bfr[0].PhysicalBase, UATValidBit | UATPageAllocationType, false, PATUsedRegion, true)){
							if(!AppendSingleAllocationTableEntry((uint64_t)Bfr[0].PhysicalBase, UATValidBit | UATPageAllocationType, PATUsedRegion, true)){
								DebugPrintf("\nFailed to Append SubDescriptor");	__chalt(true);}}
				PML4Entry *L4 = Page;	*L4 = (PML4Entry){
					.Accessed = false, .ExecuteDisable = false, .PageInMemory = true, .PageLevelCacheDisable = false, 
					.PageLevelWriteThrough = false, .PhysicalAddress = 0x00, .Present = true, .ReadWrite = true, .UserMode = false};
				SetPagePhysicalAddress(*L4, (uint64_t)Bfr[0].PhysicalBase >> PAGE_SHIFT);
			}
			//	Now is the complex part of enabling the slop
			//	Generating the proceeding Arrays, starting with 1GB, then 2MB, then 4KB.
			{
				while(_1GB){
					QuickPageWalk(Base, Page){
						PDPTEntry1GB *L3 = Page;	*L3 = (PDPTEntry1GB){.Accessed = false, .Dirty = false, .ExecuteDisable = false, .Global = false, .PageAttributeTable = false, 
							.PageInMemory = true, .PageLevelCacheDisable = false, .PageLevelWriteThrough = false, .PageSize = true, .PhysicalAddress = 0x00, .Present = true, 
							.ProtectionKey = 0x00, .ReadWrite = true, .UserMode = false};
						SetPagePhysicalAddress(*L3, UINT64_MAX);
						AddPageCoordinate(&Base, PAGE_SIZE3);
						_1GB--;
					}
				}
				//	Allocate for sub-pages
				if(_2MB || _4KB){
					//	We need to allocate additional Memory.
					uint64_t Count = 0;
					SimpleMemoryRangeDescriptor_t *Bfr = QueryFreeMemory(InvalidPointer, 1, &Count, TypeMask1, sizeof(TypeMask1));
					//	Count should be equal to 1, but we dont really care if it's greater than one.
					if(!Bfr || !Count){DebugPrintf("\nFailed to discover Free Memory");	__chalt(true);}
					Base.EnableL2 = true;
						if(!UpdateAllocationTableByPhysical((uint64_t)Bfr[0].PhysicalBase, UATValidBit | UATPageAllocationType, false, PATUsedRegion, true)){
							if(!AppendSingleAllocationTableEntry((uint64_t)Bfr[0].PhysicalBase, UATValidBit | UATPageAllocationType, PATUsedRegion, true)){
								DebugPrintf("\nFailed to Append SubDescriptor");	__chalt(true);}}
					PDPTEntryDirectory *L3 = Page;	*L3 = (PDPTEntryDirectory){.Accessed = false, .ExecuteDisable = false, .PageInMemory = true, .PageLevelCacheDisable = false, 
						.PageLevelWriteThrough = false, .PageSize = false, .PhysicalAddress = 0x00, .Present = true, .ReadWrite = true, .UserMode = false};
					SetPagePhysicalAddress(*L3, (uint64_t)Bfr[0].PhysicalBase >> PAGE_SHIFT);
				}
			}	{
				while(_2MB){
					QuickPageWalk(Base, Page){
						PageDirectoryEntry2MB *L2 = Page;	*L2 = (PageDirectoryEntry2MB){.Accessed = false, .Dirty = false, .ExecuteDisable = false, .Global = false, .PageAttributeTable = false, 
							.PageInMemory = true, .PageLevelCacheDisable = false, .PageLevelWriteThrough = false, .PageSize = true, .PhysicalAddress = 0x00, .Present = true, 
							.ProtectionKey = 0x00, .ReadWrite = true, .UserMode = false};
						SetPagePhysicalAddress(*L2, UINT64_MAX);
						AddPageCoordinate(&Base, PAGE_SIZE2);
						_2MB--;
					}
				}
				if(_4KB){
					//	We need to allocate additional Memory.
					uint64_t Count = 0;
					SimpleMemoryRangeDescriptor_t *Bfr = QueryFreeMemory(InvalidPointer, 1, &Count, TypeMask1, sizeof(TypeMask1));
					//	Count should be equal to 1, but we dont really care if it's greater than one.
					if(!Bfr || !Count){DebugPrintf("\nFailed to discover Free Memory");	__chalt(true);}
					Base.EnableL2 = true;
						if(!UpdateAllocationTableByPhysical((uint64_t)Bfr[0].PhysicalBase, UATValidBit | UATPageAllocationType, false, PATUsedRegion, true)){
							if(!AppendSingleAllocationTableEntry((uint64_t)Bfr[0].PhysicalBase, UATValidBit | UATPageAllocationType, PATUsedRegion, true)){
								DebugPrintf("\nFailed to Append SubDescriptor");	__chalt(true);}}
					PageDirectoryEntry *L2 = Page;	*L2 = (PageDirectoryEntry){.Accessed = false, .ExecuteDisable = false, .PageInMemory = true, .PageLevelCacheDisable = false, 
						.PageLevelWriteThrough = false, .PageSize = false, .PhysicalAddress = 0x00, .Present = true, .ReadWrite = true, .UserMode = false};
					SetPagePhysicalAddress(*L2, (uint64_t)Bfr[0].PhysicalBase >> PAGE_SHIFT);
				}
			}	{
				while(_4KB){
					QuickPageWalk(Base, Page){
						PageTableEntry4KB *L1 = Page;	*L1 = (PageTableEntry4KB){.Accessed = false, .Dirty = false, .ExecuteDisable = false, .Global = false, .PageAttributeTable = false, 
							.PageInMemory = true, .PageLevelCacheDisable = false, .PageLevelWriteThrough = false, .PhysicalAddress = 0x00, .Present = true, 
							.ProtectionKey = 0x00, .ReadWrite = true, .UserMode = false};
						SetPagePhysicalAddress(*L1, UINT64_MAX);
						AddPageCoordinate(&Base, PAGE_SIZE);
						_4KB--;
					}
				}
			}
			return PC;
		}
	}
	const VirtualAddress VA = U64ToVirtualAddress(UINT64_MAX);
	return InitPageCoordinate(VA);
}

#pragma region Initialisation Memory Management/Discovery
static EFI_MEMORY_DESCRIPTOR *QueryCandidatePool(
	const void *D_, const uint32_t StructSize, uint64_t *TotalPages, const uint64_t N, const uint64_t RequiredPages, 
	const EFI_MEMORY_TYPE *TypeRequirements, const uint64_t TypeRequirementBufferSize, const uint64_t AttrMask
){
	int64_t Candidate = -1;
	uint64_t NCandidateEntries = UINT64_MAX, CandidateElapsedPages = UINT64_MAX;
	DebugPrintf("\nRequirements:\t%z Pages\tAttributes: %2z\tTypeRequirements: ", (uint64_t)RequiredPages, (uint64_t)AttrMask);
		for(uint32_t cc = 0; cc < (TypeRequirementBufferSize / sizeof(EFI_MEMORY_TYPE)); ++cc){DebugPrintf("%z, ", (uint64_t)(TypeRequirements[cc]));}
	for(uint64_t cc = 0; cc < N; ++cc){
		GetEfiMemoryDescriptor(D, D_, cc, StructSize);
		bool Usable = memcheckl((uint32_t *)TypeRequirements, D->Type, TypeRequirementBufferSize) && ((AttrMask & D->Attribute) == 0);
		if(Usable){
			uint64_t RemainingPages = (uint64_t)RequiredPages, ElapsedPages = 0, cc_ = cc, Address = D->PhysicalStart;
			//	Check that the Next Entry is contiguous, since D2 starts as D, then cc_ == 0 will always go through
			for(; (cc_ < N) && RemainingPages; ++cc_){
				GetEfiMemoryDescriptor(D2, D_, cc_, StructSize);
				if(D2->PhysicalStart != Address){break;}
				ElapsedPages += D2->NumberOfPages;
				Address = D2->PhysicalStart + (D2->NumberOfPages * PAGE_SIZE);
				Usable = memcheckl((uint32_t *)TypeRequirements, D2->Type, TypeRequirementBufferSize) && ((AttrMask & D2->Attribute) == 0);
				if(Usable){if(D2->NumberOfPages > RemainingPages){RemainingPages = 0;	break;}else{RemainingPages -= D2->NumberOfPages;}}else{break;}
			}
			//	The currently Viewed entry elapses more pages than the candidate.
			if(RemainingPages){continue;}else{
				if((ElapsedPages - RequiredPages) <= (CandidateElapsedPages - RequiredPages)){
					CandidateElapsedPages = ElapsedPages;	NCandidateEntries = cc_ - cc;	Candidate = cc;
				}else{continue;}
			}
		}
	}
	if((Candidate == -1) || (NCandidateEntries == UINT64_MAX) || (CandidateElapsedPages == UINT64_MAX)){DebugPrintf("\nFailed to find valid Candidate");	*TotalPages = 0;	return NULL;}
	GetEfiMemoryDescriptor(D, D_, Candidate, StructSize);
	memset((void *)(D->PhysicalStart), 0, CandidateElapsedPages * PAGE_SIZE);
	DebugPrintf("\nCandidate: 0x%0z, TotalPages: %1z", D->PhysicalStart, CandidateElapsedPages);
	*TotalPages = CandidateElapsedPages;
	return D;
}
//*	A fork of QueryCandidatePool, modifies EFI_MEMORY_DESCRIPTOR Type.
static EFI_MEMORY_DESCRIPTOR *AllocateCandidatePool(const void *D_, const uint32_t StructSize, uint64_t *TotalPages, const uint64_t N, const uint64_t RequiredPages, 
	const EFI_MEMORY_TYPE *TypeRequirements, const uint64_t TypeRequirementBufferSize, const uint64_t AttrMask
){
	EFI_MEMORY_DESCRIPTOR *Candidate = QueryCandidatePool(D_, StructSize, TotalPages, N, RequiredPages, TypeRequirements, TypeRequirementBufferSize, AttrMask);
	if(Candidate){
		uint64_t Tmp = *TotalPages;
		for(uint64_t cc = 0; Tmp; ++cc){
			GetEfiMemoryDescriptor(D, Candidate, cc, StructSize);
			D->Type = KernelBinarySpace;
			if(Tmp < D->NumberOfPages){break;}else{Tmp -= D->NumberOfPages;}
		}
		memset((void *)(Candidate->PhysicalStart), 0, (*TotalPages) * PAGE_SIZE);
		return Candidate;
	}
	return NULL;
}
//*	Describes the run of contiguous, same-Type descriptors starting at index *Counter.
//*	On return: *ElapsedEntries / *ElapsedPages describe that run, *Counter indexes the first entry AFTER it,
//*	and the return value is the FIRST entry OF the run (NULL once *Counter >= N).
static EFI_MEMORY_DESCRIPTOR *QueryContiguousDescriptors(const void *D_, const uint32_t StructSize, 
	uint64_t *ElapsedEntries, uint64_t *ElapsedPages, uint64_t *Counter, const uint64_t N
){
	(*ElapsedEntries) = 0;
	(*ElapsedPages) = 0;
	if((*Counter) >= N){return NULL;}
	GetEfiMemoryDescriptor(D, D_, (*Counter), StructSize);
	uint64_t Address = D->PhysicalStart;
	for(; (*Counter) < N; ++(*Counter)){
		GetEfiMemoryDescriptor(D2, D_, (*Counter), StructSize);
		//	A gap, an overlap or a Type change ends the run. The first entry always passes, so Entries >= 1.
		if((D2->PhysicalStart != Address) || (D2->Type != D->Type)){break;}
		Address = D2->PhysicalStart + (D2->NumberOfPages * PAGE_SIZE);
		(*ElapsedEntries)++;	(*ElapsedPages) += D2->NumberOfPages;
	}
	return D;
}
#pragma region Initialisation
static __inline void InitialiseTemporaryPageTable(uint64_t TotalMemorySize){
	static __linkersection(.bss) __align(4096) PML5Table       L5;
	static __linkersection(.bss) __align(4096) PML4Table       L4;
	static __linkersection(.bss) __align(4096) PDPTEntry1GB    L3[PagingTableLength];
    bool _L5 = IsLA57Enabled();
	if(_L5){
		for(uint32_t cc = 0; cc < PagingTableLength; cc++){
			L5[cc] = (PML5Entry){
				.ExecuteDisable = false, .PageInMemory = false, .PageLevelCacheDisable = false, 
				.Present = true, .UserMode = false, .PageLevelWriteThrough = false, 
				.PhysicalAddress = (uint64_t)&L4 >> PAGE_SHIFT, .ReadWrite = true
			};
			SetPagePhysicalAddress(L5[cc], (uint64_t)&L4 >> PAGE_SHIFT);
		}
	}
	for(uint32_t cc = 0; cc < PagingTableLength; cc++){
		L4[cc] = (PML4Entry){
			.ExecuteDisable = false, .PageInMemory = false, .PageLevelCacheDisable = false, 
			.Present = true, .UserMode = false, .PageLevelWriteThrough = false, 
			.PhysicalAddress = (uint64_t)&L3 >> PAGE_SHIFT, .ReadWrite = true
		};
		SetPagePhysicalAddress(L4[cc], (uint64_t)&L3 >> PAGE_SHIFT);
	}
	uint64_t GBs = __roundup(TotalMemorySize, PAGE_SIZE3) / PAGE_SIZE3, TotalL3 = __min(GBs, PagingTableLength);
    for(uint64_t cc = 0; cc < TotalL3; cc++){
		L3[cc] = (PDPTEntry1GB){
			.Accessed = false, .ExecuteDisable = false, .PageInMemory = false, 
            .PageLevelCacheDisable = false, .PageLevelWriteThrough = false, 
            .PhysicalAddress = cc, .Present = true, .UserMode = false, .ReadWrite = true, 
            .Global = false, .Dirty = false, .PageSize = true, .PageAttributeTable = false
        };
		SetPagePhysicalAddress(L3[cc], cc);
    }
    LoadPageTree((void *)(_L5? (void *)&L5: (void *)&L4), 0x10, 0x08, (bool64_t)_L5);
	return;
}
//	We now only Initialise an L2 Identity Mapped Page Table.
//	Mainly because we require at least that much Precision to ensure that QueryFreeMemory(At least in the Beginning of Initialisation) can cope with 
VirtualAddress InstantiatePageTable(const void *D_, const uint64_t StructSize, const uint64_t N, const uint64_t TotalMemoryPages){
	if(!_1GPageSupport()){DebugPrintf("\nThe CPU does not support 1G Pages, cannot Continue Execution");	__chalt(true);}
	if(IsLA57Enabled()){DebugPrintf("\nInstantiatePageTable: 5-level paging is not supported");				__chalt(true);}
	bool _L = IsLA57Enabled();
	void *PT = GetPageTable(&_L, _1To1MappingMode, NULL);
	(GetPageTable(&_L, _UpdatePageTableMode, PT));
	InitialiseTemporaryPageTable(TotalMemoryPages * PAGE_SIZE);

	//	We will accomodate the neccessary L3/L2/L1 for mapped Tables, 
	//	
	//	within L5/L4 Tables, the Proceeding L4s from the Identity Mapped Table will correspond to the "Free" Page Table that respects EFI_MEMORY_DESCRIPTOR Flag Requirements.
	const uint64_t L2Entries = __roundupt(TotalMemoryPages * PAGE_SIZE, PAGE_SIZE2) / PAGE_SIZE2, 
			L3Entries = __roundup(L2Entries, PagingTableLength) / PagingTableLength, 
			L4Entries = __roundup(L3Entries, PagingTableLength) / PagingTableLength;
	const uint64_t L2Tables = __roundup(L2Entries, PagingTableLength) / PagingTableLength, 
					L3Tables = __roundup(L3Entries, PagingTableLength) / PagingTableLength, 
					L4Tables = __roundup(L4Entries, PagingTableLength) / PagingTableLength;
	uint64_t RequiredPages = L2Tables + L3Tables + L4Tables, CandidatePages = 0;
	EFI_MEMORY_DESCRIPTOR *Candidate = AllocateCandidatePool(D_, StructSize, &CandidatePages, N, RequiredPages, TypeMask1, sizeof(TypeMask1), AttributeMask);
	if(!Candidate){
		DebugPrintf("\nThe System does not have enough Installed Random-Access-Memory or the Startup Configuration may be corrupted, Cannot continue Operation\n\t\"Please Restart the Machine\".");	__chalt(true);}
	void *RootTable = (void *)(Candidate->PhysicalStart);
	uint64_t Offset = 0, Pointer = 0;	if(IsLA57Enabled()){
		DebugPrintf("\nInitialising L5 Table");
		PML5Entry *L5 = RootTable + Offset;
		memset((void *)L5, 0, PAGE_SIZE);
		Pointer += sizeof(PML5Table);
		Offset += sizeof(PML5Table);
		for(uint64_t cc = 0; cc < L4Tables; ++cc){
			L5[cc] = (PML5Entry){
				.Accessed = false, .ExecuteDisable = false, .PageInMemory = true, .UserMode = false, .ReadWrite = true, 
				.PageLevelCacheDisable = false, .PageLevelWriteThrough = false, .PhysicalAddress = 0x00, .Present = true};
			SetPagePhysicalAddress(L5[cc], ((uint64_t)RootTable + Pointer + (cc * sizeof(PML4Table))) >> PAGE_SHIFT);
		}
	}
	DebugPrintf("\nInitialising L4 Table");
	Pointer += L4Tables * sizeof(PML4Table);
	for(uint64_t cc = 0; cc < L4Tables; ++cc){
		PML4Entry *L4 = RootTable + Offset;
		Offset += sizeof(PML4Table);
		memset((void *)L4, 0, PAGE_SIZE);
		for(uint64_t cc_ = 0; cc_ < L3Tables; ++cc_){
			L4[cc_] = (PML4Entry){.Accessed = false, .ExecuteDisable = false, .PageInMemory = true, .UserMode = false, .ReadWrite = true, 
				.PageLevelCacheDisable = false, .PageLevelWriteThrough = false, .PhysicalAddress = 0x00, .Present = true};
			uint64_t Phys = ((uint64_t)RootTable) + Pointer + (cc_ * sizeof(PDPTTable));
			SetPagePhysicalAddress(L4[cc_], Phys >> PAGE_SHIFT);
		}
	}
	DebugPrintf("\nInitialising L3 Table");
	Pointer += L3Tables * sizeof(PDPTTable);
	for(uint64_t cc = 0; cc < L3Tables; ++cc){
		PDPTEntryDirectory *L3 = RootTable + Offset;
		memset((void *)L3, 0, PAGE_SIZE);
		Offset += sizeof(PDPTTable);
		for(uint64_t cc_ = 0; cc_ < L2Tables; ++cc_){
			L3[cc_] = (PDPTEntryDirectory){.Accessed = false, .ExecuteDisable = false, .PageInMemory = true, .PageLevelCacheDisable = false, 
				.PageLevelWriteThrough = false, .PageSize = false, .PhysicalAddress = 0x00, .Present = true, .ReadWrite = true, .UserMode = false};
			uint64_t Phys = (uint64_t)RootTable + Pointer + (cc_ * sizeof(PageDirectory));
			SetPagePhysicalAddress(L3[cc_], Phys >> PAGE_SHIFT);
		}
	}
	DebugPrintf("\nInitialising L2 Table");
	uint64_t Max = __roundupt(TotalMemoryPages * PAGE_SIZE, PAGE_SIZE2) / PAGE_SIZE2;
	for(uint64_t cc = 0; cc < L2Tables; ++cc){
		PageDirectoryEntry2MB *L2 = RootTable + Offset;
		Offset += sizeof(PageDirectory);
		for(uint64_t cc_ = 0; cc_ < PagingTableLength; ++cc_){
			L2[cc_] = (PageDirectoryEntry2MB){.Accessed = false, .Dirty = false, .ExecuteDisable = false, .Global = false, 
				.PageAttributeTable = false, .PageInMemory = true, .PageLevelCacheDisable = false, .PageLevelWriteThrough = false, 
				.PageSize = true, .PhysicalAddress = 0x00, .Present = true, .ProtectionKey = 0x00, .ReadWrite = true, .UserMode = false};
			SetPagePhysicalAddress(L2[cc_], ((cc * PagingTableLength) + cc_) % Max);
		}
	}
	DebugPrintf("\nLoading CR3");
	LoadPageTree(RootTable, 0x10, 0x08, (bool64_t)_L);
	GetPageTable(&_L, _UpdatePageTableMode, RootTable);
	InvalidatePages(NULL, TotalMemoryPages * PAGE_SIZE);
	PageCoordinate Out = {.EnableL1 = false, .EnableL2 = false, .EnableL3 = false, .EnableL4 = true, 
		.EnableL5 = IsLA57Enabled(), .L1 = 0x00, .L2 = L2Entries % PagingTableLength, .L3 = L2Tables, .L4 = L3Tables, .L5 = L4Tables};
	DebugPrintf("\nPage Table Max:\t0b%2z", Out);
	return InitVirtualAddress(Out);
}

void PrintPageTable(PageCoordinate PC){
	bool L5 = IsLA57Enabled();
	uint32_t Lvl = L5? 5: 4;
	void *_pt = GetPageTable(&L5, _StoredPageTableMode, NULL);
	void *E = WalkEmulatedPageTree(PC, _pt, &Lvl);
	if(!E){DebugPrintf("\n{null}");}else{
		switch(Lvl){
			case 5: {
				PML5Entry *L5 = E;
				DebugPrintf("\nPML5\t{P:%z  RW:%z  UM:%z  PLWT:%z  PLCD:%z  A:%z  PIM:%z  PADR:0x%0z  ExD:%z}", 
					(uint64_t)L5->Present, (uint64_t)L5->ReadWrite, (uint64_t)L5->UserMode, (uint64_t)L5->PageLevelWriteThrough, (uint64_t)L5->PageLevelCacheDisable, 
					(uint64_t)L5->Accessed, (uint64_t)L5->PageInMemory, (uint64_t)L5->PhysicalAddress, (uint64_t)L5->ExecuteDisable);
				break;
			} case 4: {
				PML4Entry *L4 = E;
				DebugPrintf("\n\tPML4\t{P:%z  RW:%z  UM:%z  PLWT:%z  PLCD:%z  A:%z  PIM:%z  PADR:0x%0z  ExD:%z}", 
					(uint64_t)L4->Present, (uint64_t)L4->ReadWrite, (uint64_t)L4->UserMode, (uint64_t)L4->PageLevelWriteThrough, (uint64_t)L4->PageLevelCacheDisable, 
					(uint64_t)L4->Accessed, (uint64_t)L4->PageInMemory, (uint64_t)L4->PhysicalAddress, (uint64_t)L4->ExecuteDisable);
				break;
			} case 3: {
				PDPTEntry1GB *L3 = E;	if(L3->PageSize){
					DebugPrintf("\n\t\tPDPT(1GB)\t{P:%z  RW:%z  UM:%z  PLWT:%z  PLCD:%z  A:%z  D:%z  G:%z  PIM:%z  PAT:%z  PADR:0x%0z  PK:%z  ExD:%z}", 
					(uint64_t)L3->Present, (uint64_t)L3->ReadWrite, (uint64_t)L3->UserMode, (uint64_t)L3->PageLevelWriteThrough, (uint64_t)L3->PageLevelCacheDisable, (uint64_t)L3->Accessed, (uint64_t)L3->Dirty, 
					(uint64_t)L3->Global, (uint64_t)L3->PageInMemory, (uint64_t)L3->PageAttributeTable, (uint64_t)L3->PhysicalAddress, (uint64_t)L3->ProtectionKey, (uint64_t)L3->ExecuteDisable);
				}else{DebugPrintf("\n\t\tPDPT\t{P:%z  RW:%z  UM:%z  PLWT:%z  PLCD:%z  A:%z  PIM:%z  PADR:0x%0z  ExD:%z}", 
					(uint64_t)((PDPTEntryDirectory *)L3)->Present, (uint64_t)((PDPTEntryDirectory *)L3)->ReadWrite, (uint64_t)((PDPTEntryDirectory *)L3)->UserMode, 
					(uint64_t)((PDPTEntryDirectory *)L3)->PageLevelWriteThrough, (uint64_t)((PDPTEntryDirectory *)L3)->PageLevelCacheDisable, (uint64_t)((PDPTEntryDirectory *)L3)->ExecuteDisable, 
					(uint64_t)((PDPTEntryDirectory *)L3)->Accessed, (uint64_t)((PDPTEntryDirectory *)L3)->PageInMemory, (uint64_t)((PDPTEntryDirectory *)L3)->PhysicalAddress);
				}
				break;
			} case 2: {
				PageDirectoryEntry2MB *L2 = E;	if(L2->PageSize){
					DebugPrintf("\n\t\t\tPD\t{P:%z  RW:%z  UM:%z  PLWT:%z  PLCD:%z  A:%z  D:%z  G:%z  PIM:%z  PAT:%z  PADR:0x%0z  PK:%z  ExD:%z}", 
						(uint64_t)L2->Present, (uint64_t)L2->ReadWrite, (uint64_t)L2->UserMode, (uint64_t)L2->PageLevelWriteThrough, (uint64_t)L2->PageLevelCacheDisable, (uint64_t)L2->Accessed, (uint64_t)L2->Dirty, 
						(uint64_t)L2->Global, (uint64_t)L2->PageInMemory, (uint64_t)L2->PageAttributeTable, (uint64_t)L2->PhysicalAddress, (uint64_t)L2->ProtectionKey, (uint64_t)L2->ExecuteDisable);
				}else{DebugPrintf("\n\t\t\tPD(2MB)\t{P:%z  RW:%z  UM:%z  PLWT:%z  PLCD:%z  A:%z  PIM:%z  PADR:0x%0z  ExD:%z}", 
					(uint64_t)((PageDirectoryEntry2MB *)L2)->Present, (uint64_t)((PageDirectoryEntry2MB *)L2)->ReadWrite, (uint64_t)((PageDirectoryEntry2MB *)L2)->UserMode, 
					(uint64_t)((PageDirectoryEntry2MB *)L2)->PageLevelWriteThrough, (uint64_t)((PageDirectoryEntry2MB *)L2)->PageLevelCacheDisable, (uint64_t)((PageDirectoryEntry2MB *)L2)->ExecuteDisable, 
					(uint64_t)((PageDirectoryEntry2MB *)L2)->Accessed, (uint64_t)((PageDirectoryEntry2MB *)L2)->PageInMemory, (uint64_t)((PageDirectoryEntry2MB *)L2)->PhysicalAddress);
				}
				break;
			} case 1: {
				PageTableEntry4KB *L1 = E;		DebugPrintf("\n\t\t\t\tPE\t{P:%z  RW:%z  UM:%z  PLWT:%z  PLCD:%z  A:%z  D:%z  G:%z  PIM:%z  PAT:%z  PADR:0x%0z  PK:%z  ExD:%z}", 
					(uint64_t)L1->Present, (uint64_t)L1->ReadWrite, (uint64_t)L1->UserMode, (uint64_t)L1->PageLevelWriteThrough, (uint64_t)L1->PageLevelCacheDisable, (uint64_t)L1->Accessed, (uint64_t)L1->Dirty, 
					(uint64_t)L1->Global, (uint64_t)L1->PageInMemory, (uint64_t)L1->PageAttributeTable, (uint64_t)L1->PhysicalAddress, (uint64_t)L1->ProtectionKey, (uint64_t)L1->ExecuteDisable);
				break;
			} 
		}
	}
}

//	Heavily relies of UEFI's 1:1 Mapping Mode
bool InitialiseAllocationState(void *D_, const uint64_t StructSize, const uint64_t N, const uint64_t TotalMemory){
	DebugPrintf("\nInitialising Memory\t%s\tStructSize:%z\tN:%z\tTotalMemory:%z", 
		(GetIState())->PhysicalDescriptorIndexes? "Initialised": "Uninitialised", StructSize, N, TotalMemory);
	if((GetIState())->PhysicalDescriptorIndexes){return true;}	//	Memory already Initialised
	uint64_t		DescriptorAllocation = 0, 
	//	Allocation for SubDescriptors, Temporary. We can expand later.
	//		An entry per 2 Mb
	//		For the 4Gb Default, it's around 2048 Entries in total
					SubDescriptorAllocation = 0, 
	//	Allocation for Physical to Index Wrappers.
					PhysToIndexAllocation = (TotalMemory / PAGE_SIZE) * sizeof(uint16_t);
	//	End address of the last piece seen for each class (TypeMask0..3); -1 = none yet.
	int64_t				TypeEnd[4] = {-1, -1, -1, -1};

	for(uint64_t cc = 0; cc < N; ++cc){
		GetEfiMemoryDescriptor(D, D_, cc, StructSize);
		int Class = memcheckl((uint32_t *)TypeMask0, D->Type, sizeof(TypeMask0))? 0: (memcheckl((uint32_t *)TypeMask1, D->Type, sizeof(TypeMask1))? 1: 
			(memcheckl((uint32_t *)TypeMask2, D->Type, sizeof(TypeMask2))? 2: (memcheckl((uint32_t *)TypeMask3, D->Type, sizeof(TypeMask3))? 3: -1)));
		if(Class < 0){continue;}
		//	A new Descriptor starts whenever this piece is not contiguous with the previous piece of its class.
		if(TypeEnd[Class] != (int64_t)D->PhysicalStart){DescriptorAllocation += sizeof(MemoryRangeDescriptor);}
		TypeEnd[Class] = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);
		//	Allocate Extra for Additional Allocations.
		SubDescriptorAllocation += __roundupt(D->NumberOfPages > (nMB(2ULL) / PAGE_SIZE)? __roundupt(D->NumberOfPages * PAGE_SIZE, nMB(2ULL)) / nMB(2ULL): D->NumberOfPages, SubDescriptorRoundup);
	}
	uint64_t TotalRequiredPages = ((SubDescriptorAllocation * sizeof(MemoryRangeSubDescriptor)) + PhysToIndexAllocation + DescriptorAllocation + sizeof(uint64_t));
			TotalRequiredPages = __roundupt(TotalRequiredPages, PAGE_SIZE) / PAGE_SIZE;

	VirtualAddress VA = {0};
	volatile InternalAllocationState *IState = GetIState();
	DebugPrintf("\nInitialising Page Table");
	VA = InstantiatePageTable(D_, StructSize, N, TotalMemory / PAGE_SIZE);

	uint64_t ElapsedPages = 0;
	EFI_MEMORY_DESCRIPTOR *Candidate = AllocateCandidatePool(D_, StructSize, &ElapsedPages, N, TotalRequiredPages, TypeMask1, sizeof(TypeMask1), AttributeMask);
	if(!Candidate){DebugPrintf("\nNot enough Memory installed on System; or System Configuration is corrupted. Please Restart.");	__chalt(true);}
	// DebugPrintf("\nDescriptor Pool: 0x%0z\t%z Pages", Candidate->PhysicalStart, (uint64_t)ElapsedPages);
	for(uint64_t cc = 0; cc < N; ++cc){
		GetEfiMemoryDescriptor(D, D_, cc, StructSize);
		bool Usable = memcheckl((uint32_t *)TypeMask1, D->Type, sizeof(TypeMask1)) && ((AttributeMask& D->Attribute) == 0);
		DebugPrintf("\n[%z]:\tType: %z(%s){Attr: 0b%2z\tPhys: 0x%0z\tNoPages: %z}", (uint64_t)cc, (uint64_t)D->Type, 
			(Usable? "Usable": "  ??  "), D->Attribute, D->PhysicalStart, D->NumberOfPages);
	}
	DebugPrintf("\nInitialising Internal Allocation State");

	*IState = (volatile InternalAllocationState){
		.Descriptors = (MemoryRangeDescriptor *)__roundupt(Candidate->PhysicalStart, PAGE_SIZE), 
		.PhysicalDescriptorIndexes = (uint16_t *)__roundupt(Candidate->PhysicalStart + DescriptorAllocation, PAGE_SIZE), 
		.SubDescriptors = (MemoryRangeSubDescriptor **)__roundupt(Candidate->PhysicalStart + DescriptorAllocation + PhysToIndexAllocation, PAGE_SIZE), 
		.TotalDescriptors = DescriptorAllocation / sizeof(MemoryRangeDescriptor), .TotalSubDescriptors = (SubDescriptorAllocation), .LastAllocation = 0, 
		.CurrentSubDescriptorPool = 0, .TotalSubDescriptorPools = 1, .DuplicatePageTableBase = InitPageCoordinate(VA), .Ready = false, .MemoryMaxPages = TotalMemory / PAGE_SIZE, 
	};	*(IState->SubDescriptors) = (MemoryRangeSubDescriptor *)__roundupt(Candidate->PhysicalStart + DescriptorAllocation + PhysToIndexAllocation, PAGE_SIZE) + sizeof(uint64_t);
	DebugPrintf("\nD:0x%0z, I:0x%0z, SD:0x%0z, TD:%z, TSD:%z, CSDP:%z, TSDP:%z, DP:0b%2z, Max:%z", IState->Descriptors, IState->PhysicalDescriptorIndexes, IState->SubDescriptors, 
		IState->TotalDescriptors, IState->TotalSubDescriptors, IState->CurrentSubDescriptorPool, IState->TotalSubDescriptorPools, IState->DuplicatePageTableBase, IState->MemoryMaxPages);
	for(uint64_t cc = 0; cc < IState->TotalSubDescriptorPools; ++cc){DebugPrintf(", [%z]0x%0z", cc, IState->SubDescriptors[cc]);}
	TypeEnd[0] = TypeEnd[1] = TypeEnd[2] = TypeEnd[3] = -1;
	uint64_t CurrentSubDescriptor = 0;	{
		int64_t ArrayCtr = -1;
		uint64_t TotalEntries = 0x00, Entries = 0, Pages = 0, Counter = 0, 
				LastAddress = ((EFI_MEMORY_DESCRIPTOR *)D_)->PhysicalStart,	Takeaway = 0;
		EFI_MEMORY_DESCRIPTOR *D = NULL;
		while((D = QueryContiguousDescriptors(D_, StructSize, &Entries, &Pages, &Counter, N)) && N > TotalEntries){
			if(!Pages){continue;}
			TotalEntries += Entries;	ArrayCtr++;
			int Class = memcheckl((uint32_t *)TypeMask0, D->Type, sizeof(TypeMask0))? 0: (memcheckl((uint32_t *)TypeMask1, D->Type, sizeof(TypeMask1))? 1: 
			(memcheckl((uint32_t *)TypeMask2, D->Type, sizeof(TypeMask2))? 2: (memcheckl((uint32_t *)TypeMask3, D->Type, sizeof(TypeMask3))? 3: -1)));
			if(Class < 0){continue;}
			IState->Descriptors[ArrayCtr] = (MemoryRangeDescriptor){.SubDescriptorIndex = CurrentSubDescriptor, .EfiMemoryType = D->Type, .PageLimit = Pages, .Physical = D->PhysicalStart};
			TypeEnd[Class] = D->PhysicalStart + (Pages * PAGE_SIZE);
			for(uint64_t cc_ = 0; cc_ < Pages; ++cc_){IState->PhysicalDescriptorIndexes[(D->PhysicalStart / PAGE_SIZE) + cc_] = (ArrayCtr & UINT16_MAX);}
			//	We split into 2mB descriptors, 
			uint32_t NSubdescriptors = __roundupt(Pages > (nMB(2ULL) / PAGE_SIZE)? __roundupt(Pages * PAGE_SIZE, nMB(2ULL)) / nMB(2ULL): Pages, SubDescriptorRoundup);
			bool IsNonPhysical = memcheckl((uint32_t *)TypeMask3, D->Type, sizeof(TypeMask3)), 
				IsSubDescriptorMapping = ((uint64_t)(IState->SubDescriptors) >= D->PhysicalStart) && ((uint64_t)(IState->SubDescriptors) < (D->PhysicalStart + (Pages * PAGE_SIZE)))? true: false, 
				Touchable = !memcheckl((uint32_t *)TypeMask4, D->Type, sizeof(TypeMask4)), Freeable = !memcheckl((uint32_t *)TypeMask0, D->Type, sizeof(TypeMask0)) && Touchable, 
				IsUsed = memcheckl((uint32_t *)TypeMask2, D->Type, sizeof(TypeMask2)) || memcheckl((uint32_t *)TypeMask0, D->Type, sizeof(TypeMask0)), 
				IsUsable = Touchable && (IsUsed || IsSubDescriptorMapping || Freeable);
			DebugPrintf("\nSubDescriptor[%i(0x%0z):%l] Details:\tIsUsed? %s\tIsUsable: %s\tIsPhysical: %s\tIsSubDescriptorMapping: %s\tIsFreeable: %s\tIsTouchable: %s", 
				(uint32_t)ArrayCtr, (uint64_t)(IState->SubDescriptors[IState->CurrentSubDescriptorPool] + CurrentSubDescriptor), (uint32_t)NSubdescriptors, 
				(IsUsed? "Used": "??"), (IsUsable? "Usable": "??"), (IsNonPhysical? "??": "Physical"), (IsSubDescriptorMapping? "SD Mapping": "??"), (Freeable? "Freeable": "??"), (Touchable? "Touchable": "??"));
			const uint64_t Precision = NSubdescriptors? (D->NumberOfPages * PAGE_SIZE) / NSubdescriptors: 0x00;
			for(uint64_t cc = 0; cc < NSubdescriptors; ++cc){
				const uint64_t L2 = (D->PhysicalStart + (Precision * cc)) / PAGE_SIZE, L3 = L2? (L2 / PagingTableLength): 0, L4 = L3? (L3 / PagingTableLength): 0x00, L5 = L4? (L4 / PagingTableLength): 0x00;
				IState->SubDescriptors[IState->CurrentSubDescriptorPool][CurrentSubDescriptor + cc] = (MemoryRangeSubDescriptor){
					.AllocationType = (IsUsed? PATUsedRegion: 0x00) | (IsNonPhysical? PATNonPhysicalDescriptor: 0x00) | (Touchable? 0x00: PATUntouchableRegion) | 
						(IsSubDescriptorMapping? PATSubDescriptorMapping: 0x00) | (Freeable? 0x00: (PATUnfreeableRegion | PATUntouchableRegion)), .Valid = IsUsed, 
					.PC = IsUsed? (PageCoordinate){.EnableL1 = false, .EnableL2 = false, .EnableL3 = true, .EnableL4 = true, .EnableL5 = IsLA57Enabled(), 
						.L5 = L5, .L4 = L4, .L3 = L3, .L2 = L2, .L1 = 0}: (PageCoordinate){0}, .EndOfList = (((cc + 1) - NSubdescriptors) == 0? true: false), .VirtualNeighbour = UINT32_MAX};
			}
			CurrentSubDescriptor += (NSubdescriptors + SubDescriptorRoundup);
			Takeaway += (LastAddress > D->PhysicalStart? D->PhysicalStart - LastAddress: 0x00);
			LastAddress = D->PhysicalStart + (Pages * PAGE_SIZE);
		};
	}
	IState->Ready = true;
	DebugPrintf("\nPtr: 0x%0z", AllocNoUpdate(PagingTableLength * 64, ReadWritable | SupervisorMode | ExecuteEnable | PageLevelCacheEnable, 0x00));

	// uint64_t Test = 0;
	// __bochs();
	// SimpleMemoryRangeDescriptor_t *Arr = QueryFreeMemory(InvalidPointer, 32 * PagingTableLength, &Test);
	// DebugPrintf("\nOutput:\t{");
	// while(Test){
	// 	DebugPrintf("\n\t[%z]{%z\t%z\t%z}", Test, Arr[Test - 1].PhysicalBase, Arr[Test - 1].PhysicalLimit, Arr[Test - 1].Type);
	// 	Test--;
	// }
	// DebugPrintf("\n}");
	// BufferPageTableAlloc(Test, Arr, PC)DebugPrintf("\t0b%2z", PC);
	__chalt(true);
}

#pragma region Memory Helpers

uint64_t TotalMemorySize(void){return (GetIState())->MemoryMaxPages * PAGE_SIZE;}
bool MemoryIsMapped(void *ptr){InternalAllocationState *IState = GetIState();	
	return (_GetSubDescriptorByVirtual(ptr, IState, NULL) == NULL? false: true);}
uint64_t TotalMappedMemory(void *ptr){
	InternalAllocationState *IState = GetIState();
	uint64_t out = 0;
	while(true){
		const MemoryRangeSubDescriptor *SD = (const MemoryRangeSubDescriptor *)_GetSubDescriptorByVirtual(ptr, IState, NULL);
		if(!SD){break;}
		uint64_t Elapsed = _GetPageCoordinateElapsedBytes(SD->PC);
		out += Elapsed;
		ptr += Elapsed;
	}
	return out;
}

void SortSubDescriptors(MemoryRangeDescriptor *D){
	InternalAllocationState *IState = GetIState();
	if(D->SubDescriptorIndex > (IState->TotalSubDescriptors / IState->TotalSubDescriptorPools)){return;}
	MemoryRangeSubDescriptor *SD = IState->SubDescriptors[IState->CurrentSubDescriptorPool] + D->SubDescriptorIndex;
	const uint64_t SDCount = _GetSubDescriptorCount(D);
	for(uint64_t cc = 0; true; ++cc){
		//	Find the Entry w/ the Lowest Diff that is Proceeding
		const uint64_t Curr = _GetPageCoordinatePhysicalAddress(SD[cc].PC);
		uint64_t Diff = UINT64_MAX;
		MemoryRangeSubDescriptor *SD_ = SD;
		uint64_t cc_ = 1, Select = 0;
		for(; true; ++cc_){
			if((uint64_t)(SD_ + cc_) == (uint64_t)(SD + cc)){continue;}
			const uint64_t Cmp = _GetPageCoordinatePhysicalAddress(SD_[cc_].PC);
			if((Cmp < Curr) && ((Curr - Cmp) < Diff)){Diff = Curr - Cmp;	Select = cc_;}
			if(SD_[cc_].EndOfList){break;}
		}
		if((Diff != UINT64_MAX) && Diff){
			MemoryRangeSubDescriptor Tmp = SD[cc];
			SD[cc] = SD_[Select];
			SD_[Select] = Tmp;
			SD[cc].EndOfList = ((cc + 1) - SDCount) == 0;
			SD_[Select].EndOfList = ((Select + 1) - SDCount) == 0;
		}
		if(SD[cc].EndOfList){break;}
	}
}

//*	Returns Identity-Mapped Memory, so the Physical and Virtual Addresses are interchangeable.
//*	The Memory is Fragmented, so the caller must NEVER make assumptions about Memory
#define QueryFreeMemoryMaxRanges	(204)
_Static_assert((sizeof(SimpleMemoryRangeDescriptor_t) * QueryFreeMemoryMaxRanges) <= PAGE_SIZE, "QueryFreeMemory's buffer must fit in one page");
SimpleMemoryRangeDescriptor_t *QueryFreeMemory(void *Physical, uint64_t Pages, uint64_t *Count, const EFI_MEMORY_TYPE *TypeMask, uint64_t TypeMaskLength){
	volatile InternalAllocationState *IState = GetIState();
	static __linkersection(.bss) SimpleMemoryRangeDescriptor_t out[QueryFreeMemoryMaxRanges];
	uint64_t Ctr = 0, _Count = 0;
	if(!Count){Count = &_Count;}
	*Count = 0;
	if(!IState->Ready){return NULL;}
	if(!Pages){DebugPrintf("\nNo Requested Pages");	return out;}
	if(Pages > IState->MemoryMaxPages){DebugPrintf("\nInvalid NoPages");	return NULL;}
	const uint64_t Start = Physical == InvalidPointer? 0: __roundup(NormaliseAddress((uint64_t)Physical, true), PAGE_SIZE);
	uint64_t Need = Pages;
	//	PhysicalDescriptorIndexes is indexed per PAGE; Start is a byte address.
	if((Start / PAGE_SIZE) >= IState->MemoryMaxPages){DebugPrintf("\nStart is beyond Physical Memory");	return NULL;}
	const uint64_t StartIdx = IState->PhysicalDescriptorIndexes[Start / PAGE_SIZE];
	//	Descriptors are in ascending physical order, so the result is too.
	for(uint64_t cc = StartIdx; (cc < IState->TotalDescriptors) && Need; ++cc){
		const MemoryRangeDescriptor *D = IState->Descriptors + cc;
		if(!memcheckl((void *)TypeMask, D->EfiMemoryType, TypeMaskLength)){continue;}
		if(D->SubDescriptorIndex > (IState->TotalSubDescriptors / IState->TotalSubDescriptorPools)){continue;}
		SortSubDescriptors((MemoryRangeDescriptor *)D);
		const MemoryRangeSubDescriptor *SD = IState->SubDescriptors[IState->CurrentSubDescriptorPool] + D->SubDescriptorIndex;
		//	We need to find gaps in the Physical Address Space of D, by looking at the SubDescriptors
		//		Update SMRD at Ctr to map the Remaining Bytes/Pages of D.
		//		Find first Valid SD.
		//		Update SMRD at Ctr to point to Base of SD.
		//		Increment Ctr
		//		Repeat across Descriptors, until Need == 0
		if((Ctr + 2) >= QueryFreeMemoryMaxRanges){DebugPrintf("\nQueryFreeMemory buffer full");		break;}
		out[Ctr] = (SimpleMemoryRangeDescriptor_t){.PhysicalBase = (void *)D->Physical, .PhysicalLimit = D->Physical + (D->PageLimit * PAGE_SIZE), .Type = D->EfiMemoryType};
		for(uint64_t cc_ = 0; Need; ++cc_){
			const uint64_t Phys = _GetPageCoordinatePhysicalAddress(SD[cc_].PC), Tmp = _GetPageCoordinateElapsedPages(SD[cc_].PC);
			//	Make sure the SD is not elapsed by any entry in Arr
			const uint64_t DEnd = D->Physical + (D->PageLimit * PAGE_SIZE);
			const bool Claimed = ((SD[cc_].AllocationType & AllocationMask) != 0) && SD[cc_].Valid, Sane = (Phys >= D->Physical) && (Phys < DEnd);
			if(Claimed && !Sane){DebugPrintf("\nSubDescriptor[%z] decodes to 0x%0z, outside [0x%0z, 0x%0z): ignored", cc_, Phys, D->Physical, DEnd);}
			if(Claimed && Sane){
				out[Ctr].PhysicalLimit = Phys;
				DebugPrintf("\n[%z]{0x%0z\t0x%0z\t%z}", Ctr, out[Ctr].PhysicalBase, out[Ctr].PhysicalLimit, out[Ctr].Type);
				if(!UpdateAllocationTableByPhysical((uint64_t)out[Ctr].PhysicalBase, UATPageAllocationType | UATValidBit, false, PATUsedRegion, false)){
					if(!AppendSingleAllocationTableEntry((uint64_t)out[Ctr].PhysicalBase, UATPageAllocationType | UATValidBit, PATUsedRegion, false)){
						DebugPrintf("\nFailed to Append SubDescriptor");	__chalt(true);}}
				//	The next free range begins AFTER the used block, not at its base (that overlapped it).
				uint64_t TailBase = Phys + (((Tmp != 0) && (Tmp < (UINT64_MAX / PAGE_SIZE)))? (Tmp * PAGE_SIZE): 0);
				if(TailBase > (D->Physical + (D->PageLimit * PAGE_SIZE))){TailBase = D->Physical + (D->PageLimit * PAGE_SIZE);}
				out[Ctr + 1] = (SimpleMemoryRangeDescriptor_t){
					.PhysicalBase = (void *)TailBase, .PhysicalLimit = D->Physical + (D->PageLimit * PAGE_SIZE), .Type = D->EfiMemoryType};
				if(Need > ((out[Ctr].PhysicalLimit - (uint64_t)out[Ctr].PhysicalBase) / PAGE_SIZE)){
					Need -= (out[Ctr].PhysicalLimit - (uint64_t)out[Ctr].PhysicalBase) / PAGE_SIZE;}else{Need = 0;}
				Ctr++;
			}else{
				// DebugPrintf("\n[%z]{\t0b%2z\t\t%z\t0b%2z\t%z\t0x%0z}", (uint64_t)cc_, (uint64_t)SD[cc_].AllocationType, 
				// 	(uint64_t)(SD[cc_].EndOfList), SD[cc_].PC, (uint64_t)(SD[cc_].Valid), (uint64_t)SD[cc_].VirtualNeighbour);
				// if(SD[cc_].Valid){QuickPageWalk(SD[cc_].PC, Page){DebugPrintf("\tPage:\t0x%0z\tAddress:\t0x%0z\tSize(Pages):\t0x%0z\t%z", Page, Phys, Tmp, PageLvl);}else{continue;}}
			}
			if(SD[cc_].EndOfList){break;}
		}
	}
	if(Need){DebugPrintf("\nFailed to Find enough Memory"); return NULL;}		//	not enough free memory (*Count is still 0)
	*Count = (Ctr + 1);
	return out;
}

bool _AppendSingleAllocationTableEntry(uint64_t Physical, UATBitMask _BitMask, va_list ls){
	InternalAllocationState *IState = GetIState();
	if(Physical != UINT64_MAX){
		GetDescriptor(D, Physical, IState){
			if(D->SubDescriptorIndex > (IState->TotalSubDescriptors / IState->TotalSubDescriptorPools)){return false;}
			MemoryRangeSubDescriptor *SD = IState->SubDescriptors[IState->CurrentSubDescriptorPool] + D->SubDescriptorIndex;
			uint8_t N = 0;
			//	Check for an entry that is not used
			do{
				if(!__check(SD[N].AllocationType, PATUsedRegion)){
					SD[N].AllocationType = __check(_BitMask, UATPageAllocationType)? va_arg(ls, PageAllocationType): 0x00;
					SD[N].PC = __check(_BitMask, UATPageCoordinate)? va_arg(ls, PageCoordinate): (PageCoordinate){0};
					SD[N].Valid = __check(_BitMask, UATValidBit)? va_arg(ls, uint32_t): 0x00;
					SD[N].VirtualNeighbour = __check(_BitMask, UATVirtualNeighbour)? va_arg(ls, uint32_t): 0x00;
					return true;
				}
				++N;
			}while(!SD[N].EndOfList);
			return false;
		}else{DebugPrintf("\nFailed to find associated Descriptor for Physical Region");}
	}else{
		//!	We need to frigging walk the Entire Descriptor Table
		//!		On everything slime out the Caller Function.
		for(uint64_t cc = 0; cc < IState->TotalDescriptors; ++cc){
			MemoryRangeDescriptor *D = IState->Descriptors + cc;	{
				MemoryRangeSubDescriptor *SD = IState->SubDescriptors[IState->CurrentSubDescriptorPool] + D->SubDescriptorIndex;
				uint8_t N = 0;
				//	Check for an entry that is not used, and is
				do{	if(!__check(SD[N].AllocationType, PATUsedRegion)){break;}	++N;}while(!SD[N].EndOfList);
				//	Ensure that the Entry is usable.
				if(__check(SD[N].AllocationType, PATUsedRegion)){continue;}
				// SD[N].EndOfList = false;
				SD[N + 1].AllocationType = __check(_BitMask, UATPageAllocationType)? va_arg(ls, PageAllocationType): 0x00;
				SD[N + 1].PC = __check(_BitMask, UATPageCoordinate)? va_arg(ls, PageCoordinate): (PageCoordinate){0};
				SD[N + 1].Valid = __check(_BitMask, UATValidBit)? va_arg(ls, uint32_t): 0x00;
				SD[N + 1].VirtualNeighbour = __check(_BitMask, UATVirtualNeighbour)? va_arg(ls, uint32_t): 0x00;
				// SD[N + 1].EndOfList = true;
				return true;
			}
		}
		//!	Not enough Memory
		//*	Allocate additional SubDescriptor pool; giving all 'sub'pools an extra %macro% SubdescriptorRoundup, number of entries.
		const uint64_t TotalBytes = ((IState->TotalSubDescriptors + (IState->TotalDescriptors * SubDescriptorRoundup)) * sizeof(MemoryRangeSubDescriptor)) + (sizeof(uint64_t) * (IState->TotalSubDescriptorPools + 1)), 
						TotalPages = __roundup(TotalBytes, PAGE_SIZE) / PAGE_SIZE;
		void *Base = AllocNoUpdate(TotalPages, ReadWritable | SupervisorMode | GlobalEnable, 0x00);
		uint64_t *Tmp = (uint64_t *)Base;
		for(uint64_t cc = 0; cc < IState->TotalSubDescriptorPools; ++cc){
			Tmp[cc] = (uint64_t)(Tmp + (IState->TotalSubDescriptorPools + 1)) + (((IState->TotalSubDescriptors / IState->TotalSubDescriptorPools) + SubDescriptorRoundup) * sizeof(MemoryRangeSubDescriptor));
			memcpy((void *)Tmp[cc], IState->SubDescriptors[cc], IState->TotalSubDescriptors / IState->TotalSubDescriptorPools);
			//	Update all allocation tables to have a New EndOfList.
			for(uint64_t cc_ = 0; cc_ < SubDescriptorRoundup; ++cc_){
				((MemoryRangeSubDescriptor *)Tmp[cc])[(IState->TotalSubDescriptors / IState->TotalSubDescriptorPools) + cc_ - 1].EndOfList = ((cc_ + 1) - SubDescriptorRoundup) == 0;}
		}
		IState->TotalSubDescriptors = (IState->TotalSubDescriptors + (IState->TotalDescriptors * SubDescriptorRoundup));
		IState->TotalSubDescriptorPools++;

	}
	return false;
}
bool AppendSingleAllocationTableEntry(uint64_t Physical, UATBitMask _BitMask, ...){
	va_list ls;	va_start(ls, _BitMask);
	bool out = _AppendSingleAllocationTableEntry(Physical, _BitMask, ls);
	va_end(ls);
	return out;
}

uint64_t _UpdateAllocationTableByPhysical(uint64_t Physical, UATBitMask _BitMask, va_list ls){
	if(Physical == UINT64_MAX){return 0;}
	InternalAllocationState *IState = GetIState();
	uint64_t Offset = 0x00;
	MemoryRangeSubDescriptor *SD = _GetSubDescriptorByPhysical((void *)Physical, IState, &Offset);
	if(!SD){va_list _ls;	va_copy(_ls, ls);
		//	Update's list starts with the AND/OR flag; Append's does not.
		if(__check(_BitMask, UATPageAllocationType)){(void)va_arg(_ls, uint32_t);}
		if(_AppendSingleAllocationTableEntry(Physical, _BitMask, _ls)){
			SD = _GetSubDescriptorByPhysical((void *)Physical, IState, &Offset);}else{va_end(_ls);	return 0;}
		va_end(_ls);
	}
	if(SD){
		if(__check(_BitMask, UATPageAllocationType)){
			SD->AllocationType = va_arg(ls, uint32_t)? 
				(SD->AllocationType & va_arg(ls, PageAllocationType)): 
				(SD->AllocationType | va_arg(ls, PageAllocationType));}
		if(__check(_BitMask, UATPageCoordinate)){SD->PC = va_arg(ls, PageCoordinate);}
		if(__check(_BitMask, UATValidBit)){SD->Valid = va_arg(ls, uint32_t);}
		if(__check(_BitMask, UATVirtualNeighbour)){SD->VirtualNeighbour = va_arg(ls, uint32_t);}
		return _GetPageCoordinateElapsedBytes(SD->PC);
	}
	return 0;
}
#define UpdateAllocationTableByVirtual(Virtual, ...)	UpdateAllocationTableByPhysical((uint64_t)MapPhysical(Virtual), __VA_ARGS__)
uint64_t UpdateAllocationTableByPhysical(uint64_t Physical, UATBitMask _BitMask, ...){
	va_list ls;	va_start(ls, _BitMask);
	uint64_t out = _UpdateAllocationTableByPhysical(Physical, _BitMask, ls);
	va_end(ls);
	return out;
}

#pragma region Allocation/Freeing
//*	Takes Size bytes (a power of two: 4KB, 2MB or 1GB), Size-aligned, out of the free-range snapshot in Bfr.
//*	The range is shrunk, split or removed so the same physical memory is never handed out twice.
static bool _TakePhysicalRange(SimpleMemoryRangeDescriptor_t *Bfr, uint64_t *Count, const uint64_t Size, uint64_t *Out){
	for(uint64_t cc = 0; cc < *Count; ++cc){
		const uint64_t Base = (uint64_t)Bfr[cc].PhysicalBase, Limit = Bfr[cc].PhysicalLimit;
		const uint64_t Aligned = (Base + (Size - 1)) & ~(Size - 1);
		if((Aligned < Base) || (Aligned >= Limit) || ((Limit - Aligned) < Size)){continue;}
		const uint64_t TailBase = Aligned + Size;
		const bool Head = (Aligned > Base), Tail = (TailBase < Limit);
		const SimpleMemoryRangeDescriptor_t Orig = Bfr[cc];
		if(Head){
			Bfr[cc].PhysicalLimit = Aligned;
			if(Tail && (*Count < QueryFreeMemoryMaxRanges)){
				for(uint64_t i = *Count; i > (cc + 1); --i){Bfr[i] = Bfr[i - 1];}
				Bfr[cc + 1] = Orig;	Bfr[cc + 1].PhysicalBase = (void *)TailBase;
				(*Count)++;
			}
		}else if(Tail){
			Bfr[cc].PhysicalBase = (void *)TailBase;
		}else{
			for(uint64_t i = cc; (i + 1) < *Count; ++i){Bfr[i] = Bfr[i + 1];}
			(*Count)--;
		}
		*Out = Aligned;
		return true;
	}
	return false;
}

//*	Allocates Memory, without updating the SubDescriptor Table.
//*	Returns the START of the new virtual range, or InvalidPointer on failure.
void *AllocNoUpdate(uint64_t Pages, PageAllocationFlags Flags, uint8_t ProtectionKey){
	const uint64_t Total = __roundup(Pages * PAGE_SIZE, PAGE_SIZE);
	if(!Total){return InvalidPointer;}
	uint64_t Count = 0;
	SimpleMemoryRangeDescriptor_t *Bfr = QueryFreeMemory(InvalidPointer, Total / PAGE_SIZE, &Count, TypeMask1, sizeof(TypeMask1));
	if(!Bfr || !Count){DebugPrintf("\nFailed to discover Free Memory");	__chalt(true);}
	QuickPageTableAlloc(Total, PC);
	//	PC is advanced as pages are mapped, so remember where the allocation starts.
	const PageCoordinate Start = PC;
	uint64_t Remaining = Total;
	while(Remaining){
		QuickPageWalk(PC, Page){
			uint64_t Span = 0, Phys = 0;
			switch(PageLvl){
				case 3:	{if(!((PDPTEntry1GB *)Page)->PageSize){return InvalidPointer;}	Span = PAGE_SIZE3;	break;}
				case 2:	{if(!((PageDirectoryEntry2MB *)Page)->PageSize){return InvalidPointer;}	Span = PAGE_SIZE2;	break;}
				case 1:	{Span = PAGE_SIZE;	break;}
				default:{return InvalidPointer;}
			}
			//	A large page needs a whole, aligned physical block of that size; no undersized fudge.
			if(!_TakePhysicalRange(Bfr, &Count, Span, &Phys)){DebugPrintf("\nNo physical block of 0x%0z bytes", Span);	return InvalidPointer;}
			switch(PageLvl){
				case 3: {
					PDPTEntry1GB *L3 = Page;
					*L3 = (PDPTEntry1GB){.Accessed = false, .Dirty = false, .ExecuteDisable = !__check(Flags, ExecuteEnable), 
						.Global = __check(Flags, GlobalEnable), .PageAttributeTable = __check(Flags, PageAttributeTableEnable), 
						.PageInMemory = true, .PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), 
						.PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable), .PageSize = true, .PhysicalAddress = 0x00, 
						.Present = true, .ProtectionKey = ProtectionKey, .ReadWrite = __check(Flags, ReadWritable), .UserMode = __check(Flags, UserMode)};
					SetPagePhysicalAddress(*L3, Phys >> PAGE_SHIFT3);
					break;
				} case 2: {
					PageDirectoryEntry2MB *L2 = Page;
					*L2 = (PageDirectoryEntry2MB){.Accessed = false, .Dirty = false, .ExecuteDisable = !__check(Flags, ExecuteEnable), 
						.Global = __check(Flags, GlobalEnable), .PageAttributeTable = __check(Flags, PageAttributeTableEnable), 
						.PageInMemory = true, .PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), 
						.PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable), .PageSize = true, .PhysicalAddress = 0x00, 
						.Present = true, .ProtectionKey = ProtectionKey, .ReadWrite = __check(Flags, ReadWritable), .UserMode = __check(Flags, UserMode)};
					SetPagePhysicalAddress(*L2, Phys >> PAGE_SHIFT2);
					break;
				} default: {
					PageTableEntry4KB *L1 = Page;
					*L1 = (PageTableEntry4KB){.Accessed = false, .Dirty = false, .ExecuteDisable = !__check(Flags, ExecuteEnable), 
						.Global = __check(Flags, GlobalEnable), .PageAttributeTable = __check(Flags, PageAttributeTableEnable), 
						.PageInMemory = true, .PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), .Present = true, 
						.PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable), .PhysicalAddress = 0x00, 
						.ProtectionKey = ProtectionKey, .ReadWrite = __check(Flags, ReadWritable), .UserMode = __check(Flags, UserMode)};
					SetPagePhysicalAddress(*L1, Phys >> PAGE_SHIFT);
					break;
				}
			}
			AddPageCoordinate(&PC, Span);
			Remaining = (Remaining > Span)? (Remaining - Span): 0;
		}else{return InvalidPointer;}
	}
	VirtualAddress VA = InitVirtualAddress(Start);
	return (void *)VirtualAddressToU64(VA);
}

void *AllocatePages(void *Base, uint64_t NBytes, PageAllocationFlags Flags, uint8_t ProtectionKey){
	InternalAllocationState *IState = GetIState();
	uint64_t Count = 0;
	NBytes = __roundup(NBytes, PAGE_SIZE);
	SimpleMemoryRangeDescriptor_t *Bfr = QueryFreeMemory(Base, NBytes / PAGE_SIZE, &Count, TypeMask1, sizeof(TypeMask1));
	if(!Bfr || !Count){DebugPrintf("\nFailed to discover Free Memory");	__chalt(true);}
	BufferPageTableAlloc(Count, Bfr, PC);
	MemoryRangeSubDescriptor *Tmp[Count];
	{uint64_t Offset = 0;	PageCoordinate _PC = PC;
		for(uint64_t cc = 0; cc < Count; ++cc){
			if(!(Tmp[cc] = _GetSubDescriptorByPhysical(Bfr[cc].PhysicalBase, IState, NULL))){
				MemoryRangeSubDescriptor *Next = (cc + 1) < Count? _GetSubDescriptorByPhysical(Bfr[cc + 1].PhysicalBase, IState, NULL): InvalidPointer;
				if(!Next){Next = InvalidPointer;}
				if(AppendSingleAllocationTableEntry((uint64_t)Bfr[cc].PhysicalBase, 
					UATPageAllocationType | UATPageCoordinate | UATValidBit | UATVirtualNeighbour, PATUsedRegion, _PC, true, 
					(uint32_t)(Next == InvalidPointer? UINT32_MAX: ((uint64_t)Next - (uint64_t)IState->SubDescriptors[IState->CurrentSubDescriptorPool]) / sizeof(MemoryRangeSubDescriptor)))
				){Tmp[cc] = _GetSubDescriptorByPhysical(Bfr[cc].PhysicalBase, IState, &Offset);}else{return InvalidPointer;}
			}
			const uint64_t Elapsed = _GetPageCoordinateElapsedBytes(_PC);
			const VirtualAddress VA = InitVirtualAddress(_PC);
			UpdatePageTable((void *)VirtualAddressToU64(VA), Elapsed, Flags, ProtectionKey);
			AddPageCoordinate(&_PC, Elapsed);	Offset += Elapsed;
		}
	}
	VirtualAddress VA = InitVirtualAddress(PC);
	uint64_t Virtual = VirtualAddressToU64(VA);
	UpdatePageTable((void *)Virtual, NBytes, Flags, ProtectionKey);
	for(uint64_t cc = 0; cc < Count; ++cc){
		(UpdateAllocationTableByPhysical((uint64_t)(Bfr[cc].PhysicalBase), UATMaximumValue, false, PATUsedRegion, PC, (uint32_t)true, 
			((uint64_t)(Tmp[cc < (Count - 1)? cc + 1: cc]) - (uint64_t)(IState->SubDescriptors[IState->CurrentSubDescriptorPool])) / sizeof(MemoryRangeSubDescriptor)));
		uint64_t _Tmp = Bfr[cc].PhysicalLimit - (uint64_t)Bfr[cc].PhysicalBase;
		while(_Tmp){
			QuickPageWalk(PC, Page){
				switch(PageLvl){
					case 3:	{SetPagePhysicalAddress(*(PDPTEntry1GB *)Page, ((uint64_t)Bfr[cc].PhysicalBase) >> PAGE_SHIFT3);
						if(_Tmp < PAGE_SIZE3){_Tmp = 0;}else{_Tmp -= PAGE_SIZE3;}	AddPageCoordinate(&PC, PAGE_SIZE3);	break;}
					case 2:	{SetPagePhysicalAddress(*(PageDirectoryEntry2MB *)Page, ((uint64_t)Bfr[cc].PhysicalBase) >> PAGE_SHIFT2);
						if(_Tmp < PAGE_SIZE2){_Tmp = 0;}else{_Tmp -= PAGE_SIZE2;}	AddPageCoordinate(&PC, PAGE_SIZE2);	break;}
					case 1:	{SetPagePhysicalAddress(*(PageTableEntry4KB *)Page, ((uint64_t)Bfr[cc].PhysicalBase) >> PAGE_SHIFT);	
						if(_Tmp < PAGE_SIZE){_Tmp = 0;}else{_Tmp -= PAGE_SIZE;}	AddPageCoordinate(&PC, PAGE_SIZE);	break;}
				}
			}else{break;}
		}
	}
	return (void *)Virtual;
}

void FreePages(void *Virtual){
	InternalAllocationState *IState = GetIState();
	if(_GetSubDescriptorByVirtual(Virtual, IState, NULL)){
		UpdateAllocationTableByVirtual(Virtual, UATPageAllocationType, true, ~PATUsedRegion);
		UpdatePageTable(Virtual, TotalMappedMemory(Virtual), PresentBitUsetBit, 0x00);
	}
}