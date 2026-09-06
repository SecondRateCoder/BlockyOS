#include "GDT.h"
#include "kernel/libcrt/hardware/paging/paging.h"
#include "kernel/libcrt/memory/allocator/malloc.h"

extern bool __sysvabi _LoadGDTR(GDTR64 *Register);
extern bool __sysvabi _ReadGDTR(GDTR64 *Register);

//	We assume a Virtual Base was used, we replace with a Physical Base.
//	We dont trust the Input Pointer and 
bool LoadGDTR(GDTR64 *Register){
	void *PagedPTR = AllocatePages(NULL, Register->Limit, 
		(ReadWritable | SupervisorMode | PageLevelCacheEnable | GlobalEnable), 0x0);
	memcpy(PagedPTR, (void *)Register->Base, Register->Limit);
	Register->Base = (uint64_t)MapPhysical(PagedPTR);
	return _LoadGDTR(Register);
}

//	We assume a Virtual Base was used, we replace with a Physical Base.
//	We dont trust the Input Pointer and 
bool ReadGDTR(GDTR64 *Register){
	if(_ReadGDTR(Register)){
		//	Patch the Physical Address with a Virtual one
		Register->Base = (uint64_t)MapVirtual((void *)Register->Base);
		return true;
	}
	return false;
}

extern bool __sysvabi _LoadLDTR(LDTR64 *Register);
extern bool __sysvabi _ReadLDTR(LDTR64 *Register);

//	We assume a Virtual Base was used, we replace with a Physical Base.
//	We dont trust the Input Pointer and 
bool LoadLDTR(LDTR64 *Register){
	void *PagedPTR = AllocatePages(NULL, Register->Limit, 
		(ReadWritable | SupervisorMode | PageLevelCacheEnable | GlobalEnable), 0x0);
	memcpy(PagedPTR, (void *)Register->Base, Register->Limit);
	Register->Base = (uint64_t)MapPhysical(PagedPTR);
	return _LoadLDTR(Register);
}

//	We assume a Virtual Base was used, we replace with a Physical Base.
//	We dont trust the Input Pointer and 
bool ReadLDTR(LDTR64 *Register){
	if(_ReadLDTR(Register)){
		//	Patch the Physical Address with a Virtual one
		Register->Base = (uint64_t)MapVirtual((void *)Register->Base);
		return true;
	}
	return false;
}

bool AllocateSystemDescriptor(void *PhysicalBase, uint32_t Limit, uint8_t Priviledge, GDSSType64 Type, GDSSFlags flags){
	GDTR64 R;
	if(ReadGDTR(&R)){
		GDTSystemSegmentDescriptor64 *Table = (GDTSystemSegmentDescriptor64 *)R.Base;
		for(uint32_t cc = 0; cc < (R.Limit / sizeof(GDTSystemSegmentDescriptor64)); ++cc){
			if(!Table[cc].ABPresent){
				Table[cc] = (GDTSystemSegmentDescriptor64){
					.ABPresent = true, .ABPriviledgeLevel = Priviledge, 
					.ABSystemSegmentBit = true, .ABType = Type, 
					.F32BitModeBit = __check(flags, _32BitSystemSegment), 
					.FGranularity = __check(flags, Granularity4KB) && !__check(flags, Granularity1Byte), 
					.FLongModeBit = __check(flags, _LongModeSystemSegment), 
					.LimitHigh = (Limit >> 16) & 0xF, .LimitLow = Limit & 0xFFFF, 
					.LinearBaseHigh = ((uint64_t)PhysicalBase >> 24) & 0xFFFFFFFFFF, 
					.LinearBaseLow = (uint64_t)PhysicalBase & 0xFFFFFF
				};
				return true;
			}
		}
		if(UINT16_MAX >= ((R.Limit + 1) * sizeof(GDTSystemSegmentDescriptor64))){
			Table[R.Limit / sizeof(GDTSystemSegmentDescriptor64)] = (GDTSystemSegmentDescriptor64){
				.ABPresent = true, .ABPriviledgeLevel = Priviledge, 
				.ABSystemSegmentBit = true, .ABType = Type, 
				.F32BitModeBit = __check(flags, _32BitSystemSegment), 
				.FGranularity = __check(flags, Granularity4KB) && !__check(flags, Granularity1Byte), 
				.FLongModeBit = __check(flags, _LongModeSystemSegment), 
				.LimitHigh = (Limit >> 16) & 0xF, .LimitLow = Limit & 0xFFFF, 
				.LinearBaseHigh = ((uint64_t)PhysicalBase >> 24) & 0xFFFFFFFFFF, 
				.LinearBaseLow = (uint64_t)PhysicalBase & 0xFFFFFF
			};
			R.Limit += sizeof(GDTSystemSegmentDescriptor64);
			LoadGDTR(&R);
			return true;
		}
	}
	return false;
}