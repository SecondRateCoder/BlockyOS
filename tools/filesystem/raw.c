#include "raw.h"

uint32_t __blocksize;

uint32_t getblocksize(){return __blocksize;}
void configureblocksize(uint32_t blocksize){__blocksize = blocksize;}

rawenv *startup(char *path){
	printf("\n\nStarting File Interface: %s", path);
	rawenv *re = malloc(sizeof(rawenv));
	if(!(re->f = fopen(path, "rb+"))){free(re);		return NULL;}
	re->pos = 0;
	return re;
}

void *readblock(rawenv *re, size_t pos, size_t blocks){
	printf("\nReading: Org:%zu New:%zu Size:%zu", re->pos, pos, blocks);
	if(re->pos != pos){fseek(re->f, pos * __blocksize, SEEK_SET);		re->pos = pos;}
	void *out = malloc(__blocksize * blocks);
	if(out){if(blocks == fread(out, __blocksize, blocks, re->f)){re->pos += blocks;		return out;}}
	return NULL;
}

bool writeblock(rawenv *re, void *buffer, size_t pos, size_t blocks){
	printf("\nWriting: Org:%zu New:%zu Size:%zu", re->pos, pos, blocks);
	if(re->pos != pos){fseek(re->f, pos * __blocksize, SEEK_SET);		re->pos = pos;}
	if(blocks == fwrite(buffer, __blocksize, blocks, re->f)){re->pos += blocks;		return true;}
	return false;
}

void dispose(rawenv *re){
	printf("\nDisposing File Interface");
	fflush(re->f);
	fclose(re->f);
	free(re);
}