#include "paging.h"

uint32_t GetMaxPhysicalAddressBits(void){
	uint32_t eax, ebx, ecx, edx;
	// CPUID leaf 0x80000008 returns physical/virtual address size limits
	__cpuid(0x80000008, eax, ebx, ecx, edx);
	return eax & UINT8_MAX; // Bits 0-7 contain MAXPHYADDR (M)
}

void *MapPhysical(void *Virtual){
	bool Level5;
	void *PT = GetPageTable(&Level5, _StoredPageTableMode, NULL);
	PML4Table *L4 = (PML4Table *)MapVirtual(PT);
	PDPTTable *L3;
	PageDirectory *L2;
	PageTable *L1;
	VirtualAddress Virt = *((VirtualAddress *)&Virtual);
	if(Level5){
		PML5Table *L5 = (PML5Table *)MapVirtual(PT);
		if(!((*L5)[Virt.L5].Present)){return NULL;}
		L4 = (PML4Table *)MapVirtual((void *)((*L5)[Virt.L5].PhysicalAddress << 12));
	}

	if(!((*L4)[Virt.L4].Present)){return NULL;}else{
		L3 = (PDPTTable *)MapVirtual((void *)((*L4)[Virt.L4].PhysicalAddress << 12));
	}
	if(!(L3->directories[Virt.L3].Present)){return NULL;}else{
		if(L3->directories[Virt.L3].PageSize){
			// 1 GB pages are aligned to 1 GB, so the frame address must be shifted by 30 bits.
			return (void *)((L3->pages1GB[Virt.L3].PhysicalAddress << 30) + ((uint64_t)Virtual & 0x3FFFFFFF));
		}else{L2 = (PageDirectory *)MapVirtual((void *)(L3->directories[Virt.L3].PhysicalAddress << 12));}
	}

	if(!(L2->tables[Virt.L2].Present)){return NULL;}else{
		if(L2->tables[Virt.L2].PageSize){
			// 2 MB pages are aligned to 2 MB, so the frame address must be shifted by 21 bits.
			return (void *)((L2->pages2MB[Virt.L2].PhysicalAddress << 21) + ((uint64_t)Virtual & 0x1FFFFF));
		}else{L1 = (PageTable *)MapVirtual((void *)(L2->tables[Virt.L2].PhysicalAddress << 12));}
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
	PageCoordinate Temp = InitPageCoordinate(VA);
	Temp.EnableL5 = (*Level == 5);
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
	void *Temp = GetPageTable(&L5Support, _StoredPageTableMode, NULL);
	while(*Level){
		switch(*Level){
			case 5: {
				if(!(IsLA57Enabled())){break;}
				if(!Coordinate.EnableL5){return Temp;}else{
					PML5Table *L5 = Temp;
					if(L5Support){
						if(Coordinate.L5 >= PagingTableLength){return NULL;}
						Temp = (void *)MapVirtual((void *)(L5[Coordinate.L5]->PhysicalAddress << 12));
					}
				}
				break;
			} case 4: {
				if(!Coordinate.EnableL4){return Temp;}else{
					PML4Table *L4 = Temp;
					if(Coordinate.L4 >= PagingTableLength){return NULL;}
					Temp = (void *)MapVirtual((void *)(L4[Coordinate.L4]->PhysicalAddress << 12));
				}
				break;
			} case 3: {
				if(!Coordinate.EnableL3){return Temp;}else{
					PDPTTable *L3 = Temp;
					if(Coordinate.L3 >= PagingTableLength){return NULL;}
					if(L3->directories[Coordinate.L3].PageSize){return L3->directories + Coordinate.L3;}
					Temp = (void *)MapVirtual((void *)(L3->directories[Coordinate.L3].PhysicalAddress << 12));
				}
				break;
			} case 2: {
				if(!Coordinate.EnableL2){return Temp;}else{
					PageDirectory *L2 = Temp;
					if(Coordinate.L2 >= PagingTableLength){return NULL;}
					if(L2->tables[Coordinate.L2].PageSize){return L2->tables + Coordinate.L2;}
					Temp = (void *)MapVirtual((void *)(L2->tables[Coordinate.L2].PhysicalAddress << 12));
				}
				break;
			} case 1: {
				if(!Coordinate.EnableL1){return Temp;}else{
					PageTable *L1 = Temp;
					if(Coordinate.L1 >= PagingTableLength){return NULL;}
					return MapVirtual(L1 + Coordinate.L1);
				}
			} default: {return NULL;}
		}
		(*Level)--;
	}
}