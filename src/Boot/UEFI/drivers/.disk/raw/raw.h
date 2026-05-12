#pragma once

#include "efi.h"
#include "efilib.h"

#include "src/Boot/UEFI/tools/tools.h"

typedef size_t LBA;

typedef struct rawenv{
    EFI_DISK_IO_PROTOCOL *D;
    EFI_BLOCK_IO_PROTOCOL *B;
    UINT32 configBlockSize;
#ifdef __DEBUG__
    bool EnableVerbose;
#endif
}rawenv;

void configureBlockSize(rawenv *re, UINT32 configBlockSize);
rawenv *startup(UINT32 MediaID, UINT32 configblocksize);
rawenv *startup_me(EFI_HANDLE Image, UINT32 configblocksize);
void *readblock(rawenv *re, UINTN pos, UINTN blocks);
BOOLEAN writeblock(rawenv *re, void *buffer, size_t pos, size_t blocks);
void dispose(rawenv *re);

void EnableVerbose(rawenv *re);
void DisableVerbose(rawenv *re);