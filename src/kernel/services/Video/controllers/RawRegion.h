#pragma once

#include "kernel/libcrt/def.h"
#include "kernel/libcrt/services.h"
#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/hardware/IDT/ISR.h"
#include "kernel/libcrt/hardware/GDT/GDT.h"
#include "kernel/libcrt/memory/string.h"
#include "kernel/libcrt/hardware/PCIe/PCIe.h"
#include "kernel/libcrt/hardware/PCIe/Devices.h"
#include "kernel/libcrt/memory/allocator/malloc.h"

#include "kernel/libcrt/hardware/IDT/APIC/LocalAPICTimer.h"
typedef uint32_t Pixel;
typedef struct{
	CommonMutex Mtx;
	//  This Buffer is what is written to by the User.
	//  We repoint the Buffer to the Back Buffer in the Page Table.
	uint32_t	*Base, 
				*BackBuffer;
	uint32_t	PixelWidth, PixelHeight, 
				PixelX, PixelY;
	//  In ms.
	uint8_t		Ready;
}__packed VmaTag;

typedef struct{
	void *LocalAPICBase;
	uint32_t	*videomemory;
	uint64_t	PixelSize, PixelWidth, PixelHeight;
	VmaTag		*MemoryTags;
	uint32_t	TotalAllocations;
}__packed RawVideoMemoryAllocator;

LocalAPICTimerCallbackDefinition(VMAMemoryPoll){
	uint64_t TagPoll = (uint64_t)call;
	RawVideoMemoryAllocator *RVMA = persistent;
	if(TagPoll > RVMA->TotalAllocations){TagPoll = 0x00;}

	//  We flush a Tag per Poll.
	VmaTag *Tag = RVMA->MemoryTags + TagPoll;
	if(!Tag->Ready){return (void *)(TagPoll++);}
	UnlockMutex(Tag->Mtx);
		
	//  Swap the Buffers.
	void *PBase = MapPhysical(Tag->Base), 
		*PBack = MapPhysical(Tag->BackBuffer);
	RepointVirtualAddress(Tag->Base, PBack);
	RepointVirtualAddress(Tag->BackBuffer, PBase);

	//  Blit Buffer
	for(uint32_t y = 0; y < __min(RVMA->PixelHeight, Tag->PixelHeight); y++){
		memcpy(RVMA->videomemory + (y * RVMA->PixelWidth) + Tag->PixelX, 
			Tag->BackBuffer, __min(RVMA->PixelWidth, Tag->PixelWidth) * sizeof(Pixel));
	}
	LockMutex(Tag->Mtx);
	return (void *)(TagPoll++);
}

//  RequestVideoMemory
//  ReleaseVideoMemory
//  FlushVideoMemory
RawVideoMemoryAllocator *InitialiseVideoMemoryAllocator(void *videomemory, 
	void *acpibase, uint32_t PixelSize, uint32_t PixelWidth, uint32_t PixelHeight
){
	RawVideoMemoryAllocator *RVMA = mcalloc(1, sizeof(RawVideoMemoryAllocator));
	*RVMA = (RawVideoMemoryAllocator){
		.videomemory = SafeAllocatePages(videomemory, 
			sizeof(Pixel) * PixelWidth * PixelHeight, (ReadWritable | SupervisorMode), 0x00, 0x20), 
		.MemoryTags = NULL, .LocalAPICBase = GetLocalAPICBase(acpibase, NULL), 
		.PixelSize = PixelSize, .PixelHeight = PixelHeight, 
		.PixelWidth = PixelWidth, .TotalAllocations = 0x00, 
	};
	uint8_t IVector, IST;
	IDTEntrySegmentSelector64 sselector;
	if(AllocateInterruptVector(&IVector) && AllocateIST(&IST, &sselector)){
		AddTimerEvent(IVector, 0x01, 0x00, 0x00, (LocalAPICTimerCallback *)&VMAMemoryPoll, RVMA, NULL);
	}else{mfree(RVMA);  RVMA = NULL;}
	return RVMA;
}

void *RequestVideoMemory(RawVideoMemoryAllocator *HND, 
	uint32_t X, uint32_t Y, uint32_t *W, uint32_t *H
){
	if(!(W || H)){return NULL;}
	uint32_t _W = __min(HND->PixelWidth - X, *W), 
			_H = __min(HND->PixelHeight - Y, *H);
		if(!(_W || _H)){return NULL;}
	DisableLocalAPIC(HND->LocalAPICBase);
	HND->MemoryTags = mrealloc(HND->MemoryTags, sizeof(VmaTag) * (HND->TotalAllocations++));
	HND->MemoryTags[HND->TotalAllocations - 1] = (VmaTag){
		.BackBuffer = mcalloc((*W) * (*H), sizeof(Pixel)), 
		.Base = mcalloc((*W) * (*H), sizeof(Pixel)), 
		.PixelHeight = _W, .PixelWidth = _H, 
		.PixelX = X, .PixelY = Y
	};
	EnableLocalAPIC(HND->LocalAPICBase);
	return HND->MemoryTags[HND->TotalAllocations - 1].Base;
}

void FlushVideoMemory(RawVideoMemoryAllocator *HND, void *VM, CommonMutex Mtx){
	DisableLocalAPIC(HND->LocalAPICBase);
	for(uint32_t cc= 0 ; cc < HND->TotalAllocations; ++cc){
		if(HND->MemoryTags[cc].Base == VM){
			HND->MemoryTags[cc].Ready = true;
			HND->MemoryTags[cc].Mtx = Mtx;
			break;
		}
	}
	EnableLocalAPIC(HND->LocalAPICBase);
	return;
}

void ReleaseVideoMemory(RawVideoMemoryAllocator *HND, void *VM){
	DisableLocalAPIC(HND->LocalAPICBase);
	for(uint32_t cc= 0 ; cc < HND->TotalAllocations; ++cc){
		if(HND->MemoryTags[cc].Base == VM){
			mfree(HND->MemoryTags[cc].Base);
			mfree(HND->MemoryTags[cc].BackBuffer);
			memcpy(HND->MemoryTags + cc, HND->MemoryTags + cc + 1, 
				sizeof(VmaTag) * (HND->TotalAllocations - (cc + 1)));
			HND->TotalAllocations--;
			break;
		}
	}
	EnableLocalAPIC(HND->LocalAPICBase);
	return;
}