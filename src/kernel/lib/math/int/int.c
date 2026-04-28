#include "int.h"

size_t decode64(const uint8_t *array){
    size_t value = 0;
    for(size_t i = 0; i < sizeof(size_t); i++){
        value |= ((size_t)array[i]) << (i * 8);
    }
    return value;
}
int decode32(const uint8_t *array){
    int value;
    for(size_t i = 0; i < sizeof(int); i++){
        value |= ((int)array[i]) << (i * 8);
    }
    return value;
}
// int *decode32a(const uint8_t *array, size_t Offset, const size_t Length){
//     if(((float)Length)/(float)sizeof(int) != 0){return NULL;}
//     int *result = alloca(Length, calling_id());
//     for (size_t i = Offset; i < (Length+Offset); i++) {
//         result[i] = decode_int(array, Offset + i * sizeof(int));
//     }
//     return result;
// }
uint32_t decode_32u(const uint8_t *array){
    uint32_t value = 0;
    for(int i = 0; i < 4; i++){
        value |= ((uint32_t)array[i]) << (i * 8);
    }
    return value;
}

size_t clamp_sizet(size_t lower, size_t upper, size_t value){return value > upper? upper : (value < lower? lower : value);}

void encode64(uint8_t *array, size_t value){
    for(size_t i = 0; i < sizeof(size_t); i++){
        array[i] = (uint8_t)((value >> (i * 8)) & 0xFF);
    }
    return;
}
void encode32(uint8_t *array, int value){
    for(size_t i = 0; i < sizeof(int); i++){
        array[i] = (uint8_t)((value >> (i * 8)) & 0xFF);
    }
    return;
}
void encode32u(uint8_t *array, const uint32_t value){
    for(int i = 0; i < 4; i++){
        array[i] = (uint8_t)((value >> (i*8)) && 0xFF);
    }
    return;
}

size_t bit_max(uint8_t num_bits){
    size_t out = 2;
    while(num_bits--){out *= out;}
    return out;
}