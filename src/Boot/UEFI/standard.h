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
	}Stack;
	struct{
		UINTN MemoryDescriptorBufferSize, 
			MemoryDescriptorMapKey, 
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

#define SetPixel(Bf, A, R, G, B, Fmt) do { \
    UINT8 *p = (UINT8 *)(Bf); \
    p[0] = (UINT8)(Fmt == PixelRedGreenBlueReserved8BitPerColor ? R : (Fmt == PixelBlueGreenRedReserved8BitPerColor ? B : 0)); \
    p[1] = (UINT8)(G); \
    p[2] = (UINT8)(Fmt == PixelRedGreenBlueReserved8BitPerColor ? B : (Fmt == PixelBlueGreenRedReserved8BitPerColor ? R : 0)); \
    p[3] = (UINT8)(A); \
} while(0)

#define SetPixels(Bf, Pixels, N, Fmt) switch(Fmt) { \
    case PixelRedGreenBlueReserved8BitPerColor: { \
        __memcpy(Bf, Pixels, (N) * sizeof(UINT32)); \
        break; \
    } \
    case PixelBlueGreenRedReserved8BitPerColor: { \
        const UINT32 *src = (const UINT32 *)(Pixels); \
        UINT8 *dst = (UINT8 *)(Bf); \
        for (UINT64 __N = 0; __N < (N); ++__N) { \
            dst[(__N * 4) + 2] = (UINT8)((src[__N] >> 16) & 0xFF); /* Red */ \
            dst[(__N * 4) + 1] = (UINT8)((src[__N] >> 8) & 0xFF);  /* Green */ \
            dst[(__N * 4) + 0] = (UINT8)(src[__N] & 0xFF);         /* Blue */ \
            dst[(__N * 4) + 3] = (UINT8)((src[__N] >> 24) & 0xFF); /* Alpha */ \
        } \
        break; \
    } \
}

#define KernalMainDef(NAME)		void __sysvabi __naked NAME##_km(__bootinfo * __restrict__ bootin, LoadedPeExecutable *This)
typedef void __sysvabi __naked (*kernelmain)(__bootinfo * __restrict__ bootin);
__bootinfo *gatherbootinfo(EFI_HANDLE Image);
UINT8 CreateBootEntry(EFI_GUID *BootGuid, EFI_GUID *AltGuid, CHAR16 *OutBootVarName);
const char *EfiStatusToString(EFI_STATUS Status);

#define bochs_breakpoint	__bochs_breakpoint
extern void __bochs_breakpoint(void);