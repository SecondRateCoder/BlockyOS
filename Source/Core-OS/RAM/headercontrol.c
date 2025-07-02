#include "./Source/Core-OS/RAM/memtypes.h"

size_t header_encode(uint8_t *array, header_t h){
    
    return context_size + h.size;
}
size_t context_encode(uint8_t array[context_size], context_t c){
    return context_size;
}
void memcpy(uint8_t *dest, ID_t ID, const size_t offset, const uint8_t *src, const size_t n){
    size_t i = 0;
    while(i < n){
        dest[offset + i] = src[i];
        i++;
    }
}
void memcpy_headerMeta(const header_t H){
    uint8_t context_encoded[context_size];
    context_encode(context_encoded, H.context);
    RAMMeta-=context_size;
    memcpy_unsafe(RAMMeta, );
}