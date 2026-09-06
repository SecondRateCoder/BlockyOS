#include "paging.h"

uint32_t GetMaxPhysicalAddressBits(void){
	uint32_t eax, ebx, ecx, edx;
	// CPUID leaf 0x80000008 returns physical/virtual address size limits
	__cpuid(0x80000008, eax, ebx, ecx, edx);
	return eax & UINT8_MAX; // Bits 0-7 contain MAXPHYADDR (M)
}

void *MapPhysical(void *Virtual){
	bool Level5;
	void *PT = GetPageTable(&Level5);
	PML4Table *L4 = (PML4Table *)PHYS_TO_VIRT(PT);
	PDPTTable *L3;
	PageDirectory *L2;
	PageTable *L1;
	VirtualAddress Virt = *((VirtualAddress *)&Virtual);
	if(Level5){
		PML5Table *L5 = (PML5Table *)PHYS_TO_VIRT(PT);
		if(!((*L5)[Virt.L5].Present)){return NULL;}
		L4 = (PML4Table *)PHYS_TO_VIRT((*L5)[Virt.L5].PhysicalAddress << 12);
	}

	if(!((*L4)[Virt.L4].Present)){return NULL;}else{
		L3 = (PDPTTable *)PHYS_TO_VIRT((*L4)[Virt.L4].PhysicalAddress << 12);
	}
	if(!(L3->directories[Virt.L3].Present)){return NULL;}else{
		if(L3->directories[Virt.L3].PageSize){
			// 1 GB pages are aligned to 1 GB, so the frame address must be shifted by 30 bits.
			return (void *)((L3->pages1GB[Virt.L3].PhysicalAddress << 30) + ((uint64_t)Virtual & 0x3FFFFFFF));
		}else{L2 = (PageDirectory *)PHYS_TO_VIRT(L3->directories[Virt.L3].PhysicalAddress << 12);}
	}

	if(!(L2->tables[Virt.L2].Present)){return NULL;}else{
		if(L2->tables[Virt.L2].PageSize){
			// 2 MB pages are aligned to 2 MB, so the frame address must be shifted by 21 bits.
			return (void *)((L2->pages2MB[Virt.L2].PhysicalAddress << 21) + ((uint64_t)Virtual & 0x1FFFFF));
		}else{L1 = (PageTable *)PHYS_TO_VIRT(L2->tables[Virt.L2].PhysicalAddress << 12);}
	}

	//	12 bits(1 byte and 4 bits)
	if(!((*L1)[Virt.L1].Present)){return NULL;}
	return (void *)(((*L1)[Virt.L1].PhysicalAddress << 12) + ((uint64_t)Virtual & 0xFFF));
}

void *WalkPageTreeByPhysical(void *Physical, uint32_t *Level){
	void *Virtual = MapVirtual(Physical);
	if(Virtual){return WalkPageTreeByVirtual(Virtual, Level);}
	return NULL;
}

void *WalkPageTreeByVirtual(void *Virtual, uint32_t *Level){
	VirtualAddress VA = *((VirtualAddress *)Virtual);
	(*Level) = IsLA57Enabled()? 5: 4;
	PageCoordinate Temp = {
		.EnableL1 = true, .EnableL2 = true, .EnableL3 = true, 
		.EnableL4 = true, .EnableL5 = *Level == 5, 
		.L1 = VA.L1, .L2 = VA.L2, .L3 = VA.L3, 
		.L4 = VA.L4, .L5 = VA.L5
	};
	return WalkPageTree(Level, Temp);
}

void InvalidatePages(void *Virtual, uint64_t Bytes){
	Bytes = __roundup(Bytes, PAGE_SIZE) / PAGE_SIZE;
	for(uint32_t cc = 0; cc < Bytes; cc++){InvalidatePage(Virtual + (cc * PAGE_SIZE));}
	return;
}
void InvalidatePage(void *Virtual){
    __asm__ __volatile__ ("invlpg %0" : : "m" (*(char *)Virtual) : "memory");
	return;
}

void *WalkPageTree(uint32_t *Level, PageCoordinate Coordinate){
	bool L5Support;
	void *Temp = GetPageTable(&L5Support);
	while(*Level){
		switch(*Level){
			case 5: {
				if(!Coordinate.EnableL5){return Temp;}else{
					PML5Table *L5 = Temp;
					if(L5Support){
						if(Coordinate.L5 > PagingTableLength){return NULL;}
						Temp = (void *)MapVirtual((void *)(L5[Coordinate.L5]->PhysicalAddress << 12));
					}
				}
				break;
			} case 4: {
				if(!Coordinate.EnableL4){return Temp;}else{
					PML4Table *L4 = Temp;
					if(Coordinate.L4 > PagingTableLength){return NULL;}
					Temp = (void *)MapVirtual((void *)(L4[Coordinate.L4]->PhysicalAddress << 12));
				}
				break;
			} case 3: {
				if(!Coordinate.EnableL3){return Temp;}else{
					PDPTTable *L3 = Temp;
					if(Coordinate.L3 > PagingTableLength){return NULL;}
					if(L3->directories[Coordinate.L3].PageSize){return MapVirtual(L3->directories + Coordinate.L4);}
					Temp = (void *)MapVirtual((void *)(L3->directories[Coordinate.L3].PhysicalAddress << 12));
				}
				break;
			} case 2: {
				if(!Coordinate.EnableL2){return Temp;}else{
					PageDirectory *L2 = Temp;
					if(Coordinate.L2 > PagingTableLength){return NULL;}
					if(L2->tables[Coordinate.L2].PageSize){return MapVirtual(L2->tables + Coordinate.L2);}
					Temp = (void *)MapVirtual((void *)(L2->tables[Coordinate.L2].PhysicalAddress << 12));
				}
				break;
			} case 1: {
				if(!Coordinate.EnableL1){return Temp;}else{
					PageTable *L1 = Temp;
					if(Coordinate.L1 > PagingTableLength){return NULL;}
					return MapVirtual(L1 + Coordinate.L1);
				}
				break;
			} default: {return NULL;}
		}
		(*Level)--;
	}
}

bool RepointVirtualAddress(void *Virtual, void *NewPhysical){
    uint32_t level = 1;
    // WalkPageTreeByVirtual returns a pointer to the L1 page table entry when level is set/reached, 
    // or we can locate it using the existing page walk utilities.
    // Let's use the coordinate walker to find the exact L1 entry:
    VirtualAddress VA = *((VirtualAddress *)&Virtual);
    uint32_t walkLevel = IsLA57Enabled() ? 5 : 4;
    
    PageCoordinate coord = {
        .EnableL1 = true, .EnableL2 = true, .EnableL3 = true, 
        .EnableL4 = true, .EnableL5 = walkLevel == 5, 
        .L1 = VA.L1, .L2 = VA.L2, .L3 = VA.L3, 
        .L4 = VA.L4, .L5 = VA.L5
    };

    PageTableEntry4KB *entry = (PageTableEntry4KB *)WalkPageTree(&walkLevel, coord);
	//	Virtual address is not mapped or entry is invalid
    if(!entry || !entry->Present){return false;}

    // Update the physical address frame (shifting right by 12 bits)
    entry->PhysicalAddress = (uint64_t)NewPhysical >> 12;

    // Invalidate the TLB for this virtual address so the CPU reloads the translation
    InvalidatePage(Virtual);

    return true;
}

//!	AI
static bool ReverseWalk(uint64_t TargetFrame, uint64_t CurrentVirt, uint64_t *FoundVirt){
	uint32_t Level = 0;
	void *TableVirt = NULL;	{
		TableVirt = GetPageTable((bool *)&Level);
		Level = Level? 5: 4;
	};
	for(uint32_t i = 0; i < PagingTableLength; i++){
		uint64_t BranchVirt = CurrentVirt | ((uint64_t)i << (12 + (Level - 1) * 9));
		switch(Level){
			//	4 KB Leaf Entries
			case 1: {
				PageTable *l1 = (PageTable *)TableVirt;
				if((*l1)[i].Present){
					if((*l1)[i].PhysicalAddress == TargetFrame){
						*FoundVirt = BranchVirt;
						return true;
					}
				}
				continue;
			} case 2: {
				PageDirectory *l2 = (PageDirectory *)TableVirt;
				if(l2->tables[i].Present){
					if(l2->tables[i].PageSize){
						//	2 MB Leaf Page: Target frame is physical_address >> 21
						if(l2->pages2MB[i].PhysicalAddress == (TargetFrame >> 9)){
							*FoundVirt = BranchVirt;
							return true;
						}
					}else if(ReverseWalk(TargetFrame, BranchVirt, FoundVirt)){return true;}
				}
				continue;
			} case 3: {
				PDPTTable *l3 = (PDPTTable *)TableVirt;
				if(l3->directories[i].Present){
					if(l3->directories[i].PageSize){
						//	1 GB Leaf Page: Target frame is physical_address >> 30
						if(l3->pages1GB[i].PhysicalAddress == (TargetFrame >> 18)){
							*FoundVirt = BranchVirt;
							return true;
						}
					}else if(ReverseWalk(TargetFrame, BranchVirt, FoundVirt)){return true;}
				}
				continue;
			} case 4: {
				PML4Table *l4 = (PML4Table *)TableVirt;
				if((*l4)[i].Present && ReverseWalk(TargetFrame, BranchVirt, FoundVirt)){return true;}
				continue;
			}
		}

		//	PML4
		if(Level == 4){
		}

		//	PML5
		if(Level == 5){
			PML5Table *l5 = (PML5Table *)TableVirt;
			if(!(*l5)[i].Present){continue;}
			void *childPhys = (void *)((*l5)[i].PhysicalAddress << 12);
			if(ReverseWalk(TargetFrame, BranchVirt, FoundVirt)){return true;}
			continue;
		}
	}
	return false;
}
void *__sysvabi MapPTVirtual(void *Physical){
	bool Level5;
	void *RootPhys = GetPageTable(&Level5);
	uint64_t TargetFrame = (uint64_t)Physical >> 12, 
			Offset = (uint64_t)Physical & 0xFFF, 
			FoundVirt = 0;
	//	We need to enumerate
	if(ReverseWalk(TargetFrame, 0, &FoundVirt)){
		// Sign-extend high canonical bits for kernel addresses
		if(!Level5 && (FoundVirt & (1ULL << 47))){FoundVirt |= 0xFFFF000000000000ULL;}else
		if(Level5 && (FoundVirt & (1ULL << 56))){FoundVirt |= 0xFF00000000000000ULL;}
		return (void *)(FoundVirt + Offset);
	}

	return NULL; // Physical address is unmapped
}

typedef struct{
	uint64_t FoundPhysAddr;
	uint64_t MaxPhysAddr;
	uint64_t CurrentBlockStart;
	uint64_t CurrentBlockSize;
	uint64_t TargetSize;
	bool FoundGap;
}SearchContext;

// Helper to process mapped physical ranges and track contiguous spans / max address
static void ProcessPhysicalRange(SearchContext *ctx, uint64_t physStart, uint64_t rangeSize) {
	const uint64_t physEnd = physStart + rangeSize;

	// Track highest physical address seen in the page table
	if(physEnd > ctx->MaxPhysAddr){ctx->MaxPhysAddr = physEnd;}

	if(ctx->FoundGap){return;}

	// Check if this range continues the current contiguous block
	if(ctx->CurrentBlockSize == 0){
		ctx->CurrentBlockStart = physStart;
		ctx->CurrentBlockSize = rangeSize;
	}else if(physStart == ctx->CurrentBlockStart + ctx->CurrentBlockSize){
		ctx->CurrentBlockSize += rangeSize;
	}else{
		// Discontinuity found: reset tracker to new range
		ctx->CurrentBlockStart = physStart;
		ctx->CurrentBlockSize = rangeSize;
	}

	// Check if we have accumulated enough contiguous physical memory
	if(ctx->CurrentBlockSize >= ctx->TargetSize){
		ctx->FoundPhysAddr = ctx->CurrentBlockStart;
		ctx->FoundGap = true;
	}
}

// Recursive walker (read-only inspection)
static void WalkTreeReadOnly(void *TableVirt, int Level, SearchContext *ctx){
	if(!TableVirt || ctx->FoundGap){return;}
	for(int i = 0; i < PagingTableLength; i++){
		if(ctx->FoundGap){break;}
		switch(Level){
			case 5: {
				PML5Table *l5 = (PML5Table *)TableVirt;
				if((*l5)[i].Present){WalkTreeReadOnly(PHYS_TO_VIRT((uint64_t)(*l5)[i].PhysicalAddress << 12), 4, ctx);}
				continue;
			} case 4: {
				PML4Table *l4 = (PML4Table *)TableVirt;
				if((*l4)[i].Present){WalkTreeReadOnly(PHYS_TO_VIRT((uint64_t)(*l4)[i].PhysicalAddress << 12), 3, ctx);}
				continue;
			} case 3: {
				PDPTTable *l3 = (PDPTTable *)TableVirt;
				if(l3->directories[i].Present){
					if(l3->directories[i].PageSize){ // 1 GB Huge Page Leaf
						ProcessPhysicalRange(ctx, (uint64_t)l3->pages1GB[i].PhysicalAddress << 30, PAGE_SIZE);
					}else{WalkTreeReadOnly(PHYS_TO_VIRT((uint64_t)l3->directories[i].PhysicalAddress << 12), 2, ctx);}
				}
				continue;
			} case 2: {
				PageDirectory *l2 = (PageDirectory *)TableVirt;
				if(l2->tables[i].Present){
					if(l2->tables[i].PageSize){ // 2 MB Large Page Leaf
						ProcessPhysicalRange(ctx, (uint64_t)l2->pages2MB[i].PhysicalAddress << 21, PAGE_SIZE2);
					}else{WalkTreeReadOnly(PHYS_TO_VIRT((uint64_t)l2->tables[i].PhysicalAddress << 12), 1, ctx);}
				}
				continue;
			} case 1: {
				PageTable *l1 = (PageTable *)TableVirt;
				if((*l1)[i].Present){ProcessPhysicalRange(ctx, (uint64_t)(*l1)[i].PhysicalAddress << 12, PAGE_SIZE3);}
				continue;
			}
		}
	}
}