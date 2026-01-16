#include "f-rat.h"

drive_header drive;
FILE *disk;

FAT_e *FAT;
uint32_t loadedFATs;

// Index for each depth step, for 1 open file only
// Limit to only 10 OPEN files.
FrATBASEsector *openFiles[10];
char *openFileNames[10];
// 1 for each Base sector.
FrATsector openSectors[10];
// Progress into open sector
uint8_t progress[10];

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

bool IMG_Setup(void){
	for(uint8_T cc =0; cc < 10; ++cc){openFiles[cc] = malloc(sizeofFrATBASEsector));}
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
	if(fread(FAT, 1, (drive.sectors_per_fat * drive.bytes_per_sector), disk) == (drive.sectors_per_fat * drive.bytes_per_sector)){
		printf("Failed to read %d bytes.", drive.sectors_per_fat * drive.bytes_per_sector);
		exit(EXIT_FAILURE);
	}
	loadedFATs = 0;
	while(loadedFATs < (drive.sectors_per_fat * drive.bytes_per_sector) && FATused(FAT[loadedFATs].LBA)){
		loadedFATs++;
	}
	return EXIT_SUCCESS;
}

bool closeFile(char *name){
	for(uitn8_t cc =0; cc < 10; ++cc){
		if(openFileNames[cc]){
			if(!strncmp(openFileNames[cc], name, strlen(name) - 1){
				if(selectedFile == cc){selectedFile = -1;}
				free(openFiles[cc]);
				free(openFileNames[cc]);
				memcpy(openFileNames + cc, openFileNames + cc + 1, 10 - cc);
				return true;
			}		
		}
	}
	return false;
}

bool ptrStepIn(uint8_t ptr, uint8_t file){
	if(openFiles[cc] && ptr < (drive.bytes_per_sector - sizeof(FrATBASEsector) / sizeof(FAT_e)){
		fseek(disk, DDATA_LBA + LBAget(openSectors[cc].entries[ptr] + (progress / 128)), SEEK_SET);
		fread(openSectors + ptr, sizeof(FrATsector), 1, disk);
	}else{
		printf("File: %d is not open", file);
		return false;
	}
	return true;
}

bool ptrStepOver(int8_t ptr, uint8_t file){
	if(openFiles[cc]){
		if(ptr < (drive.bytes_per_sector - sizeof(FrATBASEsector) / sizeof(FAT_e)){
			progress += ptr;
			if(progress > 128 - sizeof(FrATBASEsector)){progress = 128 - sizeof(FrATBASEsector);}
		}else{
			printf("ptr(%d) is too large, must be below(%d)", (drive.bytes_per_sector - sizeof(FrATBASEsector) / sizeof(FAT_e));
			return false;
		}
	}else{
		printf("File: %d is not open", file);
		return false;
	}
	return true;
}

int8_t openFile(char *name){
	FrATBASEsector *ptr;
	int8_t out = 0;
	for(; out < 10; ++cc){
		if(openFiles[out] == NULL){
			openFiles[out] = malloc(sizeof(FrATBASEsector));
			ptr = openFiles[out];
		}
	}
	if(!ptr){return -1;}
	for(uint32_t cc = 0; cc < loadedFATs; ++cc){
		fseek(disk, CLUSTERMAP_LBA + LBAget(FAT[loadedFATs].LBA), SEEK_SET);
		if(fread(*ptr, sizeof(FrATBASEsector), 1, disk) == 1){
			FrATsector *sector = malloc(sizeof(FrATsector));
			for(uint8_t cc_ = ptr->Num_extensionaddresses + 1; cc_ < (drive.bytes_per_sector - sizeof(FrATBASEsector) / sizeof(FAT_e); ++cc_){
				if(FATused(ptr->entries[cc_])){continue;}
				fseek(disk, DDATA_LBA + LBAget(ptr->entries[cc_]), SEEK_SET);
				fread(sector, sizeof(FrATsector), 1, disk);
				// Placeholder:
				// [0]: Date([16]: Year, [16]: Date, [16]: Time)
				// [1]: Name
				if(SECTORMETA_NAME(sector->flags)){
					uint8_t *data = ((uint8_t *)sector->entries) + 48;
					if(!strncmp(data, name, strlen(name) - 1)){
						progress[out]= 0;
						openFileNames[out] = strdup(data);
						free(sector);
						return out;
					}
				}
			}
			free(sector);
		}
	}
	openFiles[out] = NULL;
	free(ptr);
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
    }while(value > 0);
    // reverse into output buffer
    for(int i = 0; i < pos; i++){out[i] = buf[pos - 1 - i];}
    out[pos] = '\0';
}

uint16_t pack_time(struct tm *t){
    uint16_t sec_half = (t->tm_sec / 2) & 0x1F;   // normal FAT seconds/2
    uint16_t sec_flag = (t->tm_sec >= 30) ? 1 : 0; // lowest bit flag

    uint16_t mins = (t->tm_min & 0x3F) << 5;      // 6 bits
    uint16_t hrs  = (t->tm_hour & 0x1F) << 11;    // 5 bits

    return (sec_half << 1) | sec_flag | mins | hrs;
}

uint8_t strcheck(char *str, const char c){
	const size_t temp = strlen(str);
	for(size_t cc =0; cc < temp; ++cc){if(str[cc] == c){return true;}}
	return false;
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
