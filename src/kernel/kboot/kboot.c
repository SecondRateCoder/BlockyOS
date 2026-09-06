#include "kboot.h"

//  Non-Overwritable Memory:    
//      EfiRuntimeServicesCode, EfiRuntimeServicesData, 
//      EfiACPIReclaimMemory(Reclaim after Parsing ACPI Tables)
//      EfiACPIMemoryNVS(Hardware-Reserved Non-Volatile Storage)
//      EfiUnusableMemory(Unstable/Unusable Physical Memory)
//      EfiMemoryMappedIO, EfiMemoryMappedIOPortSpace
//      EfiLoaderData, 
//  Reclaimable Memory:
//      EfiLoaderCode, 
//      EfiBootServicesCode, EfiBootServicesData, 
//      EfiConventionalMemory

IDTTable64 __linkersection(IDT) InterruptTable;
extern void *InternalInterruptAddressTable[IDTLength];
extern IDTEntrySegmentSelector64 SegmentSelectorTable[IDTLength];

GDTSystemSegmentDescriptor64 GDTTable[GDTTableDefaultLength];

void __sysvabi main(__bootinfo * __restrict__ info, ExecutableSection *This, UINT32 N){
	//* Set Up Interrupt Descriptor Table (IDT) & GDT
	//  Initialise Interrupt Table
	IDTR64 temp = {.Base = (uint64_t)InterruptTable, .Limit = sizeof(InterruptTable) - 1};
	for(uint32_t cc = 0; cc < IDTLength; ++cc){
		InterruptTable[cc] = (IDTEntry64){
			.DescriptorPriviledgeLevel = 0x0, .GateType = IDT64InterruptGateType, 
			.InterruptStackTableOffset = 0x0, .OffsetHigh = ((uint64_t)InternalInterruptAddressTable[cc] >> 16) & 0xFFFFFFFFFFFF, 
			.OffsetLow = InternalInterruptAddressTable[cc] && 0xFFFF, .Present = true, 
			.SegmentSelector = *((uint16_t *)(SegmentSelectorTable + cc)), 
		};
	}
	LoadIDT(&temp);
	for(uint32_t cc = 0; cc < info->devices.CTableLength; ++cc){
		if(memcmp(&(info->devices.CTable[cc].VendorGuid), (EFI_GUID[]){ACPI_TABLE_GUID}, sizeof(EFI_GUID)) || 
			memcmp(&(info->devices.CTable[cc].VendorGuid), (EFI_GUID[]){ACPI_20_TABLE_GUID}, sizeof(EFI_GUID))
		){InitLocalAPIC(info->devices.CTable[cc].VendorTable, 0xFF, false);			break;}
	}


	InitialiseAllocationState(info->memory.MemoryDescriptors, info->memory.NMemoryDescriptors, info->memory.TotalMemorySize);

	//* Take Over the Page Tables(Virtual Memory)
	//* Initialize a Stack
	//* Enable Hardware Interrupts(STI)
}