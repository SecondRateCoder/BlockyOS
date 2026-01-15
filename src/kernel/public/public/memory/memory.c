#include "../src/kernel/public/public/public.h"

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