#include "./kernel/lib32/generic/standard.h"

bool memwithin(void *buffer, size_t buffer_size, void *address, size_t address_size){
    return ((size_t)address) > ((size_t)buffer) && ((size_t)address) < ((size_t)buffer + buffer_size) &&
            ((size_t)address + address_size) < ((size_t)buffer + buffer_size);
}

void memset(void *buffer, size_t len, uint8_t val){
    for(size_t cc = 0; cc < len; ++cc){((uint8_t *)buffer)[cc] = val;}
}

bool memcmp(void *a, void *b, size_t len){
    for(size_t cc =0; cc < len; ++cc){
        if(((uint8_t *)a)[cc] != ((uint8_t *)b)[cc]){
            return false;
        }
    }
    return true;
}

void memcpy(void *dst, void *src, size_t len){
    for(size_t cc =0; cc < len; ++cc){
        ((uint8_t *)dst)[cc] = ((uint8_t *)src)[cc];
    }
    return;
}

bool memcheckb(void *a, size_t len, u8_t byte){
    for(size_t cc =0; cc < len; ++cc){
        if(((uint8_t *)a)[cc] != byte){
            return false;
        }
    }
    return true;
}

bool memcheckh(void *a, size_t len, u16_t byte){
    for(size_t cc =0; cc < len; ++cc){
        if(((uint16_t *)a)[cc] != byte){
            return false;
        }
    }
    return true;
}

bool memcheckl(void *a, size_t len, long byte){
    for(size_t cc =0; cc < len; ++cc){
        if(((long *)a)[cc] != byte){
            return false;
        }
    }
    return true;
}

bool memcheckll(void *a, size_t len, size_t byte){
    for(uint32_t cc =0; cc < len; ++cc){
        if(((size_t *)a)[cc] != byte){
            return false;
        }
    }
    return true;
}