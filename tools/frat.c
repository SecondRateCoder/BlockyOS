// gcc gcc -o C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\tools\frat.exe C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\tools\frat.c
// C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\tools\frat.exe "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\Build\Build-2025-09-28-41\floppy-2025-09-28-41.img" "skibidi.txt" ""

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

bool sectors_read(const FILE *disk, uint32_t lba, uint32_t count, void *out);
bool FAT_read(const FILE *disk);
size_t* hash(const char *str);
bool bs_read(FILE *disk);

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

typedef struct file_header{
	uint8_t name_len,
			ext_len,
			/*
				[0]: Is Directory, as such the pointed file address should be reffered to as a Directory entry...,
				[1]: Is Linux Executable,
				[2]: Is Windows Executable,
				[3 - 7]: ???
			*/
			_attributes,
			_reserved;
			uint16_t creationtime,
			creationdate,
			access_date,
			modifiedtime,
			modifieddate;
			uint32_t size[2];
			uint32_t lit_addr[2];
			uint8_t name;
}__attribute__((packed)) file_header;

typedef struct directory_entry{
	uint8_t _attributes;
	/*
		Simple optimisation to prevent the searching of FAT.
	*/
	uint8_t FAT_index;
	/*
		Format for the name:
			size_t,
			uint128_t,
			uint256_t,
			uint512_t
	*/
	uint8_t name_fmt;
	void *name;
}__attribute__((packed)) directory_entry;

drive_header header;
file_header *FAT;

size_t strlen_c(char *txt, char *target){
	size_t out = 0;
	if(strncmp(txt, target, strlen(target))){
		out += strlen(target);
	}
	for(; out < strlen(txt); ++out){
		if(txt[cc] == target[0]){
			if(strncmp(&txt[cc], target, strlen(target))){
				return out;
			}
		}
	}
	return out;
}

size_t parse_str(char *str, char *seperator, char **out){
	size_t len = 0;
	for(size_t cc = 0; cc < strlen(str); ++cc){
		if(str[cc] == seperator[0]){
			if(strncmp(str[cc], seperator, strlen(seperator))){
				out[len - 1] = malloc(strlen_c(&str[cc], seperator));
				memcpy(out[len - 1], str[cc], strlen_c(&str[cc], seperator));
				len++;
			}
		}
	}
	return len;
}

bool sectors_read(const FILE *disk, uint32_t lba, uint32_t count, void *out){
	bool ok = true;
	ok = ok && (fseek(disk, lba* header.bytes_per_sector, SEEK_SET) == 0);
	const size_t file_ptr = ftell(disk);
	const uint32_t read_count = fread(out, header.bytes_per_sector, count, disk);
	ok = ok && (read_count == count);
	return ok;
}

bool bs_read(FILE *disk){
	fseek(disk, 0, SEEK_SET);
	return fread(&header, sizeof(drive_header), 1, disk);
}

bool FAT_read(const FILE *disk){
	FAT = (uint8_t *)malloc((header.sectors_per_fat* header.bytes_per_sector) + 5);
	return sectors_read(disk, header.reserved_sectors, header.sectors_per_fat, FAT);
}

file_header *get_file(const char *name){
	size_t *hash_ = hash(name);
}


#define HASH_64BIT_LIMIT 12
size_t* hash(const char *str){
    if(!str){return NULL;}
    size_t *hash = malloc(sizeof(size_t)* 2);
    if(!hash){return NULL;}
    hash[0] = 5381;
    hash[1] = 0;
    size_t cc = 0;
    int c;
    while((c = *str++)){
        ++cc;
        if(cc > HASH_64BIT_LIMIT){hash[1] = ((hash[1] << 5) + hash[1]) + c;
        }else{hash[0] = ((hash[0] << 5) + hash[0]) + c;}
    }
    return hash;
}