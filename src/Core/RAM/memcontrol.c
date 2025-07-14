#include "./Source/Core-OS/RAM/memtypes.h"

void memcpy_unsafe(uint8_t *dest, const size_t offset, const uint8_t *src, const size_t n){
    size_t i =0;
    while(i < n){
        dest[offset +i] = src[i];
        i++;
    }
}

void memmove_unsafe(uint8_t *dest, size_t size, size_t offset, uint8_t *src, size_t size_){
    size_t MaxSize = (size > size_? size_: size) + offset, srcCounter =0;
    while(offset < MaxSize){
        dest[offset] = src[srcCounter];
        ++srcCounter;
        ++offset;
    }
}

uint8_t *slice_bytes(uint8_t *src, size_t start, size_t Length){
    uint8_t *result;
    size_t cc =0;
    while(cc < Length){
        result[cc] = src[start + cc];
        ++cc;
    }
    return result;
}

//Not robust rn but like I gues it is what it is ngl.
uint8_t *define_hid(){return hash(encode_size_t(RAMmeta), sizeof(size_t));}

uint8_t *define_pid(){
    size_t temp = RAMmeta;
    uint32_t result =0;
    while(temp < _ram_end){
        if(headerpeek_unsafe(temp, HContextPeekerAttr_IsProcess) == true){
            result += decode_uint32(slice_bytes(RAM, temp, IDSize));
        }
        temp += context_size;
    }
    uint8_t array[sizeof(uint32_t)];
    encode_uint32(array, result, 0);
    return array;
}
/*
bool space_validate(size_t address, size_t concurrent_size){
    size_t cc =0;
    while(cc < NumberOfBlocks){
        size_t SizeOfBlock = decode_size_t(HeaderMetadata->Data, cc + sizeof(hcontext_t) + sizeof(size_t)), Address = decode_size_t(HeaderMetadata->Data, cc + sizeof(hcontext_t));
        if(clamp_size_t(Address, Address + Size, address + concurrent_size) != address + size){return false;}
        cc+=(MetadataBlockCount);
    }
    return false;
}
bool address_validate(size_t Offset){
    int cc =0;
    while(cc < NumberOfBlocks){
        if(decode_size_t(HeaderMetadata->Data, cc) == Offset){return true;}
    }
    return false;
}
*/