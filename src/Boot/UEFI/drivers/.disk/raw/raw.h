#pragma once

#include "efi.h"
#include "efilib.h"

#include "src/Boot/UEFI/tools/tools.h"

#define GPT_LBA 1
#define GPT_BLOCKS(blockSize) ((sizeof(miniGPT) / (blockSize)) + ((sizeof(miniGPT) % (blockSize)) != 0))

typedef size_t LBA;

typedef struct rawenv_t{
    /// @brief If true then the Interface is a Block IO Interface;
    bool isPart;
    EFI_BLOCK_IO *Blk;
    EFI_DEVICE_PATH *Dev;
    UINT32 MediaID;
    UINT32 CalcBlock, ConfBlock, RealBlock;
#ifdef __DEBUG__
    bool EnableVerbose;
#endif
}rawenv_t, *rawenv;

void EnableVerbose(rawenv re);
void DisableVerbose(rawenv re);

UINT32 getblocksize(rawenv re);
void setblocksize(rawenv re, UINT32 new);

rawenv startup(UINT32 MediaID, UINT32 configuredBlockSize);
void *readblock(rawenv re, UINTN pos, UINTN blocks);
void writeblock(rawenv re, void *buffer, UINTN pos, UINTN blocks);

void dispose(rawenv re);