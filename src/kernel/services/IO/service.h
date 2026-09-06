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

#include "kernel/libcrt/mutex.h"
#include "controllers/ATAPI.h"
#include "controllers/NVMe.h"

#define GPTNameSize		72
#define GPTNameLength	(GPTNameSize / sizeof(char16_t))
#define GPTLBA			(0x01)
typedef char16_t		GPTNameStr[GPTNameLength];
typedef uint64_t		LBA;
typedef uint128_t		GUID;

typedef struct miniGPT{
	char		sig[8];
	uint32_t	rev;
	uint32_t	hSize;
	uint32_t	hChecksum;
	uint32_t	r;
	LBA			localLBA;
	LBA			alternateLBA;
	LBA			fUsable;
	LBA			lUsable;
	GUID		dGUID;
	LBA			partEntryLoc;
	uint32_t	nPartEntries;
	uint32_t	partEntrySize;
	uint32_t	partArrayChecksum;
}__attribute__((packed)) miniGPT;
typedef struct partdim{LBA base, high;}partdim;
typedef struct GPTentry{
	GUID		_GUID;
	GUID		uGUID;
	LBA			sLBA;
	LBA			eLBA;
	uint64_t	attr;
	GPTNameStr	name;
}__attribute__((packed)) GPTentry;

enumdef(uint32_t, AHCIConfigureReturnFlags){NoError = 0x0, NotEnoughVectors = 0x1, CouldntFindDevice, UnusablePCIDevice, UnusablePCIeDevice = UnusablePCIDevice, CouldntFindMSIxCapabilities};
typedef struct{
	AHCIConfigureReturnFlags Flags;
	uint64_t Data;
}AHCIConfigureReturn;

typedef union{
	void					*acpibase;
	uint32_t				Priviledge;
	uint32_t				N;
	struct{
		uint32_t			IOAPIC;
		uint8_t				Channel, 
							Drive;
		ATAPIQueue			Out;
	}ATAPI;
	struct{
		bool				*DeviceEnable;
		uint32_t			*LocalAPICs;
		uint32_t			NVectors;
		AHCIConfigureReturn	Out;
	}AHCI;
	struct{
		struct NVMeHandle	Out;
	}NVMe;
}GenericMassStorageDeviceConfig;

const PCIDeviceSpecifier SupportedIODevices[] = {
	// GetPCIstruct(MassStorage_SATA_AHCI), 
	{.ProgIF = 0x01, .SubClass = 0x06, .ClassCode = 0x01}, 
	// GetPCIstruct(MassStorage_NVMe_NVMHCI), 
	{.ProgIF = 0x01, .SubClass = 0x08, .ClassCode = 0x01}, 
	// GetPCIstruct(MassStorage_NVMe_Express), 
	{.ProgIF = 0x02, .SubClass = 0x08, .ClassCode = 0x01}, 
	// GetPCIstruct(MassStorage_NVMe_Other), 
	{.ProgIF = 0x80, .SubClass = 0x08, .ClassCode = 0x01}, 
	// GetPCIstruct(MassStorage_IDE_NativeBusMaster), 
	{.ProgIF = 0x85, .SubClass = 0x01, .ClassCode = 0x01}, 
	// GetPCIstruct(MassStorage_IDE_BothNativeBusMaster)
	{.ProgIF = 0x8A, .SubClass = 0x01, .ClassCode = 0x01}
};
const uint8_t TotalSupportedIODevices = sizeof(SupportedIODevices) / sizeof(PCIDeviceSpecifier);

enumdef(uint8_t, GenericDiskCommand){
	IDENTIFY_DEVICE = 0xEC, 
	//	48-bit LBA
	READ_DMA_EXT = 0x25, 
	//	48-bit LBA
	WRITE_DMA_EXT = 0x35, 
	//	Instructs the drive to write any cached volatile data safely onto the physical storage media
	FLUSH_CACHE = 0xE7, FLUSH_CACHE_EXT = 0xEA, 
	//	Native Command Queuing DMA
	READ_FPDMA_QUEUED = 0x60, 
	//	Native Command Queuing DMA
	WRITE_FPDMA_QUEUED = 0x61, 
	//	Used for optical media devices (CD/DVD/Blu-ray drives) attached to an AHCI port, 
	//		wrapping SCSI command blocks inside SATA packets.
	OPTICAL_PACKET = 0xA0
};
typedef GenericDiskCommand	UsableDiskCommand;

void *MassStorageHandle(GenericMassStorageDeviceConfig *cfg, PCIDeviceSpecifier device, CommonMutex Mtx);

#define ReBlocks(re, size)		(((size) / (re)->CBlockSize) + (((size) % (re)->CBlockSize) != 0))
#define ReSetGUID(SET, VALUE)	SET = (GUID){VALUE[0], VALUE[1]}
typedef struct{
	PCIDeviceSpecifier	Device;
	CommonMutex			Mutex;
	uint64_t			Queue;
	void				*Handle;
	uint32_t			RBlockSize;
	uint32_t			CBlockSize;
}renv_t, *rawenv;

DLLImport uint64_t AllocateMassStorageQueue(void *Handle, PCIDeviceSpecifier Device, CommonMutex Mtx, uint32_t Vector);
DLLImport void FreeMassStorageQueue(void *Handle, PCIDeviceSpecifier Device, CommonMutex Mtx, uint64_t Queue);
DLLImport void MassStorageRead(void *Handle, PCIDeviceSpecifier Device, CommonMutex Mtx, uint64_t Queue, uint64_t Offset, uint64_t Bytes, void *Data);
DLLImport void MassStorageWrite(void *Handle, PCIDeviceSpecifier Device, CommonMutex Mtx, uint64_t Queue, uint64_t Offset, uint64_t Bytes, void *Data);
DLLImport rawenv OpenRawHandle(GenericMassStorageDeviceConfig *cfg, PCIDeviceSpecifier device, uint32_t Vector, uint32_t BlockSize, CommonMutex Mtx);
DLLImport void WriteRawHandleBytes(rawenv re, uint64_t OFFSET, uint64_t BYTES, void *DATA);
DLLImport void WriteRawHandleBlocks(rawenv re, LBA LBA, uint64_t BLOCKS, void *DATA);
#define ReadRawHandleBytes	ReadRawHandleBlocks
DLLImport void *ReadRawHandleBlocks(rawenv re, LBA LBA, uint64_t BLOCKS);