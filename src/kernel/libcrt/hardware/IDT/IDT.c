#include "IDT.h"
#include "kernel/libcrt/memory/memory.h"
#include "kernel/libcrt/hardware/paging/paging.h"
#include "kernel/libcrt/memory/allocator/malloc.h"

bool __sysvabi LoadIDTR(IDTR64 *Register){
	void *PagedPTR = AllocatePages(NULL, Register->Limit, 
		(ReadWritable | SupervisorMode | PageLevelCacheEnable | GlobalEnable), 0x0);
	memcpy(PagedPTR, (void *)Register->Base, Register->Limit);
	Register->Base = (uint64_t)MapPhysical(PagedPTR);
	return _LoadIDTR(Register);
}
bool __sysvabi ReadIDTR(IDTR64 *Register){
	if(_ReadIDTR(Register)){
		Register->Base = (uint64_t)MapVirtual((void *)Register->Base);
		return true;
	}
	return false;
}

void SetIDTe(IDTTable64 Table, uint32_t N, uint64_t hndlr, 
	IDTEntrySegmentSelector64 sselector, uint8_t ist, 
	uint8_t gatetype, bool enable
){
    Table[N] = (IDTEntry64){
		.DescriptorPrivilegeLevel = sselector.PriviledgeLevel, .SegmentSelector = sselector.Segment, 
		.InterruptStackTableOffset = ist, .PresentEntry = enable, .GateType = gatetype, 
		.OffsetHigh = (hndlr >> 32) & UINT32_MAX, .OffsetMid = (hndlr >> 16) & UINT16_MAX, .OffsetLow = hndlr & UINT16_MAX, 
	};
}