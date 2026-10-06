#include "kboot.h"
#include "kdefs.h"

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
GDTSystemSegmentDescriptor64 GDTTable[(GDTTableDefaultLength / 2)] = {0};

void InitialiseGDT(){
	// uint64_t codeLimit = (((uint64_t)&CODELIMIT - (uint64_t)&CODEBASE) >> 12) - 1;
	// uint64_t dataLimit = (((uint64_t)&DATALIMIT - (uint64_t)&DATABASE) >> 12) - 1;
	// GDTR64 R = {.Base = (uint64_t)(void *)GDTTable, .Limit = sizeof(GDTTable)};
	// //*	Kernel Code Segment.
	// ((GDTDescriptor *)GDTTable)[1] = (GDTDescriptor){
	// 	.F32BitModeBit = false, .FGranularity = true, .FLongModeBit = true, 
	// 	.LimitHigh = codeLimit >> 16, .LimitLow = codeLimit, 
	// 	.BaseHigh = ((uint64_t)&CODEBASE) >> 24, .BaseLow = ((uint64_t)&CODEBASE), 
		
	// 	.ABPresent = true, .ABPriviledgeLevel = 0x00, 
	// 	.ABReadWritableBit = false, .ABSystemSegmentBit = false, 
	// 	.ABAccessedBit = false, .ABExecutableBit = true, .ABDirectionBit = true, 
	// };
	// //*	Kernel Data Segment.
	// ((GDTDescriptor *)GDTTable)[2] = (GDTDescriptor){
	// 	.F32BitModeBit = false, .FGranularity = true, .FLongModeBit = true, 
	// 	.LimitHigh = dataLimit >> 16, .LimitLow = dataLimit, 
	// 	.BaseHigh = ((uint64_t)&DATABASE) >> 24, .BaseLow = ((uint64_t)&DATABASE), 
		
	// 	.ABPresent = true, .ABPriviledgeLevel = 0x00, 
	// 	.ABReadWritableBit = true, .ABSystemSegmentBit = false, 
	// 	.ABAccessedBit = false, .ABExecutableBit = false, .ABDirectionBit = false, 
	// };
	// //*	User Code Segment.
	// ((GDTDescriptor *)GDTTable)[3] = (GDTDescriptor){
	// 	.F32BitModeBit = false, .FGranularity = true, .FLongModeBit = true, 
	// 	.LimitHigh = codeLimit >> 16, .LimitLow = codeLimit, 
	// 	.BaseHigh = ((uint64_t)&CODEBASE) >> 24, .BaseLow = ((uint64_t)&CODEBASE), 
		
	// 	.ABPresent = true, .ABPriviledgeLevel = 0x03, 
	// 	.ABReadWritableBit = false, .ABSystemSegmentBit = false, 
	// 	.ABAccessedBit = false, .ABExecutableBit = true, .ABDirectionBit = true, 
	// };
	// //*	User Data Segment.
	// ((GDTDescriptor *)GDTTable)[4] = (GDTDescriptor){
	// 	.F32BitModeBit = false, .FGranularity = true, .FLongModeBit = true, 
	// 	.LimitHigh = dataLimit >> 16, .LimitLow = dataLimit, 
	// 	.BaseHigh = ((uint64_t)&DATABASE) >> 24, .BaseLow = (uint64_t)&DATABASE, 
		
	// 	.ABPresent = true, .ABPriviledgeLevel = 0x03, 
	// 	.ABReadWritableBit = true, .ABSystemSegmentBit = false, 
	// 	.ABAccessedBit = false, .ABExecutableBit = false, .ABDirectionBit = false, 
	// };
	// //	All TSS's can be dynamically allocated later.

	// LoadGDTR(&R);
}

void InitialiseIDT(void *ACPI){
	IDTR64 temp = {.Base = (uint64_t)InterruptTable, .Limit = sizeof(InterruptTable) - 1};
	for(uint32_t cc = 0; cc < IDTLength; ++cc){
		InterruptTable[cc] = (IDTEntry64){
			.DescriptorPriviledgeLevel = 0x0, .GateType = IDT64InterruptGateType, 
			.InterruptStackTableOffset = 0x0, .OffsetHigh = ((uint64_t)InterruptCallbacks[cc] >> 16) & 0xFFFFFFFFFFFF, 
			.OffsetLow = InterruptCallbacks[cc] && UINT16_MAX, .Present = true, .SegmentSelector = {0}
		};
	}
	LoadIDTR(&temp);
	InitLocalAPIC(ACPI, 0x00, false);
}

ISRCallbackDefinition(GenericIO){ISRCallbackReturn;}
volatile bool _f = false;
rawenv re;
void __sysvabi __main(__bootinfo * __restrict__ bootin){
	if(_f){
		//* Set Up Interrupt Descriptor Table (IDT) & GDT
		InitialiseGDT();
	
		void *ACPIBase = NULL;
		for(uint32_t cc = 0; cc < bootin->devices.CTableLength; ++cc){
			if(memcmp(&(bootin->devices.CTable[cc].VendorGuid), (EFI_GUID[]){ACPI_TABLE_GUID}, sizeof(EFI_GUID)) || 
				memcmp(&(bootin->devices.CTable[cc].VendorGuid), (EFI_GUID[]){ACPI_20_TABLE_GUID}, sizeof(EFI_GUID))
			){ACPIBase = bootin->devices.CTable[cc].VendorTable;			break;}
		}
	
		//	We need to Initialise the 
		//  Initialise Interrupt Table
		InitialiseIDT(ACPIBase);
		
		InitialiseAllocationState(bootin->memory.MemoryDescriptors, 
			bootin->memory.MemoryDescriptorBufferSize / bootin->memory.MemoryDescriptorStructSize, bootin->memory.TotalMemorySize);
		
		InitaliseVMA(bootin->Video.videomemory, ACPIBase, 
			bootin->Video.PixelSize, bootin->Video.PixelWidth, bootin->Video.PixelHeight);
		uint32_t W = bootin->Video.PixelWidth * bootin->Video.PixelSize, 
				H = bootin->Video.PixelHeight * bootin->Video.PixelSize;
		void *vm = AllocateVideoMemory(0, 0, &W, &H);
		SelectVideoContext(vm, (void **)ASCII, ASCIILength);
		
		bool de[32] = {0};	memset(de, true, sizeof(de));
		uint32_t APIC = GetLocalAPICID();
		GenericMassStorageDeviceConfig cfg = {
			.acpibase = ACPIBase, .AHCI = {.DeviceEnable = de, .NVectors = 1, .Out = {0}, .LocalAPICs = &APIC}, 
			.ATAPI = {.Channel = 0, .Drive = 0, .IOAPIC = 0, .Out = {0}}, .N = 0, 
			.NVMe = {.Out = {0}}, .Priviledge = 0x00
		};
		InitMutex(Mtx);
		uint8_t IV;
		AllocateInterruptVector(&IV);
		
		re = OpenRawHandle(&cfg, GetPCIstruct(MassStorage_SATA_AHCI), IV, 512, 0x00, Mtx);
	}

	while(true){;}
	return;
	//* Take Over the Page Tables(Virtual Memory)
	//* Initialize a Stack
	//* Enable Hardware Interrupts(STI)
}