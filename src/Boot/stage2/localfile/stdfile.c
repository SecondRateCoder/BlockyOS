#include "stdfile.h"
#include "./kernel/lib32/stdprogram/stdprogram.h"

void envInit(stdfileENVIROMENT *env){
    getDrive(&env->drive);
    env->FATCHUNKS = 0;
    x86DISKREAD(FATLBA(env->drive), (void *)env->FAT);
    memset(env->files, sizeof(env->files), 0);
}

stdfileENVIROMENT *getstdfileEnv(){
    void *EIP = getEIP();
    standardHeader *header = getCODEBase(EIP);
    if(header){return (stdfileENVIROMENT *)&(header->standardChildren.stdfile);}
    return NULL;
}

/// @brief Read off sectors.
/// @param FILE The Handle of the associated File.
/// @param bytes The bytes to read.
/// @remark Can ONLY read up to 10 sectors.
/// @return A buffer of 10 sectors, with unused bytes set to 0.
void *readSectors(FILEhandle *file, uint32_t bytes, bool useFrATstep){
    static SECTOR reads[maxSingleRead];
    FILE *f = getFile(file);
    uint8_t readNum = (bytes / sectorBytes) + (bytes % sectorBytes? 1: 0);
    int16_t minus = (bytes % sectorBytes);
    if(readNum > maxSingleRead){return NULL;}
    if(f && readNum != 0){
        uint8_t cc = 0;
        for(; cc < maxSingleRead; ++cc){
			if(useFrATstep){
				packedLBA address;
				unsigned long temp = sectorBytes;
				FrATstep((undefinedSector *)(&(f->file)), &temp, (packedLBA *)&(address));
				x86DISKREAD(getLBA(address), (void *)(reads + cc));
			}else{
				x86DISKREAD(getLBA(f->Progress), (void *)(reads + cc));
				storeLBA(f->Progress, getLBA(f->Progress) + sectorBytes);
			}
        }
        memset((void *)(reads + cc) + minus, sectorBytes - minus, 0);
        return (void *)reads;
    }
    return NULL;
}

void writeSectors(FILEhandle *file, void *buffer, uint32_t bytes, bool useFrATstep){
    static SECTOR extraWrite;
    bool extraWrite_ = (bytes % sectorBytes? true: false);
    FILE *f = getFile(file);
    memset(extraWrite, sizeof(SECTOR), 0);
    uint8_t writes = (bytes / sectorBytes);
    memcpy(extraWrite, buffer + (writes * sectorBytes), (bytes % sectorBytes));
    if(f && writes != 0){
        uint8_t cc = 0;
        for(; cc < writes; ++cc){
			if(useFrATstep){
				packedLBA address;
				unsigned long temp = sectorBytes;
				FrATstep((undefinedSector *)&(f->file), &temp, (packedLBA *)&(address));
				x86DISKWRITE(getLBA(address), buffer + (cc * sectorBytes));
			}else{
				x86DISKWRITE(getLBA(f->Progress), buffer + (cc * sectorBytes));
            	storeLBA(f->Progress, getLBA(f->Progress) + sectorBytes);
			}
        }
        x86DISKWRITE(getLBA(f->Progress), (void *)(writes + cc));
        storeLBA(f->Progress, getLBA(f->Progress) + sectorBytes);
        return;
    }
    return;
}

/// @brief Read off a flat buffer of sectors.
/// @param LBA The Address to read from.
/// @param sectors The number of sectors to read.
/// @return A buffer of a max size of 10 sectors.
void *flatRead(uint32_t LBA, uint8_t sectors){
	static SECTOR out[10] = {0};
	if(sectors <= 10){
		uint8_t cc =0;
		while(cc < sectors){
			x86DISKREAD(LBA, (void *)(out + cc));
			cc++;
		}
		return out;
	}
	return NULL;
}

void ASMCALL getDrive(driveHeader *out){
    SECTOR s;
    x86DISKREAD(0, (void *)&s);
    memcpy(out, s, sizeof(driveHeader));
}

FILE *getFile(FILEhandle *handle){
    stdfileENVIROMENT *env;
    if((env = getstdfileEnv())){
        for(uint8_t cc = 0; cc < MAX_FHANDLES; ++cc){
            if(&env->files[cc].handle == handle && env->files[cc].handle == *handle){
                return env->files + cc;
            }
        }
    }
}

void closeFile(uint8_t file){
	if(file < MAX_FHANDLES){
		stdfileENVIROMENT *env;
		if((env = getstdfileEnv())){
			env->usedFiles[file] = false;
		}
	}
}

FILE *getUsable(){
	stdfileENVIROMENT *env;
	if((env = getstdfileEnv())){
		for(uint8_t cc =0; cc < MAX_FHANDLES; ++cc){
			if(env->usedFiles[cc] == false){return (env->files + cc);}
		}
	}
	return NULL;
}