#pragma once

#include "efi.h"
#include "guid.h"
#include "efilib.h"
#include "kernel/libcrt/def.h"
#include "drivers/executable/eload.h"

#define BOOT_OPTION_ATTR (LOAD_OPTION_ACTIVE)

#define KERNELEXE "SystemBoot\\kboot.apexe"

#define VideoMaxFrameBuffers	1

#define BOOTOPTION8 "BlockyOS Boot Manager"
#define BOOTOPTION16 (L"" BOOTOPTION8)
#define BOOTDESC16 BOOTOPTION16

typedef struct __bootinfo{
	struct memory{
		UINT32 NMemoryDescriptors, 
			MemoryDescriptorBufferSize, 
			MemocryDescriptorMapKey, 
			MemoryDescriptorStructSize;
		UINT32 MDescriptorsVersion;
		UINT64 TotalMemorySize;
		EFI_MEMORY_DESCRIPTOR *MemoryDescriptors;
	}memory;
	struct devices{
		__efiDevNode **devices;
		UINT32 nnodes;
		EFI_CONFIGURATION_TABLE *CTable;
		UINT32 CTableLength;
	}devices;
	struct bootentry{
		CHAR16 BootEntryName[32];
		UINT8 BootEntryCode;
		void *bootMain;
		struct{
			ExecutableSection *Sections;
			UINT32 NSections;
		}This;
		struct{
			LoadedService *Services;
			UINT32 NServices;
		}Services;
	}bootentry;
	struct{
		void *videomemory;
		UINT64 PixelSize, PixelWidth, PixelHeight;
		EFI_GRAPHICS_OUTPUT_MODE_INFORMATION CurrentVideoMode;
	}Video;
}__bootinfo;

typedef struct {
	// e.g. L"Boot0007"
	CHAR16 VariableName[12];
	// 0=Error, 1=Exists, 2=Added
	UINT8  Status;
} BOOT_ENTRY_RESULT;


typedef void __sysvabi (*kernelmain)(__bootinfo * __restrict__ bootin, ExecutableSection *This, UINT32 N);
__bootinfo *gatherbootinfo();
UINT8 CreateBootEntry(EFI_GUID *BootGuid, EFI_GUID *AltGuid, CHAR16 *OutBootVarName);