#include "malloc.h"
#include "kernel/libcrt/memory/memory.h"

void *ReallocatePages(void *Phys, void *Base, PageAllocationFlags Flags, uint8_t ProtectionKey, uint64_t size){
    void *New = AllocatePages(Phys, size, (ReadWritable | SupervisorMode), ProtectionKey);
    memcpy(New, Base, TotalMappedMemory(Base));
    uint32_t lvl = 0x05;
    void *pgt = WalkPageTreeByVirtual(New, &lvl), 
        *NPhys = MapPhysical(New);
    //  Update the Page Table Flags.
    switch(lvl){
        case 0x03: {
            for(uint32_t cc = 0; cc < (size / PAGE_SIZE3); ++cc){
                ((PDPTEntry1GB *)pgt)[cc] = (PDPTEntry1GB){
                    .Present = true, .ProtectionKey = ProtectionKey, 
                    .PhysicalAddress = (uint64_t)NPhys + (cc * PAGE_SIZE3), 
                    .PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable), 
                    .PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), 
                    .ExecuteDisable = !__check(Flags, ExecuteEnable), 
                    .UserSupervisor = __check(Flags, SupervisorMode), 
                    .ReadWrite = __check(Flags, ReadWritable), 
                };
            }
            break;
        } case 0x02: {
            for(uint32_t cc = 0; cc < (size / PAGE_SIZE2); ++cc){
                ((PageDirectoryEntry2MB *)pgt)[cc] = (PageDirectoryEntry2MB){
                    .Present = true, .ProtectionKey = ProtectionKey, 
                    .PhysicalAddress = (uint64_t)NPhys + (cc * PAGE_SIZE3), 
                    .PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable), 
                    .PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), 
                    .ExecuteDisable = !__check(Flags, ExecuteEnable), 
                    .UserSupervisor = __check(Flags, SupervisorMode), 
                    .ReadWrite = __check(Flags, ReadWritable), 
                };
            }
            break;
        } case 0x01: {
            for(uint32_t cc = 0; cc < (size / PAGE_SIZE); ++cc){
                ((PageTableEntry4KB *)pgt)[cc] = (PageTableEntry4KB){
                    .Present = true, .ProtectionKey = ProtectionKey, 
                    .PhysicalAddress = (uint64_t)NPhys + (cc * PAGE_SIZE3), 
                    .PageLevelWriteThrough = __check(Flags, PageLevelWriteThroughEnable), 
                    .PageLevelCacheDisable = !__check(Flags, PageLevelCacheEnable), 
                    .ExecuteDisable = !__check(Flags, ExecuteEnable), 
                    .UserSupervisor = __check(Flags, SupervisorMode), 
                    .ReadWrite = __check(Flags, ReadWritable), 
                };
            }
            break;
        }
    }
    InvalidatePages(New, size);
    FreePages(Base);
}

void __noinline __visibilitydefault FreeAlignedPages(void *Page){FreePages((void *)*(((uint64_t *)Page) - 1));}
void *AllocateAlignedPages(void *Phys, uint64_t size, PageAllocationFlags Flags, uint8_t ProtectionKey, uint8_t Align){
    return AllocateAlignedPagesFromRange(Phys, UINT64_MAX, size, Flags, ProtectionKey, Align);
}

void *ReallocateAlignedPages(void *Phys, void *ptr, PageAllocationFlags Flags, uint8_t ProtectionKey, uint64_t size, uint8_t Align){
    void *PTR = ReallocatePages(Phys, ptr, Flags, ProtectionKey, __roundup(size + sizeof(uint64_t), Align));
	*((uint64_t *)__roundup((uint64_t)PTR, Align)) = (uint64_t)PTR;
	return (uint64_t *)__roundup((uint64_t)PTR, Align) + 1;
}

void *SafeAllocatePages(void *Physical, uint32_t Bytes, PageAllocationFlags Flags, uint8_t ProtectionKey, uint8_t Align){
    void *Virtual = NULL;
    uint32_t lvl = 0x05;
    if((Virtual = MapVirtual(Physical)) && WalkPageTreeByPhysical(Physical, &lvl)){return Virtual;}
    return AllocateAlignedPages(Physical, Bytes, Flags, ProtectionKey, Align);
}