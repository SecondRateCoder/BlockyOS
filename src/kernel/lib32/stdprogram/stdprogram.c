#include "stdprogram.h"

uinl32_t loadedPrograms;
standardHeader **Programs;

standardHeader getCODEBase(void *CODE){
    for(uinl32_t cc =0; cc < loadedPrograms; ++cc){
        if(memwithin(Programs[cc]->CODE, Programs[cc]->loadedCODEPages * (kB * 4), CODE)){
            return *Programs[cc];
        }
    }
}

standardHeader getDATABase(void *DATA){
    for(uinl32_t cc =0; cc < loadedPrograms; ++cc){
        if(memwithin(Programs[cc]->DATA, Programs[cc]->loadedDATAPages * (kB * 4), DATA)){
            return *Programs[cc];
        }
    }
}