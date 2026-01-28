#pragma once

// #include "stdfile.h"
#include "./kernel/public/public/math/int/_bool.h"
#include "./kernel/lib32/stdio/stdio.h"

#define default_max 2000

#define FATl_t uint32_t
#define MASK_GEN(BITS) (1 << (BITS - 1))

#define FAT_LBA(drive) ((((drive).reserved_sectors + 1) * (drive).bytes_per_sector * (drive).sectors_per_cluster))
#define BASEMAP_LBA(drive) (FAT_LBA(drive) + ((drive).fat_count * (drive).sectors_per_fat * (drive).bytes_per_sector))
#define DDATA_LBA(drive) (BASEMAP_LBA(drive) + (20 * (drive).bytes_per_sector))

#define USED_FAT (FAT[0].segment)

// What do I need to do:
//*		Create a file,
//*		Delete a File,
//*		Open a file from a Directory,
//*		Swap a File between 2 Directories,
//*		Read file data to terminal,
//*		Write to a file from a terminal,
//*		Write from a drive file into the open file,
//*		List all the items in a Directory

#define bytes_per_sector_ 128

#define LBAget(ull) (((unsigned long long)(ull)) & 0x7FFFFFFFFFFFFFFF)
#define FATused(ull) (((unsigned long long)(ull)) & 0x1000000000000000)
#define FATusedt(ull) (((unsigned long long)(ull)) ^ 0x1000000000000000)
typedef struct FAT_e{
	// ALWAYS clear top-most bit,
	// This is the LBA of the file's base Sector.
	uint64_t LBA;
}FAT_e;

// 0b0000000000000000
#define FrATBASEsector_Ring(base) ((uint8_t)((base).flags & 0b0000000000000111))
#define FrATBASEsector_ReadEnable(base) ((uint8_t)((base).flags & 0b0000000000001000))
#define FrATBASEsector_WriteEnable(base) ((uint8_t)((base).flags & 0b0000000000010000))
#define FrATBASEsector_TransferFinish(base) ((uint8_t)((base).flags & 0b0000000000100000))
#define FrATBASEsector_TransferDirectionBit(base) ((uint8_t)((base).flags & 0b0000000001000000))
#define FrATBASEsector_ReadFinish(base) ((uint8_t)((base).flags & 0b0000000010000000))
#define FrATBASEsector_WriteFinish(base) ((uint8_t)((base).flags & 0b0000000100000000))
#define FrATBASEsector_IsArchive(base) ((uint8_t)((base).flags & 0b0000001000000000))
#define FrATBASEsector_IsDirectory(base) ((uint8_t)((base).flags & 0b0000010000000000))
#define FrATBASEsector_IsExecutable(base) ((uint8_t)((base).flags & 0b0000100000000000))
#define FrATBASEsector_Reserved(base) ((uint8_t)((base).flags & 0b0000000000000000))

#define FrATBASEsector_RingSet(base, bit) ((uint8_t)((base).flags |= (bit & 0b00000111)))
#define FrATBASEsector_ReadEnableSet(base, bit) ((uint8_t)((base).flags |= ((bit)? 0b0000000000001000: 0x0)))
#define FrATBASEsector_WriteEnableSet(base, bit) ((uint8_t)((base).flags |= ((bit)? 0b0000000000010000: 0x0)))
#define FrATBASEsector_TransferFinishSet(base, bit) ((uint8_t)((base).flags |= ((bit)? 0b0000000000100000: 0x0)))
#define FrATBASEsector_TransferDirectionBitSet(base, bit) ((uint8_t)((base).flags |= ((bit)? 0b0000000001000000: 0x0)))
#define FrATBASEsector_ReadFinishSet(base, bit) ((uint8_t)((base).flags |= ((bit)? 0b0000000010000000: 0x0)))
#define FrATBASEsector_WriteFinishSet(base, bit) ((uint8_t)((base).flags |= ((bit)? 0b0000000100000000: 0x0)))
#define FrATBASEsector_IsArchiveSet(base, bit) ((uint8_t)((base).flags |= ((bit)? 0b0000001000000000: 0x0)))
#define FrATBASEsector_IsDirectorySet(base, bit) ((uint8_t)((base).flags |= ((bit)? 0b0000010000000000: 0x0)))
#define FrATBASEsector_IsExecutableSet(base, bit) ((uint8_t)((base).flags |= ((bit)? 0b0000100000000000: 0x0)))
#define FrATBASEsector_ReservedSet(base, bit) ((uint8_t)((base).flags |= (bit & 0b01111111)))

#define FBsector_entries ((bytes_per_sector_ - 16) / sizeof(FAT_e))
#define FrATbs_SIZE(sector) ((FBsector_entries * sizeof(FAT_e)) * ((Fsector_entries * sizeof(FAT_e)) * (sector).Num_extensionaddresses * (sector).depth))

typedef struct FrATBASEsector{
	union{
		struct{
			uint32_t FATindex;
			union{
				struct{
					uint8_t last_sector_used_bytes;
					uint8_t dataaddresses_start;
					// [0-2]: Ring
					// [3]: Read Enable
					// [4]: Write Enable
					// [5]: Transfer Finish
					// [6]: Transfer Direction Bit
					// [7]: Read Finish
					// [8]: Write Finish
					// [9]: Is Archive
					// [10]: Is Directory
					// [11]: Is Executable
					// [12-15]: Reserved
					uint16_t flags;
				};
				uint32_t jam;
			};
			union{
				struct{
					uint16_t Num_extensionaddresses;
					// For most files, this is 1
					uint8_t depth;
				};
				uint32_t bottom;
			};
			uint32_t reserved;
		};
		uint64_t sector[2];
	};
	union{
		uint8_t data[FBsector_entries * sizeof(FAT_e)];
		FAT_e entries[FBsector_entries];
	};
}__attribute__((packed)) FrATBASEsector;

#define Fsector_entries ((bytes_per_sector_ - 8) / sizeof(FAT_e))
#define FrATs_SIZE(sector) ((Fsector_entries * sizeof(FAT_e)) * (1 + ((sector).Num_extensionaddresses * (sector).depth)))
// Date-Time
#define FrATs_DATI(flags) ((flags) & 0b00000001)
#define FrATs_NAME(flags) ((flags) & 0b00000010)
#define FrATs_DATA(flags) ((flags) & 0b00000100)
#define FrATs_DATIs(flags) ((flags) |= 0b00000001)
#define FrATs_NAMEs(flags) ((flags) |= 0b00000010)
#define FrATs_DATAs(flags) ((flags) |= 0b00000100)
// #define SECTORMETA_NAME(flags) ((flags) & 0b00000001)

typedef struct FrATsector{
	union{
		struct{
			uint32_t FATindex;
			union{
				struct{
					uint8_t Num_extensionaddresses;
					uint8_t dataaddresses_start;
					uint8_t depth;
					// [0]: Has Date-Time Metadata
					// [1]: Has Name Metadata
					// [2]: Has Data
					// [3 - 7]: Reserved
					uint8_t flags;
				};
				uint32_t jam;
			};
		};
		uint64_t sector;
	};
	union{
		uint8_t data[Fsector_entries * sizeof(FAT_e)];
		FAT_e entries[Fsector_entries];
	};
}__attribute__((packed)) FrATsector;

FAT_e MallocSectorSMAP(uint32_t max);
FAT_e MallocSectorDDATA(uint32_t max);
void envPrepare();

void createFile(char *name, char *mode, size_t size);

extern const char DIGITS[];
void byte_to_base(uint8_t value, uint32_t base, char out[8]);

// //! Written by AI
static inline void FrATbs_ResolveAttributes(uint32_t size, uint8_t *depth, uint16_t *extension_addresses);
void fnv1a_120(const void *data, size_t len, uint8_t out[15]);
