#pragma once

#include "tools.h"

typedef size_t LBA;

typedef struct rawenv{
    FILE *f;
    size_t pos;
}rawenv;

uint32_t getblocksize();
void configureblocksize(uint32_t blocksize);
rawenv *startup(char *path);
void *readblock(rawenv *re, size_t pos, size_t blocks);
bool writeblock(rawenv *re, void *buffer, size_t pos, size_t blocks);
void dispose(rawenv *re);