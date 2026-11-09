#include "malloc.h"
#include "kernel/libcrt/memory/memory.h"

void *ReallocatePages(void *Phys, void *Base, PageAllocationFlags Flags, uint8_t ProtectionKey, uint64_t size){
    void *New = AllocatePages(Phys, size, Flags, ProtectionKey);
    memcpy(New, Base, TotalMappedMemory(Base));
    InvalidatePages(Base, size);
    FreePages(Base);
}

void __noinline FreeAlignedPages(void *Page){FreePages((void *)*(((uint64_t *)Page) - 1));}
void *AllocateAlignedPages(void *Phys, uint64_t size, PageAllocationFlags Flags, uint8_t ProtectionKey, uint8_t Align){
    void *Out = AllocatePages(Phys, size + sizeof(uint64_t) + Align, Flags, ProtectionKey);
	void *Tmp = (void *)__roundupt((uint64_t)Out, Align);
	*((uint64_t *)Tmp) = (uint64_t)Out;
	return Tmp + sizeof(uint64_t);
	
}
void *ReallocateAlignedPages(void *Phys, void *Base, PageAllocationFlags Flags, uint8_t ProtectionKey, uint64_t size, uint8_t Align){
    void *Ptr = ReallocatePages(Phys, Base, Flags, ProtectionKey, __roundupt(size + sizeof(uint64_t), Align));
	*((uint64_t *)__roundup((uint64_t)Ptr, Align)) = (uint64_t)Ptr;
	return (uint64_t *)__roundup((uint64_t)Ptr, Align) + 1;
}

void *SafeAllocatePages(void *Physical, uint32_t Bytes, PageAllocationFlags Flags, uint8_t ProtectionKey, uint8_t Align){
    void *Virtual = NULL;
    uint32_t lvl = 0x05;
    if((Virtual = MapVirtual(Physical)) && WalkPageTreeByPhysical(Physical, &lvl)){return Virtual;}
    return AllocateAlignedPages(Physical, Bytes, Flags, ProtectionKey, Align);
}