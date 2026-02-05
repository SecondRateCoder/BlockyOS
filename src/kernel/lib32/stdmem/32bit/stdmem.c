#include "stdmem.h"

MemInfo *memInfo;

void defineMemInfo(MemInfo *in){
    (*memInfo) = *in;
}

