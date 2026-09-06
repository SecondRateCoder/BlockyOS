#include "controllers/RawRegion.h"
#include "controllers/Text.h"
#include "service.h"

RawVideoMemoryAllocator *IState(bool w, RawVideoMemoryAllocator *ptr){
    static RawVideoMemoryAllocator internal = {0};
    if(w){internal = *ptr;}
    return &internal;
}

bool InitaliseVMA(void *videomemory, void *acpibase, uint32_t PixelSize, uint32_t PixelWidth, uint32_t PixelHeight){
    if(!*(IState(false, NULL))->videomemory){return false;}
    IState(true, InitialiseVideoMemoryAllocator(videomemory, acpibase, PixelSize, PixelWidth, PixelHeight));
    return *(IState(false, NULL))->videomemory;
}

void *AllocateVideoMemory(uint32_t X, uint32_t Y, uint32_t *W, uint32_t *H){
    if(!*(IState(false, NULL))->videomemory){return NULL;}
    return RequestVideoMemory(IState(false, NULL), X, Y, W, H);
}

bool FreeVideoMemory(void *VM){
    if(!*(IState(false, NULL))->videomemory){return NULL;}
    ReleaseVideoMemory(IState(false, NULL), VM);
    return true;
}

bool PreflushVideoMemory(void *VM, CommonMutex Mtx){
    if(!*(IState(false, NULL))->videomemory){return NULL;}
    FlushVideoMemory(IState(false, NULL), VM, Mtx);
    return true;
}