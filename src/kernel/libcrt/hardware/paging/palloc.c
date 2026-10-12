#include "paging.h"
#include "kernel/libcrt/memory/memory.h"

typedef struct{
	PageAllocationState		*VirtualBase;
	UINT64					TotalMemorySize;
}InternalAllocationState;

static volatile InternalAllocationState *GetIState(){
	static InternalAllocationState __internal = {0};
	return &__internal;
}

uint64_t TotalMemorySize(void){return (GetIState())->TotalMemorySize;}
uint64_t TotalMappedMemory(void *ptr){
	uint64_t out = 0x00;
	if(ptr){
		void *Physical = MapPhysical(ptr);
		volatile InternalAllocationState *IS = GetIState();
		if(!Physical || !IS->VirtualBase){return 0;}
		while(IS->VirtualBase[(uint64_t)Physical / PAGE_SIZE].Local){out += PAGE_SIZE;}		
	}
	return out;
}
bool MemoryIsMapped(void *Ptr){
	if(Ptr){
		void *Physical = MapPhysical(Ptr);
		volatile InternalAllocationState *IS = GetIState();
		if((__roundup((uint64_t)Physical, PAGE_SIZE) / PAGE_SIZE) > (IS->TotalMemorySize / PAGE_SIZE)){return false;}
		return __check(IS->VirtualBase[__roundup((uint64_t)Physical, PAGE_SIZE) / PAGE_SIZE].Type, PAFUsed);
	}
	return false;
}

//	Heavily relies of UEFI's 1:1 Mappign Mode
bool InitialiseAllocationState(void *D_, const uint64_t StructSize, const uint64_t N, const uint64_t TotalMemory){
	if((GetIState())->VirtualBase){return true;}	//	Memory already Initialised
	// EFI_MEMORY_DESCRIPTOR *D = (EFI_MEMORY_DESCRIPTOR *)D_;
	//	We need a Memory Region Large enough for us to claim as our own, for Initialisation.
	const UINT64 RequiredBytes = (TotalMemory / PAGE_SIZE) * sizeof(PageAllocationState), 
				Mask = EfiLoaderCode | EfiBootServicesCode | EfiBootServicesData | EfiConventionalMemory;
	UINT64 Diff = UINT64_MAX, PrimeMemory = 0;
	for(UINT64 cc = 0; cc < N; ++cc){
		EFI_MEMORY_DESCRIPTOR *D = (EFI_MEMORY_DESCRIPTOR *)(D_ + (StructSize * cc));
		if(__check(Mask, D->Type)){
			if(((D->NumberOfPages * EFI_PAGE_SIZE) >= RequiredBytes) && Diff > (D->NumberOfPages * EFI_PAGE_SIZE) - RequiredBytes){
				Diff = (D->NumberOfPages * EFI_PAGE_SIZE) - RequiredBytes;
				PrimeMemory = cc;
			}
		}
	}
	if((Diff == UINT64_MAX) && !PrimeMemory){return false;}
	// if(cc == N){return false;}//!	If a major Failure has occurred, we cannot Initialise any form of Memory Allocation. At that point it's best to lest the CPU Triple-Fault and Reboot.
	volatile InternalAllocationState *IState = GetIState();
	//	Update the Stored Page Table Pointer.
	//	Avoiding the Hang between tung and sahur, FACKAR
	bool _tmp = false;
	void *PTVirtual = GetPageTable(&_tmp, _1To1MappingMode, NULL);
	GetPageTable(&_tmp, _UpdatePageTableMode, PTVirtual);
	*IState = (InternalAllocationState){.TotalMemorySize = TotalMemory, 
		.VirtualBase = (PageAllocationState *)((EFI_MEMORY_DESCRIPTOR *)(D_ + (StructSize * PrimeMemory)))->PhysicalStart};
	
	//	We now Initialise the Table
	for(UINT64 cc = 0; cc < N; ++cc){
		EFI_MEMORY_DESCRIPTOR *D = (EFI_MEMORY_DESCRIPTOR *)(D_ + (StructSize * cc));
		PageAllocationType TType = 0;
		switch(D->Type){
			case EfiRuntimeServicesCode: 
			case EfiRuntimeServicesData: 
			case EfiUnusableMemory: 
			case EfiACPIReclaimMemory:
			case EfiACPIMemoryNVS: 
			case EfiMemoryMappedIO: 
			case EfiMemoryMappedIOPortSpace: 
				TType = PAFNoFree | PAFUsed | PAFNoTouch;
				break;
			case EfiConventionalMemory: 
			case EfiBootServicesCode: 
			case EfiLoaderCode: 
			case EfiBootServicesData:
				TType = 0x0;
				break; 
			case EfiLoaderData: 
				TType = PAFUsed;
				break;
			default:
				TType = PAFNoTouch | PAFNoFree;
				break;
		}
		union{VirtualAddress VA;	uint64_t	Phys;}	Tmp = {.Phys = D->PhysicalStart};
		for(UINT64 cc_ = 0; cc_ < D->NumberOfPages; ++cc_){
			IState->VirtualBase[(D->PhysicalStart / PAGE_SIZE) + cc_] = (PageAllocationState){
				.Coordinate = {
					.EnableL1 = true, .EnableL2 = true, .EnableL3 = true, 
					.EnableL4 = true, .EnableL5 = IsLA57Supported(), 
					.L1 = Tmp.VA.L1, .L2 = Tmp.VA.L2, .L3 = Tmp.VA.L3, 
					.L4 = Tmp.VA.L4, .L5 = Tmp.VA.L5
				}, .Type = TType, .Parent = {0}, .Local = (cc_ + 1) - D->NumberOfPages
			};
			Tmp.Phys += EFI_PAGE_SIZE;
		}
	}
	// __bochs();
	return true;
}

//	Returns Physical Address.
void *QueryFreeMemory(uint32_t _4KB){
	volatile InternalAllocationState *IS = GetIState();
	if(!_4KB || !IS->VirtualBase || _4KB > (IS->TotalMemorySize / PAGE_SIZE)){return NULL;}
	for(register uint64_t cc = 0; cc < (IS->TotalMemorySize / PAGE_SIZE); cc++){
		switch(IS->VirtualBase[cc].Type){
			case PAFNoFree:
			case PAFNoTouch: 
			case PAFUsed:		continue;
			default: {
				uint64_t cc_ = cc;
				uint64_t contiguous = 0;
				while(cc_ < (IS->TotalMemorySize / PAGE_SIZE)){
					switch(IS->VirtualBase[cc_].Type){
						case PAFNoFree:
						case PAFNoTouch: 
						case PAFUsed:		goto next_candidate;
						default:			contiguous++;
					}
					if(contiguous >= _4KB){
						return (void *)(cc * PAGE_SIZE);
					}
					cc_++;
				}
				next_candidate:
				break;
			}
		}
	}
	return NULL;
}

void UpdateAllocationTable(uint64_t Physical, uint64_t Bytes, PageCoordinate *coord, uint32_t ncoords, PageAllocationType type){
	volatile InternalAllocationState *IS = GetIState();
	Physical = __roundup((uint64_t)Physical, PAGE_SIZE) / PAGE_SIZE;
	Bytes = __roundup(Bytes, PAGE_SIZE) / PAGE_SIZE;
	uint64_t cc = 0x00;
	while(cc < Bytes){
		if(coord){IS->VirtualBase[(uint64_t)Physical].Coordinate = coord[__min(cc, ncoords)];}
		// if(type){IS->VirtualBase[(uint64_t)Physical].Type = *type;}
		type |= __check(IS->VirtualBase[(uint64_t)Physical].Type, PAFNoTouch)? PAFNoTouch: 0x00;
		type |= __check(IS->VirtualBase[(uint64_t)Physical].Type, PAFNoFree)? PAFNoFree: 0x00;
		IS->VirtualBase[(uint64_t)Physical].Type &= type;
		IS->VirtualBase[(uint64_t)Physical].Local = (cc + 1) - Bytes;
		Physical++;
		cc++;
	}
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
	bool L5;
	PageCoordinate PC = {0};
	UINT64 N = Base? __roundup((uint64_t)Base, PAGE_SIZE) / PAGE_SIZE: (IS->TotalMemorySize / PAGE_SIZE);
	void *PT = GetPageTable(&L5, _StoredPageTableMode, NULL);
	if(NBytes >= PAGE_SIZE3){
		PC.EnableL2 = false;
		PC.EnableL1 = false;
		PC.EnableL3 = true;
		PC.EnableL4 = true;
		PC.EnableL5 = L5;
	}else if(NBytes >= PAGE_SIZE2){
		PC.EnableL2 = false;
		PC.EnableL1 = true;
		PC.EnableL3 = true;
		PC.EnableL4 = true;
		PC.EnableL5 = L5;
	}else{
		PC.EnableL2 = true;
		PC.EnableL1 = true;
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
						UpdateAllocationTable(_L5->PhysicalAddress, PAGE_SIZE, &PC, 1, PAFUsed);
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
					UpdateAllocationTable(L4->PhysicalAddress, PAGE_SIZE, &PC, 1, PAFUsed);
					//	We step back to make sure this entry is Processed.
					PC.L4--;
				}
				break;
			} case 0x03: {//	1 GB Huge Pages (PDPT)
				bool Usable = false, Touchable = true;
				PDPTEntryDirectory *L3 = (PDPTEntryDirectory *)PT;
				//	Verify that the Entry has a Valid number of available Bytes.
				if(NBytes >= PAGE_SIZE3){
					Usable = true;
					for(register uint32_t cc = 0; cc < __roundup(NBytes, PAGE_SIZE3) / PAGE_SIZE3; ++cc){
						if(__check(IS->VirtualBase[N + cc].Type, PAFNoTouch) || 
							__check(IS->VirtualBase[N + cc].Type, PAFUsed)){Usable = false;		break;}
					}
				}
				for(register uint64_t cc = 0x00; (PC.L3 + cc) < PagingTableLength; ++cc){
					if(L3[cc].Present || L3[cc].PageInMemory){Touchable = false;	break;}}
				if(Touchable){
					void *BASE = MemoryIsMapped(Base)? Base: QueryFreeMemory(Usable? NBytes / PAGE_SIZE: PAGE_SIZE);
					for(register uint32_t cc = 0; (PC.L3 + cc) < PagingTableLength; cc++){
						if(Usable){
							UpdateAllocationTable((uint64_t)BASE + (cc * PAGE_SIZE3), PAGE_SIZE3, &PC, 1, PAFUsed);
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
						.L3 = PC.L3, .L4 = PC.L4, .L5 = PC.L5, .Offset = 0x00};
						uint64_t _Addr = VirtualAddressAToU64(Addr);
						memset((void *)_Addr, 0x00, __roundup(NBytes, PAGE_SIZE3));
						return (void *)_Addr;
					}
				}
				break;
			} case 0x02: {//	2 MB Medium Pages
				bool Usable = false, Touchable = true;
				PageDirectoryEntry *L2 = (PageDirectoryEntry *)PT;
				//	Verify that the Entry has a Valid number of available Bytes.
				if(NBytes >= PAGE_SIZE2){
					Usable = true;
					for(register uint32_t cc = 0; cc < __roundup(NBytes, PAGE_SIZE2) / PAGE_SIZE2; ++cc){
						if(__check(IS->VirtualBase[N + cc].Type, PAFNoTouch) || 
							__check(IS->VirtualBase[N + cc].Type, PAFUsed)){Usable = false;		break;}
					}
				}
				for(register uint64_t cc = 0x00; (PC.L2 + cc) < PagingTableLength; ++cc){
					if(L2[cc].Present || L2[cc].PageInMemory){Touchable = false;	break;}}
				if(Touchable){
					void *BASE = Base? Base: QueryFreeMemory(Usable? NBytes / PAGE_SIZE: PAGE_SIZE);
					for(register uint32_t cc = 0; (PC.L2 + cc) < PagingTableLength; cc++){
						if(Usable){
							UpdateAllocationTable((uint64_t)BASE + (cc * PAGE_SIZE2), PAGE_SIZE2, &PC, 1, PAFUsed);
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
						.L3 = PC.L3, .L4 = PC.L4, .L5 = PC.L5, .Offset = 0x00};
						uint64_t _Addr = VirtualAddressAToU64(Addr);
						memset((void *)_Addr, 0x00, __roundup(NBytes, PAGE_SIZE2));
						return (void *)_Addr;
					}
				}
				break;
			} case 0x01: {//	4 KB Small Pages
				bool Usable = false;
				PageTableEntry4KB *L1 = (PageTableEntry4KB *)PT;
				//	Verify that the Entry has a Valid number of available Bytes.
				if(NBytes >= PAGE_SIZE){
					Usable = true;
					for(register uint32_t cc = 0; cc < __roundup(NBytes, PAGE_SIZE) / PAGE_SIZE; ++cc){
						if(__check(IS->VirtualBase[N + cc].Type, PAFNoTouch) || 
							__check(IS->VirtualBase[N + cc].Type, PAFUsed)){Usable = false;		break;}
					}
				}
				if(Usable){
					for(register uint64_t cc = 0x00; (PC.L1 + cc) < PagingTableLength; ++cc){
						if(L1[cc].Present || L1[cc].PageInMemory){Usable = false;	break;}}
					void *BASE = Base? Base: QueryFreeMemory(__roundup(NBytes, PAGE_SIZE) / PAGE_SIZE);
					if(Usable){
						for(register uint32_t cc = 0; (PC.L1 + cc) < PagingTableLength; cc++){
							UpdateAllocationTable((uint64_t)BASE + (cc * PAGE_SIZE), PAGE_SIZE, &PC, 1, PAFUsed);
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
						.L3 = PC.L3, .L4 = PC.L4, .L5 = PC.L5, .Offset = 0x00};
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
	if(!IS->VirtualBase || !Physical || ((uint64_t)Physical / PAGE_SIZE) >= (IS->TotalMemorySize / PAGE_SIZE)){return NULL;}
	union{VirtualAddress Addr;	uint64_t	U64;}	Tmp = {.Addr = {
		.L1 = IS->VirtualBase[(uint64_t)Physical / PAGE_SIZE].Coordinate.L1, 
		.L2 = IS->VirtualBase[(uint64_t)Physical / PAGE_SIZE].Coordinate.L2, 
		.L3 = IS->VirtualBase[(uint64_t)Physical / PAGE_SIZE].Coordinate.L3, 
		.L4 = IS->VirtualBase[(uint64_t)Physical / PAGE_SIZE].Coordinate.L4, 
		.L5 = IS->VirtualBase[(uint64_t)Physical / PAGE_SIZE].Coordinate.L5, 
	}};
	return (void *)Tmp.U64 + ((uint64_t)Physical % PAGE_SIZE);
}

void FreePages(void *Virtual){
	if(!Virtual){return;}
	void *Physical = MapPhysical((void *)__rounddown((uint64_t)Virtual, PAGE_SIZE));
	volatile InternalAllocationState *IS = GetIState();
	if(!Physical || !IS->VirtualBase){return;}
	uint64_t Bytes = 0x00;
	while(IS->VirtualBase[__roundup((uint64_t)Physical, PAGE_SIZE) / PAGE_SIZE].Local){
		uint32_t lvl = 0x05;
		void *PT = WalkPageTree(&lvl, IS->VirtualBase[__roundup((uint64_t)Physical + Bytes, PAGE_SIZE) / PAGE_SIZE].Coordinate);
		switch(lvl){
			case 3: {
				PDPTEntry1GB *E = PT;
				E->Present = false;
				E->PageInMemory = false;
				InvalidatePage(Virtual + Bytes);
				Bytes += PAGE_SIZE3;
				break;
			} case 2: {
				PageDirectoryEntry2MB *E = PT;
				E->Present = false;
				E->PageInMemory = false;
				InvalidatePage(Virtual + Bytes);
				Bytes += PAGE_SIZE2;
				break;
			} case 1: {
				PageTableEntry4KB *E = PT;
				E->Present = false;
				E->PageInMemory = false;
				InvalidatePage(Virtual + Bytes);
				Bytes += PAGE_SIZE;
				break;
			} 
		}
	}
	UpdateAllocationTable((uint64_t)Physical, Bytes, NULL, 0x00, ~PAFUsed);
}