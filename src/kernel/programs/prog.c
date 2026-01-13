#include "src/kernel/programs/prog.h"

int prog_trigger(char *path, ...){
    char *argp = (char *)&path;
    ((int *)argp) += sizeof(path) / sizeof(int);
    ((int *)argp)++;

    
}

// void *prog_load(FILE *){

// }