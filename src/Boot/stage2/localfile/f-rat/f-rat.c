#include "f-rat.h"

void STACKLESSCALL FrATstep(undefinedSector *in, unsigned long *offset, packedLBA *out){
	// Step down recursively, calling FrATstep and passing the step down as the base, when depth or offset reached? return
	signed long *tempoffset = offset;
    static unsigned long utempoffset;
	uint8_t parallel = 0;
    FrATBASE *base = (FrATBASE *)in;    FrATCHILD *child = (FrATCHILD *)in;
    static undefinedSector temp;
	if(FLAGCHECK(in->header.flags, FrATFLAGS_BASE)){
		for(; parallel < base->header.Bounds.parallels; ++parallel){
			x86DISKREAD(getLBA(base->Addresses[parallel]), (void *)&temp);
			for(; tempoffset > 0; tempoffset -= sectorBytes){
				*out = base->Addresses[parallel];
				if(FLAGCHECK(base->Addresses[parallel].flags, LBAFLAGS_DEFINED)|| FLAGCHECK(base->Addresses[parallel].flags, LBAFLAGS_USED)){
                    utempoffset = absl(*tempoffset);
					FrATstep((undefinedSector *)(&temp), (unsigned long *)&utempoffset, out);
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
		for(; parallel < child->header.location.parallels; ++parallel){
			undefinedSector temp;
			x86DISKREAD(getLBA(child->Addresses[parallel]), (void *)&temp);
			for(; tempoffset > 0; tempoffset -= sectorBytes){
				*out = child->Addresses[parallel];
				if(FLAGCHECK(child->Addresses[parallel].flags, LBAFLAGS_DEFINED)|| FLAGCHECK(child->Addresses[parallel].flags, LBAFLAGS_USED)){
                    utempoffset = absl(*tempoffset);
					FrATstep((undefinedSector *)(&temp), (unsigned long *)&utempoffset, out);
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
    return;
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
            x86DISKREAD(start + cc, (void *)&temp);
            if(FLAGCHECK(temp.header.flags, FrATFLAGS_unDEFINED)){
                out[index] = start + cc;
                memset(&temp, sizeof(temp), 0);
                memcpy(temp.header.undefinedCode, inCode, sizeof(IDCODE));
                temp.header.flags |= FrATFLAGS_DEFINED;
                x86DISKWRITE(start + cc, (void *)&temp);
                index++;
            }
        }
    }
    return out;
}

// Implement resizing later ngl