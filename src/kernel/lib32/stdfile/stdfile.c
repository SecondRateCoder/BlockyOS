#include "stdfile.h"

FILE *getFILE(FILEhandle *Fh){
    get
}

stdfileENVIROMENT envPREPARE(){
    static stdfileENVIROMENT out;
    sectorSeek(-sectorpointer);
    void *temp = sectorRead_(sizeof(drive_header));
    memcpy(&(out.drive), temp, sizeof(drive_header));
    sectorSeek(FAT_LBA(out.drive));
    memset(out.FAT, sizeof(out.FAT), 0);
    memset(out.files, sizeof(out.files), 0);
    out.loadedFATs = 0;
    return out;
}

bool sectorSeek_(FILEhandle *file, long offset, bool update){
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

void sectorRead(void *buffer, size_t buffsize, size_t bytes){
    void *buffer_ = sectorRead_(bytes);
    memcpy(buffer, buffer_, buffsize);
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
