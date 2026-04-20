#include "stdfile.h"

void envInit(stdfileENVIROMENT *env){
    getDrive(&env->drive);
    env->FATCHUNKS = 0;
    _x86DISKREAD(FATLBA(env->drive), env->FAT);
    memset(env->files, sizeof(env->files), 0);
}

stdfileENVIROMENT *getEnv(){
    void *EIP = getEIP();
    standardHeader *header = getCODEBase(EIP);
    if(header){
        return &header->standardChildren.stdfile;
    }
    return NULL;
}

void FrATstepDown(undefinedSector *in, uint8_t parallel, undefinedSector *out){
    if(FLAGCHECK(in->header.flags, FrATFLAGS_BASE)){
        if(parallel < baseLBAentries){
            FrATBASE *base = (FrATBASE *)in;
            _x86DISKREAD(getLBA(base->Addresses[parallel]), out);
        }
    }else if(FLAGCHECK(in->header.flags, FrATFLAGS_CHILD)){
        if(parallel < childLBAentries){
            FrATCHILD *child = (FrATCHILD *)in;
            _x86DISKREAD(getLBA(child->Addresses[parallel]), out);
        }
    }else{return;}
}

void STACKLESSCALL FrATstep(undefinedSector *in, unsigned long *offset, packedLBA *out){
	// Step down recursively, calling FrATstep and passing the step down as the base, when depth or offset reached? return
	signed long *tempoffset = offset;
	uint8_t parallel = 0;
	if(FLAGCHECK(in->header.flags, FrATFLAGS_BASE)){
		FrATBASE *base = (FrATBASE *)in;
		for(; parallel < base->header.Bounds.parallels; ++parallel){
			undefinedSector temp;
			_x86DISKREAD(getLBA(base->Addresses[parallel]), &temp);
			for(; tempoffset > 0; tempoffset -= sectorBytes){
				*out = base->Addresses[parallel];
				if(FLAGCHECK(base->Addresses[parallel].flags, LBAFLAGS_DEFINED)|| FLAGCHECK(base->Addresses[parallel].flags, LBAFLAGS_USED)){
					FrATstep((undefinedSector *)(&temp), &tempoffset, out);
				}else{
					*out = base->Addresses[parallel];
					return;
				}
				if(!(*offset) || (*tempoffset) < 0){
					return;
				}else{continue;}
			}
		}
    }else if(FLAGCHECK(in->header.flags, FrATFLAGS_CHILD)){
		FrATCHILD *child = (FrATBASE *)in;
		for(; parallel < child->header.location.parallels; ++parallel){
			undefinedSector temp;
			_x86DISKREAD(getLBA(child->Addresses[parallel]), &temp);
			for(; tempoffset > 0; tempoffset -= sectorBytes){
				*out = child->Addresses[parallel];
				if(FLAGCHECK(child->Addresses[parallel].flags, LBAFLAGS_DEFINED)|| FLAGCHECK(child->Addresses[parallel].flags, LBAFLAGS_USED)){
					FrATstep((undefinedSector *)(&temp), &tempoffset, out);
				}else{
					*out = child->Addresses[parallel];
					return;
				}
				if(!(*offset) || (*tempoffset) < 0){
					return;
				}else{continue;}
			}
		}
    }else{return;}
}

/// @brief Read off sectors.
/// @param FILE The Handle of the associated File.
/// @param bytes The bytes to read.
/// @remark Can ONLY read up to 10 sectors.
/// @return A buffer of 10 sectors, with unused bytes set to 0.
void *readSectors(FILEhandle *file, uint32_t bytes, bool useFrATstep){
    static SECTOR reads[maxSingleRead];
    FILE *f = getFile(file);
    uint8_t reads = (bytes / sectorBytes) + (bytes % sectorBytes? 1: 0);
    int16_t minus = (bytes % sectorBytes);
    if(reads > maxSingleRead){return NULL;}
    if(f && reads != 0){
        uint8_t cc = 0;
        for(; cc < maxSingleRead; ++cc){
			if(useFrATstep){
				packedLBA address;
				unsigned long temp = sectorBytes;
				FrATstep(&f->file, &temp, &address);
				_x86DISKREAD(getLBA(address), reads + cc);
			}else{
				_x86DISKREAD(getLBA(f->Progress), reads + cc);
				storeLBA(f->Progress, getLBA(f->Progress) + sectorBytes);
			}
        }
        memset(reads[cc] + minus, sectorBytes - minus, 0);
        return reads;
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
				FrATstep(&f->file, &temp, &address);
				_x86DISKWRITE(getLBA(address), buffer + (cc * sectorBytes));
			}else{
				_x86DISKWRITE(getLBA(f->Progress), buffer + (cc * sectorBytes));
            	storeLBA(f->Progress, getLBA(f->Progress) + sectorBytes);
			}
        }
        _x86DISKWRITE(getLBA(f->Progress), writes + cc);
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
			_x86DISKREAD(LBA, out + cc);
			cc++;
		}
		return out;
	}
	return NULL;
}

void ASMCALL getDrive(driveHeader *out){
    SECTOR s;
    _x86DISKREAD(0, &s);
    memcpy(out, s, sizeof(driveHeader));
}

FILE *getFile(FILEhandle *handle){
    stdfileENVIROMENT *env;
    if((env = getEnv())){
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
		if((env = getEnv())){
			env->usedFiles[file] = false;
		}
	}
}

FILE *getUsable(){
	stdfileENVIROMENT *env;
	if((env = getEnv())){
		for(uint8_t cc =0; cc < MAX_FHANDLES; ++cc){
			if(env->usedFiles[cc] == false){return (env->files + cc);}
		}
	}
	return NULL;
}