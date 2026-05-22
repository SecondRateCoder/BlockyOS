#pragma once

#include "efi.h"
#include "guid.h"
#include "efilib.h"

#include "drivers/executable/execfile.h"
#include "drivers/socket/socket.h"
#include "tools/tools.h"

#define BOOT_OPTION_ATTR (LOAD_OPTION_ACTIVE)

#define KERNELEXE "boot\\kernel\\kernel.apexe"

#define BOOTOPTION8 "BlockyOS Boot Manager"
#define BOOTOPTION16 (L"" BOOTOPTION8)
#define BOOTDESC16 BOOTOPTION16

typedef struct __bootinfo{
	struct memory{
		UINTN nDescs, descSize, mapKey, descItemSize;
		UINT32 descVersion;
		EFI_MEMORY_DESCRIPTOR *desc;
	}memory;
	struct devices{
		__efiDevNode **devices;
		UINTN nnodes;
	}devices;
	struct bootentry{
		CHAR16 BootEntryName[32];
		UINT8 BootEntryCode;
	}bootentry;
}__bootinfo;

typedef struct {
	// e.g. L"Boot0007"
	CHAR16 VariableName[12];
	// 0=Error, 1=Exists, 2=Added
	UINT8  Status;
} BOOT_ENTRY_RESULT;


typedef void (*kernelmain)(__bootinfo * __restrict__ bootin);
__bootinfo *gatherbootinfo();
UINT8 CreateBootEntry(EFI_GUID *BootGuid, EFI_GUID *AltGuid, CHAR16 *OutBootVarName);