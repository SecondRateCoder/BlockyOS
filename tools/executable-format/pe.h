#include "exec.h"
#include "struct.h"

#define DecodePeExecutableHeader(PTR)																								\
	PeHeader *PH = (PTR);																											\
	Pe32OptionalHeader *POH = (PTR) + sizeof(PeHeader);																				\
	Pe32PlusOptionalHeader *POHPlus = (PTR) + sizeof(PeHeader);																		\
	PeRVAnSize *RVAs = (PTR) + sizeof(PeHeader) + (POH->mMagic == Pe32? sizeof(Pe32OptionalHeader): sizeof(Pe32PlusOptionalHeader));\
	PeImageSectionHeader *PISHs = (PTR) + (POH->mMagic == Pe32? sizeof(Pe32OptionalHeader) + (POH->mNumberOfRvaAndSizes * sizeof(PeRVAnSize)): sizeof(Pe32PlusOptionalHeader) + (POHPlus->mNumberOfRvaAndSizes * sizeof(PeRVAnSize)));
#define RDecodePeExecutableHeader(PTR)																					\
	PH = (PTR);																											\
	POH = (PTR) + sizeof(PeHeader);																						\
	POHPlus = (PTR)  +sizeof(PeHeader);																					\
	RVAs = (PTR) + sizeof(PeHeader) + (POH->mMagic == Pe32? sizeof(Pe32OptionalHeader): sizeof(Pe32PlusOptionalHeader));\
	PISHs = (PTR) + (POH->mMagic == Pe32? sizeof(Pe32OptionalHeader) + (POH->mNumberOfRvaAndSizes * sizeof(PeRVAnSize)): sizeof(Pe32PlusOptionalHeader) + (POHPlus->mNumberOfRvaAndSizes * sizeof(PeRVAnSize)));

void *ReadPeExecutableHeader(const char *path);
void *ReadSectionPe(char *path, void *header, char name[8]);
PeImageSectionHeader *FindSectionPe(void *header, char name[8]);
ExpandedPeExecutable *ExpandPeExecutableFormat(const char *path);
uint32_t RvaToFileOffsetPe(uint32_t rva, PeImageSectionHeader *Section);
void *GetAtRVAFromSectionDataPe(uint32_t RVA, char name[8], void *data, void *header);
void *ReadAtRVAFromSectionPe(uint32_t RVA, uint32_t Size, char name[8], char *path, void *header);