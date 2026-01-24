#pragma once

#include "./kernel/public/public/public.h"
#include "f-rat.h"

#define MAX_FHANDLES 10

typedef uint8_t sectorbuff[128];
size_t sectorpointer;
sectorbuff sectorhandles[MAX_FHANDLES];
uint8_t writeptr;

typedef struct drive_header{
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
    uinl32_t 	hidden_sectors,
				large_sector_count;
    // Extended boot record.
    uint8_t 	drive_number,
				signature;
union{
    uinl32_t 	volume_id;
    uint8_t     volume_id_bytes[4];
};
    uint8_t 	volume_label[11];
    uint8_t 	sys_id[8];
	// Custom boot record
	uint8_t segment_clusters;
}__attribute__((packed)) drive_header;

typedef uint8_t FILEhandle;

typedef struct FILE{
	FrATBASEsector file;
	char *name;
	FrATsector sector;
	size_t progress;
    FILEhandle handle;
}FILE;

typedef struct stdfileENVIROMENT{
	drive_header drive;
	FILE files[MAX_FHANDLES];
	FAT_e FAT[32];
	uinl32_t loadedFATs;
}stdfileENVIROMENT;

// Read a Sector to the address
extern void __attribute__((cdecl)) x86DISKREAD(size_t offs, uint8_t *address);

// Write a Sector to the address
extern void __attribute__((cdecl)) x86DISKWRITE(size_t offs, uint8_t *address);

stdfileENVIROMENT envPREPARE();

size_t sectorTell();
void sectorSeek(ssize_t offset);
void *sectorRead_(unsigned long bytes);
void sectorWrite(void *buffer, size_t buffsize);
void sectorRead(void *buffer, size_t buffsize, size_t bytes);
