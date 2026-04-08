// gcc gcc -o C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\tools\fat.exe C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\tools\fat.c
// C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\tools\fat.exe "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\Build\Build-2025-09-28-41\floppy-2025-09-28-41.img" "skibidi.txt" ""

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

typedef struct driveHeader{
    uint8_t BOOT_Instruction[3];
    uint8_t OEM_ID[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster,
            reserved_sectors,
            fat_count;
    uint16_t dir_entries_count,
             total_sectors;
    uint8_t media_descriptor_type;
    uint16_t sectors_per_fat,
             sectors_per_track,
             heads;
    uint32_t hidden_sectors,
             large_sector_count;
    // Extended boot record.
    uint8_t drive_number,
            signature;
    uint32_t volume_id[4];
    uint8_t volume_label[11];
    uint8_t sys_id[8];
}__attribute__((packed)) driveHeader;


typedef struct file_entry{
	uint8_t name[11],
	attributes,
	_reserved,
	creationtime_tenths;
	uint16_t creation_time,
	creation_date,
	access_date,
	fst_clusterhigh,
	modified_time,
	modified_date,
	fst_clusterlow;
	uint32_t size;
	// Properties not to be implemented by fread, for optimisation as to prevent repeat calculation
	uint32_t lba;
} __attribute__((packed)) file_entry;
#define true_filehsize sizeof(file_entry) - sizeof(uint32_t)

typedef struct ptr_item{void *ptr; uint32_t size;}ptr_item;

//! Global variables
driveHeader header;

// FAT table, Metadata table
char *FAT;

file_entry *rt_dir;
uint32_t rtdir_end;

bool bs_read(const FILE *bootfile);
bool sectors_read(const FILE *disk, uint32_t lba, uint32_t count, void *out);
bool FAT_read(const FILE *disk);
bool rtd_read(const FILE *disk);
file_entry *get_file(const char *name);
bool file_read(file_entry *entry, FILE *disk, uint8_t *out);
void free_mul(ptr_item *ptr, uint32_t num);
void file_print(file_entry *file, FILE *disk, bool print_header);

int main(int argc, char **argv){
    if(argc < 3){
        printf("Syntax: %s <disk image> <file name>\n", argv[0]);
        return -1;
    }else{printf("Valid Image...\n");}

    FILE *disk = fopen(argv[1], "rb");
	if(!disk){
		fprintf(stderr, "INVALID File path: %s", argv[1]);
		return -1;
	}else{printf("Valid file path...\n");}

	if(bs_read(disk) == false){
		fprintf(stderr, "Reading Boot sector failed");
		fclose(disk);
		return -2;
	}else{printf("Boot read success...\n");}

	if(FAT_read(disk) == false){
		fprintf(stderr, "Failed to read FAT metadata!");
		if(FAT != NULL){free(FAT); FAT = NULL;}
		fclose(disk);
		return -3;
	}else{printf("FAT successfully read...\n");}
	
	if(rtd_read(disk) == false){
		fprintf(stderr, "Failed to read Root directory...");
		if(FAT != NULL){free(FAT); FAT = NULL;}
		if(rt_dir != NULL){free(rt_dir); rt_dir = NULL;}
		fclose(disk);
		return -4;
	}else{printf("Successfully read Root directory...\n");}
	
	file_entry *file = get_file(argv[2]);
	if(!file){
		fprintf(stderr, "Failed to get file: %s", argv[2]);
		if(FAT != NULL){free(FAT); FAT = NULL;}
		if(rt_dir != NULL){free(rt_dir); rt_dir = NULL;}
		fclose(disk);
		return -5;
	}else{printf("Successfully got file: %s\n", argv[2]);}

	uint8_t *buffer = (uint8_t *)malloc(file->size + header.bytes_per_sector);
	if(file_read(file, disk, buffer) != true){
		fprintf(stderr, "Failed to get file: %s", argv[2]);
		if(FAT != NULL){free(FAT); FAT = NULL;}
		if(rt_dir != NULL){free(rt_dir); rt_dir = NULL;}
		if(buffer != NULL){free(buffer); buffer = NULL;}
		fclose(disk);
		return -6;
	}else{
		for(uint32_t cc = 0; cc < (file->size + true_filehsize); ++cc){
			if(isprint(buffer[cc])){fputc(buffer[cc], stdout);}
			else{printf("<%02x>", buffer[cc]);}
		}
	}
	printf("\n");
	
	if(FAT != NULL){free(FAT); FAT = NULL;}
	if(rt_dir != NULL){free(rt_dir); rt_dir = NULL;}
	if(buffer != NULL){free(buffer); buffer = NULL;}
	fclose(disk);
    return 0;
}

bool bs_read(const FILE *bootfile){
	fseek(bootfile, 0, SEEK_SET);
	return fread(&header, sizeof(driveHeader), 1, bootfile);
}

bool sectors_read(const FILE *disk, uint32_t lba, uint32_t count, void *out){
	bool ok = true;
	ok = ok && (fseek(disk, lba* header.bytes_per_sector, SEEK_SET) == 0);
	const size_t file_ptr = ftell(disk);
	const uint32_t read_count = fread(out, header.bytes_per_sector, count, disk);
	ok = ok && (read_count == count);
	return ok;
}

bool FAT_read(const FILE *disk){
	FAT = (uint8_t *)malloc((header.sectors_per_fat* header.bytes_per_sector) + 5);
	return sectors_read(disk, header.reserved_sectors, header.sectors_per_fat, FAT);
}

//! Possible error, this function reads off maximum values
bool rtd_read(const FILE *disk){
	// Read off an offset, reading beyond the Image's reserved sectors and Page Table.
	const uint32_t lba = header.reserved_sectors + (header.sectors_per_fat* header.fat_count);
	// Byte amount to read.
	const uint32_t size = (true_filehsize) * header.dir_entries_count;
	// Sectors to read.
	uint32_t sectors = (size / header.bytes_per_sector);
	if(size % header.bytes_per_sector != 0){sectors++;}
	// In sectors, the number of sectors that makes up the root directory
	rtdir_end = sectors + lba;
	//227 file headers
	rt_dir = (file_entry *)malloc(sectors* header.bytes_per_sector);
	const bool out = sectors_read(disk, lba, sectors, rt_dir);
	rt_dir->lba = lba;
	return out;
}

file_entry *get_file(const char *name){
	for(uint32_t cc = 0; cc < header.dir_entries_count; ++cc){
		if(rt_dir[cc].name[0] == name[0]){
			uint32_t len  = 0;
			for(; len < 11 ; ++len){if(rt_dir[cc].name[len] == ' '){break;}}
			if(memcmp(rt_dir[cc].name, name, len) == 0){return &rt_dir[cc];}
		}
	}
	return NULL;
}

bool file_read(file_entry *entry, FILE *disk, uint8_t *out){
	bool ok = true;
	uint16_t cluster_curr = entry->fst_clusterlow;
	do{
		ok = ok && sectors_read(disk, entry->lba, header.sectors_per_cluster, out);
		out += entry->size;

		uint32_t fatindex = cluster_curr* 1.5;
		if(cluster_curr % 2 == 0){cluster_curr = ((uint16_t)FAT[fatindex]) & 0x0FFF;
		}else{cluster_curr = ((uint16_t)FAT[fatindex]) >> 4;}
	}while(ok && cluster_curr >= 0xFF8);
	return ok;
}

void file_print(file_entry *file, FILE *disk, bool print_header){
	unsigned char *buffer = (unsigned char *)malloc(file->size + true_filehsize);
	file_read(file, disk, buffer);
	switch(print_header){
		case false:
			for(uint32_t cc = true_filehsize; cc < (file->size + true_filehsize); ++cc){
				if(isprint(buffer[cc])){fputc(buffer[cc], stdout);}
				else{printf("<%02x>", buffer[cc]);}
			}
			return;
		case true:
			for(uint32_t cc = 0; cc < (file->size + true_filehsize); ++cc){
					if(isprint(buffer[cc])){fputc(buffer[cc], stdout);}
					else{printf("<%02x>", buffer[cc]);}
				}
			return;
	}
}

void free_mul(ptr_item *ptr, uint32_t num){
	for(uint32_t cc = 0; cc < num;){
		free(ptr[cc].ptr);
		cc+=ptr[cc].size;
	}
}