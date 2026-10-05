#pragma once

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

#define PeChar8ToU64(Char8)		*((uint64_t *)(Char8))

#define PeHeaderOffsetAddress		0x3C
#define PeDefaultNDataDirectories	16
#define PeHeaderMagicStr			((char[4]){'P', 'E', 0, 0})
#define PeHeaderMagicU32			(((UINT32)0) << 24) | (((UINT32)0) << 16) | (((UINT32)'E') << 8) | ((UINT32)'P')


#define DecodePeExecutableHeader(PTR)																								\
	PeHeader *PH = (void *)(PTR);																											\
	Pe32OptionalHeader *POH = (void *)(PTR) + sizeof(PeHeader);																				\
	Pe32PlusOptionalHeader *POHPlus = (void *)(PTR) + sizeof(PeHeader);																		\
	PeRVAnSize *RVAs = (void *)(PTR) + sizeof(PeHeader) + (POH->mMagic == Pe32? sizeof(Pe32OptionalHeader): sizeof(Pe32PlusOptionalHeader));\
	PeImageSectionHeader *PISHs = (void *)(PTR) + sizeof(PeHeader) + PH->mSizeOfOptionalHeader;
#define RDecodePeExecutableHeader(PTR)																					\
	PH = (void *)(PTR);																											\
	POH = (void *)(PTR) + sizeof(PeHeader);																						\
	POHPlus = (void *)(PTR)  +sizeof(PeHeader);																					\
	RVAs = (void *)(PTR) + sizeof(PeHeader) + (POH->mMagic == Pe32? sizeof(Pe32OptionalHeader): sizeof(Pe32PlusOptionalHeader));\
	PISHs = (void *)(PTR) + sizeof(PeHeader) + PH->mSizeOfOptionalHeader;

void *ReadPeExecutableHeader(socket_t *sck);
void *ReadSectionPe(socket_t *sck, void *header, char name[8]);
void *LoadSectionPe(socket_t *sck, void *header, char name[8]);
PeImageSectionHeader *FindSectionPe(void *header, char name[8]);
ExpandedPeExecutable *ExpandPeExecutableFormat(socket_t *sck);
UINT32 RvaToFileOffsetPe(UINT32 rva, PeImageSectionHeader *Section);
void *GetAtRVAFromSectionDataPe(UINT32 RVA, char name[8], void *data, void *header);
void *ReadAtRVAFromSectionPe(socket_t *sck, UINT32 RVA, UINT32 Size, char name[8], void *header);
void PrintPeExecutableFormat(ExpandedPeExecutable *EXE, UINT32 DataMax);

LoadedPeExecutable *LoadExecutable(socket_t *drive, bool Info, UINT32 InfoMax, const char *file);