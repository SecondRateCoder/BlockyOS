#include "paging.h"
#include "kernel/libcrt/memory/memory.h"
#include "kernel/libcrt/SimpleOutput/SimpleOutput.h"

static const EFI_MEMORY_TYPE TypeMask0[] = {EfiRuntimeServicesCode, EfiRuntimeServicesData, EfiPalCode, EfiACPIMemoryNVS, EfiUnacceptedMemoryType, EfiUnusableMemory}, 
//	Usable Persistent Memory
							TypeMask1[] = {EfiConventionalMemory, EfiBootServicesCode, EfiBootServicesData, EfiLoaderCode, EfiPersistentMemory}, 
//	Used Persistent Memory
							TypeMask2[] = {EfiLoaderData, KernelBinarySpace, EfiACPIReclaimMemory}, 
//	Non-Physical Memory.
							TypeMask3[] = {EfiReservedMemoryType, EfiMemoryMappedIOPortSpace, EfiMemoryMappedIO};
static const uint64_t		AttributeMask = EFI_MEMORY_WP | EFI_MEMORY_RP | EFI_MEMORY_RO | EFI_MEMORY_NV | EFI_MEMORY_RUNTIME;

#define GetEfiMemoryDescriptor(Desc, Ptr, N, StructSize)	EFI_MEMORY_DESCRIPTOR *Desc = (EFI_MEMORY_DESCRIPTOR *)((void *)(Ptr) + ((N) * (StructSize)))
#define QueryPhysicalAddress(Adr, IState)			\
	if((IState) && (IState)->Ready && (IState)->MemoryMaxPages && ((IState)->MemoryMaxPages > ((uint64_t)(Adr) / PAGE_SIZE)))
#define GetDescriptor(Descriptor, Adr, IState)	\
	MemoryRangeDescriptor *Descriptor = (IState)->Ready? ((Adr < ((IState)->MemoryMaxPages * PAGE_SIZE))? (IState)->Descriptors + (IState)->PhysicalDescriptorIndexes[(uint64_t)(Adr) / PAGE_SIZE]: NULL): NULL;	\
	if(Descriptor)
//	We need the Entry correspondng epecifically to this entry
#define GetSubDescriptor(SubDescriptor, Adr, IState)\
	uint64_t SubDescriptor##Offset = 0;	\
	MemoryRangeSubDescriptor *SubDescriptor = (IState)->Ready? ((Adr < ((IState)->MemoryMaxPages * PAGE_SIZE)) &&	\
		(IState)->Descriptors[(IState)->PhysicalDescriptorIndexes[(uint64_t)(Adr) / PAGE_SIZE]].SubDescriptorIndex != InvalidSubDescriptorIndex?	\
		((IState)->SubDescriptors[(IState)->CurrentSubDescriptorPool] + (IState)->Descriptors[(IState)->PhysicalDescriptorIndexes[(uint64_t)(Adr) / PAGE_SIZE]].SubDescriptorIndex): NULL): NULL;		\
		{uint64_t SubDescriptor##cc = 0, SubDescriptor##CopyAdr = (uint64_t)Adr - (IState)->Descriptors[(IState)->PhysicalDescriptorIndexes[(uint64_t)(Adr) / PAGE_SIZE]].Physical;	\
			while(!SubDescriptor[SubDescriptor##cc].EndOfList && SubDescriptor##CopyAdr){	\
				if((SubDescriptor[SubDescriptor##cc].NoPages * PAGE_SIZE) > SubDescriptor##CopyAdr){SubDescriptor##CopyAdr -= (SubDescriptor[SubDescriptor##cc].NoPages * PAGE_SIZE);}	\
					else{SubDescriptor##CopyAdr = 0;}	SubDescriptor##Offset += SubDescriptor[SubDescriptor##cc].NoPages;	SubDescriptor##cc++;}	\
			SubDescriptor = (SubDescriptor##CopyAdr? NULL: (SubDescriptor + SubDescriptor##cc));\
		}	\
	if(SubDescriptor)
typedef struct{
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
	VirtualAddress			LastPageTableAlloc;
}InternalAllocationState;

//	State decays on each call
bool EnableInputAddress(bool toggle, bool w){
	static __linkersection(.bss) bool _toggle = false;
	bool tmp = _toggle;
	if(w){_toggle = toggle;}
	return tmp;
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
    }else if (carry){pc->L2 += carry;}
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

static volatile InternalAllocationState *GetIState(){
	static __linkersection(.bss) InternalAllocationState __internal = {0};
	return &__internal;
}

uint64_t TotalMemorySize(void){return (GetIState())->MemoryMaxPages * PAGE_SIZE;}
uint64_t TotalMappedMemory(void *ptr){
	if(ptr){
		void *Physical = MapPhysical(ptr);
		volatile InternalAllocationState *IS = GetIState();
		QueryPhysicalAddress(Physical, IS){
			GetSubDescriptor(SubDescriptor, Physical, IS){return SubDescriptor->NoPages * PAGE_SIZE;}
		}else{return 0;}
	}
	return 0;
}
bool __inline MemoryIsMapped(void *ptr){return (TotalMappedMemory(ptr) != 0);}

static __inline void *WalkEmulatedPageTree(PageCoordinate PC, void *PT, uint32_t *Lvl){
	const uint64_t Addr4K = 0x000FFFFFFFFFF000ULL;
	const uint32_t Idx[6] = {0, PC.L1, PC.L2, PC.L3, PC.L4, PC.L5};
	const bool En[6] = {false, PC.EnableL1, PC.EnableL2, PC.EnableL3, PC.EnableL4, PC.EnableL5};
	volatile uint64_t *T = (volatile uint64_t *)PT;
	uint32_t L = *Lvl;
	if((L < 1) || (L > 5)){return NULL;}

	for(; L; --L){
		if(!En[L]){break;}
		volatile uint64_t *E = T + Idx[L];
		if(!(*E)){*Lvl = L;	return NULL;}
		//	Bit 0 clear but entry non-zero: skeleton leaf. Bit 7 is PS at levels 3 and 2 (at level 1 it is PAT).
		if(!((*E) & 1) || (L == 1) || ((L <= 3) && ((*E) & (1ULL << 7)))){*Lvl = L;	return (void *)E;}
		//	Tables are reached through MapVirtual, like the rest of palloc.c (identity today).
		T = (volatile uint64_t *)((void *)((*E) & Addr4K));
	}
	*Lvl = L;
	return (void *)T;
}

bool UpdatePageTable(void *Virtual, uint64_t NBytes, PageAllocationFlags Flags, uint8_t ProtectionKey){
	bool L5Enabled = IsLA57Enabled();
	void *PageTable = GetPageTable(&L5Enabled, _StoredPageTableMode, NULL);
	while(NBytes){
		//	Level in: top level. Level out: the level of the leaf found (3: 1GB, 2: 2MB, 1: 4KB).
		uint32_t Level = IsLA57Enabled()? 5: 4;
		void *PT = WalkEmulatedPageTree(*(PageCoordinate *)&Virtual, PageTable, &Level);
		//	Nothing mapped here: fail instead of looping forever (NBytes never shrank on this path before).
		if(!PT){DebugPrintf("\nFailed to get PT");	return false;}
		switch(Level){
			case 5: {
				PML5Entry *L5 = (PML5Entry *)PT;
				L5->PageInMemory = true;
				L5->Present = true;
				L5->ExecuteDisable = !__check(Flags, ExecuteEnable);
				L5->PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable);
				L5->PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable);
				L5->ReadWrite = __check(Flags, ReadWritable);
				L5->UserMode = __check(Flags, UserMode);
				InvalidatePage(Virtual);
				Virtual += PAGE_SIZE;
				NBytes -= PAGE_SIZE;
				break;
			} case 4: {
				PML4Entry *L4 = (PML4Entry *)PT;
				L4->PageInMemory = true;
				L4->Present = true;
				L4->ExecuteDisable = !__check(Flags, ExecuteEnable);
				L4->PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable);
				L4->PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable);
				L4->ReadWrite = __check(Flags, ReadWritable);
				L4->UserMode = __check(Flags, UserMode);
				InvalidatePage(Virtual);
				Virtual += PAGE_SIZE;
				NBytes -= PAGE_SIZE;
				break;
			} case 3: {
				PDPTEntry1GB *L3 = (PDPTEntry1GB *)PT;
				L3->PageInMemory = true;
				L3->Present = true;
				L3->ExecuteDisable = !__check(Flags, ExecuteEnable);
				L3->PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable);
				L3->PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable);
				L3->ReadWrite = __check(Flags, ReadWritable);
				L3->UserMode = __check(Flags, UserMode);
				L3->ProtectionKey = ProtectionKey;
				InvalidatePage(Virtual);
				Virtual += PAGE_SIZE3;
				NBytes -= PAGE_SIZE3;
				break;
			} case 2: {
				PageDirectoryEntry2MB *L2 = (PageDirectoryEntry2MB *)PT;
				L2->PageInMemory = true;
				L2->Present = true;
				L2->ExecuteDisable = !__check(Flags, ExecuteEnable);
				L2->PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable);
				L2->PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable);
				L2->ReadWrite = __check(Flags, ReadWritable);
				L2->UserMode = __check(Flags, UserMode);
				L2->ProtectionKey = ProtectionKey;
				InvalidatePage(Virtual);
				Virtual += PAGE_SIZE2;
				NBytes -= PAGE_SIZE2;
				break;
			} case 1: {
				PageTableEntry4KB *L1 = (PageTableEntry4KB *)PT;
				L1->PageInMemory = true;
				L1->Present = true;
				L1->ExecuteDisable = !__check(Flags, ExecuteEnable);
				L1->PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable);
				L1->PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable);
				L1->ReadWrite = __check(Flags, ReadWritable);
				L1->UserMode = __check(Flags, UserMode);
				L1->ProtectionKey = ProtectionKey;
				InvalidatePage(Virtual);
				Virtual += PAGE_SIZE;
				NBytes -= PAGE_SIZE;
				break;
			} default: {return false;}
		}
	}
	return true;
}

//!	Update the Page Table to point to a Physical Address.

void *GeneratePageTable(SimpleMemoryRangeDescriptor_t *Descriptors, uint32_t N){
	//	Stub
	__chalt(true);
	return NULL;
}

static __inline EFI_MEMORY_DESCRIPTOR *QueryCandidatePool(
	const void *D_, const uint32_t StructSize, uint64_t *TotalPages, const uint64_t N, const uint64_t RequiredPages, 
	const EFI_MEMORY_TYPE *TypeRequirements, const uint64_t TypeRequirementBufferSize, const uint64_t AttrMask
){
	int64_t Candidate = -1;
	uint64_t NCandidateEntries = UINT64_MAX, CandidateElapsedPages = UINT64_MAX;
	DebugPrintf("\nRequirements:\t%z Pages\tAttributes: %2z\tTypeRequirements: ", (uint64_t)RequiredPages, (uint64_t)AttrMask);
		for(uint32_t cc = 0; cc < (TypeRequirementBufferSize / sizeof(EFI_MEMORY_TYPE)); ++cc){DebugPrintf("%z, ", (uint64_t)(TypeRequirements[cc]));}
	for(uint64_t cc = 0; cc < N; ++cc){
		GetEfiMemoryDescriptor(D, D_, cc, StructSize);
		bool Usable = memcheckl(TypeRequirements, D->Type, TypeRequirementBufferSize) && ((AttrMask & D->Attribute) == 0);
		if(Usable){
			uint64_t RemainingPages = (uint64_t)RequiredPages, ElapsedPages = 0, cc_ = cc, Address = D->PhysicalStart;
			//	Check that the Next Entry is contiguous, since D2 starts as D, then cc_ == 0 will always go through
			for(; (cc_ < N) && RemainingPages; ++cc_){
				GetEfiMemoryDescriptor(D2, D_, cc_, StructSize);
				if(D2->PhysicalStart != Address){
					DebugPrintf("\nNon-Continuous Pages: %z\t%z", (uint64_t)(D2->PhysicalStart), (uint64_t)Address);
					break;
				}
				ElapsedPages += D2->NumberOfPages;
				Address = D2->PhysicalStart + (D2->NumberOfPages * PAGE_SIZE);
				Usable = memcheckl(TypeRequirements, D2->Type, TypeRequirementBufferSize) && ((AttrMask & D2->Attribute) == 0);
				if(Usable){
					if(D2->NumberOfPages > RemainingPages){RemainingPages = 0;	break;}else{RemainingPages -= D2->NumberOfPages;}
				}else{DebugPrintf("\nAdditional Candidate %z did not meet %s Requirements", 
					(uint64_t)cc_, ((AttrMask & D2->Attribute) == 0)? "Type": "Attribute");	break;}
			}
			//	The currently Viewed entry elapses more pages than the candidate.
			if(RemainingPages){continue;}else{
				if((ElapsedPages - RequiredPages) <= (CandidateElapsedPages - RequiredPages)){
					DebugPrintf("\nCandidate Details\tElapsedPages:%z\tNEntries:%z\t(Possible Candidate)", (uint64_t)ElapsedPages, (uint64_t)(cc_ - cc));
					CandidateElapsedPages = ElapsedPages;	NCandidateEntries = cc_ - cc;	Candidate = cc;
				}else{continue;}
			}
		}
	}
	if((Candidate == -1) || (NCandidateEntries == UINT64_MAX) || (CandidateElapsedPages == UINT64_MAX)){DebugPrintf("\nFailed to find valid Candidate");	*TotalPages = 0;	return NULL;}
	GetEfiMemoryDescriptor(D, D_, Candidate, StructSize);
	memset((void *)(D->PhysicalStart), 0, CandidateElapsedPages * PAGE_SIZE);
	DebugPrintf("\nValid Candidate: 0x%0z, TotalPages: %1z", D->PhysicalStart, CandidateElapsedPages);
	*TotalPages = CandidateElapsedPages;
	return D;
}

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

static __inline EFI_MEMORY_DESCRIPTOR *QueryContiguousDescriptors(const void *D_, const uint32_t StructSize, uint64_t *TotalEntries, uint64_t *ElapsedPages, const uint64_t N){
	int64_t _CurrentType = -1;
	(*TotalEntries) = 0;
	(*ElapsedPages) = 0;
	if(!N){return NULL;}
	GetEfiMemoryDescriptor(D, D_, 0, StructSize);
	uint64_t cc_ = 0, Address = D->PhysicalStart;
	//	Check that the Next Entry is contiguous, since D2 starts as D, then cc_ == 0 will always go through
	_CurrentType = D->Type;
	for(; cc_ < N; ++cc_){
		GetEfiMemoryDescriptor(D2, D_, cc_, StructSize);
		if(D2->PhysicalStart != Address){break;}
		Address = D2->PhysicalStart + (D2->NumberOfPages * PAGE_SIZE);
		if(D2->Type == _CurrentType){(*TotalEntries)++;		(*ElapsedPages) += D2->NumberOfPages;}else{
			DebugPrintf("\nContiguous Descriptors 0x%0z -> 0x%0z, %z Pages", D2, D2 + *TotalEntries, *ElapsedPages);	break;}
	}
	DebugPrintf("\nContiguous Descriptors 0x%0z -> 0x%0z, %z Pages", D, D + *TotalEntries, *ElapsedPages);
	return D;
}
VirtualAddress InstantiatePageTable(const void *D_, const uint64_t StructSize, const uint64_t N, const uint64_t TotalMemoryPages){
	if(!_1GPageSupport()){DebugPrintf("\nThe CPU does not support 1G Pages, cannot Continue Execution");	__chalt(true);}
	InitialiseTemporaryPageTable(TotalMemoryPages * PAGE_SIZE);
	//	We Identity Map to L3 Entries
	//	Post-Identity Mapping
	//	Identity Mapping
	uint64_t TotalL3 = __roundupt(TotalMemoryPages * PAGE_SIZE, PAGE_SIZE3) / PAGE_SIZE3, 
	//	Number of Tables
			TotalL4 = __roundupt(TotalL3, PagingTableLength) / PagingTableLength, 
			TotalL5 = IsLA57Enabled()? (__roundupt(TotalL4, PagingTableLength) / PagingTableLength): 0, 
			CandidatePages = 0, TotalPages = ((__roundup(TotalL3, PagingTableLength) / PagingTableLength) + TotalL4 + TotalL5);
	void *RootPageTable = NULL;	{
		//	Number of GBs
		EFI_MEMORY_DESCRIPTOR *Candidate = QueryCandidatePool(D_, StructSize, 
			&CandidatePages, N, TotalPages, TypeMask1, sizeof(TypeMask1), AttributeMask);
		if(Candidate){
			uint64_t cc = 0;
			while(CandidatePages){
				GetEfiMemoryDescriptor(D, Candidate, cc, StructSize);
				CandidatePages -= D->NumberOfPages;
				//	Simply Mark as allocated
				D->Type = KernelBinarySpace;
				cc++;
			}
		}else{DebugPrintf("\nInvalid Page Table Candidate");	return (VirtualAddress){0};}
		uint64_t Offset = 0;
		void *PT = (void *)(Candidate->PhysicalStart);
		for(uint64_t cc = 0; cc < TotalL5; ++cc){
			PML5Table *L5 = PT + Offset;
			Offset += sizeof(PML5Table);
			for(uint64_t cc_ = 0; cc_ < PagingTableLength; ++cc_){
				(*L5)[cc_] = (PML5Entry){.Accessed = false, .ExecuteDisable = false, .PageInMemory = false, .PageLevelCacheDisable = false, 
					.PageLevelWriteThrough = false, .PhysicalAddress = 0, .Present = true, .ReadWrite = true, .UserMode = false};
				SetPagePhysicalAddress((*L5)[cc_], ((uint64_t)PT + (sizeof(PML5Table) * TotalL5) + (sizeof(PML4Table) * (cc_ % TotalL4))) >> PAGE_SHIFT);
			}
		}
		for(uint64_t cc = 0; cc < TotalL4; ++cc){
			PML4Table *L4 = PT + Offset;
			Offset += sizeof(PML4Table);
			for(uint64_t cc_ = 0; cc_ < PagingTableLength; ++cc_){
				(*L4)[cc_] = (PML4Entry){.Accessed = false, .ExecuteDisable = false, .PageInMemory = false, .PageLevelCacheDisable = false, 
					.PageLevelWriteThrough = false, .PhysicalAddress = 0, .Present = true, .ReadWrite = true, .UserMode = false};
				SetPagePhysicalAddress((*L4)[cc_], ((uint64_t)PT + (sizeof(PML5Table) * TotalL5) + (sizeof(PML4Table) * TotalL4) + (sizeof(PDPTTable) * (cc_ % TotalL3))) >> PAGE_SHIFT);
			}
		}
		for(uint64_t cc = 0; cc < TotalL3; ++cc){
			PDPTTable *L3 = PT + Offset;
			Offset += sizeof(PDPTTable);
			for(uint64_t cc_ = 0; cc_ < PagingTableLength; ++cc_){
				L3->pages1GB[cc_] = (PDPTEntry1GB){.Accessed = false, .Dirty = false, .ExecuteDisable = false, .Global = false, .PageAttributeTable = false, 
					.PageInMemory = true, .PageLevelCacheDisable = false, .PageLevelWriteThrough = false, .PageSize = true, .PhysicalAddress = 0, 
					.Present = true, .ProtectionKey = 0x00, .ReadWrite = true, .UserMode = false};
				SetPagePhysicalAddress(L3->pages1GB[cc_], (cc_ % TotalMemoryPages));
			}
		}
		RootPageTable = PT;
	}	{
		//	Paging that reflects EFI_MEMORY_DESCRIPTOR Type Requirements.
		EFI_MEMORY_DESCRIPTOR *D = D_;
		uint64_t TotalEntries = 0, Entries = 0, Pages = 0, CandidatePages = 0;
		uint64_t NL4 = 0, NL3 = 0, NL2 = 0, NL1 = 0;
		PageCoordinate PC = {.EnableL1 = false, .EnableL2 = false, .EnableL3 = false, .EnableL4 = true, 
			.EnableL5 = IsLA57Enabled(), .L1 = 0x00, .L2 = 0x00, .L3 = TotalL3, .L4 = TotalL4, .L5 = TotalL5};
		while(D = QueryContiguousDescriptors(D + 1, StructSize, &Entries, &Pages, N - TotalEntries)){
			TotalEntries += Entries;
			//	Map w/ 4kB Pages.
			//	We assume Memory is NEVER >512gB
			//	This is the Upper Limit of the Current Code.
			if(Pages < (PAGE_SIZE2 / 4)){NL1++;}else if(Pages < (PAGE_SIZE3 / 4)){NL2++;}else{NL3++;}
			if(NL1 == PagingTableLength){AddPageCoordinate(&PC, PAGE_SIZE2);					NL2++;}
			if(NL2 == PagingTableLength){AddPageCoordinate(&PC, PAGE_SIZE3);					NL3++;}
			if(NL3 == PagingTableLength){AddPageCoordinate(&PC, PAGE_SIZE3 * PagingTableLength);NL4++;}
		}
		NL4 = __roundup(NL4, PagingTableLength) / PagingTableLength;
		NL3 = __roundup(NL3, PagingTableLength) / PagingTableLength;
		NL2 = __roundup(NL2, PagingTableLength) / PagingTableLength;
		NL1 = __roundup(NL1, PagingTableLength) / PagingTableLength;
		
		EFI_MEMORY_DESCRIPTOR *Candidate = QueryCandidatePool(D_, StructSize, &CandidatePages, 
			N, NL4 + NL3 + NL2 + NL1, TypeMask1, sizeof(TypeMask1), AttributeMask);
		if(Candidate){
			uint64_t cc = 0;
			while(CandidatePages){
				GetEfiMemoryDescriptor(D, Candidate, cc, StructSize);
				CandidatePages -= D->NumberOfPages;
				D->Type = KernelBinarySpace;
				cc++;
			}
		}else{DebugPrintf("\nInvalid Duplicate Page Table Candidate");	return (VirtualAddress){0};}
		//	Full of L3 Entries
		PDPTTable *PT = (PDPTTable *)(Candidate->PhysicalStart);	{
			uint32_t Lvl = IsLA57Enabled()? 5: 4;
			uint64_t _CandidatePages = 0;
			DebugPrintf("\nGetting Page at [%l(%u):%l(%u):%l(%u):%l(%u):%l(%u)]", PC.L5, PC.EnableL5, 
				PC.L4, PC.EnableL4, PC.L5, PC.EnableL5, PC.L5, PC.EnableL5, PC.L5, PC.EnableL5);
			void *Page = WalkEmulatedPageTree(PC, RootPageTable, &Lvl);
			//	Since we; at MAX support 512GB, we will at most only need to allocate an extra PDPT Table for the NEXT Page Table.
			switch(Lvl){
				case 5: {
					EFI_MEMORY_DESCRIPTOR *Tmp = QueryCandidatePool(D_, StructSize, &_CandidatePages, N, 1, TypeMask1, sizeof(TypeMask1), AttributeMask);	{
						uint64_t cc_ = 0;
						while(_CandidatePages){GetEfiMemoryDescriptor(D, Tmp, cc_, StructSize);	Tmp->Type = KernelBinarySpace;	cc_++;	_CandidatePages -= Tmp->NumberOfPages;}
					}
					PML5Entry *L5 = Page;	*L5 = (PML5Entry){
						.Accessed = false, .ExecuteDisable = false, .PageInMemory = true, .PageLevelCacheDisable = false, 
						.PhysicalAddress = 0x00, .Present = true, .ReadWrite = true, .UserMode = false, .PageLevelWriteThrough = false};
					SetPagePhysicalAddress(*L5, (uint64_t)Tmp->PhysicalStart >> PAGE_SHIFT);
					*(PML4Entry *)(Tmp->PhysicalStart) = (PML4Entry){
						.Accessed = false, .ExecuteDisable = false, .PageInMemory = true, .PageLevelCacheDisable = false, 
						.PhysicalAddress = 0x00, .Present = true, .ReadWrite = true, .UserMode = false, .PageLevelWriteThrough = false};
					SetPagePhysicalAddress(*(PML4Entry *)(Tmp->PhysicalStart), (uint64_t)PT >> PAGE_SHIFT);
					PC.L4 = 0;
					break;
				} case 3: case 4: {
					PML4Entry *L4 = Page;	*L4 = (PML4Entry){
						.Accessed = false, .ExecuteDisable = false, .PageInMemory = true, .PageLevelCacheDisable = false, 
						.PhysicalAddress = 0x00, .Present = true, .ReadWrite = true, .UserMode = false, .PageLevelWriteThrough = false};
					SetPagePhysicalAddress(*L4, (uint64_t)PT >> PAGE_SHIFT);
					PC.L3 = 0;
					break;
				} default: {DebugPrintf("\nInvalid Page Table Entry %z", (uint64_t)Lvl);	return (VirtualAddress){0};}
			}
		}
		EFI_MEMORY_DESCRIPTOR *Tmp = D_;	{
			uint64_t TotalEntries = 0, Entries = 0, Pages = 0;
			while(Tmp = QueryContiguousDescriptors(Tmp + 1, StructSize, &Entries, &Pages, N - TotalEntries)){
				TotalEntries += Entries;
				PageAllocationFlags Flags = 0;
				switch(Tmp->Type){
					default:						{Flags = ReadWritable;}
					case EfiUnusableMemory: 
					case EfiMaxMemoryType: 
					case EfiUnacceptedMemoryType:	{continue;}
					case EfiReservedMemoryType:		{Flags = GlobalEnable | ReadOnly | SupervisorMode;	break;}
					case EfiMemoryMappedIOPortSpace:
					case EfiMemoryMappedIO:			{Flags = PageLevelWriteThroughEnable | PageLevelCacheEnable | ReadWritable;	break;}
				}
				while(Pages){
					uint32_t Lvl = IsLA57Enabled()? 5: 4;
					bool Repeat = false;
					DebugPrintf("\nGetting Page at [%l(%u):%l(%u):%l(%u):%l(%u):%l(%u)]", PC.L5, PC.EnableL5, 
						PC.L4, PC.EnableL4, PC.L5, PC.EnableL5, PC.L5, PC.EnableL5, PC.L5, PC.EnableL5);
					void *E = WalkEmulatedPageTree(PC, PT, &Lvl);
					do{
						Repeat = false;
						switch(Lvl){
							case 3: {
								if(Pages < (PAGE_SIZE3 / 8)){
									if(PC.L2 == (PagingTableLength - 1)){
										PDPTEntryDirectory *L3 = E;	*L3 = (PDPTEntryDirectory){
											.Accessed = false, .ExecuteDisable = false, .PageInMemory = true, .PageSize = false, 
											.PhysicalAddress = 0x00, .Present = true, .ReadWrite = __check(Flags, ReadWritable), .UserMode = false, 
											.PageLevelCacheDisable = __check(Flags, PageLevelCacheEnable), .PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable)};
										E = (void *)__roundupt((uint64_t)E, PAGE_SIZE);		SetPagePhysicalAddress(*L3, (uint64_t)E >> PAGE_SHIFT);
									}
									PC.EnableL3 = true;
									Repeat = true;
								}else{
									PDPTEntry1GB *L3 = E;	*L3 = (PDPTEntry1GB){
										.Accessed = false, .Dirty = false, .ExecuteDisable = false, .Global = __check(Flags, GlobalEnable), 
										.PageAttributeTable = false, .PageInMemory = true, .PageSize = true, .PhysicalAddress = 0, 
										.PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), .ReadWrite = __check(Flags, ReadWritable), 
										.PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable), .Present = true, .ProtectionKey = 0x00, .UserMode = false};
									SetPagePhysicalAddress(*L3, Tmp->PhysicalStart >> PAGE_SHIFT3);
									if(Pages < PAGE_SIZE2){Pages = 0;}else{Pages -= PAGE_SIZE2;}
									AddPageCoordinate(&PC, PAGE_SIZE3);
								}
								PC.EnableL3 = true;
								break;
							} case 2: {
								if(Pages < (PAGE_SIZE2 / 8)){
									if(PC.L2 == (PagingTableLength - 1)){
										PageDirectoryEntry *L2 = E;	*L2 = (PageDirectoryEntry){
											.Accessed = false, .ExecuteDisable = false, .PageInMemory = true, .PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), .PageSize = false, 
											.PhysicalAddress = 0x00, .Present = true, .ReadWrite = __check(Flags, ReadWritable), .UserMode = false, .PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable)};
										E = (void *)__roundupt((uint64_t)E, PAGE_SIZE);		SetPagePhysicalAddress(*L2, (uint64_t)E >> PAGE_SHIFT);
									}
									PC.EnableL2 = true;
									Repeat = true;
								}else{
									PageDirectoryEntry2MB *L2 = E;	*L2 = (PageDirectoryEntry2MB){
										.Accessed = false, .Dirty = false, .ExecuteDisable = false, .Global = __check(Flags, GlobalEnable), .PageAttributeTable = false, .PageInMemory = true, 
										.PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), .PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable), 
										.PageSize = true, .PhysicalAddress = 0, .Present = true, .ProtectionKey = 0x00, .ReadWrite = __check(Flags, ReadWritable), .UserMode = false};
									SetPagePhysicalAddress(*L2, Tmp->PhysicalStart >> PAGE_SHIFT2);
									if(Pages < PAGE_SIZE){Pages = 0;}else{Pages -= PAGE_SIZE;}
									AddPageCoordinate(&PC, PAGE_SIZE2);
								}
								PC.EnableL2 = true;
								break;
							} case 1: {
								PageTableEntry4KB *L1 = E;	*L1 = (PageTableEntry4KB){
									.Accessed = false, .Dirty = false, .ExecuteDisable = false, .Global = __check(Flags, GlobalEnable), .PageAttributeTable = false, .PageInMemory = true, 
									.PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), .PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable), 
									.PhysicalAddress = Tmp->PhysicalStart, .Present = true, .ProtectionKey = 0x00, .ReadWrite = __check(Flags, ReadWritable), .UserMode = false};
								SetPagePhysicalAddress(*L1, Tmp->PhysicalStart >> PAGE_SHIFT);
								PC.EnableL1 = true;
								Pages--;
								AddPageCoordinate(&PC, PAGE_SIZE);
								break;
							}
						}
					}while(Repeat);
				}
			}
		}
		// __bochs();
		bool _L5 = IsLA57Enabled();
		InvalidatePages(NULL, TotalMemoryPages * PAGE_SIZE);
		LoadPageTree(RootPageTable, 0x10, 0x08, (bool64_t)_L5);
		GetPageTable(&_L5, _UpdatePageTableMode, RootPageTable);
		return InitVirtualAddress(PC);
	}
}

//	Heavily relies of UEFI's 1:1 Mapping Mode
bool InitialiseAllocationState(void *D_, const uint64_t StructSize, uint64_t N, const uint64_t TotalMemory){
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
	//	Non-Usable Persistent Memory.
	
	//	End address of the last piece seen for each class (TypeMask0..3); -1 = none yet.
	int64_t				TypeEnd[4] = {-1, -1, -1, -1};
	for(uint64_t cc = 0; cc < N; ++cc){
		GetEfiMemoryDescriptor(D, D_, cc, StructSize);
		int Class = memcheckl(TypeMask0, D->Type, sizeof(TypeMask0))? 0: (memcheckl(TypeMask1, D->Type, sizeof(TypeMask1))? 1: 
			(memcheckl(TypeMask2, D->Type, sizeof(TypeMask2))? 2: (memcheckl(TypeMask3, D->Type, sizeof(TypeMask3))? 3: -1)));
		if(Class < 0){continue;}
		//	A new Descriptor starts whenever this piece is not contiguous with the previous piece of its class.
		if(TypeEnd[Class] != (int64_t)D->PhysicalStart){DescriptorAllocation += sizeof(MemoryRangeDescriptor);}
		TypeEnd[Class] = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);
		SubDescriptorAllocation += (D->NumberOfPages > (PAGE_SIZE2 / PAGE_SIZE)? (D->NumberOfPages * PAGE_SIZE) / PAGE_SIZE2: D->NumberOfPages);
	}
	uint64_t TotalRequiredPages = (SubDescriptorAllocation + PhysToIndexAllocation + DescriptorAllocation + sizeof(uint64_t));
	TotalRequiredPages = __roundup(TotalRequiredPages, PAGE_SIZE) / PAGE_SIZE;

	uint64_t ElapsedPages = 0;
	VirtualAddress VA = {0};
	//	Base of the duplicate region. Must match InstantiatePageTable: max(RAM, 4GB) rounded up to 1GB.
	uint64_t DupBase = (TotalMemory / PAGE_SIZE) * PAGE_SIZE;
	if(DupBase < (4ULL << PAGE_SHIFT3)){DupBase = 4ULL << PAGE_SHIFT3;}
	DupBase = (__roundup(DupBase, 1ULL << PAGE_SHIFT3) / (1ULL << PAGE_SHIFT3)) * (1ULL << PAGE_SHIFT3);
	volatile InternalAllocationState *IState = GetIState();
	DebugPrintf("\nInitialising Page Table");	{
		//	Temporarily Initialise Minimal Paging Dependencies.
		bool _L = IsLA57Enabled();
		void *PT = GetPageTable(&_L, _1To1MappingMode, NULL);
		(GetPageTable(&_L, _UpdatePageTableMode, PT));
		VA = InstantiatePageTable(D_, StructSize, N, TotalMemory / PAGE_SIZE);
		if(!(*(uint64_t *)&VA)){DebugPrintf("\nFailed to Instantiate the Page Table");	return false;}
	}
	EFI_MEMORY_DESCRIPTOR *Candidate = QueryCandidatePool(D_, StructSize, &ElapsedPages, N, TotalRequiredPages, TypeMask1, sizeof(TypeMask1), AttributeMask);
	if(!Candidate){return false;}
	{
		//	Walk by StructSize (the firmware's stride), and shrink the SAME counter that the loop tests.
		uint64_t cc = 0, _ElapsedPages = ElapsedPages;
		while(_ElapsedPages){
			EFI_MEMORY_DESCRIPTOR *C = (EFI_MEMORY_DESCRIPTOR *)((uint8_t *)Candidate + (cc * StructSize));
			C->Type = KernelBinarySpace;
			if(C->NumberOfPages > _ElapsedPages){_ElapsedPages = 0;}else{_ElapsedPages -= C->NumberOfPages;}
			cc++;
		}
	}
	DebugPrintf("\nDescriptor Pool: 0x%0z\t%z Pages", Candidate->PhysicalStart, (uint64_t)ElapsedPages);
	//	Possibly failing at this pOint.
	for(uint64_t cc = 0; cc < N; ++cc){
		GetEfiMemoryDescriptor(D, D_, cc, StructSize);
		bool Usable = memcheckl(TypeMask1, D->Type, sizeof(TypeMask1)) && ((AttributeMask& D->Attribute) == 0);
		DebugPrintf("\n[%z]:\tType: %z(%s){Attr: 0b%2z\tPhys: 0x%0z\tNoPages: %z}", (uint64_t)cc, (uint64_t)D->Type, 
			(Usable? "Usable": "  ??  "), D->Attribute, D->PhysicalStart, D->NumberOfPages);
	}
	DebugPrintf("\nInitialising Internal Allocation State");

	*IState = (volatile InternalAllocationState){
		.Descriptors = (MemoryRangeDescriptor *)(Candidate->PhysicalStart), .MemoryMaxPages = TotalMemory / PAGE_SIZE, 
		.PhysicalDescriptorIndexes = (uint16_t *)(Candidate->PhysicalStart + DescriptorAllocation), .Ready = false, 
		.SubDescriptors = (MemoryRangeSubDescriptor **)(Candidate->PhysicalStart + DescriptorAllocation + SubDescriptorAllocation), 
		.TotalDescriptors = DescriptorAllocation / sizeof(MemoryRangeDescriptor), .TotalSubDescriptors = SubDescriptorAllocation / sizeof(MemoryRangeSubDescriptor), 
		.TotalSubDescriptorPools = 1, .LastPageTableAlloc = VA
	};	*(IState->SubDescriptors) = (MemoryRangeSubDescriptor *)(Candidate->PhysicalStart + DescriptorAllocation + SubDescriptorAllocation + sizeof(uint64_t));
	TypeEnd[0] = TypeEnd[1] = TypeEnd[2] = TypeEnd[3] = -1;
	uint32_t CurrentSubDescriptor = 0;

	//	Need to track contiguous Memory Types.
	//		If Contiguous:		Update the Descriptor to encompass the Additional Entry.
	//		If not Contiguous:	Start another Descriptor.
	{
		int64_t ArrayCtr = -1;
		for(uint64_t cc = 0; cc < N; ++cc){
			GetEfiMemoryDescriptor(D, D_, cc, StructSize);
			int Class = memcheckl(TypeMask0, D->Type, sizeof(TypeMask0))? 0: (memcheckl(TypeMask1, D->Type, sizeof(TypeMask1))? 1: 
				(memcheckl(TypeMask2, D->Type, sizeof(TypeMask2))? 2: (memcheckl(TypeMask3, D->Type, sizeof(TypeMask3))? 3: -1)));
			if(Class < 0){continue;}
			if((ArrayCtr >= 0) && (TypeEnd[Class] == (int64_t)D->PhysicalStart)){IState->Descriptors[ArrayCtr].PageLimit += D->NumberOfPages;}
			else{
				ArrayCtr++;
				IState->Descriptors[ArrayCtr] = (MemoryRangeDescriptor){
					.SubDescriptorIndex = CurrentSubDescriptor, .EfiMemoryType = D->Type, .PageLimit = D->NumberOfPages, .Physical = D->PhysicalStart};
			}
			TypeEnd[Class] = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);
			CurrentSubDescriptor += (D->NumberOfPages > (PAGE_SIZE2 / PAGE_SIZE)? (D->NumberOfPages * PAGE_SIZE) / PAGE_SIZE2: D->NumberOfPages);
			//	Only the types InstantiatePageTable maps get a live entry, and they are enabled in the DUPLICATE
			//	(DupBase + phys), never through the identity map: an identity leaf is a whole 1GB page.
			//	Unusable/Reserved/PortSpace/Unaccepted/PalCode have no leaf, so they are not touched.
			if((D->Type == EfiRuntimeServicesCode) || (D->Type == EfiRuntimeServicesData) || (D->Type == EfiACPIMemoryNVS) || (D->Type == EfiMemoryMappedIO)){
				// __bochs();
				if(!(UpdatePageTable((void *)(DupBase + D->PhysicalStart), D->NumberOfPages * PAGE_SIZE, 
					(PageAllocationFlags)(ReadWritable | SupervisorMode | PageLevelWriteThroughEnable | GlobalEnable | ((D->Type == EfiRuntimeServicesCode)? ExecuteEnable: 0)), 0x00))){
						DebugPrintf("\nFailed to Update Page Entries accompanying Address %z to %z", DupBase + D->PhysicalStart, DupBase + D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE));}
			}
			//	Do not index past the table: it only has MemoryMaxPages entries (MMIO above RAM would overflow it).
			for(uint64_t cc_ = 0; cc_ < D->NumberOfPages; ++cc_){
				uint64_t Page = (D->PhysicalStart / PAGE_SIZE) + cc_;
				if(Page >= (TotalMemory / PAGE_SIZE)){break;}
				IState->PhysicalDescriptorIndexes[Page] = ArrayCtr;
			}
		}
	}
	__bochs();
	IState->Ready = true;
	return true;
}

//!	Returns Physical Address.
void *QueryFreeMemory(void *Physical, uint32_t NoPages){
	volatile InternalAllocationState *IS = GetIState();
	if(EnableInputAddress(false, false)){
		GetDescriptor(D, Physical, IS);
		if(D->SubDescriptorIndex != InvalidSubDescriptorIndex){
			uint32_t cc = 0, Offset = 0;
			MemoryRangeSubDescriptor *SD = IS->SubDescriptors[IS->CurrentSubDescriptorPool] + D->SubDescriptorIndex;
			int32_t LowestDiffN = -1;
			while(!(SD[cc].EndOfList)){
				if(!SD[cc].Valid){
					if(LowestDiffN == -1){if(SD[cc].NoPages >= NoPages){LowestDiffN = cc;}}else if((SD[LowestDiffN].NoPages - NoPages) == 0){break;}
					if((SD[cc].NoPages >= NoPages)){if((SD[LowestDiffN].NoPages - NoPages) > (SD[cc].NoPages - NoPages)){LowestDiffN = cc;}}
				}
				cc++;
			}
			if(LowestDiffN != -1){
				cc = 0;
				while(!(SD[cc].EndOfList) && cc < LowestDiffN){Offset += SD[cc].NoPages;	cc++;}
				return (void *)(D->Physical + (Offset * PAGE_SIZE));
			}
		}
	}else{
		QueryPhysicalAddress(Physical, IS){
			for(uint64_t cc = 0; cc < IS->TotalDescriptors; cc++){
				uint64_t cc_ = 0, Offset = 0;
				if(IS->Descriptors[cc].SubDescriptorIndex != InvalidSubDescriptorIndex){
					MemoryRangeSubDescriptor *SD = IS->SubDescriptors[IS->CurrentSubDescriptorPool] + IS->Descriptors[cc].SubDescriptorIndex;
					int32_t LowestDiffN = -1;
					while(!(SD[cc_].EndOfList)){
						if(!SD[cc_].Valid){
							if(LowestDiffN == -1){if(SD[cc_].NoPages >= NoPages){LowestDiffN = cc_;}}else if((SD[LowestDiffN].NoPages - NoPages) == 0){break;}
							if((SD[cc_].NoPages >= NoPages)){if((SD[LowestDiffN].NoPages - NoPages) > (SD[cc_].NoPages - NoPages)){LowestDiffN = cc_;}}
						}
						cc_++;
					}
					if(LowestDiffN != -1){
						cc_ = 0;
						while(!(SD[cc_].EndOfList) && cc_ < LowestDiffN){Offset += SD[cc_].NoPages;	cc_++;}
						return (void *)(IS->Descriptors[cc].Physical + (Offset * PAGE_SIZE));
					}
				}
			}
		}
	}
	return NULL;
}

MemoryRangeSubDescriptor *AllocateSubDescriptorPool(uint32_t *N){
    // Get a Region in the SubDescriptorTable Table.
    volatile InternalAllocationState *IState = GetIState();
	if(IState->TotalDescriptors < 2){*N = InvalidSubDescriptorIndex;	return NULL;}
    for(uint32_t cc = 0; cc < IState->TotalDescriptors - 1; ++cc){
        uint32_t currentStart = IState->Descriptors[cc].SubDescriptorIndex;
        if(currentStart == InvalidSubDescriptorIndex){continue;}
		
        MemoryRangeSubDescriptor *SD = IState->SubDescriptors[IState->CurrentSubDescriptorPool] + currentStart;
        uint32_t cc_ = 0;
        while(!(SD[cc_].EndOfList)){cc_++;}
        uint32_t currentEnd = currentStart + cc_, nextStart = IState->Descriptors[cc + 1].SubDescriptorIndex;

        if(nextStart != InvalidSubDescriptorIndex && nextStart > currentEnd){
            uint32_t gapSize = nextStart - currentEnd;
            if(gapSize >= *N){
                *N = gapSize;
                return IState->SubDescriptors[IState->CurrentSubDescriptorPool] + currentEnd;
            }
        }
    }
    *N = InvalidSubDescriptorIndex;
    return NULL;
}
//	Returns Physical
//	In Kernel/Supervisor Mode
//	We exist in 1:1 Mapping Mode.
//	User Mode applications will have Page Tables mapped to close them off from the rest of Memory.
void *ReAllocateSubDescriptorTable(){
	//	This function is for regenerating the Sub-Descriptor Table, with a Larger Buffer.
	//	We allocate in 512 SubDescriptor Blocks.
	volatile InternalAllocationState *IState = GetIState();
	uint32_t Level = 5;
	void *Physical = QueryFreeMemory(NULL, ((IState->TotalSubDescriptors + 512) * sizeof(MemoryRangeSubDescriptor)) / PAGE_SIZE), 
		*OldPhysical = MapPhysical(IState->SubDescriptors[IState->CurrentSubDescriptorPool]);
	{
		uint32_t cc = 0;
		GetDescriptor(OldD, OldPhysical, IState);
		MemoryRangeSubDescriptor *SD = IState->SubDescriptors[IState->CurrentSubDescriptorPool] + OldD->SubDescriptorIndex;
		while(!(SD[cc].EndOfList)){SD[cc].AllocationType = 0;	cc++;}
	}
	memcpy(Physical, IState->SubDescriptors[IState->CurrentSubDescriptorPool], IState->TotalSubDescriptors * sizeof(MemoryRangeSubDescriptor));
	GetDescriptor(D, Physical, IState);
	MemoryRangeSubDescriptor *SD = IState->SubDescriptors[IState->CurrentSubDescriptorPool] + D->SubDescriptorIndex;
	uint32_t cc = 0;
	while(!SD[cc].EndOfList){
		if(!SD[cc].Valid){
			SD[cc].Valid = true;
			SD[cc].AllocationType = PATSubDescriptorMapping;
			VirtualAddress Tmp = *(VirtualAddress *)&Physical;
			SD[cc].CoordBaseIndex = InitPageCoordinate(Tmp);
		}
		cc++;
	}
	return (void *)(D->Physical);
}

enumdef(uint64_t, AllocationTableUpdateBitMask){
	ATUSubDescriptorAllocationType = 0x01, ATUSubDescriptorValid = 0x02, 
	ATUSubDescriptorNoPages = 0x04, ATUSubDescriptorCoordBaseIndex = 0x08, 
	ATUSubDescriptor = (ATUSubDescriptorAllocationType | ATUSubDescriptorValid | ATUSubDescriptorNoPages | ATUSubDescriptorCoordBaseIndex)
};
bool UpdateAllocationTableByPhysical(AllocationTableUpdateBitMask BitMask, uint64_t Physical, ...){
	volatile InternalAllocationState *IS = GetIState();
	bool out = true;
	QueryPhysicalAddress(Physical, IS){
		GetDescriptor(D, Physical, IS){
			va_list ls;	va_start(ls, Physical);
			if(out && __check(BitMask, ATUSubDescriptorAllocationType)){
				GetSubDescriptor(SD, Physical, IS){SD->AllocationType &= va_arg(ls, PageAllocationType);}else{out = false;}}
			if(out && __check(BitMask, ATUSubDescriptorValid)){
				GetSubDescriptor(SD, Physical, IS){SD->Valid = va_arg(ls, bool);}else{out = false;}}
			if(out && __check(BitMask, ATUSubDescriptorNoPages)){
				GetSubDescriptor(SD, Physical, IS){
					uint64_t NewNoPages = va_arg(ls, uint64_t);
					int64_t Delta = (int64_t)NewNoPages - (int64_t)SD->NoPages;
					SD->NoPages = NewNoPages;
					MemoryRangeSubDescriptor *Proceeding = SD + 1;
					uint64_t cc = 0;
					while(!Proceeding[cc].EndOfList){Proceeding[cc].NoPages += Delta;	cc++;}
				}else{out = false;}}
			if(out && __check(BitMask, ATUSubDescriptorCoordBaseIndex)){
				GetSubDescriptor(SD, Physical, IS){SD->CoordBaseIndex = va_arg(ls, PageCoordinate);}else{out = false;}}
			va_end(ls);
			return out;
		}
	}
	return false;	
}

void *AllocateAlignedPagesFromRange(void *Base, uint64_t Limit, uint64_t NBytes, PageAllocationFlags Flags, uint8_t ProtectionKey, uint32_t Align){
	void *PTR = AllocatePagesFromRange(Base, Limit, __roundup(NBytes + sizeof(uint64_t), Align), Flags, ProtectionKey);
	if(!PTR){return NULL;}
	*((uint64_t *)__roundup((uint64_t)PTR, Align)) = (uint64_t)PTR;
	return (uint64_t *)__roundup((uint64_t)PTR, Align) + 1;
}
void *AllocatePagesFromRange(void *Base, uint64_t Limit, uint64_t NBytes, PageAllocationFlags Flags, uint8_t ProtectionKey){
	while(MemoryIsMapped(Base) && Base < Limit){Base += PAGE_SIZE;}
	if(Base >= Limit){return NULL;}else{return AllocatePages(Base, NBytes, Flags, ProtectionKey);}
}
void *AllocatePages(void *Base, uint64_t NBytes, PageAllocationFlags Flags, uint8_t ProtectionKey){
	volatile InternalAllocationState *IS = GetIState();
	bool L5 = IsLA57Enabled();
	if(EnableInputAddress(false, false)){
		EnableInputAddress(false, true);
		QueryPhysicalAddress(Base, IS){
			GetDescriptor(D, Base, IS){
				GetSubDescriptor(SD, Base, IS){
					if(SD->Valid && (SD->NoPages >= (NBytes / PAGE_SIZE))){return *(void **)(&SD->CoordBaseIndex);}}
			}
		}
	}else{
		for(; IS->LastAllocation < IS->TotalDescriptors; ++(IS->LastAllocation)){
			uint32_t cc = 0;
			MemoryRangeSubDescriptor *SD = IS->SubDescriptors[IS->CurrentSubDescriptorPool] + IS->Descriptors[IS->LastAllocation].SubDescriptorIndex;
			while(!(SD[cc].EndOfList)){
				if(!(SD[cc].Valid)){
					uint32_t cc_ = cc;
					uint64_t Offset = 0, MetPages = NBytes;
					while(!(SD[cc_].EndOfList) && MetPages){
						if(MetPages < SD[cc_].NoPages){MetPages = 0;}else{MetPages -= SD[cc_].NoPages;}
						Offset += SD[cc_].NoPages;		cc_++;
					}
					if(MetPages){continue;}else{
						UpdateAllocationTableByPhysical(ATUSubDescriptorAllocationType | ATUSubDescriptorValid, 
							IS->Descriptors[IS->LastAllocation].Physical + Offset, PATUsedRegion, true);
						VirtualAddress VA = InitVirtualAddress(SD[cc].CoordBaseIndex);
						//	Update the Page Table
						if(!(UpdatePageTable((void *)*(uint64_t *)&VA, NBytes, Flags, ProtectionKey))){return false;}
						memset((void *)*(uint64_t *)&VA, 0, NBytes);
						return (void *)*(uint64_t *)&VA;
					}
				}
				cc++;
			}
		}
		if(IS->LastAllocation){IS->LastAllocation = 0;	return AllocatePages(Base, NBytes, Flags, ProtectionKey);}
	}
	//	We run in Identity Mapped Paging.

}

void *MapVirtual(void *Physical){
	volatile InternalAllocationState *IS = GetIState();
	if(IS->Ready){
		QueryPhysicalAddress(Physical, IS){
			GetSubDescriptor(SD, Physical, IS){
				VirtualAddress VA = InitVirtualAddress(SD->CoordBaseIndex);
				return (void *)VirtualAddressAToU64(VA);
			}
		}
		//	57 Bits
		//	48 Bits
	}else{return (void *)((uint64_t)Physical + TotalMemorySize());}
	return NULL;
}

void FreePages(void *Virtual){
	if(!Virtual){return;}
	void *Physical = MapPhysical((void *)__rounddown((uint64_t)Virtual, PAGE_SIZE));
	volatile InternalAllocationState *IS = GetIState();
	QueryPhysicalAddress(Physical, IS){
		UpdateAllocationTableByPhysical(ATUSubDescriptorAllocationType, (uint64_t)Physical, ~PATUsedRegion);
		uint64_t _TotalMappedMemory = TotalMappedMemory(Virtual);
		while(_TotalMappedMemory){
			uint32_t Level = 5;
			void *PT = WalkPageTreeByVirtual(Virtual, &Level);
			switch(Level){
				case 3: {
					PDPTEntry1GB *L3 = PT;			L3->PageInMemory = false;	L3->Present = false;
					if(_TotalMappedMemory > PAGE_SIZE3){_TotalMappedMemory -= PAGE_SIZE3;}else{_TotalMappedMemory = 0;}
					Virtual += PAGE_SIZE3;
					break;
				} case 2: {
					PageDirectoryEntry2MB *L2 = PT;	L2->PageInMemory = false;	L2->Present = false;
					if(_TotalMappedMemory > PAGE_SIZE2){_TotalMappedMemory -= PAGE_SIZE2;}else{_TotalMappedMemory = 0;}
					Virtual += PAGE_SIZE2;
					break;
				} case 1: {
					PageTableEntry4KB *L1 = PT;		L1->PageInMemory = false;	L1->Present = false;
					if(_TotalMappedMemory > PAGE_SIZE){_TotalMappedMemory -= PAGE_SIZE;}else{_TotalMappedMemory = 0;}
					Virtual += PAGE_SIZE;
					break;
				} default: {continue;}
			}
		}
	}
}

bool RepointVirtualAddress(void *Virtual, void *NewPhysical){
	void *Physical = MapPhysical(Virtual);
	InternalAllocationState *IState = GetIState();
	bool out = false;
	QueryPhysicalAddress(Physical, IState){
		GetDescriptor(D, NewPhysical, IState){
			out = UpdateAllocationTableByPhysical(ATUSubDescriptorAllocationType | ATUSubDescriptorCoordBaseIndex, (uint64_t)Physical, ~PATUsedRegion, (PageCoordinate){0});
			VirtualAddress VA = *(VirtualAddress *)&Virtual;
			if(out){out = UpdateAllocationTableByPhysical(ATUSubDescriptorAllocationType | ATUSubDescriptorCoordBaseIndex, (uint64_t)NewPhysical, PATUsedRegion, InitPageCoordinate(VA));}
		}
	}
	return out;
}