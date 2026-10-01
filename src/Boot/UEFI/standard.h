#pragma once

#include "efi.h"
#include "guid.h"
#include "efilib.h"
#include "kernel/libcrt/def.h"
#include "drivers/executable/struct.h"
#include "tools/tools.h"
// #include "drivers/executable/eload.h"

#define BOOT_OPTION_ATTR (LOAD_OPTION_ACTIVE)

#define KERNELEXE "SystemBoot\\kboot.apexe"

#define VideoMaxFrameBuffers	1

#define BOOTOPTION8 "BlockyOS Boot Manager"
#define BOOTOPTION16 (L"" BOOTOPTION8)
#define BOOTDESC16 BOOTOPTION16

typedef struct __bootinfo{
	struct{
		void	*Stack;
		UINT32	StackSize;
	}init;
	struct{
		UINT32 NMemoryDescriptors, 
			MemoryDescriptorBufferSize, 
			MemocryDescriptorMapKey, 
			MemoryDescriptorStructSize;
		UINT32 MDescriptorsVersion;
		UINT64 TotalMemorySize;
		EFI_MEMORY_DESCRIPTOR *MemoryDescriptors;
	}memory;
	struct{
		__efiDevNode **devices;
		UINT32 nnodes;
		EFI_CONFIGURATION_TABLE *CTable;
		UINT32 CTableLength;
	}devices;
	struct{
		CHAR16 BootEntryName[32];
		UINT8 BootEntryCode;
		void *bootMain;
		ExpandedPeExecutable *This;
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


#define KernalMainDef(NAME)		void __sysvabi __naked NAME##_km(__bootinfo * __restrict__ bootin, ExecutableSection *This, UINT32 N)
typedef void __sysvabi __naked (*kernelmain)(__bootinfo * __restrict__ bootin);
__bootinfo *gatherbootinfo();
UINT8 CreateBootEntry(EFI_GUID *BootGuid, EFI_GUID *AltGuid, CHAR16 *OutBootVarName);