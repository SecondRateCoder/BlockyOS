#include "controllers/AHCI.h"
#include "controllers/ATAPI.h"
#include "controllers/NVMe.h"
#include "service.h"

void *GetMassStorageHandle(GenericMassStorageDeviceConfig *config, PCIDeviceSpecifier Device){
	uint32_t bus, slot;
	//	We initialise an ATA Device
	//	But since ATA has no Init, we js return UINT64_MAX.
	if(!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_SATA_AHCI)}, sizeof(PCIDeviceSpecifier))){
		config->AHCI.Out = ConfigureAHCIController(config->acpibase, config->AHCI.DeviceEnable, config->AHCI.LocalAPICs, config->N);
		return &(config->AHCI.Out);
	}else if(
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_NVMHCI)}, sizeof(PCIDeviceSpecifier)) || 
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_Express)}, sizeof(PCIDeviceSpecifier)) || 
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_Other)}, sizeof(PCIDeviceSpecifier))
	){
		config->NVMe.Out = ConfigureNVMeController(config->acpibase, config->N, config->Priviledge);
		return &(config->NVMe.Out);
	}else if(
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_IDE_NativeBusMaster)}, sizeof(PCIDeviceSpecifier)) || 
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_IDE_BothNativeBusMaster)}, sizeof(PCIDeviceSpecifier))
	){
        uint8_t PrimaryVector = 0x00, SecondaryVector = 0x00;
        if(AllocateInterruptVector(&PrimaryVector) && AllocateInterruptVector(&SecondaryVector)){
            config->ATAPI.Out = ConfigureATAPIController(config->acpibase, config->ATAPI.IOAPIC, PCIeSearchDevice(config->acpibase, Device, config->N, &bus, &slot), 
                config->Priviledge, config->ATAPI.Channel, config->ATAPI.Drive, (config->ATAPI.Channel == 0x00) || (config->ATAPI.Channel == 3)? PrimaryVector: SecondaryVector, SecondaryVector);
        }
		return &(config->ATAPI.Out);
		
	}
	return NULL;
}

uint64_t GetMassStorageLogicalBlockSize(void *Handle, PCIDeviceSpecifier Device, CommonMutex Mtx){
	uint32_t bus, slot;
	//	We initialise an ATA Device
	//	But since ATA has no Init, we js return UINT64_MAX.
	if(!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_SATA_AHCI)}, sizeof(PCIDeviceSpecifier))){
		HBAMemorySpace *MS = ((HBAMemorySpace *)((AHCIConfigureReturn *)Handle)->Data);
		ATAIdentifyData *ID = AHCIGetDriveInfo(MS, MS->Ports + ((AHCIConfigureReturn *)Handle)->Flags);
		return (__check(ID->field_validity, 1 << 12)? (ID->multi_sector_setting * 2): 512);
	}else if(!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_NVMHCI)}, sizeof(PCIDeviceSpecifier)) || 
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_Express)}, sizeof(PCIDeviceSpecifier)) || 
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_Other)}, sizeof(PCIDeviceSpecifier))
	){
		NVMeIdentifyNamespaceData_t *IDN = NVMePollNamespace(Handle, Mtx);
		//	We acc need to process the Returned Data so we need to Poll the Mutex.
		MutexPoll(Mtx);
		uint32_t SS = (1U << IDN->LBAF[IDN->ActiveLBAFormat].LBADataSize);
		mfree(IDN);		return SS;
	}
	if(Handle == UINT64_MAX || Handle == 0x1){return ATAPISectorSize((*(ATAPIQueue *)Handle));}
}

uint64_t GetMassStorageQueue(void *Handle, PCIDeviceSpecifier Device, uint32_t Vector, CommonMutex Mtx){
	uint32_t bus, slot;
	//	We initialise an ATA Device
	//	But since ATA has no Init, we js return UINT64_MAX.
	if(!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_SATA_AHCI)}, sizeof(PCIDeviceSpecifier))){
		HBAMemorySpace *MS = ((HBAMemorySpace *)((AHCIConfigureReturn *)Handle)->Data);
		while(!(MS->PortImplementedBitmask >> ((volatile AHCIConfigureReturn *)Handle)->Flags++)){if(((volatile AHCIConfigureReturn *)Handle)->Flags > 32){
			((volatile AHCIConfigureReturn *)Handle)->Flags = 0;}}
		return ((AHCIConfigureReturn *)Handle)->Flags - 1;
	}else if(!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_NVMHCI)}, sizeof(PCIDeviceSpecifier)) || 
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_Express)}, sizeof(PCIDeviceSpecifier)) || 
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_Other)}, sizeof(PCIDeviceSpecifier))
	){return ((uint64_t)NVMeAllocateIOSubmissionQueues(Handle, 32, Mtx) << 32) + NVMeAllocateIOCompletionQueue(Handle, 32, Vector, Mtx);}
	else if(
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_IDE_NativeBusMaster)}, sizeof(PCIDeviceSpecifier)) || 
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_IDE_BothNativeBusMaster)}, sizeof(PCIDeviceSpecifier))
	){
		uint8_t ch = ((ATAPIQueue *)Handle)->Primary + ((ATAPIQueue *)Handle)->Secondary + 
			(((ATAPIQueue *)Handle)->Primary && ((ATAPIQueue *)Handle)->Secondary) + 1;
		if(ch > 3){ch = 0;}
		ATAPIQueue Q = ATAPIGetQueue(!((ATAPIQueue *)Handle)->Slave, ch);
		return *((uint64_t *)&Q);
	}
}

void ReleaseMassStorageQueue(void *Handle, PCIDeviceSpecifier Device, uint64_t Queue, CommonMutex Mtx){
	uint32_t bus, slot;
	//	We initialise an ATA Device
	//	But since ATA has no Init, we js return UINT64_MAX.
	if(!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_NVMHCI)}, sizeof(PCIDeviceSpecifier)) || 
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_Express)}, sizeof(PCIDeviceSpecifier)) || 
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_Other)}, sizeof(PCIDeviceSpecifier))
	){NVMeFreeIOSubmissionQueue(Handle, Queue >> 32, Mtx);		NVMeFreeIOCompletionQueue(Handle, Queue & UINT32_MAX, Mtx);}
}

uint32_t GetMassStorageData(void *Handle, PCIDeviceSpecifier Device, 
	uint64_t Queue, uint64_t Position, uint64_t Bytes, void *Data, CommonMutex Mtx
){
	uint32_t bus, slot;
	//	We initialise an ATA Device
	//	But since ATA has no Init, we js return UINT64_MAX.
	if(!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_SATA_AHCI)}, sizeof(PCIDeviceSpecifier))){
		return AHCIReadBytes((HBAMemorySpace *)((AHCIConfigureReturn *)Handle)->Data, Queue, Position, Bytes, Data);
	}else if(
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_NVMHCI)}, sizeof(PCIDeviceSpecifier)) || 
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_Express)}, sizeof(PCIDeviceSpecifier)) || 
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_Other)}, sizeof(PCIDeviceSpecifier))
	){return NVMeIOReadBytes(Handle, (Queue >> 32) & UINT32_MAX, Queue &UINT32_MAX, Position, Bytes, Data, Mtx);
	}else if(
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_IDE_NativeBusMaster)}, sizeof(PCIDeviceSpecifier)) || 
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_IDE_BothNativeBusMaster)}, sizeof(PCIDeviceSpecifier))
	){return ATAPIReadRaw(*((ATAPIQueue *)(&Queue)), Mtx, 
		((ATAPIQueue *)(&Queue))->Primary + ((ATAPIQueue *)(&Queue))->Secondary, Position, Bytes, Data);}
}

uint32_t StoreMassStorageData(void *Handle, PCIDeviceSpecifier Device, 
	uint64_t Queue, uint64_t Position, uint64_t Bytes, void *Data, CommonMutex Mtx
){
	uint32_t bus, slot;
	//	We initialise an ATA Device
	//	But since ATA has no Init, we js return UINT64_MAX.
	if(!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_SATA_AHCI)}, sizeof(PCIDeviceSpecifier))){
		return AHCIWriteBytes((HBAMemorySpace *)((AHCIConfigureReturn *)Handle)->Data, Queue, Position, Bytes, Data);
	}else if(
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_NVMHCI)}, sizeof(PCIDeviceSpecifier)) || 
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_Express)}, sizeof(PCIDeviceSpecifier)) || 
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_Other)}, sizeof(PCIDeviceSpecifier))
	){return NVMeIOWriteBytes(Handle, (Queue >> 32) & UINT32_MAX, Queue & UINT32_MAX, Position, Bytes, Data, Mtx);
	}else if(
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_IDE_NativeBusMaster)}, sizeof(PCIDeviceSpecifier)) || 
		!memcmp(&Device, (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_IDE_BothNativeBusMaster)}, sizeof(PCIDeviceSpecifier))
	){return ATAPIWriteRaw(*((ATAPIQueue *)(&Queue)), Mtx, 
		((ATAPIQueue *)(&Queue))->Primary + ((ATAPIQueue *)(&Queue))->Secondary, Position, Bytes, Data);}
}


void *MassStorageHandle(GenericMassStorageDeviceConfig *cfg, PCIDeviceSpecifier device, CommonMutex Mtx){return GetMassStorageHandle(cfg, device);}
uint64_t AllocateMassStorageQueue(void *Handle, PCIDeviceSpecifier Device, CommonMutex Mtx, uint32_t Vector){return GetMassStorageQueue(Handle, Device, Vector, Mtx);}
void FreeMassStorageQueue(void *Handle, PCIDeviceSpecifier Device, CommonMutex Mtx, uint64_t Queue){ReleaseMassStorageQueue(Handle, Device, Queue, Mtx);}
void MassStorageRead(void *Handle, PCIDeviceSpecifier Device, CommonMutex Mtx, uint64_t Queue, uint64_t Offset, uint64_t Bytes, void *Data){
	uint32_t LBS = GetMassStorageLogicalBlockSize(Handle, Device, Mtx);
	MutexPoll(Mtx);
	void *Temp = mcalloc(1, __roundup(Bytes, LBS));
	GetMassStorageData(Handle, Device, Queue, Offset, Bytes, Temp, Mtx);
	MutexPoll(Mtx);
	memcpy(Data, Temp, Bytes);
	mfree(Temp);
}
void MassStorageWrite(void *Handle, PCIDeviceSpecifier Device, CommonMutex Mtx, uint64_t Queue, uint64_t Offset, uint64_t Bytes, void *Data){
	uint32_t LBS = GetMassStorageLogicalBlockSize(Handle, Device, Mtx);
	MutexPoll(Mtx);
	void *Temp = mcalloc(1, __roundup(Bytes, LBS));
	StoreMassStorageData(Handle, Device, Queue, Offset, Bytes, Temp, Mtx);
	MutexPoll(Mtx);
	memcpy(Data, Temp, Bytes);
	mfree(Temp);
}
rawenv OpenRawHandle(GenericMassStorageDeviceConfig *cfg, PCIDeviceSpecifier device, 
	uint32_t Vector, uint32_t BlockSize, LBA Partition, CommonMutex Mtx
){
	rawenv re = mcalloc(1, sizeof(renv_t));
	re->Device = device;
	re->Mutex =
	re->Handle = MassStorageHandle(cfg, device, Mtx);
	uint64_t LBS = GetMassStorageLogicalBlockSize(re->Handle, device, Mtx);
	re->CalcBlocks = (BlockSize / LBS) + ((BlockSize % LBS) != 0);
	re->RealBlockSize = LBS;
	re->PartitionBase = Partition;
	re->Queue = AllocateMassStorageQueue(re->Handle, device, Mtx, Vector);
}
void CloseRawHandle(rawenv re){
	if(
		!memcmp(&(re->Device), (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_NVMHCI)}, sizeof(PCIDeviceSpecifier)) || 
		!memcmp(&(re->Device), (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_Express)}, sizeof(PCIDeviceSpecifier)) || 
		!memcmp(&(re->Device), (PCIDeviceSpecifier[]){GetPCIstruct(MassStorage_NVMe_Other)}, sizeof(PCIDeviceSpecifier))
	){NVMeFlushVolatileData(re->Handle, (re->Queue >> 32) & UINT32_MAX, re->Queue & UINT32_MAX, re->Mutex);}
	MutexPoll(re->Mutex);
	FreeMassStorageQueue(re->Handle, re->Device, re->Mutex, re->Queue);
	mfree(re);
}

void *ReadRawHandleBlocks(rawenv re, LBA P, uint64_t BLOCKS){
	P = __rounddown(P, re->RealBlockSize * re->CalcBlocks);
	if(P < re->PartitionBase){return NULL;}
	void *Out = mcalloc(re->CalcBlocks, __roundup(re->RealBlockSize * BLOCKS, re->RealBlockSize));
	MassStorageRead(re->Handle, re->Device, re->Mutex, re->Queue, 
		(re->PartitionBase * re->RealBlockSize) + ((P - re->PartitionBase) * re->RealBlockSize * re->CalcBlocks), 
		__roundup(re->CalcBlocks * re->RealBlockSize * BLOCKS, re->RealBlockSize), Out);
	MutexPoll(re->Mutex);
	return Out;
}

bool WriteRawHandleBlocks(rawenv re, LBA P, uint64_t BLOCKS, void *DATA){
	if((P % re->RealBlockSize) != 0){return false;}
	void *In = mcalloc(re->CalcBlocks, __roundup(re->RealBlockSize * BLOCKS, re->RealBlockSize));
	memcpy(DATA, In, BLOCKS * re->RealBlockSize * re->CalcBlocks);
	MassStorageWrite(re->Handle, re->Device, re->Mutex, re->Queue, 
		(re->PartitionBase * re->RealBlockSize) + ((P - re->PartitionBase) * re->RealBlockSize * re->CalcBlocks), 
		__roundup(re->CalcBlocks * re->RealBlockSize * BLOCKS, re->RealBlockSize), In);
	MutexPoll(re->Mutex);
	mfree(In);
	return;
}
bool WriteRawHandleBytes(rawenv re, LBA P, uint64_t BYTES, void *DATA){
	void *temp = ReadRawHandleBlocks(re, P, BYTES);
	memcpy(temp + (P % re->RealBlockSize), DATA, BYTES);
	WriteRawHandleBlocks(re, P, BYTES, temp);
	mfree(temp);
	return;
}