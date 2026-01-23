#include "stdfile.h"
typedef uint8_t sectorbuff[128];  // Enough for 1 sector.
size_t sectorpointer;
sectorbuff sectorhandles[10];        // 10 sectors max read.
uint8_t writeptr;

void sectorseek(ssize_t offset){
    if(-offset < sectorpointer){
        sectorpointer += offset;
        x86UPDATEDISKPTR(sectorpointer);
    }
}

size_t sectortell(){return sectorpointer;}

void *sectorread_(unsigned long bytes){
    uint8_t last_write = writeptr;
    uint8_t reads = bytes / 128 + 
        ((bytes % 128)? 1: 0);
    if(reads > 10){
        // setColor(ANSI_RED);
        // printf("Cannot read more than 1280 bytes");
        return NULL;
    }
    for(uint8_t cc =0; cc < reads; ++cc){
        x86DISKREAD(sectorpointer, sectorhandles + writeptr);
        writeptr++;
        if(writeptr > 10){writeptr = 0;}
    }
    sectorpointer += bytes;
    return (sectorhandles + last_write);
}

void sectorread(void *buffer, size_t buffsize, size_t bytes){
    void *buffer_ = sectorread_(bytes);
    memcpy(buffer_, buffer, buffsize);
    return;
}

void sectorwrite(void *buffer, size_t buffsize){
    uint8_t writes = buffsize / 128 + 
        ((buffsize % 128)? 1: 0);
    size_t offset = 0;
    for(uint8_t cc =0; cc < write; ++cc){
        uint8_t packetsize = (buffsize - offset) > 128? 128: (buffsize - offset);
        offset += packetsize;
        static uint8_t buffer_[128];
        x86DISKWRITE(sectorpointer, buffer_);
        memcpy(buffer_, buffer + offset, packetsize);
    }
}
