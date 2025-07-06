#include "./Source/Public/Publics.h"

size_t decode_size_t(const uint8_t *array, const int Offset){
    size_t value = Offset;
    for (size_t i = 0; i < (sizeof(size_t)+Offset); i++) {
        value |= ((size_t)array[i]) << (i * 8);
    }
    return value;
}
int decode_int(const uint8_t *array, size_t Offset){
    int value;
    for (size_t i = Offset; i < (sizeof(int)+Offset); i++) {
        value |= ((int)array[i]) << (i * 8);
    }
    return value;
}
int *decode_int_array(const uint8_t *array, size_t Offset, const size_t Length){
    if(((float)Length)/(float)sizeof(int) != 0){return NULL;}
    int result[Length];
    for (size_t i = Offset; i < (Length+Offset); i++) {
        result[i] = decode_int(array, Offset + i * sizeof(int));
    }
    return result;
}
uint32_t decode_uint32(const uint8_t *array) {
    uint32_t value = 0;
    for (int i = 0; i < 4; i++) {
        value |= ((uint32_t)array[i]) << (i * 8);
    }
    return value;
}

size_t clamp_size_t(size_t lower, size_t upper, size_t value){return value > upper? upper : (value < lower? lower : value);}

uint8_t encode_size_t(uint8_t *array, size_t value, size_t offset){
    size_t i = 0;
    for (; i < sizeof(size_t); i++) {
        array[offset + i] = (uint8_t)((value >> (i * 8)) & 0xFF);
    }
    return i;
}
uint8_t encode_int(uint8_t *array, int value, size_t offset){
    size_t i = 0;
    for (; i < sizeof(int); i++) {
        array[offset + i] = (uint8_t)((value >> (i * 8)) & 0xFF);
    }
    return i;
}

bool compare_array(uint8_t *array1, size_t size, uint8_t *array2, size_t size2){
    if(size != size2){return false;}
    int cc =0;
    while(cc < sizeof(array1)){
        if(array1[cc] != array2[cc]){return false;}
        ++cc;
    }
    return true;
}