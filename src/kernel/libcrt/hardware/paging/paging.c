#include "paging.h"

uint32_t GetMaxPhysicalAddressBits(void){
	uint32_t eax, ebx, ecx, edx;
	// CPUID leaf 0x80000008 returns physical/virtual address size limits
	__cpuid(0x80000008, eax, ebx, ecx, edx);
	return eax & UINT8_MAX; // Bits 0-7 contain MAXPHYADDR (M)
}

void *WalkPageTreeByPhysical(void *Physical, uint32_t *Level){
	void *Virtual = MapVirtual(Physical);
	if(Virtual){return WalkPageTreeByVirtual(Virtual, Level);}
	return NULL;
}

void *WalkPageTreeByVirtual(void *Virtual, uint32_t *Level){
	VirtualAddress VA = *(VirtualAddress *)&Virtual;
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
	bool L5Support = false;
	void *Root = GetPageTable(&L5Support, _StoredPageTableMode, NULL);
	if(!Root){return NULL;}
	return WalkEmulatedPageTree(Coordinate, Root, Level);
}

bool _1GPageSupport(void){
    uint32_t eax, ebx, ecx, edx;

    // First confirm the extended leaf exists
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000000), "c"(0));
    if(eax < 0x80000001){return false;}

    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000001), "c"(0));
    return (edx >> 26) & 1;
}