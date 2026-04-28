#include "f-rat.h"
#include "kernel/lib32/stdkernel/Interrupt/interrupt.h"

extern uint32_t FRATINTERRUPTVECTOR;
extern interruptEntry FRATINTERRUPT;
void InitFrATInterrupt(IDTentry *IDT, uint16_t codeSegment){
    #define FRATINTERRUPTVECTOR (0x20 + 0)
    InitInterrupt(
        IDT, FRATINTERRUPTVECTOR, true, FRATINTERRUPT, codeSegment,
        IDTFLAGS32B_INTRGATE | IDTFLAGS_RING3 | IDTFLAGS_PRESENT
    );
}

/*
    eax: Function Sub-Code
    ebx: Byte Size of Params
*/
void ASMCALL FRATHANDLER(InterruptFrame *Iframe){
    switch(Iframe->eax){
        case 0: {   // Create a New file
            if(Iframe->ebx == 8){
                fcreate((char *)(Iframe->stack), (char *)(Iframe->stack + sizeof(void *)));
            }
        }
    }
}

size_t FrATSIZE(uint8_t num_parallels, uint8_t max_depth, uint16_t chunk_bytes, uint32_t unused_bytes){
    size_t out = 0;
    for(uint8_t cc = 0; cc < max_depth; ++cc){out += powll(num_parallels, cc);}
    out *= chunk_bytes;
    out -= unused_bytes;
    return out;
}


uint8_t FrATDEPTH(size_t size, uint8_t num_parallels, uint16_t chunk_bytes, uint32_t unused_bytes, bool *approx){
    uint8_t closest_depth;
    size_t closest_size;
    for(uint8_t cc = 1; cc <= DEPTHMAX; ++cc){
        size_t temp = FrATSIZE(num_parallels, cc, chunk_bytes, unused_bytes);
        // Find depth where computed size is closest to requested size
        size_t diff_current = (size > temp) ? (size - temp) : (temp - size);
        size_t diff_closest = (size > closest_size) ? (size - closest_size) : (closest_size - size);
        if(diff_current < diff_closest){
            closest_depth = cc;
            closest_size = temp;
        }
        // If exact match found, stop early
        if(temp == size){break;}
    }
    if(approx){*approx = (closest_size != size);}
    return closest_depth;
}

uint8_t FrATPARALLEL(size_t size, uint8_t depth, uint16_t chunk_bytes, uint32_t unused_bytes, bool *approx){
    uint8_t closest_parallel;
    size_t closest_size;
    for(uint8_t cc = 1; cc <= PARALLELMAX; ++cc){
        size_t temp = FrATSIZE(cc, depth, chunk_bytes, unused_bytes);
        // Find depth where computed size is closest to requested size
        size_t diff_current = (size > temp) ? (size - temp) : (temp - size);
        size_t diff_closest = (size > closest_size) ? (size - closest_size) : (closest_size - size);
        if(diff_current < diff_closest){
            closest_parallel = cc;
            closest_size = temp;
        }
        // If exact match found, stop early
        if(temp == size){break;}
    }
    if(approx){*approx = (closest_size != size);}
    return closest_parallel;
}

uint32_t *getFree(uint32_t start, uint8_t number, uint16_t maxsearch, IDCODE inCode){
    static uint32_t out[32];
    if(number <= maxsearch){
        uint32_t cc =0, index = 0;
        undefinedSector temp;
        for(; cc < maxsearch && index < 32; ++cc){
            _x86DISKREAD(start + cc, &temp);
            if(FLAGCHECK(temp.header.flags, FrATFLAGS_unDEFINED)){
                out[index] = start + cc;
                memset(&temp, sizeof(temp), 0);
                memcpy(temp.header.undefinedCode, inCode, sizeof(IDCODE));
                temp.header.flags |= FrATFLAGS_DEFINED;
                _x86DISKWRITE(start + cc, &temp);
                index++;
            }
        }
    }
    return out;
}


uint8_t fcreate(char *name, char *mode){
    FrATBASE file = {0};
    uint8_t flagout = 0;
    uint8_t parallels = 0, depth = 0;
    FLAGSET(file.header.flags, FrATFLAGS_BASE);
    if(strcheck(mode, 'R')){
        int8_t num = tobit(mode[strcmpc(mode, "R")]);
        if(num >= 0 && num <= 3){FLAGSET(file.header.flags, FrATBASEFLAGS_RING0 + num);}
    }
    if(strchecks(mode, "size=")){
        size_t index = strcmpc(mode, "size=");
        file.header.trueSize = tolong(mode + index + 1);
        file.header.Bounds.parallels = PARALLELMAX;
        bool approx = 0;
        file.header.Bounds.depth = FrATDEPTH(file.header.trueSize, PARALLELMAX, 1, 0, &approx);
        if(approx){
            file.header.trueSize = FrATSIZE(file.header.Bounds.parallels, file.header.Bounds.depth, 1, 0);
            flagout |= FrATFUNCFLAGS_APPROX;
        }
    }else{file.header.trueSize = 0;}
    if(strcheck(mode, 'r')){FLAGSET(file.header.flags, FrATBASEFLAGS_READ);}
    if(strcheck(mode, 'w')){FLAGSET(file.header.flags, FrATBASEFLAGS_WRITE);}
    if(strcheck(mode, 'a')){FLAGSET(file.header.flags, FrATBASEFLAGS_ARCHIVE);}
    if(strcheck(mode, 'd')){FLAGSET(file.header.flags, FrATBASEFLAGS_DIRECTORY);}
    if(strcheck(mode, 'e')){FLAGSET(file.header.flags, FrATBASEFLAGS_EXECUTABLE);}
    sha256_context namehash;
    sha256_init(&namehash);
    sha256_hash(&namehash, name, strlen(name));
    sha256_done(&namehash, file.header.nameCode);
    for(uint8_t cc =0; cc < 12; ++cc){
        file.header.ID[cc] = file.header.ID[cc == 0? 0: (cc - 1)] ^ (file.header.nameCode[cc] & file.header.nameCode[cc + 1]);
    }

    driveHeader tempDrive;
    getDrive(&tempDrive);
    // Get Buffer of sectors
    uint32_t *temp = getFree(DDATALBA(tempDrive), file.header.Bounds.parallels, defaultMax, file.header.ID);
    file.header.allocatedDepth = 1;
    for(uint32_t cc =0; cc < file.header.Bounds.parallels; ++cc){
        file.Addresses[cc] = (packedLBA)createLBA(LBAFLAGS_POSITIVE | LBAFLAGS_DEFINED, temp[cc]);
    }
    temp = getFree(BASEMAPLBA(tempDrive), 1, defaultMax, file.header.ID);
    _x86DISKWRITE(*temp, &file);

    return flagout;
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

// Implement resizing later ngl