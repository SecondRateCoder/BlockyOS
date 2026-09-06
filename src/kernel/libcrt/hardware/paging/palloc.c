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
		InternalAllocationState *IS = GetIState();
		while(IS->VirtualBase[(uint64_t)Physical / PAGE_SIZE].Local){out += PAGE_SIZE;}		
	}
	return out;
}
bool MemoryIsMapped(void *Ptr){
	if(Ptr){
		void *Physical = MapPhysical(Ptr);
		InternalAllocationState *IS = GetIState();
		if((__roundup((uint64_t)Physical, PAGE_SIZE) / PAGE_SIZE) > (IS->TotalMemorySize / PAGE_SIZE)){return false;}
		return __check(IS->VirtualBase[__roundup((uint64_t)Physical, PAGE_SIZE) / PAGE_SIZE].Type, PAFUsed);
	}
	return false;
}

bool InitialiseAllocationState(void *D_, uint64_t N, uint64_t TotalMemory){
	if((GetIState())->VirtualBase){return false;}	//	Memory already Initialised
	EFI_MEMORY_DESCRIPTOR *D = (EFI_MEMORY_DESCRIPTOR *)D_;
	//	We need a Memory Region Large enough for us to claim as our own, for Initialisation.
	const UINT64 RequiredBytes = (TotalMemory / PAGE_SIZE) * sizeof(PageAllocationState), 
				Mask = EfiLoaderCode | EfiBootServicesCode | EfiBootServicesData | EfiConventionalMemory;
	register UINT64 MetBytes = RequiredBytes, cc = 0;
	for(; cc < N; ++cc){
		if(__check(Mask, D[cc].Type)){
			for(UINT64 cc_ = cc; cc_ < N; ++cc_){if(__check(Mask, D[cc_].Type)){
				MetBytes = (D[cc_].NumberOfPages * PAGE_SIZE > MetBytes)? 0: MetBytes - (D[cc_].NumberOfPages * PAGE_SIZE);
			}}
		}else{MetBytes = RequiredBytes;}
		if(!MetBytes){break;}
	}
	if(cc == N){return false;}//	A major Failure has occurred, we cannot Initialise any form of Memory Allocation
	volatile InternalAllocationState *IState = GetIState();
	*IState = (InternalAllocationState){.TotalMemorySize = TotalMemory, .VirtualBase = MapVirtual((void *)D[cc].PhysicalStart)};
	memset((void *)IState->VirtualBase, 0x00, RequiredBytes);
	
	//	We now Initialise the Table
	PageAllocationState *Table = (PageAllocationState *)((void *)IState->VirtualBase);
	for(cc = 0; cc < N; ++cc){
		UINT64 Virtual = (uint64_t)MapVirtual((void *)D[cc].PhysicalStart);
		VirtualAddress Temp = *((VirtualAddress *)&Virtual);
		PageAllocationType TType = 0;
		switch(D[cc].Type){
			case EfiRuntimeServicesCode: 
			case EfiRuntimeServicesData: 
			case EfiUnusableMemory: 
			case EfiACPIMemoryNVS: 
			case EfiMemoryMappedIO: 
			case EfiMemoryMappedIOPortSpace: 
				TType = PAFNoFree | PAFUsed | PAFNoTouch;
				break;
			case EfiACPIReclaimMemory: 
			case EfiConventionalMemory: 
			case EfiBootServicesCode: 
			case EfiLoaderCode: 
			case EfiBootServicesData:
				TType = 0x0;
				break; 
			case EfiLoaderData: 
				TType = PAFUsed;
				break;
		}
		// bool Freeable = !flagcheck(NonFreeableMemoryTypeMask, ) && !((IState->VirtualBase <= Virtual) && (IState->VirtualBase + RequiredBytes) > Virtual), 
		// 	Usable = !flagcheck(NonUsableMemoryTypeMask, D[cc].Type), 
		// 	Touchable = !flagcheck(NonTouchableMemoryTypeMask,  D[cc].Type);
		for(UINT64 cc_ = 0; cc_ < D[cc].NumberOfPages; ++cc_){
			Table[(D[cc].PhysicalStart / PAGE_SIZE) + cc_] = (PageAllocationState){
				.Coordinate = {
					.EnableL1 = true, .EnableL2 = true, .EnableL3 = true, 
					.EnableL4 = true, .EnableL5 = IsLA57Supported(), 
					.L1 = Temp.L1, .L2 = Temp.L2, .L3 = Temp.L3, 
					.L4 = Temp.L4, .L5 = Temp.L5
				}, .Type = TType, .Parent = {0}, .Local = (cc_ + 1) - D[cc].NumberOfPages
			};
		}
	}
}

//	Returns Physical Address.
void *QueryFreeMemory(uint32_t _4KB){
	InternalAllocationState *IS = GetIState();
	if((_4KB * PAGE_SIZE) > (IS->TotalMemorySize / PAGE_SIZE)){return NULL;}
	for(uint64_t cc = 0; cc < (IS->TotalMemorySize / PAGE_SIZE); cc++){
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
	InternalAllocationState *IS = GetIState();
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
	*((uint64_t *)__roundup((uint64_t)PTR, Align)) = (uint64_t)PTR;
	return (uint64_t *)__roundup((uint64_t)PTR, Align) + 1;
}
void *AllocatePagesFromRange(void *Base, uint64_t Limit, uint64_t NBytes, PageAllocationFlags Flags, uint8_t ProtectionKey){
	while(MemoryIsMapped(Base) && Base < Limit){Base += PAGE_SIZE;}
	if(Base >= Limit){return NULL;}else{return AllocatePages(Base, NBytes, Flags, ProtectionKey);}
}
void *AllocatePages(void *Base, uint64_t NBytes, PageAllocationFlags Flags, uint8_t ProtectionKey){
	InternalAllocationState *IS = GetIState();
	bool L5;
	PageCoordinate PC = {0};
	UINT64 N = Base? __roundup((uint64_t)Base, PAGE_SIZE) / PAGE_SIZE: (IS->TotalMemorySize / PAGE_SIZE);
	void *PT = GetPageTable(&L5);
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
					if(!(_L5->Present)){
						(*_L5) = (PML5Entry){
							.Present = true, .PhysicalAddress = (uint64_t)QueryFreeMemory(1), 
							.ExecuteDisable = !__check(Flags, ExecuteEnable), 
							.PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), 
							.PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable), 
							.ReadWrite = __check(Flags, ReadWritable), 
							.UserSupervisor = __check(Flags, SupervisorMode), 
							.UsedPage = true, 
						};
						//	We step back to make sure this entry is Processed.
						UpdateAllocationTable(_L5->PhysicalAddress, PAGE_SIZE, &PC, 1, PAFUsed);
						PC.L5--;
					}
				}
				break;
			} case 0x04: {
				PML4Entry *L4 = (PML4Entry *)PT;
				if(!(L4->Present)){
					(*L4) = (PML4Entry){
						.Present = true, .PhysicalAddress = (uint64_t)QueryFreeMemory(1), 
						.ExecuteDisable = !__check(Flags, ExecuteEnable), 
						.PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), 
						.PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable), 
						.ReadWrite = __check(Flags, ReadWritable), 
						.UserSupervisor = __check(Flags, SupervisorMode), 
						.UsedPage = true, 
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
					for(uint32_t cc = 0; cc < __roundup(NBytes, PAGE_SIZE3) / PAGE_SIZE3; ++cc){
						if(__check(IS->VirtualBase[N + cc].Type, PAFNoTouch) || 
							__check(IS->VirtualBase[N + cc].Type, PAFUsed)){Usable = false;		break;}
					}
				}
				for(uint64_t cc = 0x00; (PC.L3 + cc) < PagingTableLength; ++cc){
					if(L3[cc].Present){Touchable = false;	break;}}
				if(Touchable){
					void *BASE = MemoryIsMapped(Base)? Base: QueryFreeMemory(Usable? NBytes / PAGE_SIZE: PAGE_SIZE);
					for(uint32_t cc = 0; (PC.L3 + cc) < PagingTableLength; cc++){
						if(Usable){
							UpdateAllocationTable((uint64_t)BASE + (cc * PAGE_SIZE3), PAGE_SIZE3, &PC, 1, PAFUsed);
							((PDPTEntry1GB *)L3)[cc] = (PDPTEntry1GB){
								.Present = true, .PhysicalAddress = (uint64_t)BASE + (cc * PAGE_SIZE3), 
								.PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable), 
								.PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), 
								.ExecuteDisable = !__check(Flags, ExecuteEnable), 
								.UserSupervisor = __check(Flags, SupervisorMode), 
								.ReadWrite = __check(Flags, ReadWritable), 
								.ProtectionKey = ProtectionKey, 
							};
						}else{
							L3[cc] = (PDPTEntryDirectory){
								.Present = true, .PhysicalAddress = (uint64_t)BASE, 
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
					for(uint32_t cc = 0; cc < __roundup(NBytes, PAGE_SIZE2) / PAGE_SIZE2; ++cc){
						if(__check(IS->VirtualBase[N + cc].Type, PAFNoTouch) || 
							__check(IS->VirtualBase[N + cc].Type, PAFUsed)){Usable = false;		break;}
					}
				}
				for(uint64_t cc = 0x00; (PC.L2 + cc) < PagingTableLength; ++cc){
					if(L2[cc].Present){Touchable = false;	break;}}
				if(Touchable){
					void *BASE = Base? Base: QueryFreeMemory(Usable? NBytes / PAGE_SIZE: PAGE_SIZE);
					for(uint32_t cc = 0; (PC.L2 + cc) < PagingTableLength; cc++){
						if(Usable){
							UpdateAllocationTable((uint64_t)BASE + (cc * PAGE_SIZE2), PAGE_SIZE2, &PC, 1, PAFUsed);
							((PageDirectoryEntry2MB *)L2)[cc] = (PageDirectoryEntry2MB){
								.Present = true, .PhysicalAddress = (uint64_t)BASE + (cc * PAGE_SIZE2), 
								.PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable), 
								.PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), 
								.UserSupervisor = __check(Flags, SupervisorMode), 
								.ExecuteDisable = !__check(Flags, ExecuteEnable), 
								.ReadWrite = __check(Flags, ReadWritable), 
								.ProtectionKey = ProtectionKey, 
							};
						}else{
							L2[cc] = (PageDirectoryEntry){
								.Present = true, .PhysicalAddress = (uint64_t)BASE, 
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
					for(uint32_t cc = 0; cc < __roundup(NBytes, PAGE_SIZE) / PAGE_SIZE; ++cc){
						if(__check(IS->VirtualBase[N + cc].Type, PAFNoTouch) || 
							__check(IS->VirtualBase[N + cc].Type, PAFUsed)){Usable = false;		break;}
					}
				}
				if(Usable){
					for(uint64_t cc = 0x00; (PC.L1 + cc) < PagingTableLength; ++cc){
						if(L1[cc].Present){Usable = false;	break;}}
					void *BASE = Base? Base: QueryFreeMemory(__roundup(NBytes, PAGE_SIZE) / PAGE_SIZE);
					if(Usable){
						for(uint32_t cc = 0; (PC.L1 + cc) < PagingTableLength; cc++){
							UpdateAllocationTable((uint64_t)BASE + (cc * PAGE_SIZE), PAGE_SIZE, &PC, 1, PAFUsed);
							L1[cc] = (PageTableEntry4KB){
								.Present = true, .PhysicalAddress = (uint64_t)BASE + (cc * PAGE_SIZE), 
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
	InternalAllocationState *IS = GetIState();
	if((__roundup((uint64_t)Physical, PAGE_SIZE) / PAGE_SIZE) > (IS->TotalMemorySize / PAGE_SIZE)){return NULL;}
	return (void *)*((uint64_t *)(&(IS->VirtualBase[(uint64_t)Physical / PAGE_SIZE].Coordinate))) + ((uint64_t)Physical % PAGE_SIZE);
}

void FreePages(void *Virtual){
	void *Physical = MapPhysical((void *)__rounddown((uint64_t)Virtual, PAGE_SIZE));
	InternalAllocationState *IS = GetIState();
	uint64_t Bytes = 0x00;
	while(IS->VirtualBase[__roundup((uint64_t)Physical, PAGE_SIZE) / PAGE_SIZE].Local){
		uint32_t lvl = 0x05;
		void *PT = WalkPageTree(&lvl, IS->VirtualBase[__roundup((uint64_t)Physical + Bytes, PAGE_SIZE) / PAGE_SIZE].Coordinate);
		switch(lvl){
			case 3: {
				PDPTEntry1GB *E = PT;
				E->Present = false;
				InvalidatePage(Virtual + Bytes);
				Bytes += PAGE_SIZE3;
				break;
			} case 2: {
				PageDirectoryEntry2MB *E = PT;
				E->Present = false;
				InvalidatePage(Virtual + Bytes);
				Bytes += PAGE_SIZE2;
				break;
			} case 1: {
				PageTableEntry4KB *E = PT;
				E->Present = false;
				InvalidatePage(Virtual + Bytes);
				Bytes += PAGE_SIZE;
				break;
			} 
		}
	}
	UpdateAllocationTable((uint64_t)Physical, Bytes, NULL, 0x00, ~PAFUsed);
}