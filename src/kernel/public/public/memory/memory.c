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