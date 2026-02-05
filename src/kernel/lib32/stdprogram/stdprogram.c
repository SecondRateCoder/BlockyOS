#include "stdprogram.h"

uint32_t loadedPrograms;
standardHeader **Programs;

standardHeader *ASMCALL getCODEBase(void *CODE){
    for(uint32_t cc =0; cc < loadedPrograms; ++cc){
        if(memwithin(Programs[cc]->CODE, Programs[cc]->loadedCODEPages * (kB * 4), CODE)){
            return Programs[cc];
        }
    }
}

standardHeader *ASMCALL getDATABase(void *DATA){
    for(uint32_t cc =0; cc < loadedPrograms; ++cc){
        if(memwithin(Programs[cc]->DATA, Programs[cc]->loadedDATAPages * (kB * 4), DATA)){
            return Programs[cc];
        }
    }
}

char *getEnv(char alias[aliasLen]){return NULL;}

/// @brief Load a flat Executable Binary with no headers, the Binary should only feature the executableBinary header and format.
/// @param Binary The Path to the Binary.
/// @param Parallel Should the Binary be loaded in a seperate Thread to the caller?
/// @param Extra arguments.
void loadBinary(char *Binary, bool Parallel, ...){return;}

void loadProgram(char *Image, uint8_t childThreads){
    loadBinary(getEnv("stdLd"), true, Image, childThreads);
}