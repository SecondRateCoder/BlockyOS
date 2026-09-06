#pragma once

#include "exec.h"
#include "struct.h"

#define PeDumpVolume			32
#define PeDumpVolumeLine		8

#define PeCodeSection			(char[8]){".text"}
#define PeDefDataSection		(char[8]){".data"}
#define PeUDefDataSection		(char[8]){".bss"}
#define PeExportSection			(char[8]){".edata"}
#define PeImportSection			(char[8]){".idata"}
#define PeLinkerOptionSection	(char[8]){".drectve"}
#define PeExceptionInfoSection	(char[8]){".pdata"}
#define PeRDefDataSection		(char[8]){".rdata"}
#define PeRelocDataSection		(char[8]){".reloc"}
#define PeResourceSection		(char[8]){".rsrc"}

#define PeHeaderOffsetAddress		0x3C
#define PeDefaultNDataDirectories	16
#define PeHeaderMagicStr			((char[4]){'P', 'E', 0, 0})
#define PeHeaderMagicU32			(((uint32_t)0) << 24) | (((uint32_t)0) << 16) | (((uint32_t)'E') << 8) | ((uint32_t)'P')


#define DecodePeExecutableHeader(PTR)																								\
	PeHeader *PH = (void *)(PTR);																											\
	Pe32OptionalHeader *POH = (void *)(PTR) + sizeof(PeHeader);																				\
	Pe32PlusOptionalHeader *POHPlus = (void *)(PTR) + sizeof(PeHeader);																		\
	PeRVAnSize *RVAs = (void *)(PTR) + sizeof(PeHeader) + (POH->mMagic == Pe32? sizeof(Pe32OptionalHeader): sizeof(Pe32PlusOptionalHeader));\
	PeImageSectionHeader *PISHs = (void *)(PTR) + (POH->mMagic == Pe32? sizeof(Pe32OptionalHeader) + (POH->mNumberOfRvaAndSizes * sizeof(PeRVAnSize)): sizeof(Pe32PlusOptionalHeader) + (POHPlus->mNumberOfRvaAndSizes * sizeof(PeRVAnSize)));
#define RDecodePeExecutableHeader(PTR)																					\
	PH = (void *)(PTR);																											\
	POH = (void *)(PTR) + sizeof(PeHeader);																						\
	POHPlus = (void *)(PTR)  +sizeof(PeHeader);																					\
	RVAs = (void *)(PTR) + sizeof(PeHeader) + (POH->mMagic == Pe32? sizeof(Pe32OptionalHeader): sizeof(Pe32PlusOptionalHeader));\
	PISHs = (void *)(PTR) + (POH->mMagic == Pe32? sizeof(Pe32OptionalHeader) + (POH->mNumberOfRvaAndSizes * sizeof(PeRVAnSize)): sizeof(Pe32PlusOptionalHeader) + (POHPlus->mNumberOfRvaAndSizes * sizeof(PeRVAnSize)));

void *ReadPeExecutableHeader(const char *path);
void *ReadSectionPe(char *path, void *header, char name[8]);
PeImageSectionHeader *FindSectionPe(void *header, char name[8]);
ExpandedPeExecutable *ExpandPeExecutableFormat(const char *path);
uint32_t RvaToFileOffsetPe(uint32_t rva, PeImageSectionHeader *Section);
void *GetAtRVAFromSectionDataPe(uint32_t RVA, char name[8], void *data, void *header);
char *ReadStringAtRVAFromSectionPe(uint32_t RVA, char name[8], char *path, void *header);
void *ReadAtRVAFromSectionPe(uint32_t RVA, uint32_t Size, char name[8], char *path, void *header);
void DumpPe(ExpandedPeExecutable *EXE);