#pragma once

#include "./kernel/lib32/public/kernpublic.h"

#define KERNEL_ID (0x4446788592ull)
#define staticInterruptTableSize 5
#define dynamicInterruptTableSize 10
#define standardHeaderIDSize 10
#define standardThreadMax 4
#define maxTranslationTables 10
#define maxLoadCMDSIZE 128

typedef struct executableBinary{
	struct FAT{
		stubFAT BINARY;
		stubFAT REALLOCMASK;
	}FAT;
	// The address where the address of in parameters should be stored.
	size_t BINARYIN;
	// The adress where the address of the output should be stored.
	size_t BINARYOUT;
}executableBinary;

/// @brief [0]: Byte Offset, [1]: Size
typedef size_t stubFAT[2];
typedef uint8_t versionArtifacts[10];
typedef struct executableImage{
	versionArtifacts versionArtifacts;
	programLoadHeader header;
	char loaderAlias[aliasLen];
	struct FAT{
		stubFAT DATABINARY;
		stubFAT REALLOCMASK;
		stubFAT RESOURCES;
		stubFAT IMPORT;
		stubFAT EXPORT;
		stubFAT INTERNALDATA;
	}FAT;
}executableImage;

typedef struct programLoadHeader{
	char ploadCommand[maxLoadCMDSIZE];
	char ploaderAlias[aliasLen];
	struct COMPONENTMAP{
		size_t REALLOCMASK, TEXT, DATA, BSS;
	}COMPONENTMAP;
}programLoadHeader;

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
            // The true association of the interrupt, unique within the Table
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

typedef struct standardKernelHeader{
    standardHeader Child;
	struct standardTables{
		standardInterruptEntry staticInterruptTable[staticInterruptTableSize];
		standardInterruptEntry dynamicInterruptTable[staticInterruptTableSize];
		standardTranslationTable translationTable[maxTranslationTables];
	}standardTables;
}standardKernelHeader;

typedef struct standardChildHeader{
    uint8_t ParentID[standardHeaderIDSize], ID[standardHeaderIDSize];
	void *CODE;
    uint8_t loadedCODEPages;
    void *myDATA;
	uint32_t DATAbytes;
}standardChildHeader;

typedef struct standardHeader{
    uint8_t ID[standardHeaderIDSize];
    FILE *program;
    void *CODE;
    uint8_t loadedCODEPages;
    void *DATA;
    uint8_t loadedDATAPages;
    struct standardChildren{
        stdfileENVIROMENT stdfile;
		standardChildHeader threads[standardThreadMax];
    }standardChildren;
}standardHeader;

uint32_t loadedPrograms;
standardHeader **Programs;

standardHeader getCODEBase(void *CODE);
standardHeader getDATABase(void *DATA);