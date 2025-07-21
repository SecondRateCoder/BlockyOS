#include "./src/Core/RAM/memtypes.h"

#define MAX_SEARCH 1024*30

size_t memsize(void *ptr){
    size_t addr = (size_t)ptr;
    while(addr < MAX_SEARCH){
        if(RAM[addr] == NULL){return NULL;}
        ++addr;
    }
}

void memcpy_unsafe(uint8_t *dest, const size_t offset, const uint8_t *src, const size_t n){
    size_t i =0;
    while(i < n){
        dest[offset +i] = src[i];
        i++;
    }
}
void memdisplace(uint8_t *src, size_t blockaddr, const size_t blocksize, const ssize_t displacement){
    if(blockaddr+displacement < 0 || src == NULL){return;}
    const size_t naddr = blockaddr + displacement, endaddr = blockaddr+blocksize;
    ssize_t counter =0;
    if(displacement < 0){
        //Write from addr to naddr.
        while(blockaddr+counter < endaddr){
            src[naddr+counter] = src[blockaddr+counter];
            src[blockaddr+counter] = 0;
            counter++;
        }
    }else if(displacement > 0){
        counter = blocksize-1;
        while(counter > -1){
            src[naddr+counter] = src[blockaddr+counter];
            src[blockaddr+counter] = 0;
            counter--;
        }
    }
}
void memmove_unsafe(uint8_t *dest, size_t size, size_t offset, uint8_t *src, size_t size_){
    size_t MaxSize = (size > size_? size_: size) + offset, srcCounter =0;
    while(offset < MaxSize){
        dest[offset] = src[srcCounter];
        src[srcCounter]=0;
        ++srcCounter;
        ++offset;
    }
}

void memclear(uint8_t *dest, size_t offset, size_t length){
    const size_t end = offset+length;
    while(offset < end){
        dest[offset] = 0;
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

uint8_t *define_pid(){
    size_t temp = RAMMeta;
    uint32_t result =0;
    while(temp < _ram_end){
        if(hcontext_attrpeek_unsafe(temp, HContextPeekerAttr_IsProcess) == true){
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