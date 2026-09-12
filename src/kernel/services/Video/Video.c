#include "controllers/RawRegion.h"
#include "controllers/Text.h"
#include "service.h"

void *__noinline VMAIState(bool w, void *ptr){
    static RawVideoMemoryAllocator internal = {0};
    if(w){internal = *((RawVideoMemoryAllocator *)ptr);}
    return &internal;
}

bool InitaliseVMA(void *videomemory, void *acpibase, uint32_t PixelSize, uint32_t PixelWidth, uint32_t PixelHeight){
    if(!*((RawVideoMemoryAllocator *)VMAIState(false, NULL))->videomemory){return false;}
    VMAIState(true, InitialiseVideoMemoryAllocator(videomemory, acpibase, PixelSize, PixelWidth, PixelHeight));
    return *((RawVideoMemoryAllocator *)VMAIState(false, NULL))->videomemory;
}

static void *vmcontext = NULL;
static uint64_t vmcontextW, vmcontextH, vmcontextPS;
static char stdout[PAGE_SIZE] = {0};
FontGlyph **gcontext = NULL;
uint32_t ngcontext = 0;
void SelectVideoContext(void *vm, void **Glyph, uint32_t NGlyphs){
	RawVideoMemoryAllocator *RVMA = VMAIState(false, NULL);
	for(uint32_t cc = 0; cc < RVMA->TotalAllocations; ++cc){
		if((RVMA->MemoryTags[cc].Base == vm) || (RVMA->MemoryTags[cc].BackBuffer == vm)){
			vmcontextW = RVMA->MemoryTags[cc].PixelWidth;	vmcontextH = RVMA->MemoryTags[cc].PixelHeight;
			vmcontextPS = RVMA->PixelSize;
			break;
		}
	}
	vmcontext = vm;
	gcontext = Glyph;
	ngcontext = NGlyphs;
}

bool VideoPrintf(uint64_t pixel, char *fmt, ...){
	if(!vmcontext || !gcontext || !ngcontext){return false;}
	va_list ls;		va_start(ls, fmt);
	uint64_t streamsize = sizeof(stdout);
	while(*fmt || streamsize){TextCopyF(stdout, &streamsize, &fmt, ls);}
	uint64_t temp = vmcontextW * vmcontextPS;
	PrintTextByGlyph(vmcontext, &temp, gcontext, ngcontext, &pixel, vmcontextPS, stdout);
	vmcontextW = temp / vmcontextPS;
	
}

void *AllocateVideoMemory(uint32_t X, uint32_t Y, uint32_t *W, uint32_t *H){
    if(!*((RawVideoMemoryAllocator *)VMAIState(false, NULL))->videomemory){return NULL;}
    return RequestVideoMemory(VMAIState(false, NULL), X, Y, W, H);
}

bool FreeVideoMemory(void *VM){
    if(!*((RawVideoMemoryAllocator *)VMAIState(false, NULL))->videomemory){return NULL;}
    ReleaseVideoMemory(VMAIState(false, NULL), VM);
    return true;
}

bool PreflushVideoMemory(void *VM, CommonMutex Mtx){
    if(!*((RawVideoMemoryAllocator *)VMAIState(false, NULL))->videomemory){return NULL;}
    FlushVideoMemory(VMAIState(false, NULL), VM, Mtx);
    return true;
}