// gcc -o C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\tools\frat.exe C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\tools\frat.c
// C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\tools\frat.exe "C:\Users\olusa\OneDrive\Documents\GitHub\BlockyOS\Build\Build-2025-09-28-41\floppy-2025-09-28-41.img" "skibidi.txt" ""

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

bool Virt_active;

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

// #define ANSI_RED(TEXT) 		(Virt_active? ANSI_COLOR_RED: "") TEXT (Virt_active? ANSI_COLOR_RESET: "")
// #define ANSI_GREEN(TEXT) 	(Virt_active? ANSI_COLOR_GREEN: "") TEXT (Virt_active? ANSI_COLOR_RESET: "")
// #define ANSI_YELLOW(TEXT) 	(Virt_active? ANSI_COLOR_YELLOW: "") TEXT (Virt_active? ANSI_COLOR_RESET: "")
// #define ANSI_BLUE(TEXT) 	(Virt_active? ANSI_COLOR_BLUE: "") TEXT (Virt_active? ANSI_COLOR_RESET: "")
// #define ANSI_MAGENTA(TEXT) 	(Virt_active? ANSI_COLOR_MAGENTA: "") TEXT (Virt_active? ANSI_COLOR_RESET: "")
// #define ANSI_CYAN(TEXT) 	(Virt_active? ANSI_COLOR_CYAN: "") TEXT (Virt_active? ANSI_COLOR_RESET: "")

#define ANSI_RED(TEXT) ANSI_COLOR_RED TEXT ANSI_COLOR_RESET
#define ANSI_GREEN(TEXT) ANSI_COLOR_GREEN TEXT ANSI_COLOR_RESET
#define ANSI_YELLOW(TEXT) ANSI_COLOR_YELLOW TEXT ANSI_COLOR_RESET
#define ANSI_BLUE(TEXT) ANSI_COLOR_BLUE TEXT ANSI_COLOR_RESET
#define ANSI_MAGENTA(TEXT)  ANSI_COLOR_MAGENTA TEXT ANSI_COLOR_RESET
#define ANSI_CYAN(TEXT) ANSI_COLOR_CYAN TEXT ANSI_COLOR_RESET
#define ANSI_LBLUE(TEXT) ANSI_COLOR_LBLUE TEXT ANSI_COLOR_RESET


typedef struct drive_header{
    uint8_t BOOT_Instruction[3],
	OEM_ID[8];
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

typedef struct FAT_entry{
	uint8_t name_fmt;
	uint8_t _attributes;
	size_t lit_addr;
	uint8_t name[];
}__attribute__((packed)) FAT_entry;

typedef struct entry_header{
	size_t num;
}entry_header;

typedef struct file_entry{
	uint8_t padding[4],
	name_len,
	/*
		[0]: Is Directory, as such the pointed file address should be reffered to as a Directory entry...,
		[1]: Is Linux Executable,
		[2]: Is Windows Executable,
		[3]: Is System File,
		[4 - 7]: ???
		*/
	_attributes;
	uint16_t creation_time,
	creation_date,
	access_date,
	modified_time,
	modified_date;
	size_t size;
	size_t lit_addr;
	uint32_t FAT_index;
	unsigned char name[];
}__attribute__((packed)) file_entry;

typedef struct temp_bf{
	void *data;
	size_t data_size;
	uint32_t ID;
	char *name;
}temp_bf;

typedef struct ATTR_FMT{
	uint8_t eff;
	char name[];
}__attribute__((packed)) ATTR_FMT;

#define ATTR_DIR 0b00000001
#define ATTR_LINEXE 0b00000010
#define ATTR_WINEXE 0b00000100
#define ATTR_FILE 0b0
#define ATTR_SYSTEM 0b00001000
#define IS_DIR(ATTR) ((ATTR & ATTR_DIR) == ATTR_DIR)
#define IS_LINEXE(ATTR) ((ATTR & ATTR_LINEXE) == ATTR_LINEXE)
#define IS_WINEXE(ATTR) ((ATTR & ATTR_WINEXE) == ATTR_WINEXE)
#define IS_SYSFILE(ATTR) ((ATTR & ATTR_SYSTEM) == ATTR_SYSTEM) // If has that attribute then should be equal to attribute
#define IS_FILE(ATTR) (!IS_DIR(ATTR))

ATTR_FMT attr_fmt_arr[] = {
	(ATTR_FMT){ATTR_DIR, "DIR"}, (ATTR_FMT){ATTR_DIR, "DIRECTORY"}, (ATTR_FMT){ATTR_DIR, "dir"}, (ATTR_FMT){ATTR_DIR, "directory"}, 
	(ATTR_FMT){ATTR_LINEXE, "L_EXE"}, (ATTR_FMT){ATTR_LINEXE, "l_exe"}, (ATTR_FMT){ATTR_LINEXE, "lexe"}, 
	(ATTR_FMT){ATTR_WINEXE, "W_EXE"}, (ATTR_FMT){ATTR_WINEXE, "w_exe"}, (ATTR_FMT){ATTR_WINEXE, "wexe"},
	(ATTR_FMT){ATTR_FILE, "FILE"}, (ATTR_FMT){ATTR_FILE, "file"},
	(ATTR_FMT){ATTR_SYSTEM, "SYSFILE"}, (ATTR_FMT){ATTR_SYSTEM, "sysfile"}, (ATTR_FMT){ATTR_SYSTEM, "SYS_FILE"}, 
	(ATTR_FMT){ATTR_SYSTEM, "sys_file"}, (ATTR_FMT){ATTR_SYSTEM, "system"}, (ATTR_FMT){ATTR_SYSTEM, "SYSTEM"}, 
};
const uint32_t attr_num = 18;

typedef struct directory_entry{
	uint8_t _attributes;
	uint32_t FAT_index,
			 name_len;
			 unsigned char name[];
}__attribute__((packed)) directory_entry;

typedef size_t uint128_t[2], uint256_t[4], uint512_t[8];

/// @brief The header of the open foppy drive.
drive_header header;
#define FDATA_START (sizeof(drive_header) + (((header.sectors_per_fat * header.fat_count) + header.reserved_sectors + header.hidden_sectors) * header.bytes_per_sector))	// 72 + (((9 * 2) + 2 + 0) * 512) = 10312 bytes
#define FAT_START (sizeof(drive_header) + ((header.reserved_sectors + header.hidden_sectors) * header.bytes_per_sector))
#define HF_SIZE(FORMAT) ((sizeof(size_t) * pow(2, FORMAT)))
#define FATe_SIZE(e) (*(e->name) != 0? (sizeof(FAT_entry) + strlen(e->name)): 0)
#define FILE_SIZE(e) (e->size + e->name_len)
#define FILE_ENDADDR(e) (FILE_SIZE(e) + e->lit_addr)

/// @brief The FAT file table.
FAT_entry *FAT;
/// @brief The number of FAT entries.
volatile FATl_t FAT_num;
/// @brief The path of directories being accessed right now.
file_entry **curr_dir;
/// @brief The number of directories leading to the current location.
size_t dir_len;
/// @brief The file being accessed right now.
file_entry *curr_file;
/// @brief The path used right now, in the format ["C:", "TT", "Blah", ].
char **curr_path;
/// @brief Mark the currently open file as being used.
bool using_file, closed, waiting_close;
/// @brief The public/gloabel disk used by all aspects of the program.
FILE *disk;
/// @brief The path to the disk.
char *disk_path;
/// @brief Buffers for temporary data storage.
temp_bf **buffers;
/// @brief The current number of buffers.
size_t buffer_num;
/// @brief The selected buffer, if 0 then not used.
uint32_t selected_buffer;
#define bf_select (selected_buffer > buffer_num? 0: selected_buffer)
/// @brief Possible commands to use.
char *commands[] = {
	"open",				// Open a file with a specific file name, by enumerating FAT. Params: <FILE NAME>
	"dopen",			// Open a file within the currently opened directory. Params: <FILE NAME>
	"close",			// Close the currently open file. Params: <NONE>
	"read",				// Read off data from the currently open file into the quick label. Params: <INDEX, NUMBER OF BYTES TO READ>
	"write",			// Write into the quick label's data with data from the currently open file. Params: <INDEX, DATA SIZE>
	"dwrite",			// Write into the quick label with data from the Console, Data starts and ends with quote marks. Params: <INDEX, DATA SIZE, DATA TO WRITE(IN ASCII)>
	"_fwrite",			// Write into the currently open file's contents with all the data of another file. Params: <FILE PATH, RECIEVING INDEX, SENDING INDEX, DATA_NUM>
	"mread",			// Read off the currently open file's Metadata. Params: <NONE>
	"mwrite",			// Read to the currently open file's Metadata. Params: <PROPERTY(LOWER_CASE)>, <VALUE(S)>
	"create",			// Create a new file. Params: <FILE NAME, SIZE, FORMAT>
	"lcreate",			// Create a new label <LABEL NAME, STARTING SIZE>
	"select",			// Select a specific label to be used. Params: <LABEL NAME>
	"help",				// Print help info. Params: <NONE>
	"llist",			// Print the currently selected label and it's data. Params: <DATA ROWS, DATA MAX>
	"exit",				// Exit the application.
	"flist",			// List all the entries in FAT.
	"clear",			// Clear the screen.
	"lremove", 			// Remove/delete a label. Params: <LABEL NAME>
	"ltell", 			// List all the labels/buffers in Memory. Params: <DATA ROWS, DATA MAX>
};
const uint32_t num_commands = 19;

bool sectors_read(const FILE *disk, uint32_t lba, uint32_t count, void *out);
bool hash_cmp(size_t *a, size_t *b, uint8_t hash_fmt);
uint8_t hash(const char *str, size_t **out);
bool bs_read(const FILE *bootfile);
void IMG_setup();
FAT_entry *read_FATe(FATl_t index);
char **parse_str(char *str, char *seperator, size_t *len);
void write_FATe(FATl_t index, FAT_entry *new_entry);
void get_FATsize(FATl_t index, FATl_t target, size_t *len, bool upto);
void FRopen(char *name);
void FRopen_d(char *path);
directory_entry *directory_enum(size_t *progress);
bool FRopen_d_(char *file);
void sighandle(int sig);
size_t _fread(void *dst, size_t element_size, size_t count);
size_t _fwrite(void *src, size_t element_size, size_t count);
FILE *_fopen(char *path);
uint8_t decode_attr_(char *str);
uint8_t decode_attr(char *str);
void buffer_gen(size_t size, char *name);
void cmd_handle(char *name);

uint8_t get_hour(uint16_t t)   { return (t >> 2) & 0x0F; }
uint8_t get_minute(uint16_t t) { return (t >> 6) & 0x1F; }
uint8_t get_second(uint16_t t) { return (t >> 11) & 0x1F; }

size_t clamp(size_t min, size_t max, size_t n){return n > max? max: (n < min? min: n);}

//! Use custom parser, that handles quote marks.
/// @brief Parse a string based on the inputted seperator string.
/// @param str The whole string.
/// @param seperator The pattern at which snippets will be sourced from.
/// @param len The number of entries in the output string array.
/// @return The output.
char **parse_str(char *str, char *seperator, size_t *len){
	if(!str || !len){return NULL;}

    int count = 0;
	size_t str_len = strlen(str), sep_len = strlen(seperator);
    char **result = NULL;

	size_t token_len = 0;
	for(; token_len < str_len; ++token_len){
		if(str[token_len] == '\"' || str[token_len] == '\''){break;}else
		if(str[token_len] == seperator[0]){if(strncmp(str, seperator, sep_len - 1) == 0){break;}}
	}
	result = realloc(result, sizeof(char *) * (count + 1));
	char *token = malloc(token_len + 1);
	memcpy(token, str, token_len);
	token[token_len + 1] = 0;
	result[count] = token;
	count++;
	size_t cc =token_len;
    for(; cc < str_len; ++cc){
		// printf(ANSI_YELLOW("Info: %s"), str + c);
		if(str[cc] == '\"' || str[cc] == '\''){
			token_len = cc + 1;
			for(; token_len < str_len; ++token_len){if(str[token_len] == '\"' || str[token_len] == '\''){break;}}
			result = realloc(result, sizeof(char *) * (count + 1));
			char *token = malloc(token_len - cc - 1);
			memcpy(token, str + cc + 1, token_len - cc - 1);
			token[token_len - cc - 1] = 0;
			result[count] = token;
			count++;
			printf(ANSI_YELLOW("\nInfo: %s"), str + cc);
			cc = token_len;
			//! LOGIC ERROR: CANNOT HANDLE NUMBERS IN COMMANDS
		}else if(str[cc] == seperator[0]){
			if(strncmp(str + cc, seperator, sep_len) == 0){
				token_len = cc + 1;
				for(; token_len < str_len; ++token_len){if(str[token_len] == seperator[0]){if(strncmp(str + cc, seperator, sep_len - 1) == 0){break;}}}
				result = realloc(result, sizeof(char *) * (count + 1));
				char *token = malloc(token_len - cc);
				memcpy(token, str + cc + 1, token_len - cc - 1);
				token[token_len - cc - 1] = 0;
				result[count] = token;
				count++;
				printf(ANSI_YELLOW("\nInfo: %s"), str + cc);
				cc = token_len - 1;
			}
		}else if(str[cc + 1] == 0){
			//! token_len points to the last end of a token
			result = realloc(result, sizeof(char *) * (count + 1));
			char *token = malloc(token_len - cc + 1);
			memcpy(token, str + cc + 1, token_len - cc - 1);
			token[token_len - cc] = 0;
			result[count] = token;
			count++;
			printf(ANSI_YELLOW("\nInfo: %s"), str + cc);
			break;
		}
		if(str[cc] == 0){break;}
	}

    *len = count;
    return result;
}

#define HASH_64BIT_LIMIT 12
/// @brief Hash a string into an integer array.
/// @param str The string to be hashed.
/// @param hash The output, malloced internally.
/// @return The size of the hash, use;(sizeof(size_t) * pow(2, n)), for true byte size.
uint8_t hash(const char *str, size_t **out){
    uint8_t out_fmt = 0;
    if(!str){return 0;}
    (*out) = malloc(sizeof(size_t) * pow(2, out_fmt));
    if(!out){return 0;}
	int cc = 0;
    int c = 0;
    while((c = str[cc])){
		(*out)[out_fmt] = (((*out)[out_fmt] << 5) + (*out)[out_fmt]) + c;
		if(cc > HF_SIZE((out_fmt == 0? 1: out_fmt)) && cc != 0){out_fmt++;	(*out) = realloc((*out), sizeof(size_t) * pow(2, out_fmt));}
		++cc;
    }
    return out_fmt + 1;
}

/// @brief Compare two hash values together.
/// @param a Hash value a.
/// @param b Hash value b.
/// @param hash_fmt The format of the hashes; (sizeof(size_t) * pow(2, n)).
/// @return Success if the values are identical.
bool hash_cmp(size_t *a, size_t *b, uint8_t hash_fmt){
	for(uint8_t cc= 0; cc < pow(2, hash_fmt); ++cc){if(a[cc] != b[cc]){return false;}}
	return true;
}

/// @brief Set-up Global variables for communicating with FAT image.
/// @param disk The Image to be contextualised to.
void IMG_setup(){
	buffers = NULL;
	buffer_num = 0;
	fseek(disk, 0, SEEK_SET);
	// header = malloc(sizeof(drive_header));
	if(_fread(&header, sizeof(drive_header), 1) != 1){
		printf(ANSI_RED("Drive header reading failed, exiting..."));
		exit(EXIT_FAILURE);
	}
	if(header.bytes_per_sector == 0){
		printf(ANSI_RED("Floppy image was NOT valid..."));
		exit(1);
	}
	fseek(disk, FAT_START, SEEK_SET);
	_fread(&FAT_num, sizeof(FATl_t), 1);
	// if(FAT_num == 0){
	// 	printf("Floppy image was NOT valid...");
	// 	exit(1);
	// }
	if(FAT_num != 0){
		FAT = malloc(sizeof(FAT_entry)* FAT_num);
		_fread(FAT, sizeof(FAT_entry), FAT_num);
		fseek(disk, FDATA_START + FAT[0].lit_addr, SEEK_SET);
		FAT = realloc(FAT, header.sectors_per_fat * header.fat_count * header.bytes_per_sector);
	}else{
		FAT = calloc(header.sectors_per_fat * header.fat_count, header.bytes_per_sector);
		fseek(disk, FDATA_START, SEEK_SET);
	}
	void *temp = malloc(sizeof(file_entry) + 5); // Addiional buffer to prevent SEGFAULT
	_fread(temp, sizeof(file_entry), 1);
	curr_dir = malloc(sizeof(size_t));
	curr_dir[0] = malloc(sizeof(file_entry));
	memcpy(curr_dir[0], temp, sizeof(file_entry));
	free(temp);
	//Move backwards to read full file entry.
	fseek(disk, 0 - ((ssize_t)sizeof(file_entry)), SEEK_CUR);
	_fread(curr_dir[0], FATe_SIZE(curr_dir[0]), 1);
}

/// @brief Get the FAT entry at a specified index.
/// @param index The index of the entry.
/// @return The FAT object.
/// @remarks The output object MUST be freed after use.
FAT_entry *read_FATe(FATl_t index){
	size_t len = 0;
	get_FATsize(0, index, &len, true);
	uint8_t *FAT_temp = (uint8_t *)FAT;
	FAT_entry *out = malloc(sizeof(FAT_entry) + (sizeof(size_t) * pow(2, ((FAT_entry *)(FAT_temp + len))->name_fmt)));
	memcpy(out, FAT_temp + len, sizeof(FAT_entry) + (sizeof(size_t) * pow(2, ((FAT_entry *)(FAT_temp + len))->name_fmt)));
	return (FAT_entry *)(FAT_temp + len);
}

/// @brief Write to the FAT entry at a specified index.
/// @param index The index of the entry.
/// @param new_entry The new FAT object.
void write_FATe(FATl_t index, FAT_entry *new_entry){
	size_t len = 0;
	get_FATsize(0, index, &len, true);
	FAT_entry *temp = FAT + len;
	size_t temp_len = FATe_SIZE(temp);
	if(temp_len > FATe_SIZE(new_entry)){
		//Process as such, Move all consecutive entries down.
		void *FAT_temp = (void *)FAT;
		memcpy(FAT_temp + (len), new_entry, FATe_SIZE(new_entry));
		memmove(FAT+len+(temp_len - FATe_SIZE(new_entry))/*End of original entry*/, FAT+len+FATe_SIZE(new_entry)/*End of new entry*/, temp_len - FATe_SIZE(new_entry));
		free(new_entry);
	}else if(len < FATe_SIZE(new_entry)){
		/*
			Algorithm explained:
				cc0: target start
				cc1: Target ending
				cc2: Original ending
				Decrement cc1 and cc2:
					copy from cc2 to cc2
				break when cc1 equates cc0
		*/
		// len == cc0
		size_t cc1 = 0;
		get_FATsize(0, FAT_num, &len, true);
		size_t cc2 = cc1 + temp_len - FATe_SIZE(new_entry);
		for(; cc1 != len; --cc1, --cc2){
			uint8_t *FTemp = ((uint8_t *)FAT);
			FTemp[cc2] = ((uint8_t *)FAT)[cc1];
		}
	}else{
		void *FAT_temp = (void *)FAT;
		memcpy(FAT_temp + (len), new_entry, FATe_SIZE(new_entry));
		free(new_entry);
	}
	return;
}

/// @brief Get the literal byte number up to a FAT item index.
/// @param index The counter, counts up to target.
/// @param target The target index.
/// @param len The output length.
/// @remark This counts the length up to index.
void get_FATsize(FATl_t index, FATl_t target, size_t *len, bool upto){
	void *FAT_temp = (void *)FAT;
	FAT_entry *FAT_item = FAT_temp + *len;
	if(upto == true){
		index++;
		if(index >= target || index > FAT_num){return;
		}else{
			*len += FATe_SIZE(FAT_item);
			get_FATsize(index + 1, target, len, true);
		}
		return;
	}else{
		*len += FATe_SIZE(FAT_item);
		if(index >= target || index > FAT_num){return;
		}else{get_FATsize(index + 1, target, len, true);}
		return;
	}
}

/// @brief Open a file, this method bypasses Directory constraints by directly enumerating FAT.
/// @param name 
void FRopen(char *name){
	size_t *hash_n = malloc(sizeof(size_t));
	// char seperator[] = "\\";
	uint8_t hash_fmt = hash(name, &hash_n);
	for(size_t cc= 0; cc < FAT_num; ++cc){
		size_t len = 0; if(cc != 0){get_FATsize(0, cc + 1, &len, true);}
		FAT_entry *entry = (FAT_entry *)(((uint8_t *)FAT) + (len == 0? 0: len - 1));
		if(entry->name_fmt == hash_fmt){
			if(hash_cmp(hash_n, (size_t *)entry->name, hash_fmt) == true){
				fseek(disk, FDATA_START + entry->lit_addr, SEEK_SET);
				curr_file = malloc(sizeof(file_entry) + 1);
				_fread(curr_file, sizeof(file_entry), 1);
				curr_file = realloc(curr_file, sizeof(file_entry) + curr_file->name_len + 1);
				_fread(curr_file->name, sizeof(char), curr_file->name_len);
				printf(ANSI_GREEN("\nFile found..."));
				return;
			}
		}
	}
	printf(ANSI_RED("\nFile not found..."));
}

/// @brief Open a file from a Path starting from the currently open Directory.
/// @param path The Path of the file
void FRopen_d(char *path){
	size_t *hash_n = NULL, out_len = 0;
	char seperator[] = "\\";
	char **out = parse_str(path, seperator, &out_len);
	uint8_t hash_fmt = hash(path, &hash_n);
	for(size_t cc =0; cc < out_len; ++cc){
		if(FRopen_d_(out[cc]) == false){
			printf(ANSI_RED("Path not found...\n\t%s"), out[cc]);
			return;
		}else{printf(ANSI_GREEN("Path found...\n\t%s"), out[cc]);}
	}
	printf(ANSI_YELLOW("Done opening..."));
}

/// @brief Enumerate all entries the currently open Directory.
/// @param The disk to be used.
/// @param progress The length of the output entries.
/// @return All the entries in the Directory.
directory_entry *directory_enum(size_t *progress){
	directory_entry *entries = malloc(sizeof(directory_entry));
	fseek(disk, FDATA_START + curr_dir[dir_len - 1]->lit_addr, SEEK_SET);
	_fread(entries, sizeof(directory_entry), 1);
	size_t true_progress = 0;
	entries = realloc(entries, sizeof(directory_entry) + entries[0].name_len);
	_fread(((uint8_t *)entries + sizeof(directory_entry)), sizeof(directory_entry) + entries[0].name_len, 1);
	if(entries[0].name[entries[0].name_len - 1] == ' ' && isalnum(entries[0].name[entries[0].name_len - 2]) == true){
		entries[0].name[entries[0].name_len - 1] = 0;
		printf(ANSI_GREEN("Valid Directory entry: %s"), entries[0].name);
	}
	true_progress = sizeof(directory_entry)+ entries->name_len;
	uint8_t *pure_entries = (uint8_t *)entries;
	//Terminates when FAT_inex == 0
	for(;((directory_entry *)pure_entries + true_progress)->FAT_index != 0; ++(*progress)){
		directory_entry *interm = malloc(sizeof(directory_entry));
		interm = realloc(interm, sizeof(directory_entry) + interm->name_len);
		_fread(interm, sizeof(directory_entry) + interm->name_len, 1);
		interm->name[interm->name_len - 1] = 0;
		entries = realloc(entries, true_progress + sizeof(directory_entry) + interm->name_len);
		memcpy(pure_entries + true_progress, interm, sizeof(directory_entry) + interm->name_len);
		true_progress += sizeof(directory_entry) + interm->name_len;
	}
}


/// @brief Open one File, by searching for it in the currently open directory.
/// @param file The file to be found.
/// @param disk The disk to be used.
bool FRopen_d_(char *file){
	size_t len = 0;
	directory_entry *curr_entries = directory_enum(&len);
	size_t *hash_n = NULL, *hash_f = NULL;
	uint8_t file_fmt = hash(file, &hash_f);
	for(size_t cc =0; cc < len; ++cc){
		uint8_t entr_fmt = hash(curr_entries[cc].name, &hash_n);
		if(entr_fmt == file_fmt){
			if(hash_cmp(hash_n, hash_f, entr_fmt) == true){
				printf(ANSI_GREEN("File: %s found"), file);
				size_t len = 0;
				get_FATsize(0, curr_entries[cc].FAT_index, &len, true);
				curr_file = ((void *)FAT) + len;
				if(curr_file->_attributes & MASK_GEN(8) != 0){
					//Is Directory...
					curr_dir = realloc(curr_dir, dir_len * sizeof(size_t));
					curr_dir[dir_len] = malloc(FATe_SIZE(curr_file));
					memcpy(curr_dir[dir_len], curr_file, FATe_SIZE(curr_file));
				}
				return true;
			}
		}
		free(hash_n);
	}
	return false;
}


void fclose_(bool silent, bool force_close){
	if(force_close == true){free(curr_file); curr_file = NULL;}
	if(using_file == true){
		if(silent != true){
			printf(ANSI_YELLOW("The currently open file is being used\nWait till current process is completed?"));
			printf(ANSI_YELLOW("yes => yes\nno => no"));
			char buffer[4];
			fgets(buffer, sizeof(buffer), stdout);
			if(strncmp(buffer, "yes", 4) == 0){
				printf(ANSI_YELLOW("Alrighty..."));
				waiting_close = true;
				return;
			}else if(strncmp(buffer, "no", 4) == 0){
				printf(ANSI_YELLOW("Alrighty..."));
				return;
			}
		}
		return;
	}
}

/// @brief Decodes a comma seperated list of entries(in a single string) into an _attributes byte.
/// @param str The string to be parsed.
/// @return A decoded attributes array.
uint8_t decode_attr(char *str){
	if(str == NULL){return 0;}
	uint32_t cc =0;
	uint8_t out = 0;
	do{
		uint32_t len  =1;
		for(; (cc + len) < strlen(str) && str[cc + len] != ','; ++len){}
		char *temp = calloc(len, 1);
		memcpy(temp, str + cc + (str[cc] == ','? 1: 0), len);
		out |= decode_attr_(temp);
		free(temp);
		cc += len;
	}while(cc < strlen(str));
	return out;
}


uint8_t decode_attr_(char *str){
	char *cpy = strdup(str);
	char seperator[] = " ";
	size_t len = 0;
	ATTR_FMT *item = NULL;
	for(size_t cc =0; cc < attr_num; cc++){
		item = ((ATTR_FMT *)(((uint8_t *)attr_fmt_arr) + len));
		len += sizeof(ATTR_FMT) + strlen(item->name) + 2;
		if(cpy[0] == item->name[0]){
			if(strncmp(cpy, item->name, strlen(item->name) - 1) == 0){
				return item->eff;
			}
		}
	}
	return 0;
}

bool Fappend(FAT_entry *entry){
	write_FATe(FAT_num, entry);
	FAT_num++;
	return true;
}

bool Frename(FATl_t index, char *name){
	FAT_entry *target = read_FATe(index);
	size_t *hash_ = NULL;
	uint8_t fmt = hash(name, &hash_);
	target = realloc(target, FATe_SIZE(target));
	target->name_fmt = fmt;
	memcpy(target->name, hash_, (sizeof(size_t) * pow(2, fmt)));
	write_FATe(index, target);
	free(target);
	free(hash_);
	return true;
}

void *Fread_curr(size_t num, size_t index){
	void *out = malloc(num);
	fseek(disk, FDATA_START + curr_file->lit_addr + index, SEEK_SET);
	_fread(out, sizeof(uint8_t), num);
	return out;
}

void *Fwrite_curr(FATl_t num, size_t index, void *in){
	fseek(disk, FDATA_START + curr_file->lit_addr + index, SEEK_SET);
	_fwrite(in, sizeof(uint8_t), num);
	fflush(disk);
}

size_t Fget_free(FATl_t start_index, size_t target_size){
	size_t len = 0, out = 0;
	get_FATsize(0, start_index, &len, false);
	for(; start_index < FAT_num; ++start_index){
		FAT_entry *entry = ((FAT_entry *)((void *)FAT + len));
		fseek(disk, FDATA_START + entry->lit_addr, SEEK_SET);
		file_entry *file = malloc(sizeof(file_entry) - (sizeof(uint8_t) * 4));
		if(clamp(entry->lit_addr, FILE_ENDADDR(file), out) == out &&
			clamp(entry->lit_addr, FILE_ENDADDR(file), out + target_size) == out + target_size){
				return out;
		}else{out = entry->lit_addr + file->size + file->name_len;}
		return out;
	}
}

void buffer_gen(size_t size, char *name){
	if(buffers == NULL || buffer_num == 0){
		buffers = malloc(sizeof(temp_bf *));
		if(buffers == NULL){
			printf("Buffer reallocation failed");
			// buffers = NULL;
			return;
		}
	}
	else{
		void *temp = buffers;
		buffers = realloc(buffers, sizeof(temp_bf *) * (buffer_num == 0? 1: buffer_num));
		if(buffers == NULL){
			printf(ANSI_RED("\nERROR!\n Canot create label: %s of size: %zu\n\tBuffer reallocation failed"), name, size);
			buffers = temp;
			return;
		}
	}
	buffers[buffer_num] = malloc(sizeof(temp_bf));
	buffers[buffer_num]->data = malloc(size);
	buffers[buffer_num]->data_size = size;
	buffers[buffer_num]->ID = buffer_num;
	buffers[buffer_num]->name = strdup(name);
	buffer_num++;
	return;
}

bool buffer_search(char *name, uint32_t *ID){
	size_t cc =0;
	if(ID != NULL && name == NULL){
		for(; cc < buffer_num; cc++){
			if(buffers[cc]->ID == *ID){
				selected_buffer = cc;
				printf(ANSI_GREEN("\nFound buffer"));
				return true;
			}
		}
	}else if(ID == NULL && name != NULL){
		for(; cc < buffer_num; ++cc){
			if(buffers[cc]->name[0] == name[0]){
				// printf("Buffer: %s", buffers[cc]->name);
				if(strncmp(buffers[cc]->name, name, strlen(buffers[cc]->name) - 1) == 0){
					selected_buffer = cc;
					printf(ANSI_GREEN("\nFound buffer"));
					return true;
				}
			}
		}
	}else{printf(ANSI_RED("\nInvalid arguments"));	return false;}
	char temp[128];
	memset(temp, 0, 128);
	if(ID != NULL){snprintf(temp, 128, "%d", *ID);}
	printf(ANSI_RED("\nBuffer %s not found"), (name != NULL? name: (ID != NULL? temp: "<NO ARGS>")));
	return false;
}

uint16_t encode_time(uint8_t hr, uint8_t min, uint8_t sec){return ((hr & 0x0F) << 2) | ((min & 0x1F) << 6) | ((sec & 0x1F) << 11);}

/*
for(uint32_t cc = true_filehsize; cc < (file->size + true_filehsize); ++cc){
	if(isprint(buffer[cc])){fputc(buffer[cc], stdout);}
	else{printf("<%02x>", buffer[cc]);}
}
return;
*/

/*
"open",		// Open a file with a specific file name, by enumerating FAT. Params: <FILE NAME>
"dopen",	// Open a file within the currently opened directory. Params: <FILE NAME>
"close",	// Close the currently open file. Params: <NONE>
"read",		// Read off data from the currently open file into the quick label. Params: <INDEX, NUMBER OF BYTES TO READ>
"write",	// Write into the quick label's data with data from the currently open file. Params: <INDEX, DATA SIZE>
"dwrite",	// Write into the quick label with data from the Console, Data starts and ends with quote marks. Params: <INDEX, DATA SIZE, DATA TO WRITE(IN ASCII)>
"_fwrite",	// Write into the currently open file's contents with all the data of another file. Params: <FILE PATH, RECIEVING INDEX, SENDING INDEX, DATA_NUM>
"mread",	// Read off the currently open file's Metadata. Params: <NONE>
"mwrite",	// Read to the currently open file's Metadata. Params: <PROPERTY(LOWER_CASE)>, <VALUE(S)>
"create",	// Create a new file. Params: <FILE NAME, SIZE, FORMAT>
"lcreate",	// Create a new label <LABEL NAME, STARTING SIZE>
"seek",		// Seek to a location in the currently open file. Params: <LOCATION(HEX)>
"cseek",	// Add to or subtract from the seeking pointer of the currently open file. Params: <OFFSET>
"select",	// Select a specific label to be used. Params: <LABEL NAME>
*/
void cmd_handle(char *name){
	char sep[] = " ";
	size_t out_len = 0;
	//Buffers[0] is the quick-use buffer.
	printf("\ncommand: %s", name);
	char **out = parse_str(name, sep, &out_len);
	for(size_t cc = 0; cc < out_len; ++cc){printf(ANSI_YELLOW("\nParsed string output: \"%s\""), out[cc]);}
	uint32_t cc= 0;
	bool no_func = false;
	for(; cc < out_len; ++cc){
		for(uint32_t cc_cmd= 0; cc_cmd < num_commands && cc < out_len; ++cc_cmd){
			// printf(ANSI_YELLOW("\nComparing: %s with %s"), out[cc], commands[cc_cmd]); 
			if(out[cc][0] == commands[cc_cmd][0]){
				if(strncmp(out[cc], commands[cc_cmd], strlen(commands[cc_cmd])) == 0){
					switch(cc_cmd){
						/*
						"open",		// Open a file with a specific file name, by enumerating FAT. Params: <FILE NAME>
						"dopen",	// Open a file within the currently opened directory. Params: <FILE NAME>
						"close",	// Close the currently open file. Params: <NONE>
						"read",		// Read off data from the currently open file into the quick label. Params: <INDEX, NUMBER OF BYTES TO READ>
						"write",	// Write into the quick label's data with data from the currently open file. Params: <INDEX, DATA SIZE>
						"dwrite",	// Write into the quick label with data from the Console, Data starts and ends with quote marks. Params: <INDEX, DATA SIZE, DATA TO WRITE(IN ASCII)>
						"_fwrite",	// Write into the currently open file's contents with all the data of another file. Params: <FILE PATH, RECIEVING INDEX, SENDING INDEX, DATA_NUM>
						"mread",	// Read off the currently open file's Metadata. Params: <NONE>
						"mwrite",	// Read to the currently open file's Metadata. Params: <PROPERTY(LOWER_CASE)>, <VALUE(S)>
						"create",	// Create a new file. Params: <FILE NAME, SIZE, FORMAT>
						"lcreate",	// Create a new label <LABEL NAME, STARTING SIZE>
						"seek",		// Seek to a location in the currently open file. Params: <LOCATION(HEX)>
						"cseek",	// Add to or subtract from the seeking pointer of the currently open file. Params: <OFFSET>
						"select",	// Select a specific label to be used. Params: <LABEL NAME>
						*/
						case 0:	//open <FILE NAME>
							no_func = true;
							if((out_len - cc) < 2){
								printf(ANSI_RED("\ninvalid open command: SYNTAX: open <FILE NAME>"));
								cc+=2;
								continue;
							}
							FRopen(out[cc + 1]);
							cc+=2;
							continue;
						case 1:	//dopen <FILE NAME>
							no_func = true;
							if((out_len - cc) < 2){
								printf(ANSI_RED("\ninvalid dopen command: SYNTAX: dopen <FILE NAME>"));
								cc+=2;
								continue;
							}
							FRopen_d(out[cc + 1]);
							cc+=2;
							continue;
						case 2:	//close <NONE>
							no_func = true;
							fclose_(false, false);
							cc++;
							continue;
						case 3:	//read <INDEX, NUMBER OF BYTES TO READ>
							no_func = true;
							if((out_len - cc) < 3){
								printf(ANSI_RED("\ninvalid read command: SYNTAX: read <INDEX, NUMBER OF BYTES TO READ>"));
								cc+=3;
								continue;
							}
							if(buffer_num != 0 && curr_file != NULL){
								buffers[bf_select]->data = realloc(buffers[bf_select]->data, strtol(out[cc + 2], NULL, 10));
								buffers[bf_select]->data = Fread_curr(strtol(out[cc + 2], NULL, 10), strtol(out[cc + 1], NULL, 10));
							}
							cc+=3;
							continue;
						case 4:	//write <INDEX, DATA SIZE>
							no_func = true;
							// fseek(disk, FDATA_START + curr_file->lit_addr + strtol(out[cc + 1], NULL, 10), SEEK_SET);
							// buffers[0]->data = Fread_curr(strtol(out[cc + 2], NULL, 10), strtol(out[cc + 1], NULL, 10));
							if((out_len - cc) < 3){
								printf(ANSI_RED("\ninvalid write command: SYNTAX: write <INDEX, DATA SIZE>"));
								cc+=3;
								continue;
							}
							if(curr_file != NULL){
								if(buffers[bf_select]->data_size < strtol(out[cc + 2], NULL, 10)){
									printf(ANSI_RED("\nThe inputted size: %s is larger than the quick label(Size: %d)"), out[cc + 1], buffers[bf_select]->data_size);
									char buffer = 0;
									printf(ANSI_YELLOW("Continue with write? (Y/N)"));
									fgets(&buffer, 1, stdin);
									switch(buffer){
										case 'y':
											Fwrite_curr(buffers[bf_select]->data_size, strtol(out[cc + 1], NULL, 10), buffers[bf_select]->data);
											break;
										case 'n':
											break;
											// goto case4_finish;
									}
								}
								Fwrite_curr(strtol(out[cc + 2], NULL, 10), strtol(out[cc + 1], NULL, 10), buffers[bf_select]->data);
							}else{printf(ANSI_RED("\nERROR! No file has been opened!"));}
							cc+=3;
							continue;
						case 5:	//dwrite <INDEX, DATA TO WRITE(IN ASCII)>
							no_func = true;
							bool tmp = 0;
							if((out_len - cc) < 3){
								printf(ANSI_RED("\ninvalid dwrite command: SYNTAX: dwrite <INDEX, DATA SIZE, DATA TO WRITE(IN ASCII)>"));
								cc+=4;
								continue;
							}
							if(buffer_num == 0){
								printf(ANSI_RED("\nERROR!\nThere must be a buffer/label in existence..."));
								cc+=3;
								continue;
							}
							if(buffers[bf_select]->data_size < strlen(out[cc + 2] + 1) - 1){
text_attmpt:
								printf(ANSI_RED(
									"ERROR!!\n The selected label/buffer(%s) has a data size of %zu but the inputted data is larger than this(%zu)\n"
								) ANSI_YELLOW("Continue with write? (y/n):\n\t"),
									buffers[bf_select]->name, buffers[bf_select]->data_size, strlen(out[cc + 2] + 1) - 1);
								char buff = fgetc(stdin);
								if(buff == 'n'){
										cc+=3;
										continue;
								}else if(buff != 'y'){
									printf(ANSI_RED("Invalid input"));
									goto text_attmpt;
								}
							}
							buffers[bf_select]->data = realloc(buffers[bf_select]->data, strlen(out[cc + 2]));
							memcpy(buffers[bf_select]->data, out[cc + 2] + 1, (strlen(out[cc + 2] + 1) - 1 > buffers[bf_select]->data_size? buffers[bf_select]->data_size: strlen(out[cc + 2] + 1) - 1));
							cc+=3;
							continue;
						case 6:	//_fwrite <FILE PATH, RECIEVING INDEX, SENDING INDEX, DATA_NUM, (NOT MANDATORY)FORMAT>
							no_func = true;
							if((out_len - cc) < 5){
								printf(ANSI_RED("\ninvalid _fwrite command: SYNTAX: _fwrite <FILE PATH, RECIEVING INDEX, SENDING INDEX, DATA_NUM, (NOT MANDATORY)FORMAT>"));
								printf(ANSI_YELLOW("\nFormat(Only one):\n\tEXTERNAL,\n\tINTERNAL"));
								cc+=5;
								continue;
							}
							if(curr_file != NULL){
								file_entry entry_f = *curr_file;
								// free(curr_file);
								// curr_file = NULL;
								if(out_len >= 6){
									for(uint8_t cc_ = 0; cc_ < strlen(out[cc + 5]); ++cc_){out[cc + 5][cc_] = tolower(out[cc + 5][cc_]);}
									if(!strncmp(out[cc + 5], "external", 8)){
										FILE *f = fopen(out[cc + 1], "rb");
										void *data = malloc(strtol(out[cc + 4], NULL, 10));
										fseek(f, strtol(out[cc + 3], NULL, 10), SEEK_SET);
										fread(data, 1, strtol(out[cc + 4], NULL, 10), f);
										Fwrite_curr(strtol(out[cc + 4], NULL, 10), strtol(out[cc + 2], NULL, 10), data);
									}else if(!strncmp(out[cc + 5], "internal", 8)){goto internal_read;}
								}else{
									internal_read:
									FRopen(out[cc + 1]);
									void *read = Fread_curr(strtol(out[cc + 4], NULL, 10), strtol(out[cc + 3], NULL, 10));
									fclose_(true, true);
									*curr_file = entry_f;
									Fwrite_curr(strtol(out[cc + 4], NULL, 10), strtol(out[cc + 2], NULL, 10), read);
									free(read);
								}
							}else{printf(ANSI_RED("\nERROR! No file has been opened!"));}
							cc+=5;
							continue;
						case 7:	// mread <NONE>
							no_func = true;
							if(curr_file != NULL){
								if(curr_file->FAT_index > FAT_num){printf(ANSI_RED("Current file has invalid FAT entry!"));
								}else{
									printf(
										ANSI_YELLOW(
											"\nFAT:\n"
											"\n\tname_fmt:%hhu"
											"\n\t_attributes:%hhu"
											"\n\tliteral address: %zu"
											"\n\tname hash:..."
										),
										((FAT_entry *)((uint8_t *)FAT + curr_file->FAT_index))->name_fmt,
										((FAT_entry *)((uint8_t *)FAT + curr_file->FAT_index))->_attributes,
										((FAT_entry *)((uint8_t *)FAT + curr_file->FAT_index))->lit_addr
									);
								}
								printf(
									ANSI_YELLOW(
										"\nFile:\n"
										"\tname length: %u\n"
										"\tattributes:\n"
											"\t\tDirectory?: %s\n"
											"\t\tLinux exe?: %s\n"
											"\t\tWindows exe?: %s\n"
											"\t\tSystem file?: %s\n"
											"\t\tIs file?: %s\n"
											"\t\t...: ...\n"	
										"\tcreation time: %hhu\\%hhu\\%hhu\n"
										"\tcreation date: %hu\n"
										"\taccess date: %hu\n"
										"\tmodified time: %hhu\\%hhu\\%hhu\n"
										"\tmodified date: %hu\n"
										"\tsize: %zu\n"
										"\tliteral address: %zu\n"
										"\tFAT index: %u\n"
										"\tname: %s\n"
									),
									curr_file->name_len, 
									(IS_DIR(curr_file->_attributes)? "TRUE": "FALSE"), (IS_LINEXE(curr_file->_attributes)? "TRUE": "FALSE"), (IS_WINEXE(curr_file->_attributes)? "TRUE": "FALSE"), 
									(IS_SYSFILE(curr_file->_attributes)? "TRUE": "FALSE"), (IS_FILE(curr_file->_attributes)? "TRUE": "FALSE"), 
									get_hour(curr_file->creation_time), get_minute(curr_file->creation_time), get_second(curr_file->creation_time), 
									curr_file->creation_date, curr_file->access_date, 
									get_hour(curr_file->modified_time), get_minute(curr_file->modified_time), get_second(curr_file->modified_time), 
									curr_file->modified_date, 
									curr_file->size, curr_file->lit_addr, curr_file->FAT_index, curr_file->name
								);
								for(uint32_t cc = 0; cc < FILE_SIZE(curr_file); ++cc){
									if(isprint(((uint8_t *)curr_file)[cc])){fputc(((uint8_t *)curr_file)[cc], stdout);}
									else{printf(ANSI_YELLOW("<%02x>"), ((uint8_t *)curr_file) + cc);}
								}
							}else{printf(ANSI_RED("\nERROR! No file has been opened!"));}
							cc++;
							continue;
						case 8:	// mwrite <PROPERTY(LOWER_CASE), VALUE(S), (NOT MANDATORY)FORMAT>
							no_func = true;
							/*
								Properties:
									uint8_t name_fmt;
									uint8_t _attributes;
									size_t lit_addr;
									uint8_t name[];
							*/
							if((out_len - cc) < 3){
								printf(ANSI_RED("\ninvalid mwrite command: SYNTAX: _fwrite <FILE PATH, RECIEVING INDEX, SENDING INDEX, DATA_NUM>"));
								cc+=2;
								continue;
							}
							if(curr_file != NULL){
								if(strncmp(*out, "name", 4) == 0){
									size_t *hash_n = NULL;
									curr_file = realloc(curr_file, sizeof(file_entry) + strlen(out[1]));
									memcpy(curr_file->name, out[1], strlen(out[1]));
									size_t fat_index = 0;
									get_FATsize(0, curr_file->FAT_index, &fat_index, true);
									((FAT_entry *)(((void *)FAT)+fat_index))->name_fmt = hash(out[1], &hash_n);
									memcpy(((FAT_entry *)(((void *)FAT)+fat_index))->name, hash_n, HF_SIZE(((FAT_entry *)(((void *)FAT)+fat_index))->name_fmt));
								}
							}else{printf(ANSI_RED("\nERROR! No file has been opened!"));}
							cc+=2;
							continue;
						case 9: // create <FILE NAME, SIZE, FORMAT>
							no_func = true;
							if((out_len - cc) < 4){
								printf(ANSI_RED("\ninvalid create command: SYNTAX: create <FILE NAME, SIZE, FORMAT>"));
								cc+=4;
								continue;
							}
							size_t len = 0;
							get_FATsize(0, FAT_num + 1, &len, true);
							size_t flen = Fget_free(0, ((out_len - cc) > 2?strtol(out[cc + 1], NULL, 10): 0));
							size_t *hash_n = NULL;
							uint8_t fmt = hash(out[cc + 1], &hash_n);
							FAT_entry *entry = malloc(sizeof(FAT_entry));
							*entry = (FAT_entry){
								.lit_addr = flen,
								.name_fmt = fmt,
								._attributes = 0,
							};
							// Directly copy in the hash
							uint8_t *temp = ((uint8_t *)FAT) + len;
							memcpy(temp, entry, sizeof(FAT_entry));
							memcpy(temp + sizeof(FAT_entry), hash_n, HF_SIZE(fmt));
							time_t now = time(NULL);
							struct tm *t = localtime(&now);
							file_entry *file = malloc(sizeof(file_entry) + strlen(out[cc + 1]));
							*file = (file_entry){
								.padding = {0, 0, 0, 0},
								.name_len = strlen(out[cc + 1]),
								._attributes = decode_attr(((out_len - cc) >= 4?out[cc + 3]: NULL)),
								.creation_time = encode_time(t->tm_hour, t->tm_min, t->tm_sec),
								.creation_date = t->tm_yday,
								.access_date = t->tm_yday,
								.modified_time = encode_time(t->tm_hour, t->tm_min, t->tm_sec),
								.modified_date = t->tm_yday,
								.size = strtol(out[cc + 2], NULL, 10),
								.lit_addr = flen,
								.FAT_index = FAT_num++,
							};
							printf(
								ANSI_YELLOW(
									"\nFile:\n"
									"\tname length: %u\n"
									"\tattributes:\n"
										"\t\tDirectory?: %s\n"
										"\t\tLinux exe?: %s\n"
										"\t\tWindows exe?: %s\n"
										"\t\tSystem file?: %s\n"
										"\t\tIs file?: %s\n"
										"\t\t...: ...\n"	
									"\tcreation time: %hhu\\%hhu\\%hhu\n"
									"\tcreation date: %hu\n"
									"\taccess date: %hu\n"
									"\tmodified time: %hhu\\%hhu\\%hhu\n"
									"\tmodified date: %hu\n"
									"\tsize: %zu\n"
									"\tliteral address: %zu\n"
									"\tFAT index: %u\n"
									"\tname: %s\n"
								),
								file->name_len, 
								(IS_DIR(file->_attributes) == true? "TRUE": "FALSE"), (IS_LINEXE(file->_attributes) == true? "TRUE": "FALSE"), (IS_WINEXE(file->_attributes) == true? "TRUE": "FALSE"), 
								(IS_SYSFILE(file->_attributes) == true? "TRUE": "FALSE"), (IS_FILE(file->_attributes) == true? "TRUE": "FALSE"), 
								get_hour(file->creation_time), get_minute(file->creation_time), get_second(file->creation_time), 
								file->creation_date, file->access_date, 
								get_hour(file->modified_time), get_minute(file->modified_time), get_second(file->modified_time), 
								file->modified_date, 
								file->size, file->lit_addr, file->FAT_index, out[cc + 1]
							);
							memcpy(file->name, out[cc + 1], strlen(out[cc + 1]));
							fseek(disk, FDATA_START + entry->lit_addr, SEEK_SET);	// Adress: 10240 bytes
							printf("\nCreating file/directory: %s\n", out[cc + 1]);
							_fwrite(file, sizeof(file_entry) + strlen(out[cc + 1]), 1);
							// FAT_num++;
							cc+=4;
							continue;
						case 10:	// lcreate <LABEL NAME, STARTING SIZE>
							no_func = true;
							if((out_len - cc) < 3){
								printf(ANSI_RED("\ninvalid lcreate command: SYNTAX: lcreate <LABEL NAME, STARTING SIZE>"));
								cc+=3;
								continue;
							}
							if(strtol(out[cc + 2], NULL, 10) == 0){
								printf(ANSI_RED("\n ERROR! \nA label cannot have a size of 0..."));
								cc+=3;
								continue;
							}
							buffer_gen(strtol(out[cc + 2], NULL, 10), out[cc + 1]);
							printf(ANSI_YELLOW("\nCreating buffer %s of size: %zu\n"), out[cc + 1], strtol(out[cc + 2], NULL, 10));
							cc+=3;
							continue;
						case 11: 	// select <LABEL NAME>
							no_func = true;
							if((out_len - cc) < 2){
								printf(ANSI_RED("\ninvalid select command: SYNTAX: select <LABEL NAME>"));
								cc+=2;
								continue;
							}
							buffer_search(out[cc + 1], NULL);
							cc+=2;
							continue;
						case 12:	// help <NONE>	
							no_func = true;
							printf(ANSI_YELLOW(
								"\n"
								"open,			Open a file with a specific file name, by enumerating FAT. Params: <FILE NAME>\n"
								"dopen,			Open a file within the currently opened directory. Params: <FILE NAME>\n"
								"close,			Close the currently open file. Params: <NONE>\n"
								"read,			Read off data from the currently open file into the quick label. Params: <INDEX, NUMBER OF BYTES TO READ>\n"
								"write,			Write into the quick label's data with data from the currently open file. Params: <INDEX, DATA SIZE>\n"
								"dwrite,		Write into the quick label with data from the Console, Data starts and ends with quote marks. Params: <INDEX, DATA TO WRITE(IN ASCII)>\n"
								"_fwrite,		Write into the currently open file's contents with all the data of another file. Params: <FILE PATH, RECIEVING INDEX, SENDING INDEX, DATA_NUM>\n"
								"mread,			Read off the currently open file's Metadata. Params: <NONE>\n"
								"mwrite,		Read to the currently open file's Metadata. Params: <PROPERTY(LOWER_CASE)>, <VALUE(S)>\n"
								"create,		Create a new file. Params: <FILE NAME, SIZE, FORMAT>\n"
								"lcreate,		Create a new label <LABEL NAME, STARTING SIZE>\n"
								"select,		Select a specific label to be used. Params: <LABEL NAME>\n"
								"help,			Print help info. Params: <NONE>\n"
								"llist,			Print the currently selected label and it's data. Params: <DATA ROWS, DATA MAX>\n"
								"exit,			Exit the application.\n"
								"flist			List all the entries in FAT.\n"
								"clear			Clear the screen.\n"
								"lremove 		Remove/delete a label. Params: <LABEL NAME>"
								"ltell 			List all the labels/buffers in Memory. Params: <DATA ROWS, DATA MAX>"
							));
							cc++;
							continue;
						case 13:	// llist <DATA ROWS, DATA MAX>
							no_func = true;
							if((out_len - cc) < 3){
								printf(ANSI_RED("\ninvalid llist command: SYNTAX: llist <DATA ROWS, DATA MAX>"));
								cc+=2;
								continue;
							}
							printf(ANSI_YELLOW(
								"\nSelected label: %d\n"
								"Name: %s\n"
								"ID: %d\n"
								"Data size: %d\n"
								"Data:\n"
							), bf_select, buffers[bf_select]->name, buffers[bf_select]->ID, buffers[bf_select]->data_size);
							if((strtol(out[cc + 2], NULL, 10) % strtol(out[cc + 1], NULL, 10)) != 0){
								printf(ANSI_RED(
									"\nERROR! Invalid arguments... "
									"\nDATA ROWS MUST BE A FACTOR OF DATA MAX"
									"\nDATA MAX MUST BE LESS THAN DATA ROWS"
									"\n..."
								));
								cc+=2;
								continue;
							}
							if(buffers[bf_select]->data_size < strtol(out[cc + 2], NULL, 10)){
								printf(ANSI_RED("\nCannot read up to %d bytes, Label: %s\'s Max size is: %d... Printing up to %d\n"),
								strtol(out[cc + 2], NULL, 10), buffers[bf_select]->name, buffers[bf_select]->data_size, buffers[bf_select]->data_size);
							}
							uint32_t cc_ = 0;
							for(uint32_t y = 0; y < (strtol(out[cc + 2], NULL, 10) / strtol(out[cc + 1], NULL, 10)) && cc_ < strtol(out[cc + 2], NULL, 10); ++y){
								for(uint32_t x = 0; x < strtol(out[cc + 1], NULL, 10); ++x){
									if(isprint(((uint8_t *)buffers[bf_select]->data)[cc_])){printf(ANSI_LBLUE("%d"), ((uint8_t *)buffers[bf_select]->data)[cc_]);}
									else{printf(ANSI_LBLUE("%02x"), ((uint8_t *)buffers[bf_select]->data)[cc_]);}
									cc_++;
								}
								printf("\n");
							}
							// for(uint32_t cc__ = 0; cc__ < buffers[cc_]->data_size && cc__ < strtol(out[cc + 2], NULL, 10); ++cc__){
							// 	if(isprint(((uint8_t *)buffers[cc_]->data)[cc__])){
							// 	else{printf(ANSI_BLUE("<%02x> "), ((uint8_t *)buffers[cc_]->data)[cc__]);}
							// 	if((cc % strtol(out[cc + 1], NULL, 10)) == 0){printf("\n");}
							// }
							cc+=3;
							continue;
						case 14:	// exit
							no_func = true;
							exit(EXIT_SUCCESS);
						case 15:	// flist
							no_func = true;
							for(FATl_t cc_ = 0; cc_ < FAT_num; ++cc_){
								size_t len = 0;
								get_FATsize(0, cc_ + 1, &len, true);
								FAT_entry *entry = (FAT_entry *)(((uint8_t *)FAT) + len);
								// entry->name
								// entry->name_fmt
								// entry->_attributes
								// entry->lit_addr
								printf(ANSI_YELLOW(
									"\n[%d]:\n"
									"\t\nName fmt: %hhd\n"
									"\tattributes:\n"
										"\t\tDirectory?: %s\n"
										"\t\tLinux exe?: %s\n"
										"\t\tWindows exe?: %s\n"
										"\t\tSystem file?: %s\n"
										"\t\tIs file?: %s\n"
										"\t\t...: ...\n"
									"\tliteral addr: %zu"
								),
								cc_, entry->name_fmt, 
								(IS_DIR(entry->_attributes) == true? "TRUE": "FALSE"), (IS_LINEXE(entry->_attributes) == true? "TRUE": "FALSE"), (IS_WINEXE(entry->_attributes) == true? "TRUE": "FALSE"), 
								(IS_SYSFILE(entry->_attributes) == true? "TRUE": "FALSE"), (IS_FILE(entry->_attributes) == true? "TRUE": "FALSE"), 
								entry->lit_addr
								);
								printf("\nHash: ");
								for(uint8_t cc_ =0; cc_ < HF_SIZE(entry->name_fmt); ++cc_){printf(ANSI_YELLOW("%zu"), entry->name[cc_]);}
							}
							cc++;
							continue;
						case 16:	// clear
							no_func = true;
							printf("\x1b[2J\x1b[H");
							cc++;
							continue;
						case 17:	// lremove <LABEL NAME>
							no_func = true;
							if((out_len - cc) < 2){
								printf(ANSI_RED("\ninvalid lremove command: SYNTAX: lremove <LABEL NAME>"));
								cc+=2;
								continue;
							}
							uint32_t selected_temp = bf_select;
							if(buffer_search(out[cc + 1], NULL)){
								free(buffers[bf_select]->data);
								free(buffers[bf_select]->name);
								free(buffers[bf_select]);
								for(uint32_t cc = bf_select; cc < buffer_num; ++cc){buffers[bf_select] = buffers[bf_select + 1];}
								buffer_num--;
								selected_buffer = selected_temp;
							}
							cc+=2;
							continue;
						case 18:	// ltell <DATA ROWS, DATA MAX>
							no_func = true;
							if((out_len - cc) < 3){
								printf(ANSI_RED("\ninvalid ltell command: SYNTAX: ltell <DATA ROWS, DATA MAX>"));
								cc+=2;
								continue;
							}
							for(size_t cc_ =0; cc_ < buffer_num; ++cc_){
								printf(ANSI_YELLOW(
									"\n[%d]\n"
									"\tName: %s\n"
									"\tID: %d\n"
									"\tData size: %d\n"
									"\tData:\n"
								), cc_, buffers[cc_]->name, buffers[cc_]->ID, buffers[cc_]->data_size);
								if(strtol(out[cc + 1], NULL, 10) != 0 && strtol(out[cc + 1], NULL, 10) != 0){
									if((strtol(out[cc + 2], NULL, 10) % strtol(out[cc + 1], NULL, 10)) != 0){
										printf(ANSI_RED(
											"\nERROR! Invalid arguments... "
											"\nDATA ROWS MUST BE A FACTOR OF DATA MAX"
											"\nDATA MAX MUST BE LESS THAN DATA ROWS"
											"\n..."
										));
										cc+=2;
										continue;
									}
								}
								if(buffers[cc_]->data_size < strtol(out[cc + 2], NULL, 10)){
									printf(ANSI_RED("\nCannot read up to %d bytes, Label: %s\'s Max size is: %d... Printing up to %d\n"),
									strtol(out[cc + 2], NULL, 10), buffers[cc_]->name, buffers[cc_]->data_size, buffers[cc_]->data_size);
								}
								uint32_t cc__ = 0;
								for(uint32_t y = 0; y < (strtol(out[cc + 2], NULL, 10) / strtol(out[cc + 1], NULL, 10)) && cc_ < strtol(out[cc + 2], NULL, 10); ++y){
									printf("\t");
									for(uint32_t x = 0; x < strtol(out[cc + 1], NULL, 10); ++x){
										if(isprint(((uint8_t *)buffers[cc_]->data)[cc__])){printf(ANSI_LBLUE("%d"), ((uint8_t *)buffers[cc_]->data)[cc__]);}
										else{printf(ANSI_LBLUE("%02x"), ((uint8_t *)buffers[cc_]->data)[cc__]);}
										cc__++;
									}
									printf("\n");
								}
							}
							cc+=3;
							continue;
						default:
							printf(ANSI_RED("\nERROR! Unidentified command:") " " ANSI_LBLUE("\"%s\""), name);
							cc=out_len;
							continue;
					}
				}
			}
		}
	}
	if(!no_func){printf(ANSI_RED("\nERROR! Unidentified command:") " " ANSI_LBLUE("\"%s\""), name);}
	for(uint32_t cc_ =0; cc_ < out_len; ++cc_){free(out[cc_]);}
	free(out);
}

#include <signal.h>

#ifdef _WIN32
#include <windows.h>
void enable_ansi() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
	Virt_active = true;
}
#endif

int main(uint32_t arg_cc, char **arg_vector){
	signal(SIGTERM, sighandle);
	signal(SIGABRT, sighandle);
	Virt_active = false;
	#ifdef _WIN32
		enable_ansi();
	#endif
	for(uint32_t cc = 0; cc < arg_cc; ++cc){printf("\nArg[%u]: %s", cc, arg_vector[cc]);}
	disk = _fopen(arg_vector[1]);
	IMG_setup();
	// cmd_handle("create skibidi 0 DIR");			// Create an arbitrary FAT entry, that is a directory and has not entries.
	// cmd_handle("create skibidi.txt 100 FILE");	// Create an arbitrary FAT enty that is a File and hash 100 bytes of data
	// cmd_handle("lcreate SKIBIDI 10");
 	// cmd_handle("open skibidi.txt");				// Open "skibidi.txt"
	// cmd_handle("dwrite 0 10 \"9999999999\"");		// Write 10 characters of data: "\"9999999999\"" into temp_buffers[0]; the quick buffer, at index 0
	// cmd_handle("write 0, 10");					// Write ten bytes of temp_buffers[0]; the quick buffer into the currently open file
	bool RUN = true;
	do{
		char buffer[256];
		printf("\nInput a command...\n");
		memset(buffer, 256, 0);
		fgets(buffer, 256, stdin);
		buffer[strlen(buffer) - 1] = 0;	// Remove new-line terminator.
		cmd_handle(buffer);
	}while(RUN);
	return EXIT_SUCCESS;
}

void sighandle(int sig){
	printf(ANSI_RED("FATAL ERROR!, CODE: %d"), sig);
	switch(sig){
		case SIGTERM:
		case SIGABRT:
			fseek(disk, FAT_START, SEEK_SET);
			size_t len = 0;
			get_FATsize(0, FAT_num, &len, true);
			_fwrite(&FAT_num, sizeof(size_t), 1);
			_fwrite(FAT, 1, len);
			fflush(disk);
			return;
	}
}

char *buff_path;
/// @brief The format used for understanding the current mode.
uint8_t fmt;
FILE *_fopen(char *path){
	buff_path = strdup(path);
	fmt = 'r';
	FILE *temp = fopen(path, "rb+");
	return temp;
}

size_t _fread(void *dst, size_t element_size, size_t count){
	// if(fmt == 'w'){
	// 	fclose(disk);
	// 	disk = fopen(buff_path, "rb+");
	// 	fmt = 'r';
	// }
	return fread(dst, element_size, count, disk);
}

size_t _fwrite(void *src, size_t element_size, size_t count){
	// if(fmt == 'r'){
	// 	fclose(disk);
	// 	disk = fopen(buff_path, "rb+");
	// 	fmt = 'w';
	// }
	size_t out = fwrite(src, element_size, count, disk);
	fflush(disk);
	return out;
}