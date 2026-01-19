#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <inttypes.h>

#define FATl_t uint32_t
#define MASK_GEN(BITS) (1 << (BITS - 1))
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_LBLUE "\x1b[94m"

#define ANSI_COLOR_RESET   "\x1b[0m"

#define ANSI_RED(TEXT) "\n" ANSI_COLOR_RED TEXT ANSI_COLOR_RESET
#define ANSI_GREEN(TEXT) "\n" ANSI_COLOR_GREEN TEXT ANSI_COLOR_RESET
#define ANSI_YELLOW(TEXT) "\n" ANSI_COLOR_YELLOW TEXT ANSI_COLOR_RESET
#define ANSI_BLUE(TEXT) "\n" ANSI_COLOR_BLUE TEXT ANSI_COLOR_RESET
#define ANSI_MAGENTA(TEXT) "\n" ANSI_COLOR_MAGENTA TEXT ANSI_COLOR_RESET
#define ANSI_CYAN(TEXT) "\n" ANSI_COLOR_CYAN TEXT ANSI_COLOR_RESET
#define ANSI_LBLUE(TEXT) "\n" ANSI_COLOR_LBLUE TEXT ANSI_COLOR_RESET

#define FAT_LBA (((drive.reserved_sectors + 1) * drive.bytes_per_sector * drive.sectors_per_cluster))
#define BASEMAP_LBA (FAT_LBA + (drive.fat_count * drive.sectors_per_fat * drive.bytes_per_sector))
#define DDATA_LBA (BASEMAP_LBA + (20 * drive.bytes_per_sector))

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
typedef struct drive_header{
    uint8_t 	BOOT_Instruction[3],
				OEM_ID[8];
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
    uint32_t 	volume_id[4];
    uint8_t 	volume_label[11];
    uint8_t 	sys_id[8];
	// Custom boot record
	uint16_t sectormap_entries;
}__attribute__((packed)) drive_header;

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

#ifdef _WIN32
void enable_ansi(void);
#endif
void IMG_Setup(char *disk_path);

bool closeFile(char *name);
int8_t openFile(char *name);
FAT_e MallocSectorSMAP(uint32_t max);
FAT_e MallocSectorDDATA(uint32_t max);
bool ptrStep(int8_t ptr, uint8_t file);
bool FreeSector(uint8_t file, uint32_t ptr);
void createFile(uint8_t dir, size_t size, char *name, char *mode);
bool createSector(uint8_t file, char *mode, uint8_t data[Fsector_entries]);
bool openSectorRecurse(uint8_t progress, uint8_t ext_item, uint8_t file, uint8_t true_depth);


uint16_t pack_time(struct tm *t);
uint8_t strcheck(char *str, const char c);
extern const char DIGITS[];
void byte_to_base(uint8_t value, uint32_t base, char out[8]);

//! Written by AI
static inline void FrATbs_ResolveAttributes(uint32_t size, uint8_t *depth, uint16_t *extension_addresses);
void fnv1a_120(const void *data, size_t len, uint8_t out[15]);
