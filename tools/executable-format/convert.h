#pragma once

#include "exec.h"
#include "pe.h"
#include "ref/blake2.h"
#include "ref/blake2-impl.h"
#include "tools/json/minijson.h"

extern char *logfile;

#define STR8_TO_UINT64(str)	(								\
	((uint64_t)((const unsigned char*)(str))[0] <<  0)	|	\
	((uint64_t)((const unsigned char*)(str))[1] <<  8)	|	\
	((uint64_t)((const unsigned char*)(str))[2] << 16)	|	\
	((uint64_t)((const unsigned char*)(str))[3] << 24)	|	\
	((uint64_t)((const unsigned char*)(str))[4] << 32)	|	\
	((uint64_t)((const unsigned char*)(str))[5] << 40)	|	\
	((uint64_t)((const unsigned char*)(str))[6] << 48)	|	\
	((uint64_t)((const unsigned char*)(str))[7] << 56)	 	\
)

typedef struct{
    RelativeVirtualOffset *RVAs;
    size_t Count;
    size_t Capacity;
}EATReferenceList;
typedef struct{
    RelativeVirtualOffset *RVAs;
    size_t Count;
    size_t Capacity;
}IATReferenceList;

#define SpecialSectionN(N)	SpecialSection##N
#define SpecialSection0	".edata\0\0"
#define SpecialSection1	".idata\0\0"
#define SpecialSection2	".reloc\0\0"

void *memdup(void *mem, size_t n);
char* PoolGetPath(const char* fileSnippet);
BeSectionFlags ConvertPeSectionFlagsBe(PeSectionCharacteristics Characteristics);

bool InitImportSection(char *path, ExpandedPeExecutable *Image, SectionNameBe OutName);
bool InitExportSection(const char *path, ExpandedPeExecutable *Image, SectionNameBe Name);
bool InitResourceSection(const char *path, SectionNameBe Name, 
	uint64_t SectionSize, BeResourceConfigurator *Root
);
bool InitExceptionSection(const char *path, ExpandedPeExecutable *Image, SectionNameBe Name);
bool InitRelocationSection(const char *path, ExpandedPeExecutable *Image, SectionNameBe Name);
