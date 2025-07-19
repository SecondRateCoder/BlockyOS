#include <./src/Core/General.h>
#include "./src/Devices/PLoader/Programs.h"
#include "./src/Core/RAM/memtypes.h"


#define pthread Thread
typedef struct Thread{
    hcontext_t *context;
    uint8_t *program_counter;
    int priority;
}Thread;

pthread spawn_thread(void *function){
    const size_t size = memsize(function), addr = (size_t)alloca(size, KERNEL_ID), haddr = get_hcontextaddr(addr);
    hcontext_attrwrite_unsafe(&true, haddr, HContextPeekerAttr_IsThread);
    memcpy_unsafe(RAM, addr, (uint8_t *)function, size);
    return (pthread){
        .context = (hcontext_t *)haddr,
        .program_counter = 0,
        .priority = 0
    };
}

pthread spawn_kthread(void *kfunc){
    return spawn_thread(kfunc);
}