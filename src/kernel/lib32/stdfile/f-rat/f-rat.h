#pragma once

#include "./kernel/lib32/stdfile/stdfile.h"
#include "./kernel/lib32/generic/standard.h"

#define defaultMax 2000
#define PATHMAX 256
#define DEPTHMAX 56
#define PARALLELMAX 20
#define sectorBytes 256

// What do I need to do:
//*		Create a file,
//*		Delete a File,
//*		Open a file from a Directory,
//*		Swap a File between 2 Directories,
//*		Read file data to terminal,
//*		Write to a file from a terminal,
//*		Write from a drive file into the open file,
//*		List all the items in a Directory

typedef enum FrATFLAGS{
    FrATFLAGS_unDEFINED = 						0x0000,
	FrATFLAGS_BASE = 							0x1000,
	FrATFLAGS_CHILD = 							0x2000,
    FrATFLAGS_DEFINED =                         0x3000,

    FrATBASEFLAGS_RING0 =                       0x0000,
    FrATBASEFLAGS_RING1 =                       0x0001,
    FrATBASEFLAGS_RING2 =                       0x0002,
    FrATBASEFLAGS_RING3 =                       0x0003,

    FrATBASEFLAGS_READ =                        0x0004,
    FrATBASEFLAGS_WRITE =                       0x0008,
    FrATBASEFLAGS_TRANSFERFINISH =              0x0010,
    FrATBASEFLAGS_TRANSFERDIRECTION =           0x0020,
    FrATBASEFLAGS_READFINISH =                  0x0040,
    FrATBASEFLAGS_WRITEFINISH =                 0x0080,
    FrATBASEFLAGS_ARCHIVE =                     0x0100,
    FrATBASEFLAGS_DIRECTORY =                   0x0200,
    FrATBASEFLAGS_EXECUTABLE =                  0x0400,
    FrATBASEFLAGS_RESERVED =                    0x0800,

	FrATCHILDFLAGS_DATETIME = 					0x0001,
	FrATCHILDFLAGS_NAME = 						0x0002,
	FrATCHILDFLAGS_DATA = 						0x0004,

    FrATFUNCFLAGS_APPROX =                      0x0001
}FrATFLAGS;

#define baseLBAentries 	((sectorBytes - sizeof(FrATBASEheader)) / sizeof(packedLBA))
#define baseDATAbytes 	((sectorBytes - sizeof(FrATBASEheader)) / sizeof(DATA_t))
typedef struct FrATBASEheader{
	uint16_t flags;
	IDCODE ID;
	SHA256HASH nameCode;
	DepthWidth_t Bounds;
	size_t trueSize;
    uint8_t allocatedDepth;
}PACKEDSTRUCT FrATBASEheader;
typedef struct FrATBASE{
	FrATBASEheader header;
	union{
		packedLBA Addresses[baseLBAentries];
		DATA_t DATA[baseDATAbytes];
	};
}FrATBASE;

#define childLBAentries 	((sectorBytes - sizeof(FrATCHILDheader)) / sizeof(packedLBA))
#define childDATAbytes 		((sectorBytes - sizeof(FrATCHILDheader)) / sizeof(DATA_t))
typedef struct FrATCHILDheader{
	uint16_t flags;
	/// @brief This is a Hash of the parent's childCode + it's index on the same depth
	IDCODE childCode;
	DepthWidth_t location;
}PACKEDSTRUCT FrATCHILDheader;
typedef struct FrATCHILD{
	FrATCHILDheader header;
	union{
		packedLBA Addresses[childLBAentries];
		DATA_t DATA[childDATAbytes];
	};
}FrATCHILD;

size_t FrATSIZE(uint8_t num_parallels, uint8_t max_depth, uint16_t chunk_bytes, uint32_t unused_bytes);
uint8_t FrATDEPTH(size_t size, uint8_t num_parallels, uint16_t chunk_bytes, uint32_t unused_bytes, bool *approx);
uint8_t FrATPARALLEL(size_t size, uint8_t depth, uint16_t chunk_bytes, uint32_t unused_bytes, bool *approx);
uint32_t *getFree(uint32_t start, uint8_t number, uint16_t maxsearch, IDCODE inCode);

void InitFrATInterrupt(IDTentry *IDT, uint16_t codeSegment);
extern void ASMCALL g_fcreate(char *name, char *mode);