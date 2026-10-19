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
	if((IState) && (IState)->MemoryMaxPages && ((IState)->MemoryMaxPages > ((uint64_t)(Adr) / PAGE_SIZE)))
#define GetDescriptor(Descriptor, Adr, IState)	\
	MemoryRangeDescriptor *Descriptor = (Adr < ((IState)->MemoryMaxPages * PAGE_SIZE))? (IState)->Descriptors + (IState)->PhysicalDescriptorIndexes[(uint64_t)(Adr) / PAGE_SIZE]: NULL;	\
	if(Descriptor)
//	We need the Entry correspondng epecifically to this entry
#define GetSubDescriptor(SubDescriptor, Adr, IState)\
	uint64_t SubDescriptor##Offset = 0;	\
	MemoryRangeSubDescriptor *SubDescriptor = (Adr < ((IState)->MemoryMaxPages * PAGE_SIZE)) &&	\
		(IState)->Descriptors[(IState)->PhysicalDescriptorIndexes[(uint64_t)(Adr) / PAGE_SIZE]].SubDescriptorIndex != InvalidSubDescriptorIndex?	\
		((IState)->SubDescriptors[(IState)->CurrentSubDescriptorPool] + (IState)->Descriptors[(IState)->PhysicalDescriptorIndexes[(uint64_t)(Adr) / PAGE_SIZE]].SubDescriptorIndex): NULL;		\
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
	bool					NonIdentityMappedState;
	//	Describes the Upper Echelons of Page Allocations.
}InternalAllocationState;

//	State decays on each call
bool EnableInputAddress(bool toggle, bool w){
	static __linkersection(.bss) bool _toggle = false;
	bool tmp = _toggle;
	if(w){_toggle = toggle;}
	return tmp;
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


bool UpdatePageTable(void *Virtual, uint64_t NBytes, PageAllocationFlags Flags, uint8_t ProtectionKey){
	while(NBytes){
		uint32_t Level = 5;
		void *PT = WalkPageTreeByVirtual(Virtual, &Level);
		if(!PT){DebugPrintf("\nFailed to get PT");}else{
			switch(Level){
				case 5: {
					PML5Entry *L3 = (PML5Entry *)PT;
					L3->PageInMemory = true;
					L3->Present = true;
					L3->ExecuteDisable = !__check(Flags, ExecuteEnable);
					L3->PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable);
					L3->PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable);
					L3->ReadWrite = __check(Flags, ReadWritable);
					L3->UserMode = __check(Flags, SupervisorMode);
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
					L4->UserMode = __check(Flags, SupervisorMode);
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
					L3->UserMode = __check(Flags, SupervisorMode);
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
					L2->UserMode = __check(Flags, SupervisorMode);
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
					L1->UserMode = __check(Flags, SupervisorMode);
					L1->ProtectionKey = ProtectionKey;
					InvalidatePage(Virtual);
					Virtual += PAGE_SIZE;
					NBytes -= PAGE_SIZE;
					break;
				} default: return false;
			}
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

EFI_MEMORY_DESCRIPTOR *QueryCandidatePool(
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
		DebugPrintf("\n[%z]:\tType: %z(%s){Attr: 0b%2z\tPhys: 0x%0z\tNoPages: %z}", (uint64_t)cc, (uint64_t)D->Type, 
			(Usable? "Usable": "  ??  "), D->Attribute, D->PhysicalStart, D->NumberOfPages);
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
	if((Candidate == -1) || (NCandidateEntries == UINT64_MAX) || (CandidateElapsedPages == UINT64_MAX)){DebugPrintf("\nFailed to find valid Candidate");}
	GetEfiMemoryDescriptor(D, D_, Candidate, StructSize);
	memset((void *)(D->PhysicalStart), 0, CandidateElapsedPages * PAGE_SIZE);
	DebugPrintf("\nValid Candidate: 0x%0z, TotalPages: %1z", D->PhysicalStart, CandidateElapsedPages);
	*TotalPages = CandidateElapsedPages;
	return D;
}

void InitialiseTemporaryPageTable(uint64_t TotalMemorySize){
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
EFI_MEMORY_DESCRIPTOR *QueryContiguousDescriptors(const void *D_, const uint32_t StructSize, uint64_t *TotalEntries, uint64_t *ElapsedPages, const uint64_t N){
	int64_t _CurrentType = -1;
	(*TotalEntries) = 0;
	(*ElapsedPages) = 0;
	GetEfiMemoryDescriptor(D, D_, 0, StructSize);
	uint64_t cc_ = 0, Address = D->PhysicalStart;
	//	Check that the Next Entry is contiguous, since D2 starts as D, then cc_ == 0 will always go through
	_CurrentType = D->Type;
	for(; cc_ < N; ++cc_){
		GetEfiMemoryDescriptor(D2, D_, cc_, StructSize);
		if(D2->PhysicalStart != Address){return D;}
		Address = D2->PhysicalStart + (D2->NumberOfPages * PAGE_SIZE);
		if(D2->Type == _CurrentType){(*TotalEntries)++;		(*ElapsedPages)++;}else{return D;}
	}
	return NULL;
}

//	Identity:	virt == phys over [0, IdentityBytes), 1GB pages, RW + executable.
//				IdentityBytes = max(RAM, 4GB) rounded up to 1GB (so the LAPIC/IOAPIC/HPET hole is reachable).
//	Duplicate:	virt == IdentityBytes + phys. Leaves point at the right frames and carry the permissions of
//				the EFI type, but are not present; UpdatePageTable/AllocatePages switch them on.
//				Types that must never be mapped get no leaf at all.
extern VirtualAddress InstantiatePageTable(const void *D_, const uint64_t StructSize, const uint64_t N, const uint64_t TotalMemoryPages){
	VirtualAddress Out = {0};
	if(!_1GPageSupport()){return Out;}
	InitialiseTemporaryPageTable(TotalMemoryPages * PAGE_SIZE);

	const uint64_t S4K = 1ULL << PAGE_SHIFT, S2M = 1ULL << PAGE_SHIFT2, S1G = 1ULL << PAGE_SHIFT3, S512G = 512ULL << PAGE_SHIFT3, 
		Addr4K = 0x000FFFFFFFFFF000ULL, Addr2M = 0x000FFFFFFFE00000ULL, Addr1G = 0x000FFFFFC0000000ULL, 
		//	Raw entry bits (same layout as the structs in paging.h; PageInMemory is bit 6 in table entries, bit 9 in leaves)
		fP = 1ULL << 0, fRW = 1ULL << 1, fPWT = 1ULL << 3, fPCD = 1ULL << 4, fTbl = 1ULL << 6, fPS = 1ULL << 7, fLeaf = 1ULL << 9, fNX = 1ULL << 63;
	//	false: duplicate leaves stay non-present until UpdatePageTable/AllocatePages enables them.
	const bool L5 = IsLA57Enabled(), Live = false;
	uint32_t PhysBits = GetMaxPhysicalAddressBits(), EferLow, EferHigh;
	if(PhysBits > 52){PhysBits = 52;}
	const uint64_t MaxPhys = 1ULL << PhysBits;
	//	XD bits are reserved (and fault) unless EFER.NXE is set.
	__asm__ volatile("rdmsr" : "=a"(EferLow), "=d"(EferHigh) : "c"(0xC0000080U));
	const bool NX = (EferLow >> 11) & 1;

	//	Identity sizing, everything rounded up.
	uint64_t IdentityBytes = TotalMemoryPages * PAGE_SIZE;
	if(IdentityBytes < (4 * S1G)){IdentityBytes = 4 * S1G;}
	IdentityBytes = (__roundup(IdentityBytes, S1G) / S1G) * S1G;
	const uint64_t IdentityGB = IdentityBytes / S1G,								//	1GB leaf entries
					IdentityPDPT = __roundup(IdentityGB, PagingTableLength) / PagingTableLength,	//	PDPT tables == PML4 entries
					IdentityPML4 = L5? (__roundup(IdentityPDPT, PagingTableLength) / PagingTableLength): 1;	//	PML4 tables == PML5 entries
	if((!L5 && IdentityPDPT > PagingTableLength) || (L5 && IdentityPML4 > PagingTableLength)){return Out;}

	//	Duplicate sizing: the highest address any descriptor reaches, then a worst case table count.
	uint64_t Top = 0xFEC00000ULL + 0x400000ULL;
	for(uint64_t cc = 0; cc < N; ++cc){
		GetEfiMemoryDescriptor(D, D_, cc, StructSize);
		uint64_t End = D->PhysicalStart + (D->NumberOfPages * S4K);
		if((End <= MaxPhys) && (End > Top)){Top = End;}
	}
	const uint64_t Dup = IdentityBytes;
	if(!L5 && ((Dup + Top) > (1ULL << 47))){return Out;}
	uint64_t DupPages = 4 + (((__roundup(Top, S512G) / S512G) + 2) * 2) + ((__roundup(Top, S1G) / S1G) + 2) + (2 * N), 
		PoolPages = (L5? 1: 0) + IdentityPML4 + IdentityPDPT + DupPages + 8;

	//	Claim a contiguous pool (QueryCandidatePool zeroes it).
	uint64_t CandidatePages = 0;
	EFI_MEMORY_DESCRIPTOR *Candidate = QueryCandidatePool(D_, StructSize, &CandidatePages, N, PoolPages, TypeMask1, sizeof(TypeMask1), AttributeMask);
	if(!Candidate || (CandidatePages < PoolPages)){return Out;}
	const uint64_t PT = Candidate->PhysicalStart, L4Off = L5? S4K: 0, L3Off = L4Off + (IdentityPML4 * S4K);

	//	Identity tables, laid out as [PML5][PML4 tables][PDPT tables]. Entry g of a level points at table g of the next.
	if(L5){
		PML5Table *T5 = (PML5Table *)PT;
		for(uint64_t e = 0; e < IdentityPML4; ++e){
			(*T5)[e] = (PML5Entry){
				.Present = true, .ReadWrite = true, .UserMode = false, .PageLevelWriteThrough = false, .PageLevelCacheDisable = false, 
				.PageInMemory = true, .ExecuteDisable = false, .PhysicalAddress = (PT + L4Off + (e * S4K)) >> PAGE_SHIFT
			};
		}
	}
	for(uint64_t t = 0; t < IdentityPML4; ++t){
		PML4Table *T4 = (PML4Table *)(PT + L4Off + (t * S4K));
		uint64_t Entries = __min(PagingTableLength, IdentityPDPT - (t * PagingTableLength));
		for(uint64_t e = 0; e < Entries; ++e){
			(*T4)[e] = (PML4Entry){
				.Present = true, .ReadWrite = true, .UserMode = false, .PageLevelWriteThrough = false, .PageLevelCacheDisable = false, 
				.PageInMemory = true, .ExecuteDisable = false, .PhysicalAddress = (PT + L3Off + (((t * PagingTableLength) + e) * S4K)) >> PAGE_SHIFT
			};
		}
	}
	for(uint64_t t = 0; t < IdentityPDPT; ++t){
		PDPTTable *T3 = (PDPTTable *)(PT + L3Off + (t * S4K));
		uint64_t Entries = __min(PagingTableLength, IdentityGB - (t * PagingTableLength));
		for(uint64_t e = 0; e < Entries; ++e){
			T3->pages1GB[e] = (PDPTEntry1GB){
				.Present = true, .ReadWrite = true, .UserMode = false, .PageLevelWriteThrough = false, .PageLevelCacheDisable = false, 
				.PageSize = true, .Global = false, .Dirty = false, .PageInMemory = true, .PageAttributeTable = false, 
				.PhysicalAddress = (t * PagingTableLength) + e, .ProtectionKey = 0x00, .ExecuteDisable = false
			};
		}
	}

	//	Duplicate region. It shares the root with the identity map; extra tables come from the rest of the pool.
	//	cc == N is a synthetic entry for the platform MMIO window (IOAPIC/HPET/LAPIC), which is usually not in the EFI map.
	const uint64_t Root = L5? PT: (PT + L4Off), End = PT + (CandidatePages * S4K);
	uint64_t Next = PT + L3Off + (IdentityPDPT * S4K);
	for(uint64_t cc = 0; cc <= N; ++cc){
		uint64_t Phys, Bytes;
		bool Write = false, Exec = false, UC = false;
		if(cc == N){Phys = 0xFEC00000ULL;	Bytes = 0x400000ULL;	Write = true;	UC = true;
		}else{
			GetEfiMemoryDescriptor(D, D_, cc, StructSize);
			Phys = D->PhysicalStart;	Bytes = D->NumberOfPages * S4K;
			switch((uint32_t)D->Type){
				//	.text/.data/.bss share one allocation: RWX until the linker script exports section bounds.
				case (uint32_t)KernelBinarySpace:
					Write = true;	Exec = true;	break;
				case EfiLoaderCode: case EfiBootServicesCode: case EfiRuntimeServicesCode:
					Exec = true;	break;
				case EfiLoaderData: case EfiBootServicesData: case EfiRuntimeServicesData: case EfiConventionalMemory: 
				case EfiACPIReclaimMemory: case EfiACPIMemoryNVS: case EfiPersistentMemory:
					Write = true;	break;
				case EfiMemoryMappedIO:
					Write = true;	UC = true;	break;
				//	Reserved, Unusable, MMIOPortSpace, Unaccepted, PalCode and anything else: never mapped.
				default:
					Bytes = 0;	break;
			}
		}
		if((Phys + Bytes) > MaxPhys){Bytes = 0;}
		const uint64_t Flags = (Live? (fP | fLeaf): 0) | (Write? fRW: 0) | ((!Exec && NX)? fNX: 0) | (UC? (fPWT | fPCD): 0);
		uint64_t V = Dup + Phys, P = Phys;
		while(Bytes){
			//	Largest page that fits at this position. Leaf is the level that holds the leaf entry (3: 1GB, 2: 2MB, 1: 4KB).
			uint32_t Leaf = 1;
			uint64_t Step = S4K;
			if(!((V | P) & (S1G - 1)) && (Bytes >= S1G)){Leaf = 3;	Step = S1G;}
			else if(!((V | P) & (S2M - 1)) && (Bytes >= S2M)){Leaf = 2;	Step = S2M;}

			volatile uint64_t *T = (volatile uint64_t *)Root;
			bool Covered = false;
			for(uint32_t Level = L5? 5: 4; Level > Leaf; --Level){
				volatile uint64_t *E = T + ((V >> (PAGE_SHIFT + (9 * (Level - 1)))) & 0x1FF);
				//	An entry is empty only when it is entirely zero: non-present skeleton leaves are not empty.
				if(!(*E)){
					if((Next + S4K) > End){return Out;}
					memset((void *)Next, 0, S4K);
					*E = Next | fP | fRW | fTbl;
					Next += S4K;
				}else if((*E) & fPS){Covered = true;	break;}
				T = (volatile uint64_t *)((*E) & Addr4K);
			}
			if(!Covered){
				volatile uint64_t *E = T + ((V >> (PAGE_SHIFT + (9 * (Leaf - 1)))) & 0x1FF);
				if(!(*E)){*E = (P & ((Leaf == 3)? Addr1G: ((Leaf == 2)? Addr2M: Addr4K))) | Flags | ((Leaf > 1)? fPS: 0);}
			}
			V += Step;	P += Step;	Bytes -= Step;
		}
	}

	//	Hand the pool to the allocator's "in use" class. Descriptors are walked by StructSize, not by
	//	sizeof(EFI_MEMORY_DESCRIPTOR): the firmware's stride is usually larger.
	for(uint64_t cc = 0, Remaining = CandidatePages; Remaining; ++cc){
		EFI_MEMORY_DESCRIPTOR *C = (EFI_MEMORY_DESCRIPTOR *)((uint8_t *)Candidate + (cc * StructSize));
		C->Type = KernelBinarySpace;
		Remaining = (C->NumberOfPages >= Remaining)? 0: (Remaining - C->NumberOfPages);
	}

	//	paging.asm returns 1 on success, 0 or 2 if the LA57 state did not match.
	__bochs();
	if(LoadPageTree((void *)Root, 0x10, 0x08, (bool64_t)L5) != 1){return Out;}
	bool _L5 = L5;
	GetPageTable(&_L5, _UpdatePageTableMode, (void *)Root);	//	Physical == identity address.

	//	First virtual address above the duplicate: where IState->LastPageTableAlloc continues.
	*(uint64_t *)&Out = Dup + Top;
	return Out;
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
	
	int64_t				TypeContiguous0 = -1, TypeContiguous1 = -1, TypeContiguous2 = -1, TypeContiguous3 = -1;
	for(uint64_t cc = 0; cc < N; ++cc){
		GetEfiMemoryDescriptor(D, D_, cc, StructSize);
		if(memcheckl(TypeMask0, D->Type, sizeof(TypeMask0))){
			if(TypeContiguous0 == -1){TypeContiguous0 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);}
			else if(TypeContiguous0 != D->PhysicalStart){
				TypeContiguous0 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);
				SubDescriptorAllocation += (D->NumberOfPages > (PAGE_SIZE2 / PAGE_SIZE)? (D->NumberOfPages * PAGE_SIZE) / PAGE_SIZE2: D->NumberOfPages);
				DescriptorAllocation += sizeof(MemoryRangeDescriptor);}
		}else if(memcheckl(TypeMask1, D->Type, sizeof(TypeMask1))){
			if(TypeContiguous1 == -1){TypeContiguous1 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);}
			else if(TypeContiguous1 != D->PhysicalStart){
				TypeContiguous0 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);
				SubDescriptorAllocation += (D->NumberOfPages > (PAGE_SIZE2 / PAGE_SIZE)? (D->NumberOfPages * PAGE_SIZE) / PAGE_SIZE2: D->NumberOfPages);
				DescriptorAllocation += sizeof(MemoryRangeDescriptor);}
		}else if(memcheckl(TypeMask2, D->Type, sizeof(TypeMask2))){
			if(TypeContiguous2 == -1){TypeContiguous2 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);}
			else if(TypeContiguous2 != D->PhysicalStart){
				TypeContiguous0 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);
				SubDescriptorAllocation += (D->NumberOfPages > (PAGE_SIZE2 / PAGE_SIZE)? (D->NumberOfPages * PAGE_SIZE) / PAGE_SIZE2: D->NumberOfPages);
				DescriptorAllocation += sizeof(MemoryRangeDescriptor);}
		}else if(memcheckl(TypeMask3, D->Type, sizeof(TypeMask3))){
			if(TypeContiguous3 == -1){TypeContiguous3 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);}
			else if(TypeContiguous3 != D->PhysicalStart){
				TypeContiguous0 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);
				SubDescriptorAllocation += (D->NumberOfPages > (PAGE_SIZE2 / PAGE_SIZE)? (D->NumberOfPages * PAGE_SIZE) / PAGE_SIZE2: D->NumberOfPages);
				DescriptorAllocation += sizeof(MemoryRangeDescriptor);}
		}else{continue;}
	}
	uint64_t TotalRequiredPages = (SubDescriptorAllocation + PhysToIndexAllocation + DescriptorAllocation + sizeof(uint64_t));
	TotalRequiredPages = __roundup(TotalRequiredPages, PAGE_SIZE) / PAGE_SIZE;

	uint64_t ElapsedPages = 0;
	VirtualAddress VA = {0};
	volatile InternalAllocationState *IState = GetIState();
	DebugPrintf("\nInitialising Page Table");	{
		//	Temporarily Initialise Minimal Paging Dependencies.
		bool _L = IsLA57Enabled();
		void *PT = GetPageTable(&_L, _1To1MappingMode, NULL);
		(GetPageTable(&_L, _UpdatePageTableMode, PT));
		VA = InstantiatePageTable(D_, StructSize, N, TotalMemory / PAGE_SIZE);
	}
	EFI_MEMORY_DESCRIPTOR *Candidate = QueryCandidatePool(D_, StructSize, &ElapsedPages, N, TotalRequiredPages, TypeMask1, sizeof(TypeMask1), AttributeMask);	{
		uint64_t cc = 0, _ElapsedPages = ElapsedPages;
		while(_ElapsedPages){
			Candidate[cc].Type = KernelBinarySpace;
			if(Candidate[cc].NumberOfPages > _ElapsedPages){ElapsedPages = 0;}else{_ElapsedPages -= Candidate[cc].NumberOfPages;}
			cc++;
		}
	}
	DebugPrintf("\nDescriptor Pool: 0x%0z\t%z Pages", Candidate->PhysicalStart, (uint64_t)ElapsedPages);
	memset((void *)(Candidate->PhysicalStart), 0, (uint64_t)(ElapsedPages) * PAGE_SIZE);

	*IState = (volatile InternalAllocationState){
		.Descriptors = (MemoryRangeDescriptor *)(Candidate->PhysicalStart), .MemoryMaxPages = TotalMemory / PAGE_SIZE, 
		.PhysicalDescriptorIndexes = (uint16_t *)(Candidate->PhysicalStart + DescriptorAllocation), .NonIdentityMappedState = false, 
		.SubDescriptors = (MemoryRangeSubDescriptor **)(Candidate->PhysicalStart + DescriptorAllocation + SubDescriptorAllocation), 
		.TotalDescriptors = DescriptorAllocation / sizeof(MemoryRangeDescriptor), .TotalSubDescriptors = SubDescriptorAllocation / sizeof(MemoryRangeSubDescriptor), 
		.TotalSubDescriptorPools = 1
	};	*(IState->SubDescriptors) = (MemoryRangeSubDescriptor *)(Candidate->PhysicalStart + DescriptorAllocation + SubDescriptorAllocation + sizeof(uint64_t));
	TypeContiguous0 = TypeContiguous1 = TypeContiguous2 = TypeContiguous3 = -1;
	uint32_t CurrentSubDescriptor = 0;

	//	Need to track contiguous Memory Types.
	//		If Contiguous:		Update the Entry to encompass the Additional Entry.
	//		If not Contiguous:	Increment to another Entry
	__bochs();
	for(uint64_t cc = 0, ArrayCtr = 0; cc < N; ++cc){
		GetEfiMemoryDescriptor(D, D_, cc, StructSize);
		if(memcheckl(TypeMask0, D->Type, sizeof(TypeMask0))){
			if(TypeContiguous0 == -1){TypeContiguous0 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);}else 
			if(TypeContiguous0 != D->PhysicalStart){ArrayCtr++;		TypeContiguous0 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);}else{
				IState->Descriptors[ArrayCtr].PageLimit += D->NumberOfPages;	continue;}
			IState->Descriptors[ArrayCtr] = (MemoryRangeDescriptor){
				.SubDescriptorIndex = CurrentSubDescriptor, .EfiMemoryType = D->Type, .PageLimit = D->NumberOfPages, .Physical = D->PhysicalStart};
			CurrentSubDescriptor += (D->NumberOfPages > (PAGE_SIZE2 / PAGE_SIZE)? (D->NumberOfPages * PAGE_SIZE) / PAGE_SIZE2: D->NumberOfPages);
			UpdatePageTable((void *)(D->PhysicalStart), D->NumberOfPages * PAGE_SIZE, ReadWritable | SupervisorMode | PageLevelWriteThroughEnable | GlobalEnable, 0x00);
		}else if(memcheckl(TypeMask1, D->Type, sizeof(TypeMask1))){
			if(TypeContiguous1 == -1){TypeContiguous1 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);}else 
			if(TypeContiguous1 != D->PhysicalStart){ArrayCtr++;		TypeContiguous1 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);}else{
				IState->Descriptors[ArrayCtr].PageLimit += D->NumberOfPages;	continue;}
			IState->Descriptors[ArrayCtr] = (MemoryRangeDescriptor){
				.SubDescriptorIndex = CurrentSubDescriptor, .EfiMemoryType = D->Type, .PageLimit = D->NumberOfPages, .Physical = D->PhysicalStart};
			CurrentSubDescriptor += (D->NumberOfPages > (PAGE_SIZE2 / PAGE_SIZE)? (D->NumberOfPages * PAGE_SIZE) / PAGE_SIZE2: D->NumberOfPages);
		}else if(memcheckl(TypeMask2, D->Type, sizeof(TypeMask2))){
			if(TypeContiguous2 == -1){TypeContiguous2 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);}else 
			if(TypeContiguous2 != D->PhysicalStart){ArrayCtr++;		TypeContiguous2 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);}else{
				IState->Descriptors[ArrayCtr].PageLimit += D->NumberOfPages;	continue;}
			IState->Descriptors[ArrayCtr] = (MemoryRangeDescriptor){
				.SubDescriptorIndex = CurrentSubDescriptor, .EfiMemoryType = D->Type, .PageLimit = D->NumberOfPages, .Physical = D->PhysicalStart};
			CurrentSubDescriptor += (D->NumberOfPages > (PAGE_SIZE2 / PAGE_SIZE)? (D->NumberOfPages * PAGE_SIZE) / PAGE_SIZE2: D->NumberOfPages);
		}else if(memcheckl(TypeMask3, D->Type, sizeof(TypeMask3))){
			if(TypeContiguous3 == -1){TypeContiguous3 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);}else 
			if(TypeContiguous3 != D->PhysicalStart){ArrayCtr++;		TypeContiguous3 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);}else{
				IState->Descriptors[ArrayCtr].PageLimit += D->NumberOfPages;	continue;}
			IState->Descriptors[ArrayCtr] = (MemoryRangeDescriptor){
				.SubDescriptorIndex = CurrentSubDescriptor, .EfiMemoryType = D->Type, .PageLimit = D->NumberOfPages, .Physical = D->PhysicalStart};
			CurrentSubDescriptor += (D->NumberOfPages > (PAGE_SIZE2 / PAGE_SIZE)? (D->NumberOfPages * PAGE_SIZE) / PAGE_SIZE2: D->NumberOfPages);
			UpdatePageTable((void *)(D->PhysicalStart), D->NumberOfPages * PAGE_SIZE, ReadWritable | SupervisorMode | PageLevelWriteThroughEnable | GlobalEnable, 0x00);
		}else{continue;}
		for(uint64_t cc_ = 0; cc_ < D->NumberOfPages; ++cc_){IState->PhysicalDescriptorIndexes[(D->PhysicalStart / PAGE_SIZE) + cc_] = ArrayCtr;}
	}
	__bochs();
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
	if(IS->NonIdentityMappedState){
		QueryPhysicalAddress(Physical, IS){
			GetSubDescriptor(SD, Physical, IS){
				VirtualAddress VA = InitVirtualAddress(SD->CoordBaseIndex);
				return (void *)VirtualAddressAToU64(VA);
			}
		}
		//	57 Bits
		//	48 Bits
	}else{return (void *)((uint64_t)Physical & (IsLA57Enabled()? 0x1FFFFFFFFFFFFFF: 0xFFFFFFFFFFFF));}
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