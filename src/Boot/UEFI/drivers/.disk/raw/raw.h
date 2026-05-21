#pragma once

#include "efi.h"
#include "efilib.h"

#include "src/Boot/UEFI/tools/tools.h"

#define GPTsig "EFI PART"
#define GPT_LBA 1

typedef UINTN LBA;

typedef struct miniGPT{
	char sig[8];
	UINT32 rev;
	UINT32 hSize;
	UINT32 hChecksum;
	UINT32 r;
	LBA localLBA;
	LBA alternateLBA;
	LBA fUsable;
	LBA lUsable;
	EFI_GUID dGUID;
	LBA partEntryLoc;
	UINT32 nPartEntries;
	UINT32 partEntrySize;
	UINT32 partArrayChecksum;
}__attribute__((packed)) miniGPT;
typedef struct partdim{LBA base, high;}partdim;
typedef struct GPTentry{
	EFI_GUID GUID;
	EFI_GUID uGUID;
	LBA sLBA;
	LBA eLBA;
	UINTN attr;
	GPTeNSTR name;
}__attribute__((packed)) GPTentry;

typedef struct rawenv_t{
    /// @brief If true then the Interface is a Block IO Interface;
    bool isPart;
	EFI_HANDLE handle;
    EFI_BLOCK_IO *Blk;
    EFI_GUID GUID;
    UINT32 CalcBlock, ConfBlock, RealBlock;
#ifdef __DEBUG__
    bool EnableVerbose;
#endif
}rawenv_t, *rawenv;

void EnableVerbose(rawenv re);
void DisableVerbose(rawenv re);

UINT32 getblocksize(rawenv re);
void setblocksize(rawenv re, UINT32 new);

rawenv startup(EFI_GUID GUID, EFI_GUID altGUID, UINT32 configuredBlockSize);

void writebytes(rawenv re, void *data, UINTN bytepos, UINTN nbytes);
void *writeblocks(rawenv re, void *data, LBA pos, UINTN bytes);

void *readbytes(rawenv re, LBA pos, UINT16 offset, UINTN nbytes);
void *readblocks(rawenv re, LBA pos, UINTN bytes);

void dispose(rawenv re);