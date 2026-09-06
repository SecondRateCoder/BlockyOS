#include "kernel/lib32/generic/standard.h"

bool memwithin(void *buffer, uint64_t buffer_size, void *address, uint64_t address_size){
    return ((uint64_t)address) > ((uint64_t)buffer) && ((uint64_t)address) < ((uint64_t)buffer + buffer_size) &&
            ((uint64_t)address + address_size) < ((uint64_t)buffer + buffer_size);
}

void memset(void *buffer, uint64_t len, uint8_t val){
    for(uint64_t cc = 0; cc < len; ++cc){((uint8_t *)buffer)[cc] = val;}
}

bool memcmp(void *a, void *b, uint64_t len){
    for(uint64_t cc =0; cc < len; ++cc){
        if(((uint8_t *)a)[cc] != ((uint8_t *)b)[cc]){
            return false;
        }
    }
    return true;
}

void memcpy(void *dst, void *src, uint64_t len){
    for(uint64_t cc =0; cc < len; ++cc){
        ((uint8_t *)dst)[cc] = ((uint8_t *)src)[cc];
    }
    return;
}

bool memcheckb(void *a, uint64_t len, uint8_t b){
    for(uint64_t cc =0; cc < len; ++cc){
        if(((uint8_t *)a)[cc] != b){
            return false;
        }
    }
    return true;
}

bool memcheckh(void *a, uint64_t len, u16_t w){
    for(uint64_t cc =0; cc < len; ++cc){
        if(w){
            return false;
        }
    }
    return true;
}

bool memcheckl(void *a, uint64_t len, long l){
    for(uint64_t cc =0; cc < len; ++cc){
        if(((long *)a)[cc] != l){
            return false;
        }
    }
    return true;
}

bool memcheckll(void *a, uint64_t len, uint64_t z){
    for(uint32_t cc =0; cc < len; ++cc){
        if(((uint64_t *)a)[cc] != z){
            return false;
        }
    }
    return true;
}