#include "f-rat.h"

#ifdef _WIN32
#include <windows.h>
#include <consoleapi.h>
void enable_ansi(void){
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
	Virt_active = true;
}
#endif
int consume(uint32_t arg_cc, char **arg_vector){
    #ifdef _WIN32
		enable_ansi();
	#endif
    for(uint32_t cc = 0; cc < arg_cc; ++cc){printf("\nArg[%u]: %s", cc, arg_vector[cc]);}
    if((disk = fopen(arg_vector[1], "rb+")) == NULL){
        IMG_Setup();
    }
}

void IMG_Setup(void){
	fseek(disk, 0, SEEK_SET);
    if(fread(&drive, sizeof(drive_header), 1, disk) != 1){
		printf(ANSI_RED("Drive drive reading failed, exiting..."));
		exit(EXIT_FAILURE);
	}
    if(drive.bytes_per_sector == 0){
		printf(ANSI_RED("Floppy image was NOT valid..."));
		exit(EXIT_FAILURE);
	}

	FAT = malloc(drive.sectors_per_fat * drive.bytes_per_sector);
	memset(FAT, 0, (size_t)(drive.sectors_per_fat * drive.bytes_per_sector));
	fseek(disk, FAT_START, SEEK_CUR);
	fread(FAT, 1, (drive.sectors_per_fat * drive.bytes_per_sector), disk);

	if(USED_CLUSTERMAP){
		basebyte_counter = MAPBASE_SIZE * USED_CLUSTERMAP;
		base_buffer = malloc(sizeof(size_t) * (USED_CLUSTERMAP + 1));
		MAP_ext_b *temp_buffer = malloc(basebyte_counter);
		base_buffer[0] = temp_buffer;
		fseek(disk, CLUSTERMAP_START, SEEK_SET);
		fread(temp_buffer, MAPBASE_SIZE, USED_CLUSTERMAP, disk);
		//Validate to make sure the buffer encompasses the whole of the Cluster Map
		size_t true_counter = 0;
		for(uint32_t cc =0; cc < USED_CLUSTERMAP; ++cc){
			base_buffer[cc + 1] = (void *)temp_buffer + true_counter;
			true_counter += ((MAP_ext_b *)(((void *)temp_buffer) + true_counter))->num_extensions * drive.bytes_per_sector * drive.sectors_per_cluster;
			true_counter += sizeof(MAP_ext_b);
			if(true_counter > basebyte_counter){
				temp_buffer = realloc(temp_buffer, true_counter);
				fread(temp_buffer + basebyte_counter, 1, true_counter - basebyte_counter, disk);
				basebyte_counter = true_counter;
			}
		}
	}
}

void FATCompress(void){
	FAT_e *temp = malloc(drive.bytes_per_sector * drive.sectors_per_fat);
	uint32_t counter = 0;
	for(uint32_t cc = 1; cc < USED_FAT; ++cc){
		if(FAT[cc].segment != 0){
			temp[counter] = FAT[cc];
			counter++;
		}
	}
	free(FAT);
	FAT = temp;
	return;
}

void writed_MAPDATA(MAP_ext_b *file, INDEX_e address, char *path, long offset, uint8_t segment_size, long segment, long max_data){
	FILE *file = fopen(path, "rb");
	fseek(file, 0, SEEK_SET);
	for(long cc = 0; cc < segment; ++cc){fseek(file, segment_size, SEEK_CUR);}
	fseek(file, offset, SEEK_CUR);
	uint8_t *data = malloc(max_data);
	size_t read_ = fread(data, 1, max_data, file);
	if(read_ < max_data){printf(ANSI_RED("Could no read %d bytes from %s"), max_data, path);}
	write_MAPDATA(file, address, read_, read_);
	free(read_);
}
void write_MAPDATA(MAP_ext_b *file, INDEX_e address, uint8_t *data, uint8_t num){
	if(IS_FILE(file->metadata.attributes)){
		if(
			(drive.bytes_per_sector * drive.sectors_per_cluster * address.segment) < file->metadata.size ||
			address.offset < (MAPBASE_SIZE - sizeof(MAP_ext_b))
		){
			MAP_ext_e cluster_address = file->cluster_entries[address.segment + MAPBASE_ENTRYSTART(file)];
			fseek(disk, CLUSTERMAP_START, SEEK_SET);
			for(uint32_t cc = 0; cc < cluster_address.segment; ++cc){fseek(disk, drive.segment_clusters, SEEK_CUR);}
			fseek(disk, cluster_address.offset, SEEK_CUR);
			fwrite(data, 1, num, disk);
		}
	}
}

void print_MAPDATA(MAP_ext_b *file, uint8_t base, INDEX_e address){
	// if(elements > (drive.bytes_per_sector * drive.sectors_per_cluster)){return;}
	if(IS_FILE(file->metadata.attributes)){
		uint8_t max_ = ((drive.bytes_per_sector * drive.sectors_per_cluster * address.segment) + address.offset) > file->metadata.size ? 
							((drive.bytes_per_sector * drive.sectors_per_cluster * address.segment) + address.offset) % file->metadata.size: address.offset;
		MAP_ext_e cluster_address = file->cluster_entries[address.segment + MAPBASE_ENTRYSTART(file)];
		fseek(disk, CLUSTERMAP_START, SEEK_SET);
		for(uint32_t cc = 0; cc < cluster_address.segment; ++cc){fseek(disk, drive.segment_clusters, SEEK_CUR);}
		fseek(disk, cluster_address.offset, SEEK_CUR);
		char *out = malloc(address.offset);
		fread(out, 1, address.offset, disk);
		for(uint8_t cc = 0; cc < address.offset; ++cc){
			char print[9] = {0};
			byte_to_base(out[cc], base, print);
			printf(ANSI_CYAN("%s "), print);
		}
	}
}

static const char DIGITS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
void byte_to_base(uint8_t value, uint32_t base, char out[8]){
    if(base < 2 || base > 36){
        out[0] = '\0';
        return;
    }

    char buf[8];       // enough for base-2 representation of a byte
    int pos = 0;
    do{
        buf[pos++] = DIGITS[value % base];
        value /= base;
    }while (value > 0);

    // reverse into output buffer
    for(int i = 0; i < pos; i++){out[i] = buf[pos - 1 - i];}
    out[pos] = '\0';
}


void print_MAPbase(MAP_ext_b *base){
	printf(ANSI_YELLOW
		("\nName: %s"
		"\n Security Code: %u%u%u%u%u%u%u%u%u%u%u%u%u%u%u"
		"\n Num extensions: %u"
		"\nMetadata: "
		"\n\t Creation time: %d/%d/%d"
		"\n\t Modified time: %d/%d/%d"
		"\n\t Creation Date: %"PRIu16
		"\n\t Access Date: %"PRIu16
		"\n\t Modified Date: %"PRIu16
		"\n\tSize: %zu"
		"\n Attributes:"
		"\n\tIs Directory: %s"
		"\n\tIs File: %s"
		"\n\tIs System: %s"
		"\n\tWrite Finish: %s"
		"\n\tPoll Finish: %s"
		"\n\tIs Archive: %s"
		"\n\tIs Encrypted: %s"
		"\n\tIs Corrupted: %s"
		"\nFAT: [%d:%d]"),
		// 0b0000000000000000, 2 bytes.
		base->cluster_entries, base->security_code, base->num_extensions,
		(base->metadata.creation_time & 0xf800), ((base->metadata.creation_time & 0x07e0) - base->metadata.creation_time & 0x0001), (base->metadata.creation_time & 0x001e), 
		(base->metadata.modified_time & 0xf800), ((base->metadata.modified_time & 0x07e0) - base->metadata.modified_time & 0x0001), (base->metadata.modified_time & 0x001e), 
		base->metadata.creation_date, base->metadata.access_date, base->metadata.modified_date,
		base->metadata.size, 
		(base->metadata.attributes & 0b00000001? "TRUE": "FALSE"), (base->metadata.attributes & 0b00000010? "TRUE": "FALSE"), (base->metadata.attributes & 0b00000100? "TRUE": "FALSE"), (base->metadata.attributes & 0b00001000? "TRUE": "FALSE"), 
		(base->metadata.attributes & 0b00010000? "TRUE": "FALSE"), (base->metadata.attributes & 0b00100000? "TRUE": "FALSE"), (base->metadata.attributes & 0b01000000? "TRUE": "FALSE"), (base->metadata.attributes & 0b10000000? "TRUE": "FALSE"), 
		base->FAT
	);
}

void print_DIRENTRIES(MAP_ext_b *base, char *mode){
	if(IS_DIR(base->metadata.attributes)){
		MAP_ext_e temp = {0, 0, 0};
		for(uint32_t cc = MAPBASE_ENTRYSTART(base); cc < ((base->num_extensions * drive.bytes_per_sector * drive.sectors_per_cluster)/sizeof(MAP_ext_e)); cc++){
			MAP_ext_b *file = get_Item(base->cluster_entries[cc]);
			if(strcheck(mode, 'n')){printf(ANSI_YELLOW("\nName: $s"), file->cluster_entries);}
			if(strcheck(mode, 'c')){printf(ANSI_YELLOW("\n Security Code: %u%u%u%u%u%u%u%u%u%u%u%u%u%u%u"), file->security_code);}
			if(strcheck(mode, 'e')){printf(ANSI_YELLOW("\n: %u"), file->num_extensions);}
			if(strcheck(mode, 'M')){printf(ANSI_YELLOW
				("\nMetadata: "
				"\n\t Creation time: %d/%d/%d"
				"\n\t Modified time: %d/%d/%d"
				"\n\t Creation Date: %"PRIu16
				"\n\t Access Date: %"PRIu16
				"\n\t Modified Date: %"PRIu16
				"\n\tSize: %zu"),
				(base->metadata.creation_time & 0xf800), ((base->metadata.creation_time & 0x07e0) - base->metadata.creation_time & 0x0001), (base->metadata.creation_time & 0x001e), 
				(base->metadata.modified_time & 0xf800), ((base->metadata.modified_time & 0x07e0) - base->metadata.modified_time & 0x0001), (base->metadata.modified_time & 0x001e), 
				base->metadata.creation_date, base->metadata.access_date, base->metadata.modified_date,
				base->metadata.size
			);}
			if(strcheck(mode, 'A')){printf(ANSI_YELLOW
				("\n Attributes:"
				"\n\tIs Directory: %s"
				"\n\tIs File: %s"
				"\n\tIs System: %s"
				"\n\tWrite Finish: %s"
				"\n\tPoll Finish: %s"
				"\n\tIs Archive: %s"
				"\n\tIs Encrypted: %s"
				"\n\tIs Corrupted: %s"),
				(base->metadata.attributes & 0b00000001? "TRUE": "FALSE"), (base->metadata.attributes & 0b00000010? "TRUE": "FALSE"), (base->metadata.attributes & 0b00000100? "TRUE": "FALSE"), (base->metadata.attributes & 0b00001000? "TRUE": "FALSE"), 
				(base->metadata.attributes & 0b00010000? "TRUE": "FALSE"), (base->metadata.attributes & 0b00100000? "TRUE": "FALSE"), (base->metadata.attributes & 0b01000000? "TRUE": "FALSE"), (base->metadata.attributes & 0b10000000? "TRUE": "FALSE")
			);}
			if(strcheck(mode, 'f')){printf("\nFAT: [%d:%d]", file->FAT);}
		}
	}
}

void Fcreate(char *name, char *mode){
	time_t temp;	time(&temp);
	struct tm *time_ = localtime(&temp);
	FAT_update = BASEMalloc();
	FAT_updateindex = USED_FAT + 1;
	base_update = malloc(sizeof(MAP_ext_b) + strlen(name));
	(*base_update) = (MAP_ext_b){
		.security_code = {0},
		.padding = {0},
		.num_extensions = 0,
		.name_len = strlen(name),
		.FAT.address = (size_t)USED_FAT,
		.metadata = {
			.attributes = 		((strcheck(mode, 'f') << 6) & (!strcheck(mode, "d"))) + (strcheck(mode, 's') << 5) +
								(strcheck(mode, 'w') << 4) + (strcheck(mode, 'p') << 3) + (strcheck(mode, 'a') << 2) +
								(strcheck(mode, 'e') << 1) + strcheck(mode, 'c'),
			.creation_date = 	time_->tm_yday, 	.access_date = time_->tm_yday, 		.modified_date = time_->tm_yday, 
			.creation_time = 	pack_time(time_),
			.modified_time = 	pack_time(time_),
			.size = 0,
		},
	};
	memcpy(base_update + sizeof(MAP_ext_b), name, strlen(name));
	free(time_);
	FUpdate();
}

uint16_t pack_time(struct tm *t){
    uint16_t sec_half = (t->tm_sec / 2) & 0x1F;   // normal FAT seconds/2
    uint16_t sec_flag = (t->tm_sec >= 30) ? 1 : 0; // lowest bit flag

    uint16_t mins = (t->tm_min & 0x3F) << 5;      // 6 bits
    uint16_t hrs  = (t->tm_hour & 0x1F) << 11;    // 5 bits

    return (sec_half << 1) | sec_flag | mins | hrs;
}

void FDelete(FAT_e entry){
	for(uint32_t cc = 0; cc < USED_FAT; ++cc){
		if(FAT[cc].address == entry.address){
			FAT_updateindex = cc;
			FUpdate();
		}
	}
}

MAP_ext_b *base_update;
/// @brief If [0:ANY] then the item ar FAT_updateindex will be removed.
FAT_e FAT_update;
/// @brief If 0 then FAT space allocated for the header_update if it's not NULL
uint32_t FAT_updateindex;
void FUpdate(){
	// Handle FAT Update,
	FATUpdate(FAT_updateindex, FAT_update);
	// Handle MAP_ext_b Update,
	if(base_update){
		*base_buffer = realloc(*base_buffer, basebyte_counter + sizeof(MAP_ext_b) + (base_update->num_extensions * drive.bytes_per_sector * drive.sectors_per_cluster));
		memcpy(*base_buffer + basebyte_counter, base_update, sizeof(MAP_ext_b) + (base_update->num_extensions * drive.bytes_per_sector * drive.sectors_per_cluster));
		base_buffer = realloc(base_buffer, sizeof(void *) * (USED_CLUSTERMAP + 2));
	}
	fseek(disk, FAT_START, SEEK_SET);
	if(FAT && (USED_FAT * sizeof(FAT_e)) < (drive.bytes_per_sector * drive.sectors_per_fat)){
		fwrite(FAT, sizeof(FAT_e), USED_FAT, disk);
		fwrite(FAT, sizeof(FAT_e), USED_FAT, disk);
	}
	BASESync();
	// free(FAT);
	// free(*base_buffer);
	// free(base_buffer);
	FAT_update.address = 0;
	FAT_updateindex = 0;
	free(base_update);
}

void FATUpdate(uint32_t index, INDEX_e index_e){
	if(index){
		FAT_e temp = FAT[index];
		// Delete at index, if segment is 0. Otherwise Update at index
		if(index_e.segment){
			if(index > USED_FAT){
				FAT = realloc(FAT, sizeof(FAT_e) * index);
				FAT[index - 1] = index_e;
			}else{FAT[index] = index_e;}
		}else{
			memcpy(FAT + index, FAT + index + 1, USED_FAT - index);
			// Remove entry from base buffer
			memcpy(base_buffer + temp.offset + 1, base_buffer + temp.offset + 2, sizeof(void *) * (basebyte_counter - temp.offset - 1));
			USED_CLUSTERMAP--;
		}
	}
	FATCompress();
}

FAT_e BASEMalloc(){
	base_buffer = realloc(base_buffer, sizeof(void *) * USED_CLUSTERMAP++);
	base_buffer[USED_CLUSTERMAP - 1] = sizeof(MAP_ext_b) + (base_buffer[USED_CLUSTERMAP - 2]->num_extensions * drive.bytes_per_sector * drive.sectors_per_cluster) + base_buffer + USED_CLUSTERMAP - 2;
	*base_buffer = realloc(*base_buffer, basebyte_counter + sizeof(MAP_ext_b));
	BASESync();
	return (FAT_e){.segment = 0, .address=USED_CLUSTERMAP - 1};
}

void BASEfree(FAT_e value){
	if(value.segment){
		memcpy(base_buffer + value.offset + 1, base_buffer + value.offset + 2, sizeof(void *) * (basebyte_counter - value.offset - 1));
		BASESync();
		USED_CLUSTERMAP--;
	}
}

void BASESync(){
	fseek(disk, CLUSTERMAP_START, SEEK_SET);
	if(base_buffer){for(uint32_t cc = 0; cc < USED_CLUSTERMAP; ++cc){
		fwrite(base_buffer[cc], sizeof(MAP_ext_b) + (base_buffer[cc]->num_extensions * drive.bytes_per_sector * drive.sectors_per_cluster), 1, disk);
	}}
}

uint8_t strcheck(char *str, const char c){
	const size_t temp = strlen(str);
	for(size_t cc =0; cc < temp; ++cc){if(str[cc] == c){return true;}}
	return false;
}

MAP_ext_b *get_Item(FAT_e entry){
	if(entry.segment){return base_buffer[entry.offset];}
	return NULL;
}

MAP_ext_b *DIRECTORY_Open(size_t index, FAT_e Directory){
	MAP_ext_b *directory_ = get_Item(Directory);
	if(index < ((directory_->num_extensions * drive.bytes_per_sector * drive.sectors_per_cluster)/sizeof(FAT_e))){
		if(IS_DIR(directory_->metadata.attributes)){
			FAT_e item = directory_->cluster_entries[index + MAPBASE_ENTRYSTART(directory_)];
			return get_Item(item);
		}
	}
	return NULL;
}

bool DIRECTORY_Swap(FAT_e new_dir, FAT_e last_dir, size_t FILE_index){
	MAP_ext_b *directory_new = get_Item(new_dir), *directory_last = get_Item(last_dir);
	size_t item = 0;
	const size_t max_ = ((directory_new->num_extensions * drive.bytes_per_sector * drive.sectors_per_cluster)/sizeof(FAT_e));
	for(; item < max_; ++item){if(directory_new->cluster_entries[item + MAPBASE_ENTRYSTART(directory_new)].segment == 0){break;}}
	if(item == max_){MAPEXTB_PLUSalloc(new_dir);}

	FAT_e file_FAT = directory_last->cluster_entries[FILE_index + MAPBASE_ENTRYSTART(directory_last)];
	directory_last->cluster_entries[FILE_index + MAPBASE_ENTRYSTART(directory_last)].segment = 0;

	directory_new->cluster_entries[item + MAPBASE_ENTRYSTART(directory_new)] = file_FAT;
	return true;
}

void MAPEXTB_PLUSalloc(FAT_e index){
	const uint32_t temp = sizeof(MAP_ext_b) + (get_Item(index)->num_extensions * drive.bytes_per_sector * drive.sectors_per_cluster);
	MAP_ext_b *item = malloc(temp);
	memcpy(item, *base_buffer + temp, temp);
	memcpy(*base_buffer + basebyte_counter - temp, item, temp);
	*base_buffer = realloc(*base_buffer, basebyte_counter + (drive.bytes_per_sector * drive.sectors_per_cluster));
	free(item);
	item = *base_buffer + basebyte_counter - temp;
	item->num_extensions++;
	BASESync();
}

//! Written by AI

void fnv1a_120(const void *data, size_t len, uint8_t out[15]){
    const uint8_t *p = data;

    uint32_t h0 = 0x811C9DC5u;
    uint32_t h1 = 0xA1B2C3D4u;
    uint32_t h2 = 0x9E3779B9u;
    uint32_t h3 = 0xC2B2AE35u;
    uint32_t h4 = 0x165667B1u;

    for(size_t i = 0; i < len; i++){
        uint8_t c = p[i];

        h0 ^= c; h0 *= 16777619u;
        h1 ^= c; h1 = (h1 << 5) | (h1 >> 27); h1 *= 2166136261u;
        h2 ^= c; h2 *= 1099511627u;
        h3 ^= c; h3 = (h3 << 7) | (h3 >> 25); h3 *= 16777619u;
        h4 ^= c; h4 *= 2166136261u;
    }

    // Final avalanche
    h0 ^= h1 ^ h2 ^ h3 ^ h4;
    h1 ^= h0; h2 ^= h3; h3 ^= h4; h4 ^= h2;

    uint32_t tmp[5] = {h0, h1, h2, h3, h4};
    memcpy(out, tmp, 15);   // 120 bits
}
