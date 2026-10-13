#include "paging.h"
#include "kernel/libcrt/memory/memory.h"
#include "kernel/libcrt/SimpleOutput/SimpleOutput.h"

#define GetEfiMemoryDescriptor(Desc, Ptr, N, StructSize)	EFI_MEMORY_DESCRIPTOR *Desc = (EFI_MEMORY_DESCRIPTOR *)((void *)(Ptr) + ((N) * (StructSize)))
#define QueryPhysicalAddress(Adr, IState)			\
		if((IState) && (IState)->MemoryMaxPages && ((IState)->MemoryMaxPages > ((uint64_t)(Adr) / PAGE_SIZE)))
#define GetDescriptor(Descriptor, Adr, IState)	\
		MemoryRangeDescriptor *Descriptor = (Adr > (IState)->MemoryMaxPages)? (IState)->Descriptors + (IState)->PhysicalDescriptorIndexes[(uint64_t)(Adr) / PAGE_SIZE]: NULL;	\
		if(Descriptor)
//	We need the Entry correspondng epecifically to this entry
#define GetSubDescriptor(SubDescriptor, Adr, IState)\
		MemoryRangeSubDescriptor *SubDescriptor = (Adr > (IState)->MemoryMaxPages) &&	\
			(IState)->Descriptors[(IState)->PhysicalDescriptorIndexes[(uint64_t)(Adr) / PAGE_SIZE]].SubDescriptorIndex != InvalidSubDescriptorIndex?	\
			((IState)->SubDescriptors[(IState)->CurrentSubDescriptorPool] + (IState)->Descriptors[(IState)->PhysicalDescriptorIndexes[(uint64_t)(Adr) / PAGE_SIZE]].SubDescriptorIndex): NULL;		\
			{uint64_t SubDescriptor##cc = 0, SubDescriptor##CopyAdr = (uint64_t)Adr - (IState)->Descriptors[(IState)->PhysicalDescriptorIndexes[(uint64_t)(Adr) / PAGE_SIZE]].Physical;	\
				while(!SubDescriptor[SubDescriptor##cc].EndOfList && SubDescriptor##CopyAdr){	\
					if((SubDescriptor[SubDescriptor##cc].NoPages * PAGE_SIZE) > SubDescriptor##CopyAdr){SubDescriptor##CopyAdr -= (SubDescriptor[SubDescriptor##cc].NoPages * PAGE_SIZE);}	\
						else{SubDescriptor##CopyAdr = 0;}	SubDescriptor##cc++;}	\
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
}InternalAllocationState;

static volatile InternalAllocationState *GetIState(){
	__bochs();
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
bool MemoryIsMapped(void *Ptr){
	volatile InternalAllocationState *IS = GetIState();
	void *Physical = MapPhysical(Ptr);
	QueryPhysicalAddress(Physical, IS){
		GetSubDescriptor(SD, Physical, IS){return SD->EndOfList;}
	}else{return false;}
	
}

EFI_MEMORY_DESCRIPTOR *QueryCandidatePool(
	const void *D_, const uint32_t StructSize, uint32_t *TotalPages, const uint32_t N, const uint64_t RequiredPages, 
	const EFI_MEMORY_TYPE *TypeRequirements, const uint16_t NTypeRequirements, const uint32_t AttributeMask
){
	int64_t Candidate = -1;
	uint64_t NCandidateEntries = UINT64_MAX, CandidateElapsedPages = UINT64_MAX, ProcessedN = N;
	for(uint64_t cc = 0; cc < N; ++cc){
		//	Update to fuse multiple descriptors to map the necessary bytes.
		GetEfiMemoryDescriptor(D, D_, N, StructSize);
		bool Usable = memcheckl(TypeRequirements, NTypeRequirements, D->Type);
		DebugPrintf("\n[%z]:\tType: %z(%s){Attr: %z\tPhys: %z\tNoPages: %z}", (uint64_t)cc, (uint64_t)D->Type, 
			(Usable? "Usable": "??"), D->Attribute, D->PhysicalStart, D->NumberOfPages);
		if(Usable){
			uint64_t RemainingPages = (uint64_t)RequiredPages, ElapsedPages = 0, cc_ = cc;
			//	Check that the Next Entry is contiguous, since D2 starts as D, then cc_ == 0 will always go through
			uint64_t Address = (D->PhysicalStart / PAGE_SIZE);
			for(; cc_ < N; ++cc_){
				EFI_MEMORY_DESCRIPTOR *D2 = (EFI_MEMORY_DESCRIPTOR *)(D_ + (StructSize * cc_));
				if(((D2->PhysicalStart / PAGE_SIZE) != Address)){
					DebugPrintf("\nNon-Continuous Pages: %z\t%z", (uint64_t)(D2->PhysicalStart / PAGE_SIZE), (uint64_t)Address);
					break;
				}
				if(D2->PhysicalStart >= nGB(4)){DebugPrintf("\nOverarching Address");		break;}
				ElapsedPages += D2->NumberOfPages;
				Address = (D2->PhysicalStart / PAGE_SIZE) + D2->NumberOfPages;
				Usable = D2->Type == EfiLoaderCode? true: (D2->Type == EfiBootServicesCode? true: 
					(D2->Type == EfiBootServicesData? true: D2->Type == EfiConventionalMemory? true: false));
				if(Usable && !__check(AttributeMask, D2->Attribute)){
					if(D2->NumberOfPages > RemainingPages){RemainingPages = 0;	break;}else{RemainingPages -= D2->NumberOfPages;}
				}else{break;}
			}
			//	The currently Viewed entry elapses more pages than the candidate.
			if(RemainingPages || ((ElapsedPages - RequiredPages) > (CandidateElapsedPages - RequiredPages))){continue;}else{
				CandidateElapsedPages = ElapsedPages;
				NCandidateEntries = cc_ - cc;
				Candidate = cc;
			}
		}
	}
	if((Candidate != -1) && (NCandidateEntries != UINT64_MAX) && (CandidateElapsedPages != UINT64_MAX)){
		//	Fuse the Descriptors under one general EfiConventionalMemory Tag.
		*TotalPages = CandidateElapsedPages;
		return (EFI_MEMORY_DESCRIPTOR *)(D_ + (StructSize * Candidate));
	}else{
		DebugPrintf("\nFailed to find valid Candidate");
		return NULL;
	}
}

//	Heavily relies of UEFI's 1:1 Mappign Mode
bool InitialiseAllocationState(void *D_, const uint64_t StructSize, uint64_t N, const uint64_t TotalMemory){
	if((GetIState())->PhysicalDescriptorIndexes){return true;}	//	Memory already Initialised
	uint64_t		TotalRequiredMemory = 0, 
					DescriptorAllocation = 0, 
	//	Allocation for SubDescriptors, Temporary. We can expand later.
	//		An entry per 2 Mb
	//		For the 4Gb Default, it's around 2048 Entries in total
					SubDescriptorAllocation = 0, 
	//	Allocation for Physical to Index Wrappers.
					PhysToIndexAllocation = (TotalMemory / PAGE_SIZE) * sizeof(uint16_t);
	//	Non-Usable Persistent Memory.
	const EFI_MEMORY_TYPE TypeMask0[] = {EfiRuntimeServicesCode, EfiRuntimeServicesData, EfiACPIReclaimMemory, EfiACPIMemoryNVS}, 
	//	Usable Persistent Memory
						TypeMask1[] = {EfiConventionalMemory, EfiBootServicesCode, EfiBootServicesData, EfiLoaderCode}, 
	//	Used Persistent Memory
						TypeMask2[] = {EfiLoaderData}, 
	//	Non-Physical Memory.
						TypeMask3[] = {EfiReservedMemoryType, EfiMemoryMappedIOPortSpace, EfiMemoryMappedIO};
	uint32_t	AttributeMask = EFI_MEMORY_WB | EFI_MEMORY_XP;
	int64_t				TypeContiguous0 = -1, TypeContiguous1 = -1, TypeContiguous2 = -1, TypeContiguous3 = -1;
	for(uint64_t cc = 0; cc < N; ++cc){
		GetEfiMemoryDescriptor(D, D_, cc, StructSize);
		if(memcheckl(TypeMask0, D->Type, ARRSIZE(TypeMask0))){
			if(TypeContiguous0 == -1){TypeContiguous0 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);}
			else if(TypeContiguous0 != D->PhysicalStart){
				TypeContiguous0 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);
				SubDescriptorAllocation += (D->NumberOfPages > (PAGE_SIZE2 / PAGE_SIZE)? (D->NumberOfPages * PAGE_SIZE) / PAGE_SIZE2: D->NumberOfPages);
				DescriptorAllocation += sizeof(MemoryRangeDescriptor);}
		}else if(memcheckl(TypeMask1, D->Type, ARRSIZE(TypeMask1))){
			if(TypeContiguous1 == -1){TypeContiguous1 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);}
			else if(TypeContiguous1 != D->PhysicalStart){
				TypeContiguous0 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);
				SubDescriptorAllocation += (D->NumberOfPages > (PAGE_SIZE2 / PAGE_SIZE)? (D->NumberOfPages * PAGE_SIZE) / PAGE_SIZE2: D->NumberOfPages);
				DescriptorAllocation += sizeof(MemoryRangeDescriptor);}
		}else if(memcheckl(TypeMask2, D->Type, ARRSIZE(TypeMask2))){
			if(TypeContiguous2 == -1){TypeContiguous2 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);}
			else if(TypeContiguous2 != D->PhysicalStart){
				TypeContiguous0 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);
				SubDescriptorAllocation += (D->NumberOfPages > (PAGE_SIZE2 / PAGE_SIZE)? (D->NumberOfPages * PAGE_SIZE) / PAGE_SIZE2: D->NumberOfPages);
				DescriptorAllocation += sizeof(MemoryRangeDescriptor);}
		}else if(memcheckl(TypeMask3, D->Type, ARRSIZE(TypeMask3))){
			if(TypeContiguous3 == -1){TypeContiguous3 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);}
			else if(TypeContiguous3 != D->PhysicalStart){
				TypeContiguous0 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);
				SubDescriptorAllocation += (D->NumberOfPages > (PAGE_SIZE2 / PAGE_SIZE)? (D->NumberOfPages * PAGE_SIZE) / PAGE_SIZE2: D->NumberOfPages);
				DescriptorAllocation += sizeof(MemoryRangeDescriptor);}
		}else{continue;}
	}
	TotalRequiredMemory = (SubDescriptorAllocation + PhysToIndexAllocation + DescriptorAllocation + sizeof(uint64_t));
	TotalRequiredMemory = __roundup(TotalRequiredMemory, PAGE_SIZE) / PAGE_SIZE;

	uint32_t TotalPages = 0;
	volatile InternalAllocationState *IState = GetIState();
	EFI_MEMORY_DESCRIPTOR *D = QueryCandidatePool(D_, StructSize, &TotalPages, N, TotalRequiredMemory, TypeMask1, ARRSIZE(TypeMask1), AttributeMask);
	memset(D, 0, TotalPages * PAGE_SIZE);
	*IState = (volatile InternalAllocationState){
		.Descriptors = D->PhysicalStart, .MemoryMaxPages = TotalMemory / PAGE_SIZE, 
		.PhysicalDescriptorIndexes = TotalMemory + DescriptorAllocation, 
		.SubDescriptors = TotalMemory + DescriptorAllocation + SubDescriptorAllocation, 
		.TotalDescriptors = DescriptorAllocation / sizeof(MemoryRangeDescriptor), 
		.TotalSubDescriptors = SubDescriptorAllocation / sizeof(MemoryRangeSubDescriptor), 
		.TotalSubDescriptorPools = 1
	};
	*(IState->SubDescriptors) = TotalMemory + DescriptorAllocation + SubDescriptorAllocation + sizeof(uint64_t);
	TypeContiguous0 = TypeContiguous1 = TypeContiguous2 = TypeContiguous3 = -1;
	uint32_t CurrentContiguous = UINT32_MAX, CurrentSubDescriptor = 0;
	//	Need to track contiguous Memory Types.
	//		If Contiguous:		Update the Entry to encompass the Additional Entry.
	//		If not Contiguous:	Increment to another Entry
	for(uint64_t cc = 0, ArrayCtr = 0; cc < N; ++cc){
		GetEfiMemoryDescriptor(D, D_, cc, StructSize);
		if(memcheckl(TypeMask0, D->Type, ARRSIZE(TypeMask0))){
			if(TypeContiguous0 == -1){
				TypeContiguous0 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);	CurrentContiguous = 0;}else 
			if(TypeContiguous0 != D->PhysicalStart){ArrayCtr++;}else{
				IState->Descriptors[ArrayCtr].PageLimit += D->NumberOfPages;	continue;}
			if(CurrentContiguous == 0){IState->Descriptors[ArrayCtr] = (MemoryRangeDescriptor){.SubDescriptorIndex = CurrentSubDescriptor, .EfiMemoryType = D->Type, .PageLimit = D->NumberOfPages, .Physical = D->PhysicalStart};}
			CurrentSubDescriptor += (D->NumberOfPages > (PAGE_SIZE2 / PAGE_SIZE)? (D->NumberOfPages * PAGE_SIZE) / PAGE_SIZE2: D->NumberOfPages);
		}else if(memcheckl(TypeMask1, D->Type, ARRSIZE(TypeMask1))){
			if(TypeContiguous1 == -1){
				TypeContiguous1 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);	CurrentContiguous = 1;}else 
			if(TypeContiguous1 != D->PhysicalStart){ArrayCtr++;}else{
				IState->Descriptors[ArrayCtr].PageLimit += D->NumberOfPages;	continue;}
			if(CurrentContiguous == 1){IState->Descriptors[ArrayCtr] = (MemoryRangeDescriptor){.SubDescriptorIndex = CurrentSubDescriptor, .EfiMemoryType = D->Type, .PageLimit = D->NumberOfPages, .Physical = D->PhysicalStart};}
			CurrentSubDescriptor += (D->NumberOfPages > (PAGE_SIZE2 / PAGE_SIZE)? (D->NumberOfPages * PAGE_SIZE) / PAGE_SIZE2: D->NumberOfPages);
		}else if(memcheckl(TypeMask2, D->Type, ARRSIZE(TypeMask2))){
			if(TypeContiguous2 == -1){
				TypeContiguous2 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);	CurrentContiguous = 2;}else 
			if(TypeContiguous2 != D->PhysicalStart){ArrayCtr++;}else{
				IState->Descriptors[ArrayCtr].PageLimit += D->NumberOfPages;	continue;}
			if(CurrentContiguous == 2){IState->Descriptors[ArrayCtr] = (MemoryRangeDescriptor){.SubDescriptorIndex = CurrentSubDescriptor, .EfiMemoryType = D->Type, .PageLimit = D->NumberOfPages, .Physical = D->PhysicalStart};}
			CurrentSubDescriptor += (D->NumberOfPages > (PAGE_SIZE2 / PAGE_SIZE)? (D->NumberOfPages * PAGE_SIZE) / PAGE_SIZE2: D->NumberOfPages);
		}else if(memcheckl(TypeMask3, D->Type, ARRSIZE(TypeMask3))){
			if(TypeContiguous3 == -1){
				TypeContiguous3 = D->PhysicalStart + (D->NumberOfPages * PAGE_SIZE);	CurrentContiguous = 3;}else 
			if(TypeContiguous3 != D->PhysicalStart){ArrayCtr++;}else{
				IState->Descriptors[ArrayCtr].PageLimit += D->NumberOfPages;	continue;}
			if(CurrentContiguous == 3){IState->Descriptors[ArrayCtr] = (MemoryRangeDescriptor){.SubDescriptorIndex = CurrentSubDescriptor, .EfiMemoryType = D->Type, .PageLimit = D->NumberOfPages, .Physical = D->PhysicalStart};}
			CurrentSubDescriptor += (D->NumberOfPages > (PAGE_SIZE2 / PAGE_SIZE)? (D->NumberOfPages * PAGE_SIZE) / PAGE_SIZE2: D->NumberOfPages);
		}else{continue;}
	}
	return true;
}

//	Returns Physical Address.
void *QueryFreeMemory(uint32_t NoPages){
	volatile InternalAllocationState *IS = GetIState();
	if(!NoPages || !IS->Descriptors || NoPages > IS->MemoryMaxPages){return NULL;}
	for(uint64_t cc = 0; cc < IS->TotalDescriptors; cc++){
		uint64_t cc_ = 0, Offset = 0;
		if(IS->Descriptors[cc].SubDescriptorIndex == InvalidSubDescriptorIndex){
			MemoryRangeSubDescriptor *SD = IS->SubDescriptors[IS->CurrentSubDescriptorPool] + IS->Descriptors[cc].SubDescriptorIndex;
			uint32_t LowestDiffN = 0;
			while(!(SD[cc_].EndOfList)){
				if((SD[cc_].NoPages >= NoPages) && !(SD[cc_].Valid)){if((SD[LowestDiffN].NoPages - NoPages) > (SD[cc_].NoPages - NoPages)){LowestDiffN = cc_;}}
				cc_++;
			}
			if(LowestDiffN){
				cc_ = LowestDiffN - 1;
				while(!(SD[cc_].EndOfList) && cc_){Offset += SD[cc_].NoPages;}
				return IS->Descriptors[cc].Physical + Offset;
			}
		}
	}
	return NULL;
}

MemoryRangeSubDescriptor *AllocateSubDescriptorPool(uint32_t *N){
	//	Get a Region in the SubDescriptorTable Table.
	volatile InternalAllocationState *IState = GetIState();
	uint64_t LowN = 0, _N = InvalidSubDescriptorIndex;
	for(uint32_t cc = 0; cc < IState->TotalDescriptors; ++cc){
		if(IState->Descriptors[cc].SubDescriptorIndex != InvalidSubDescriptorIndex){
			uint32_t cc_ = 0;
			MemoryRangeSubDescriptor *SD = IState->SubDescriptors[IState->CurrentSubDescriptorPool] + IState->Descriptors[cc].SubDescriptorIndex;
			while(!(SD[cc_].EndOfList)){cc_++;}
			//	If the Low and N are within the Current's Range then Invalid
			if((LowN >= IState->Descriptors[cc].SubDescriptorIndex) && ((LowN + _N) < (IState->Descriptors[cc].SubDescriptorIndex + cc_))){continue;}else{
				LowN = IState->Descriptors[cc].SubDescriptorIndex + cc_;
				_N = (_N > *N) && (((cc + 1) < IState->TotalDescriptors))? 
					(IState->Descriptors[cc + 1].SubDescriptorIndex - (IState->Descriptors[cc].SubDescriptorIndex + cc_)): InvalidSubDescriptorIndex;
			}
		}
	}
	*N = _N;
	return _N == InvalidSubDescriptorIndex? NULL: (IState->SubDescriptors[IState->CurrentSubDescriptorPool] + LowN);
}
void *ReAllocateSubDescriptorTable(){
	//	This function is for regenerating the Sub-Descriptor Table, with a Larger Buffer.
	//	We allocate in 512 SubDescriptor Blocks.
	volatile InternalAllocationState *IState = GetIState();
	void *Virtual = QueryFreeMemory((IState->TotalSubDescriptors + 512) * sizeof(MemoryRangeSubDescriptor)), 
		*OldPhysical = MapPhysical(IState->SubDescriptors[IState->CurrentSubDescriptorPool]), 
		*Physical = MapPhysical(Virtual);
	{
		uint32_t cc = 0;
		GetDescriptor(OldD, OldPhysical, IState);
		MemoryRangeSubDescriptor *SD = IState->SubDescriptors[IState->CurrentSubDescriptorPool] + OldD->SubDescriptorIndex;
		while(!(SD[cc].EndOfList)){SD[cc].AllocationType = 0;}
	}
	memcpy(Virtual, IState->SubDescriptors[IState->CurrentSubDescriptorPool], IState->TotalSubDescriptors * sizeof(MemoryRangeSubDescriptor));
	GetDescriptor(D, Physical, IState);
	MemoryRangeSubDescriptor *SD = IState->SubDescriptors[IState->CurrentSubDescriptorPool] + D->SubDescriptorIndex;
	uint32_t cc = 0;
	while(!SD[cc].EndOfList){
		if(!SD[cc].Valid){
			SD[cc].Valid = true;
			SD[cc].AllocationType = PATSubDescriptorMapping;
			VirtualAddress Tmp = *(VirtualAddress *)&Virtual;
			SD[cc].CoordBaseIndex = InitPageCoordinate(Tmp);
		}
	}
}

enumdef(uint64_t, AllocationTableUpdateBitMask){
	ATUSubDescriptorAllocationType = 0x01, ATUSubDescriptorValid = 0x02, 
	ATUSubDescriptorNoPages = 0x04, ATUSubDescriptorCoordBaseIndex = 0x08, 
	ATUSubDescriptor = (ATUSubDescriptorAllocationType | ATUSubDescriptorValid | ATUSubDescriptorNoPages | ATUSubDescriptorCoordBaseIndex)
};
void UpdateAllocationTableByPhysical(uint64_t BitMask, uint64_t Physical, ...){
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
					uint64_t NoPages = va_arg(ls, uint64_t), cc = 0;
					//	Update proceeding entries so they continue to map the same relative range.
					while(!SD[cc++].EndOfList){SD[cc].NoPages += ((int64_t)NoPages - (int64_t)SD->NoPages);}
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
	PageCoordinate PC = {0};
	QueryPhysicalAddress(Base, IS){
		GetSubDescriptor(SD, Base, IS){

		}
	}
	if(NBytes >= PAGE_SIZE3){
		PC.EnableL1 = false;
		PC.EnableL2 = false;
		PC.EnableL3 = true;
		PC.EnableL4 = true;
		PC.EnableL5 = L5;
	}else if(NBytes >= PAGE_SIZE2){
		PC.EnableL1 = true;
		PC.EnableL2 = false;
		PC.EnableL3 = true;
		PC.EnableL4 = true;
		PC.EnableL5 = L5;
	}else{
		PC.EnableL1 = true;
		PC.EnableL2 = true;
		PC.EnableL3 = true;
		PC.EnableL4 = true;
		PC.EnableL5 = L5;
	}
	while(true){
		if(NBytes >= PAGE_SIZE3){PC.L3++;}else if(NBytes >= PAGE_SIZE2){PC.L2++;}else{PC.L1++;}
		if(PC.L1 == 0x00){PC.L2++;}
		if(PC.L2 == 0x00 && PC.L1 == 0x00){PC.L3++;}
		if(PC.L3 == 0x00 && PC.L2 == 0x00 && PC.L1 == 0x00){PC.L4++;}
		if(L5 && PC.L4 == 0x00 && PC.L3 == 0x00 && PC.L2 == 0x00 && PC.L1 == 0x00){PC.L5++;}
		uint32_t PL = 0x05;
		void *PT = WalkPageTree(&PL, PC);
		switch(PL){
			case 0x05: {
				if(L5){
					PML5Entry *_L5 = (PML5Entry *)PT;
					if(!(_L5->Present || _L5->PageInMemory)){
						(*_L5) = (PML5Entry){
							.PageInMemory = true, .Present = true, .PhysicalAddress = (uint64_t)QueryFreeMemory(1), 
							.ExecuteDisable = !__check(Flags, ExecuteEnable), 
							.PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), 
							.PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable), 
							.ReadWrite = __check(Flags, ReadWritable), 
							.UserSupervisor = __check(Flags, SupervisorMode), 
							.PageInMemory = true, 
						};
						//	We step back to make sure this entry is Processed.
						UpdateAllocationTableByPhysical(ATUSubDescriptor, _L5->PhysicalAddress, PATUsedRegion, true, PAGE_SIZE, PC);
						PC.L5--;
					}
				}
				break;
			} case 0x04: {
				PML4Entry *L4 = (PML4Entry *)PT;
				if(!(L4->Present || L4->PageInMemory)){
					(*L4) = (PML4Entry){
						.PageInMemory = true, .Present = true, .PhysicalAddress = (uint64_t)QueryFreeMemory(1), 
						.ExecuteDisable = !__check(Flags, ExecuteEnable), 
						.PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), 
						.PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable), 
						.ReadWrite = __check(Flags, ReadWritable), 
						.UserSupervisor = __check(Flags, SupervisorMode), 
						.PageInMemory = true, 
					};
					UpdateAllocationTableByPhysical(ATUSubDescriptor, L4->PhysicalAddress, PATUsedRegion, true, PAGE_SIZE, PC);
					//	We step back to make sure this entry is Processed.
					PC.L4--;
				}
				break;
			} case 0x03: {//	1 GB Huge Pages (PDPT)
				bool Usable = false, Touchable = true;
				PDPTEntryDirectory *L3 = (PDPTEntryDirectory *)PT;
				for(uint64_t cc = 0x00; (PC.L3 + cc) < PagingTableLength; ++cc){
					if(L3[cc].Present || L3[cc].PageInMemory){Touchable = false;	break;}}
				if(Touchable){
					void *BASE = MemoryIsMapped(Base)? Base: QueryFreeMemory(Usable? NBytes / PAGE_SIZE: PAGE_SIZE);
					for(uint32_t cc = 0; (PC.L3 + cc) < PagingTableLength; cc++){
						if(Usable){
							UpdateAllocationTableByPhysical(ATUSubDescriptor, L3->PhysicalAddress, PATUsedRegion, true, PAGE_SIZE, PC);
							((PDPTEntry1GB *)L3)[cc] = (PDPTEntry1GB){
								.PageInMemory = true, .Present = true, .PhysicalAddress = (uint64_t)BASE + (cc * PAGE_SIZE3), 
								.PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable), 
								.PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), 
								.ExecuteDisable = !__check(Flags, ExecuteEnable), 
								.UserSupervisor = __check(Flags, SupervisorMode), 
								.ReadWrite = __check(Flags, ReadWritable), 
								.ProtectionKey = ProtectionKey, 
							};
						}else{
							L3[cc] = (PDPTEntryDirectory){
								.PageInMemory = true, .Present = true, .PhysicalAddress = (uint64_t)BASE, 
								.PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable), 
								.PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), 
								.UserSupervisor = __check(Flags, SupervisorMode), 
								.ExecuteDisable = !__check(Flags, ExecuteEnable), 
								.ReadWrite = __check(Flags, ReadWritable), 
							};
						}
					}
					if(!Usable){PC.L3--;}else{
						VirtualAddress Addr = {.L1 = PC.L1, .L2 = PC.L2, 
						.L3 = PC.L3, .L4 = PC.L4, .L5 = PC.L5, .Offset = 0x00, .L0 = 0};
						uint64_t _Addr = VirtualAddressAToU64(Addr);
						memset((void *)_Addr, 0x00, __roundup(NBytes, PAGE_SIZE3));
						return (void *)_Addr;
					}
				}
				break;
			} case 0x02: {//	2 MB Medium Pages
				bool Usable = false, Touchable = true;
				PageDirectoryEntry *L2 = (PageDirectoryEntry *)PT;
				for(uint64_t cc = 0x00; (PC.L2 + cc) < PagingTableLength; ++cc){
					if(L2[cc].Present || L2[cc].PageInMemory){Touchable = false;	break;}}
				if(Touchable){
					void *BASE = Base? Base: QueryFreeMemory(Usable? NBytes / PAGE_SIZE: PAGE_SIZE);
					for(uint32_t cc = 0; (PC.L2 + cc) < PagingTableLength; cc++){
						if(Usable){
							UpdateAllocationTableByPhysical((uint64_t)BASE + (cc * PAGE_SIZE2), PAGE_SIZE2, &PC, NULL, PATUsedRegion);
							((PageDirectoryEntry2MB *)L2)[cc] = (PageDirectoryEntry2MB){
								.PageInMemory = true, .Present = true, .PhysicalAddress = (uint64_t)BASE + (cc * PAGE_SIZE2), 
								.PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable), 
								.PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), 
								.UserSupervisor = __check(Flags, SupervisorMode), 
								.ExecuteDisable = !__check(Flags, ExecuteEnable), 
								.ReadWrite = __check(Flags, ReadWritable), 
								.ProtectionKey = ProtectionKey, 
							};
						}else{
							L2[cc] = (PageDirectoryEntry){
								.PageInMemory = true, .Present = true, .PhysicalAddress = (uint64_t)BASE, 
								.PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable), 
								.PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), 
								.UserSupervisor = __check(Flags, SupervisorMode), 
								.ExecuteDisable = !__check(Flags, ExecuteEnable), 
								.ReadWrite = __check(Flags, ReadWritable), 
							};
						}
					}
					if(!Usable){PC.L2--;}else{
						VirtualAddress Addr = {.L1 = PC.L1, .L2 = PC.L2, 
						.L3 = PC.L3, .L4 = PC.L4, .L5 = PC.L5, .Offset = 0x00, .L0 = 0};
						uint64_t _Addr = VirtualAddressAToU64(Addr);
						memset((void *)_Addr, 0x00, __roundup(NBytes, PAGE_SIZE2));
						return (void *)_Addr;
					}
				}
				break;
			} case 0x01: {//	4 KB Small Pages
				bool Usable = false;
				PageTableEntry4KB *L1 = (PageTableEntry4KB *)PT;
				if(Usable){
					for(uint64_t cc = 0x00; (PC.L1 + cc) < PagingTableLength; ++cc){
						if(L1[cc].Present || L1[cc].PageInMemory){Usable = false;	break;}}
					void *BASE = Base? Base: QueryFreeMemory(__roundup(NBytes, PAGE_SIZE) / PAGE_SIZE);
					if(Usable){
						for(uint32_t cc = 0; (PC.L1 + cc) < PagingTableLength; cc++){
							UpdateAllocationTableByPhysical((uint64_t)BASE + (cc * PAGE_SIZE), PAGE_SIZE, &PC, NULL, PATUsedRegion);
							L1[cc] = (PageTableEntry4KB){
								.PageInMemory = true, .Present = true, .PhysicalAddress = (uint64_t)BASE + (cc * PAGE_SIZE), 
								.ExecuteDisable = !__check(Flags, ExecuteEnable), 
								.PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), 
								.PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable), 
								.ReadWrite = __check(Flags, ReadWritable), 
								.UserSupervisor = __check(Flags, SupervisorMode), 
								.ProtectionKey = ProtectionKey, 
							};
						}
						VirtualAddress Addr = {.L1 = PC.L1, .L2 = PC.L2, 
						.L3 = PC.L3, .L4 = PC.L4, .L5 = PC.L5, .Offset = 0x00, .L0 = 0};
						uint64_t _Addr = VirtualAddressAToU64(Addr);
						memset((void *)_Addr, 0x00, __roundup(NBytes, PAGE_SIZE));
						return (void *)_Addr;
					}
				}
				break;
			}
		}
	}
}

void *MapVirtual(void *Physical){
	volatile InternalAllocationState *IS = GetIState();
	QueryPhysicalAddress(Physical, IS){
		GetSubDescriptor(SD, Physical, IS){
			VirtualAddress VA = InitVirtualAddress(SD->CoordBaseIndex);
			return *(uint64_t *)&VA;
		}
	}
	return NULL;
}

void FreePages(void *Virtual){
	if(!Virtual){return;}
	void *Physical = MapPhysical((void *)__rounddown((uint64_t)Virtual, PAGE_SIZE));
	volatile InternalAllocationState *IS = GetIState();
	QueryPhysicalAddress(Physical, IS){
		UpdateAllocationTableByPhysical(ATUSubDescriptorAllocationType, (uint64_t)Physical, ~PATUsedRegion);
		uint32_t Level = 5;
		uint64_t _TotalMappedMemory = TotalMappedMemory(Virtual);
		void *PT = WalkPageTreeByVirtual(Virtual, &Level);
		while(_TotalMappedMemory){
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