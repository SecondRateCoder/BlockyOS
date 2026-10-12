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

IDTTable64 InterruptTable;
static volatile GDTSystemSegmentDescriptor64 GDTTable[(GDTTableDefaultLength / 2)] = {0};
static volatile TSS_t tss;

void InitialiseGDT(){
    ((GDTDescriptor *)GDTTable)[0] = (GDTDescriptor){0};
    ((GDTDescriptor *)GDTTable)[1] = (GDTDescriptor){
		.ABPresent = true, .ABPriviledgeLevel = 0x00, .ABSystemSegmentBit = true,
        .ABExecutableBit = true, .ABDirectionBit = false, .ABReadWritableBit = true, .ABAccessedBit = false,
        .FLongModeBit = true, .F32BitModeBit = false, .FGranularity = true,
		
		.BaseLow = 0, .BaseHigh = 0, .LimitLow = UINT64_MAX, .LimitHigh = UINT64_MAX,
    };
    ((GDTDescriptor *)GDTTable)[2] = (GDTDescriptor){
		.ABPresent = true, .ABPriviledgeLevel = 0x00, .ABSystemSegmentBit = true,
        .ABExecutableBit = false, .ABDirectionBit = false, .ABReadWritableBit = true, .ABAccessedBit = false,
        .FLongModeBit = true, .F32BitModeBit = false, .FGranularity = true,
		
        .BaseLow = 0, .BaseHigh = 0, .LimitLow = UINT64_MAX, .LimitHigh = UINT64_MAX,
    };

    *((GDTSystemSegmentDescriptor64 *)((GDTDescriptor *)GDTTable + 3)) = (GDTSystemSegmentDescriptor64){
        .ABPresent = true, .ABPriviledgeLevel = 0x00, .ABSystemSegmentBit = false, .ABType = TSS64BitAvailable, .FGranularity = false, 

        .BaseLow = (uint64_t)&tss & 0xFFFFFF, .BaseHigh = ((uint64_t)&tss >> 24) & 0xFFFFFF, 
        .LimitHigh = ((sizeof(TSS_t) - 1) >> 16) & 0xF, .LimitLow = (sizeof(TSS_t) - 1) & 0xFFFF, 
    };
    tss = (TSS_t){.io_map_base = sizeof(TSS_t), .IST = {0}, .RSP = {0}};

    GDTR64 Tmp = {.Base = (UINT64)GDTTable, .Limit = (sizeof(GDTDescriptor) * 3 + sizeof(GDTSystemSegmentDescriptor64)) - 1};
    _LoadGDTR(&Tmp);
}

void InitialiseIDT(void *ACPI){
	for(register uint32_t cc = 0; cc < IDTLength; ++cc){
		SetIDTe(InterruptTable, cc, 0x00, (IDTEntrySegmentSelector64){0}, 0, _64BitInterruptGate, true);}
	IDTCallbackInit(InterruptTable);
	// sizeof(IDTEntry64);
	IDTR64 temp = {.Base = (uint64_t)InterruptTable, .Limit = sizeof(InterruptTable) - 1};
	__bochs();
	_LoadIDTR(&temp);
	InitLocalAPIC(ACPI, 0x2F, false);
}

ISRCallbackDefinition(GenericIO){ISRCallbackReturn;}
rawenv re;
void __sysvabi __main(__bootinfo *__restrict__ bootin){
	//* Set Up Interrupt Descriptor Table (IDT) & GDT
	__bochs();
	InitialiseGDT();

	__bochs();
	InitialiseAllocationState(bootin->memory.MemoryDescriptors, bootin->memory.MemoryDescriptorStructSize, 
		bootin->memory.MemoryDescriptorBufferSize / bootin->memory.MemoryDescriptorStructSize, bootin->memory.TotalMemorySize);

	void *ACPIBase = NULL;
	for(register uint32_t cc = 0; cc < bootin->devices.CTableLength; ++cc){
		if(!memcmp(&(bootin->devices.CTable[cc].VendorGuid), (EFI_GUID[]){ACPI_TABLE_GUID}, sizeof(EFI_GUID)) || 
			!memcmp(&(bootin->devices.CTable[cc].VendorGuid), (EFI_GUID[]){ACPI_20_TABLE_GUID}, sizeof(EFI_GUID))
		){ACPIBase = bootin->devices.CTable[cc].VendorTable;			break;}
	}

	InitialiseIDT(ACPIBase);
	
	__bochs();
	InitaliseVMA(bootin->Video.videomemory, ACPIBase, 
		bootin->Video.PixelSize, bootin->Video.PixelWidth, bootin->Video.PixelHeight);
	uint32_t W = bootin->Video.PixelWidth * bootin->Video.PixelSize, 
			H = bootin->Video.PixelHeight * bootin->Video.PixelSize;
	void *vm = AllocateVideoMemory(0, 0, &W, &H);
	SelectVideoContext(vm, (void **)ASCII, ASCIILength);
	
	__bochs();
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

	while(true){;}
	return;
	//* Take Over the Page Tables(Virtual Memory)
	//* Initialize a Stack
	//* Enable Hardware Interrupts(STI)
}