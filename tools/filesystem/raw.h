#pragma once

#include "tools.h"

#define GPTsig "EFI PART"
#define GPT_LBA 1

typedef size_t LBA;

typedef struct miniGPT{
	char sig[8];
	uint32_t rev;
	uint32_t hSize;
	uint32_t hChecksum;
	uint32_t r;
	LBA localLBA;
	LBA alternateLBA;
	LBA fUsable;
	LBA lUsable;
	_GUID dGUID;
	LBA partEntryLoc;
	uint32_t nPartEntries;
	uint32_t partEntrySize;
	uint32_t partArrayChecksum;
}__attribute__((packed)) miniGPT;
typedef struct GPTentry{
	_GUID GUID;
	_GUID uGUID;
	LBA sLBA;
	LBA eLBA;
	size_t attr;
	GPTeNSTR name;
}__attribute__((packed)) GPTentry;

typedef struct partdim{
	GPTentry e;
	LBA base, high;
}partdim;

typedef struct rawenv_t{
    /// @brief If true then the Interface is a Block IO Interface;
    FILE *file;
	char *path;
    uint32_t CalcBlock, ConfBlock, RealBlock;
#ifdef __DEBUG__
    bool EnableVerbose;
#endif
}rawenv_t, *rawenv;

void EnableVerbose(rawenv re);
void DisableVerbose(rawenv re);

uint32_t getblocksize(rawenv re);
void setblocksize(rawenv re, uint32_t new);

rawenv startup(char *path, uint32_t configuredBlockSize);

void writebytes(rawenv re, void *data, size_t bytepos, size_t nbytes);
void *writeblocks(rawenv re, void *data, LBA pos, size_t bytes);

void *readbytes(rawenv re, LBA pos, uint16_t offset, size_t nbytes);
void *readblocks(rawenv re, LBA pos, size_t bytes);

void dispose(rawenv re);