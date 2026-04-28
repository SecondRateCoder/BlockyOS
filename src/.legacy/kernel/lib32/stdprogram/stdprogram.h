#pragma once

#include "kernel/lib32/generic/standard.h"
#ifdef LOCALSTANDARDFILE
#include "Boot/Legacy/stage2/localfile/stdfile.h"
#else
#include "kernel/lib32/stdfile/stdfile.h"
#endif

#define KERNEL_ID (0x4446788592ull)
#define staticInterruptTableSize 5
#define dynamicInterruptTableSize 10
#define standardHeaderIDSize 10
#define standardThreadMax 4
#define maxTranslationTables 10
#define maxLoadCMDSIZE 128

/// @brief [0]: Byte Offset, [1]: Size
typedef size_t stubFAT[2];
typedef uint8_t versionArtifacts[6];

typedef struct programLoadHeader{
	char ploadCommand[maxLoadCMDSIZE];
	char ploaderAlias[aliasLen];
	struct COMPONENTMAP{
		size_t REALLOCMASK, TEXT, DATA, BSS;
	}COMPONENTMAP;
}programLoadHeader;

typedef struct executableBinary{
	struct exeBFAT{
		stubFAT BINARY;
		stubFAT REALLOCMASK;
	}exeB_FAT;
	// The address where the address of in parameters should be stored.
	size_t BINARYIN;
	// The adress where the address of the output should be stored.
	size_t BINARYOUT;
}executableBinary;

typedef struct executableImage{
	versionArtifacts versionArtifacts;
	programLoadHeader header;
	char loaderAlias[aliasLen];
	struct exeIFAT{
		stubFAT DATABINARY;
		stubFAT REALLOCMASK;
		stubFAT RESOURCES;
		stubFAT IMPORT;
		stubFAT EXPORT;
		stubFAT INTERNALDATA;
	}exeIFAT;
}executableImage;

//* Program Headers
typedef struct standardTranslationTable{
	union{
		uint32_t type, flags;
	};
	void *src;
	void *child;
}standardTranslationTable;

typedef struct standardInterruptEntry{
    // The associated Interrupt Vector and Code
    union{
        struct{
            // The true association of the interrupt, unique memwithin the Table
            uint16_t ID;
            // The Real Interrupt Vector associated to the vector
            uint16_t code;
        };
        uint32_t vector;
    };
    void *BASE;
    uint32_t bytes;
    uint8_t flags;
}standardInterruptEntry;

typedef struct standardChildHeader{
    uint8_t ParentID[standardHeaderIDSize], ID[standardHeaderIDSize];
	void *CODE;
    uint8_t loadedCODEPages;
    void *DATA;
	uint8_t loadedDATAPages;
}standardChildHeader;

typedef struct standardHeader{
    uint8_t ID[standardHeaderIDSize];
    char program[PATHMAX];
    struct PAGES{
        void *CODE;
        uint8_t loadedCODEPages;
        void *DATA;
        uint8_t loadedDATAPages;
    }PAGES;
    struct standardChildren{
        stdfileENVIROMENT stdfile;
		standardChildHeader threads[standardThreadMax];
    }standardChildren;
}standardHeader;

standardHeader * ASMCALL getCODEBase(void *CODE);
standardHeader * ASMCALL getDATABase(void *DATA);

typedef struct standardKernelHeader{
    standardHeader Child;
	struct standardTables{
		standardInterruptEntry staticInterruptTable[staticInterruptTableSize];
		standardInterruptEntry dynamicInterruptTable[staticInterruptTableSize];
		standardTranslationTable translationTable[maxTranslationTables];
	}standardTables;
}standardKernelHeader;

extern standardHeader * ASMCALL getStandardHeader32(void *CODE, void *DATA);
extern void * ASMCALL getEIP();

typedef struct systemProgramEnviroment{
    uint8_t loaded;
    standardHeader *Programs[32];
}systemProgramEnviroment;
extern systemProgramEnviroment systemState;

char *getEnv(char alias[aliasLen]);