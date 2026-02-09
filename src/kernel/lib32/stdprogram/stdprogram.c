#include "stdprogram.h"

systemProgramEnviroment LINKERSECTION("PROGRAMBUFFER") systemState;

standardHeader *ASMCALL getCODEBase(void *CODE){
    for(uint32_t cc =0; cc < systemState.loaded; ++cc){
        if(memwithin(systemState.Programs[cc]->PAGES.CODE, systemState.Programs[cc]->PAGES.loadedCODEPages * (kB * 4), CODE)){
            return systemState.Programs[cc];
        }
    }
}

standardHeader *ASMCALL getDATABase(void *DATA){
    for(uint32_t cc =0; cc < systemState.loaded; ++cc){
        if(memwithin(systemState.Programs[cc]->PAGES.DATA, systemState.Programs[cc]->PAGES.loadedDATAPages * (kB * 4), DATA)){
            return systemState.Programs[cc];
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