#pragma once

#include "kernel/lib32/generic/standard.h"
#include "kernel/lib32/stdprogram/stdprogram.h"

// The LBA where the FAT Tables starts
#define FATLBA(drive) (((drive).reserved_sectors + 1))
// The LBA where the BaseMap address starts
#define BASEMAPLBA(drive) (FATLBA(drive) + ((drive).fat_count * (drive).sectors_per_fat))

#define DDATALBA(drive) (BASEMAPLBA(drive) + FATMAX)

#define sectorBytes 256
#define MAX_FHANDLES 5
#define maxSingleRead 5
#define undefinedLBAentries ((sectorBytes - sizeof(undefinedSectorHeader)) / sizeof(packedLBA))
#define undefinedDATAbytes ((sectorBytes - sizeof(undefinedSectorHeader)) / sizeof(DATA_t))
#define DATABUFFER_t uint8_t *
#define FATMAX 281474976710655ull

#define kB (1024)
#define mB (kB * kB)
#define gB (mB * kB)
#define tB (gB * kB)

typedef uint32_t FAT_t;
typedef uint8_t DATA_t; 
typedef uint8_t FILEhandle;
typedef DATA_t SECTOR[sectorBytes / sizeof(DATA_t)];
typedef size_t LBA;
typedef DATA_t IDCODE[12];

typedef enum LBAFLAGS{
    LBAFLAGS_POSITIVE = 0x0,
    LBAFLAGS_unDEFINED = 0x0,
    LBAFLAGS_NEGATIVE = 0x1,
    LBAFLAGS_DEFINED = 0x2,
    LBAFLAGS_USED = 0x2,
}LBAFLAGS;
typedef struct CHS{
    uint16_t Cylinder,
             Head,
             Sector;
}PACKEDSTRUCT CHS;

#define getLBA(LBA)         	((uint32_t)(((uint32_t)(LBA).high << 16) | (LBA).low))
#define storeLBA(LBA, VALUE)	(LBA).high = (uint8_t)(((VALUE) >> 16) & 0xFF);		(LBA).low = (uint16_t)((VALUE) & 0xFFFF);
#define createLBA(FLAGS, VALUE) {   \
    (uint8_t)(FLAGS),               \
    (((VALUE) > 16) & 0xFF),        \
	((VALUE) & 0xFFFF)				\
}
typedef struct packedLBA{
    union{
        struct{
            uint8_t flags;
            uint8_t high;
            uint16_t low;
        };
        uint32_t LBA;
    };
}PACKEDSTRUCT packedLBA;
typedef struct DepthWidth_t{
    union{
        struct{
            uint8_t depth;
            uint8_t parallels;
        };
        uint16_t address;
    };
}DepthWidth_t;

typedef struct undefinedSectorHeader{
    union{
        struct{
            uint16_t flags;
			IDCODE undefinedCode;
            uint8_t padding[2];
        };
        uint32_t Header;
    };
}PACKEDSTRUCT undefinedSectorHeader;
typedef struct undefinedSector{
    undefinedSectorHeader header;
    union{
        DATA_t DATA[undefinedDATAbytes];
        packedLBA Adresses[undefinedLBAentries];
    };
}PACKEDSTRUCT undefinedSector;

typedef struct FILE{
    packedLBA FAT;
	undefinedSector file;
    FILEhandle handle;
    packedLBA Progress;
}FILE;

typedef struct driveHeader{
    uint8_t     BOOTINSTRUCTION[3];
    uint8_t 	OEM_ID[8];
    uint16_t 	bytes_per_sector;
    uint8_t 	sectors_per_cluster,
				reserved_sectors,
				fat_count;
    uint16_t 	dir_entries_count,
				total_sectors;
    uint8_t 	media_descriptor_type;
    uint16_t 	sectors_per_fat,
				sectors_per_track,
				heads;
    uint32_t 	hidden_sectors,
				large_sector_count;
    // Extended boot record.
    uint8_t 	drive_number,
				signature;
union{
    uint32_t 	volume_id;
    uint8_t     volume_id_bytes[4];
};
    uint8_t 	volume_label[11];
    uint8_t 	sys_id[8];
	// Custom boot record
	uint8_t segment_clusters;
}PACKEDSTRUCT driveHeader;

unsigned __ = sizeof(driveHeader);

#define stdfileFATsize (sectorBytes / sizeof(packedLBA))
typedef struct stdfileENVIROMENT{
	driveHeader drive;
	packedLBA FAT[stdfileFATsize];
	uint32_t FATCHUNKS;
	FILE files[MAX_FHANDLES];
    bool usedFiles[MAX_FHANDLES];
}stdfileENVIROMENT;

// Read a Sector-size chunk to the address.
extern void ASMCALL _x86DISKREAD(LBA address, uint8_t *out);
// Write a Sector-sized chunk to the disk from the address.
extern void ASMCALL _x86DISKWRITE(LBA address, uint8_t *in);

void envInit(stdfileENVIROMENT *env);
void ASMCALL getDrive(driveHeader *out);
FILE *getFile(FILEhandle *handle);
FILE *getUsable();
void closeFile(uint8_t file);
void *flatRead(uint32_t LBA, uint8_t sectors);

// Update FILE with Read address.
extern bool fUpdate(uint8_t FILE, size_t new_addr, bool update);