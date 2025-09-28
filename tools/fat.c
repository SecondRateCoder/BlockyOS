// gcc gcc -o C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\tools\fat.exe C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\tools\fat.c
// C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\tools\fat.exe "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\Build\Build-2025-09-28-41\floppy-2025-09-28-41.img" "skibidi.txt" ""

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct drive_header{
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
}__attribute__((packed)) drive_header;

typedef struct dir_header{
	uint8_t name[11],
			attributes,
			_reserved,
			creationtime_tenths;
	uint16_t creationtime,
			 creationdate,
			 access_date,
			 fst_clusterhigh,
			 modifiedtime,
			 modifieddate,
			 fst_clusterlow;
	uint32_t size;
	// Properties not to be implemented by fread, for optimisation as to prevent repeat calculation
	uint32_t lba;
} __attribute__((packed)) dir_header;

drive_header header;
dir_header *rt_dir;
unsigned char *FAT;

bool bs_read(const FILE *bootfile);
bool sectors_read(const FILE *disk, uint32_t lba, uint32_t count, void *out);
bool FAT_read(const FILE *disk);
bool rtd_read(const FILE *disk);
dir_header *get_file(const char *name);

int main(int argc, char **argv){
    if(argc < 3){
        printf("Syntax: %s <disk image> <file name>\n", argv[0]);
        return -1;
    }

    FILE *disk = fopen(argv[1], "rb");
	if(!disk){
		fprintf(stderr, "INVALID File path: %s", argv[1]);
		return -1;
	}

	if(bs_read(disk) == false){
		fprintf(stderr, "Reading Boot sector failed");
		return -2;
	}

	if(FAT_read(disk) == false){
		fprintf(stderr, "Failed to read FAT metadata!");
		free(FAT);
		return -3;
	}

	if(rtd_read(disk) == false){
		fprintf(stderr, "Failed to read Root directory...");
		free(FAT);
		free(rt_dir);
		return -4;
	}

	dir_header *directory = get_file(argv[2]);
	if(!directory){
		fprintf(stderr, "Failed to get file: %s", argv[2]);
		free(FAT);
		free(rt_dir);
		return -5;
	}

	free(FAT);
	free(rt_dir);
	fclose(disk);
    return 0;
}

bool bs_read(const FILE *bootfile){return fread(&header, sizeof(drive_header), 1, bootfile);}

bool sectors_read(const FILE *disk, uint32_t lba, uint32_t count, void *out){
	bool ok = true;
	ok = ok && (fseek(disk, lba* header.bytes_per_sector, SEEK_SET) == 0);
	const uint32_t read_count = fread(out, header.bytes_per_sector, count, disk);
	const size_t file_ptr = ftell(disk);
	ok = ok && (fread(out, header.bytes_per_sector, count, disk) == count);
	return ok;
}

bool FAT_read(const FILE *disk){
	FAT = (uint8_t *)malloc(sizeof(unsigned char)* header.sectors_per_fat* header.bytes_per_sector);
	return sectors_read(disk, header.reserved_sectors, header.sectors_per_fat, FAT);
}

bool rtd_read(const FILE *disk){
	const uint32_t lba = header.reserved_sectors + header.sectors_per_fat + header.fat_count;
	const uint32_t size = (sizeof(dir_header) - sizeof(uint32_t)) * header.dir_entries_count;
	uint32_t sectors = (size / header.bytes_per_sector);
	if(sectors % header.bytes_per_sector != 0){sectors++;}
	rt_dir = (dir_header *)malloc(sectors* header.bytes_per_sector);
	const bool out = sectors_read(disk, lba, sectors, rt_dir);
	rt_dir->lba = lba;
	return out;
}

dir_header *get_file(const char *name){
	for(uint32_t cc = 0; cc < header.dir_entries_count; ++cc){
		if(rt_dir[cc].name[0] == name[0]){
			if(memcmp(rt_dir[cc].name, name, 11) == 0){return &rt_dir[cc];}
		}
	}
}