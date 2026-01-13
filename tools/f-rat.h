#include <stdio.h>
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

#define ANSI_RED(TEXT) ANSI_COLOR_RED TEXT ANSI_COLOR_RESET
#define ANSI_GREEN(TEXT) ANSI_COLOR_GREEN TEXT ANSI_COLOR_RESET
#define ANSI_YELLOW(TEXT) ANSI_COLOR_YELLOW TEXT ANSI_COLOR_RESET
#define ANSI_BLUE(TEXT) ANSI_COLOR_BLUE TEXT ANSI_COLOR_RESET
#define ANSI_MAGENTA(TEXT)  ANSI_COLOR_MAGENTA TEXT ANSI_COLOR_RESET
#define ANSI_CYAN(TEXT) ANSI_COLOR_CYAN TEXT ANSI_COLOR_RESET
#define ANSI_LBLUE(TEXT) ANSI_COLOR_LBLUE TEXT ANSI_COLOR_RESET

#define FDATA_START (((drive.sectors_per_fat * drive.fat_count) + drive.reserved_sectors + 1) * drive.bytes_per_sector)	// 72 + (((9 * 2) + 2 + 0) * 512) = 10312 bytes
#define FAT_START ((drive.reserved_sectors + 1) * drive.bytes_per_sector)
#define CLUSTERMAP_START (drive.bytes_per_sector * (drive.reserved_sectors + 1 + (drive.fat_count * drive.sectors_per_fat)))

#define USED_FAT (FAT[0].segment)
#define USED_CLUSTERMAP (FAT[0].offset)
#define MAPBASE_SIZE (drive.bytes_per_sector * drive.sectors_per_cluster)
#define MAPBASE_ENTRYSTART(FILE) (FILE->name_len/sizeof(MAP_ext_e) + (FILE->name_len%sizeof(MAP_ext_e)? 1: 0))
// #define MAPBASE_ENTRYINDEXER(FILE, INDEX) FILE->cluster_entries[INDEX + MAPBASE_ENTRYSTART(FILE)]

#define IS_WRITE_FINISH(BYTE) (BYTE & 0x10)
#define IS_POLL_FINISH(BYTE) (BYTE & 0x08)
#define IS_ENCRYPTED(BYTE) (BYTE & 0x02)
#define IS_CORRUPTED(BYTE) (BYTE & 0x01)
#define IS_ARCHIVE(BYTE) (BYTE & 0x04)
#define IS_SYSTEM(BYTE) (BYTE & 0x20)
#define IS_FILE(BYTE) (BYTE & 0x40)
#define IS_DIR(BYTE) (BYTE & 0x80)

// What do I need to do:
//*		Create a file,
//*		Delete a File,
//*		Open a file from a Directory,
//*		Swap a File between 2 Directories,
//*		Read file data to terminal,
//*		Write to a file from a terminal,
//*		Write from a drive file into the open file,
//		List all the items in a Directory,
//		

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
	uint8_t segment_clusters;
} __attribute__((packed)) drive_header;

typedef union MAP_ext_e{
    struct{
        uint32_t segment,
        offset;
    };
    size_t address;
}__attribute__((packed)) MAP_ext_e;

typedef MAP_ext_e FAT_e;
typedef MAP_ext_e INDEX_e;

typedef struct MAP_ext_h{
    uint8_t security_code[15];
    MAP_ext_e prev_header;
    MAP_ext_e next_header;
}__attribute__((packed)) MAP_ext_h;

typedef struct MAP_ext_m{
	const uint8_t attributes;
	size_t size;
	uint16_t 	creation_time,
				modified_time;

	uint16_t 	creation_date,
				access_date,
				modified_date;
}__attribute__((packed)) MAP_ext_m;

/// @brief This is the type used to extend the cluster Map of a file, 
///         it acts as a header for connected clusters for security purposes.
///         The number of bytes should be: ((bytes_per_sector * sectors_per_cluster) - sizeof(MAP_cluster)): 
///         e.g  
typedef struct MAP_cluster{
    MAP_ext_h drive;
    uint8_t pad;
    uint8_t data[];
}__attribute__((packed)) MAP_cluster;

/// @brief This is the base type, 
///     it's header's prev_header should point to the entry in FAT that contains the file pointer.
///     This is supposed to be a extensible map of cluster pointers, where each entry points to a MAP_cluster cluster.
typedef struct MAP_ext_b{
    uint8_t security_code[15];
	const uint8_t padding[3];
    /// @brief The number of extra clusters made use of by this type.
    uint32_t num_extensions;
    FAT_e FAT;
	MAP_ext_m metadata;
	const uint8_t name_len;
    MAP_ext_e cluster_entries[];
}__attribute__((packed)) MAP_ext_b;


void fnv1a_120(const void *data, size_t len, uint8_t out[15]);

bool Virt_active;
/// @brief The FAT table, each entry pointing to a MAP base extension.
/// @remark Item #0 is the [Number of used entries: Number of used clusters].
/// @remark For ALL entries aside from [0], the segment must NOT be 0
FAT_e *FAT;
drive_header drive;
FILE *disk;

size_t basebyte_counter;
MAP_ext_b **base_buffer;

#ifdef _WIN32
void enable_ansi(void);
#endif
void IMG_Setup(void);
int consume(uint32_t arg_cc, char **arg_vector);

MAP_ext_b *DIRECTORY_Open(size_t index, FAT_e Directory);
bool DIRECTORY_Swap(FAT_e new_dir, FAT_e last_dir, size_t FILE_index);

void writed_MAPDATA(MAP_ext_b *file, INDEX_e address, char *path, long offset, uint8_t segment_size, long segment, long max_data);
void write_MAPDATA(MAP_ext_b *file, INDEX_e address, uint8_t *data, uint8_t num);

void print_MAPDATA(MAP_ext_b *file, uint8_t base, INDEX_e address);
void print_MAPbase(MAP_ext_b *base);
void print_DIRENTRIES(MAP_ext_b *base, char *mode);

#include <time.h>
void FATCompress(void);
void FDelete(FAT_e entry);
void Fcreate(char *name, char *mode);

extern MAP_ext_b *base_update;
extern FAT_e FAT_update;
extern uint32_t FAT_updateindex;
void FUpdate();
void FATUpdate(uint32_t index, INDEX_e index_e);

void BASESync();
FAT_e BASEMalloc();
void BASEfree(FAT_e value);
void MAPEXTB_PLUSalloc(FAT_e index);

uint16_t pack_time(struct tm *t);
MAP_ext_b *get_Item(FAT_e entry);
uint8_t strcheck(char *str, const char c);
extern const char DIGITS[];
void byte_to_base(uint8_t value, uint32_t base, char out[8]);

//! Written by AI
void fnv1a_120(const void *data, size_t len, uint8_t out[15]);