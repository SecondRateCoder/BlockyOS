#pragma once

#include "efi.h"
#include "efilib.h"

#include "Boot/UEFI/tools/tools.h"

#define GPTsig "EFI PART"
#define GPT_LBA 1

typedef UINT64 LBA;

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
	UINT64 attr;
	GPTeNSTR name;
}__attribute__((packed)) GPTentry;

typedef struct rawenv_t{
    /// @brief If true then the Interface is a Block IO Interface;
    bool isPart, EnableVerbose;
	EFI_HANDLE handle;
    EFI_BLOCK_IO *Blk;
    EFI_GUID GUID;
    UINT32 CalcBlock, ConfBlock, RealBlock;
	UINT64 Partition;
}rawenv_t, *rawenv;

void EnableVerbose(rawenv re);
void DisableVerbose(rawenv re);

UINT32 getblocksize(rawenv re);
void setblocksize(rawenv re, UINT32 new);

rawenv startup(EFI_GUID GUID, EFI_GUID altGUID, LBA Partition, UINT32 configuredBlockSize);

void writebytes(rawenv re, void *data, UINT64 bytepos, UINT64 nbytes);
void writeblocks(rawenv re, void *data, LBA pos, UINT64 bytes);

void *readbytes(rawenv re, LBA pos, UINT16 offset, UINT64 nbytes);
void *readblocks(rawenv re, LBA pos, UINT64 bytes);

void dispose(rawenv re);