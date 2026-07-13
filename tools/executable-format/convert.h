#pragma once

#include "exec.h"
#include "pe.h"
#include "ref/blake2.h"
#include "ref/blake2-impl.h"
#include "tools/json/minijson.h"

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

#define SpecialSectionN(N)	SpecialSection##N
#define SpecialSection0	".edata\0\0"
#define SpecialSection1	".idata\0\0"
#define SpecialSection2	".reloc\0\0"


char *GenericError(char *str, uint32_t val);
char* PoolGetPath(const char* fileSnippet);
void InitImportSection(
	char *path, ExpandedPeExecutable *Image, 
	SectionNameBe OutName, uint32_t BaseRelocationTableOffset
);