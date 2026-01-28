// gcc -o f-rat.exe -g ./f-rat.c
#include "f-rat.h"
#include "stdfile.h"

static stdfileENVIROMENT env;

FILE *getFILE(FILEhandle *ptr){
	for(uint8_t cc =0; cc < MAX_FHANDLES; ++cc){
		if(&env.files[cc].handle == ptr){
			return env.files + cc;
		}
	}
	return NULL;
}

void envPREPARE(){
	env = (stdfileENVIROMENT){
		.FATchunks = 0,
		.loadedFATs = 16,
		.loadedFiles = {0}
	};
    sectorSeekSet(0);
    sectorRead(&env.drive, sizeof(drive_header));
    sectorSeek(FAT_LBA(env.drive));
    sectorRead(env.FAT, sizeof(env.FAT));
    env.loadedFATs = 0;
    return;
}

FILEhandle *createFILE(char *name, char *mode, size_t size){
	CHECK_ENV(env);
	for(uint8_t cc =0; cc < MAX_FHANDLES; ++cc){
		if(!env.loadedFiles[cc]){
			FILE *f = env.files + cc;
			strncpy(f->name, name, strlen(name));
			uint8_t depth = f->file.depth;
			uint16_t extaddresses = f->file.Num_extensionaddresses;
			FrATbs_ResolveAttributes(size, &depth, &extaddresses);
			*f = (FILE){
				.file.FATindex = (env.FATchunks * MAX_FATHANDLES) + 1,
				.file.dataaddresses_start = (Fsector_entries / (strlen(name) + 2)) + 1,
				.handle = cc,
				.loadedSector = {0},
				.progress = 0,
				.file.depth = depth,
				.file.Num_extensionaddresses = extaddresses,
				.file.last_sector_used_bytes = (128 - 8) - (size % (FBsector_entries * Fsector_entries * f->file.depth))
			};
			if(strcheck(mode, 'd')){FrATBASEsector_IsDirectorySet(f->file, 1);} // Is Directory
			if(strcheck(mode, 'a')){
				FrATBASEsector_IsDirectorySet(f->file, 1);
				FrATBASEsector_IsArchiveSet(f->file, 1);
			} // Is Archive
			if(strcheck(mode, 'e')){FrATBASEsector_IsExecutableSet(f->file, 1);} // Is Executable
			if(strcheck(mode, 'r')){FrATBASEsector_ReadEnableSet(f->file, 1);} 	// Read Enable
			if(strcheck(mode, 'w')){FrATBASEsector_WriteEnableSet(f->file, 1);} 	// Write Enable
			if(strcheck(mode, '0')){FrATBASEsector_RingSet(f->file, 0);}else		// Ring #0
			if(strcheck(mode, '1')){FrATBASEsector_RingSet(f->file, 1);}else		// Ring #1
			if(strcheck(mode, '2')){FrATBASEsector_RingSet(f->file, 2);}else		// Ring #2
			if(strcheck(mode, '3')){FrATBASEsector_RingSet(f->file, 3);}			// Ring #3
			if(x86DISKUPDATE(LBAget((MallocSectorSMAP(default_max)).LBA), false)){
				x86DISKUPDATE(LBAget((MallocSectorSMAP(default_max)).LBA), true);
				x86DISKWRITE((void *)&f->file);
			}else{
				setColor(ANSI_RED);
				printf("Failed to Create File");
			}
			return &f->handle;

		}
	}
	return NULL;
}

FAT_e MallocSectorDDATA(uint32_t max){
	FrATsector temp;
	FAT_e out;
	sectorSeekSet(DDATA_LBA(env.drive));
	for(size_t cc =0; cc < max; ++cc){
		x86DISKREAD((uint8_t *)&temp);
		if(!temp.FATindex){
			for(uint32_t cc_ =0; cc_ < max; ++cc_){
				for(uint8_t cc__ = 0; cc__ < MAX_FATHANDLES; ++cc__){
					if(FATused(env.FAT[cc].LBA)){
						env.FAT[cc].LBA = FATusedt(sectorTell());
						out = env.FAT[cc];
						break;
					}
					sectorWrite(env.FAT, sizeof(env.FAT));
					sectorRead(env.FAT, sizeof(env.FAT));
				}
			}
		}
	}
	return out;
}

FAT_e MallocSectorSMAP(uint32_t max){
	FrATsector temp;
	FAT_e out;
	sectorSeekSet(DDATA_LBA(env.drive));
	for(size_t cc =0; cc < max; ++cc){
		x86DISKREAD((uint8_t *)&temp);
		if(!temp.FATindex){
			for(uint32_t cc_ =0; cc_ < max; ++cc_){
				for(uint8_t cc__ = 0; cc__ < MAX_FATHANDLES; ++cc__){
					if(FATused(env.FAT[cc].LBA)){
						env.FAT[cc].LBA = FATusedt(sectorTell());
						out = env.FAT[cc];
						break;
					}
					sectorWrite(env.FAT, sizeof(env.FAT));
					sectorRead(env.FAT, sizeof(env.FAT));
				}
			}
		}
	}
	return out;
}

const char DIGITS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
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

uint8_t strcheck(char *str, const char c){
	const size_t temp = strlen(str);
	for(size_t cc =0; cc < temp; ++cc){if(str[cc] == c){return true;}}
	return false;
}

//! Written by AI

static inline void FrATbs_ResolveAttributes(uint32_t size, uint8_t *depth, uint16_t *extension_addresses){
    if (*depth && !*extension_addresses) {
		*extension_addresses = size / (FBsector_entries * Fsector_entries * (*depth));
		if(!(*extension_addresses)){(*extension_addresses)++;}
	}else{
		*depth = size / (FBsector_entries * Fsector_entries * (*extension_addresses));
		if(!(*depth)){(*depth)++;}
	}
}


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
