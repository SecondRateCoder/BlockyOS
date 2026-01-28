#include "stdfile.h"

bool sectorSeekSet(size_t addr){
    if(x86DISKUPDATE(addr, false)){
        x86DISKUPDATE(addr, true);
        return true;
    }
    return false;
}

bool sectorSeek_(long offset, bool update){
    if(-offset < sectorpointer){
        if(x86DISKUPDATE(sectorpointer + offset, true)){
            sectorpointer += offset;
            return true;
        }
    }
    return false;
}

size_t sectorTell(){return sectorpointer;}

void *sectorRead_(unsigned long bytes){
    uint8_t last_write = writeptr;
    uint8_t reads = bytes / 128 + 
        ((bytes % 128)? 1: 0);
    if(reads > 10){
        // setColor(ANSI_RED);
        // printf("Cannot read more than 1280 bytes");
        return NULL;
    }
    for(uint8_t cc =0; cc < reads; ++cc){
        x86DISKREAD((uint8_t *)(sectorhandles + writeptr));
        if(sectorSeek_(128, false)){
        }
        writeptr++;
        if(writeptr > 10){writeptr = 0;}
    }
    return (sectorhandles + last_write);
}

void sectorRead(void *buffer, size_t buffsize){
    if(buffsize < MAX_READBYTES){
        void *buffer_ = sectorRead_(buffsize);
        memcpy(buffer, buffer_, buffsize);
    }
    return;
}

void sectorWrite(void *buffer, size_t buffsize){
    uint8_t writes = buffsize / 128 + 
        ((buffsize % 128)? 1: 0);
    size_t offset = 0;
    for(uint8_t cc =0; cc < writes; ++cc){
        uint8_t packetsize = (buffsize - offset) > 128? 128: (buffsize - offset);
        offset += packetsize;
        static uint8_t buffer_[128];
        x86DISKWRITE(buffer_);
        memcpy(buffer + offset, buffer_, packetsize);
    }
}
