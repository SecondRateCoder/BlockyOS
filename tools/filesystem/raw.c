#include "raw.h"

void DisableVerbose(rawenv re){
#ifdef _DEBUG
	re->EnableVerbose = false;
#endif
}

void EnableVerbose(rawenv re){
#ifdef _DEBUG
	re->EnableVerbose = true;
#endif
}

uint32_t getblocksize(rawenv re){return re->ConfBlock;}
void setblocksize(rawenv re, uint32_t new){
	re->ConfBlock = new;
	re->CalcBlock = __safediv((new + re->RealBlock - 1), re->RealBlock);
}

rawenv startup(char *path, uint32_t configuredBlockSize){
#ifdef _DEBUG
	printf("\n[Parent:%p] >>  %s  ConfBlockSize: %u    Starting Disk Env ID: ", 
		__builtin_return_address(0), path, configuredBlockSize
	);
#endif
	rawenv re = calloc(1, sizeof(rawenv_t));
#ifdef _DEBUG
	re->EnableVerbose = true;
#endif
	*re = (rawenv_t){
		.file = fopen(path, "rb+"),
		.path = strdup(path),
		.CalcBlock = 1,
		.RealBlock = configuredBlockSize,
		.ConfBlock = configuredBlockSize
	};
	return re;
}

void *readblocks(rawenv re, LBA pos, uint64_t bytes){
    if(!re){
#ifdef _DEBUG
		printf("\nDisk Interface does not exist");
#endif
		return NULL;
	}

    uint64_t blockBytes = re->RealBlock * re->CalcBlock;
    uint64_t nBlocks    = __safediv((bytes + blockBytes - 1), re->RealBlock);
    uint64_t allocSize  = nBlocks * re->RealBlock;
#ifdef _DEBUG
	printf("\nReading Bytes\n[Parent:%p] >> Reading [%u bytes(s)->%u block(s)] to LBA[%llu(%llu)-%llu(%llu)]",
		__builtin_return_address(0), bytes, nBlocks, pos, pos * re->RealBlock * re->CalcBlock, pos + nBlocks, (pos + nBlocks) * re->RealBlock * re->CalcBlock);
#endif
    void *data = calloc(1, allocSize);
    if(!data){return NULL;}
	fseek(re->file, pos * re->RealBlock * re->CalcBlock, SEEK_SET);
	fread(data, 1, allocSize, re->file);
    return data;
}
void *writeblocks(rawenv re, void *data, LBA pos, uint64_t bytes){
	if(!re){
#ifdef _DEBUG
		printf("\nDisk Interface does not exist");
#endif
		return NULL;
	}
	uint64_t nBlocks = __safediv((bytes + (re->RealBlock * re->CalcBlock) - 1), re->RealBlock);
	void *buf = calloc(nBlocks, re->RealBlock);
#ifdef _DEBUG
	printf("\nWriting Bytes\n[Parent:%p] >> Writing [%u bytes(s)->%u block(s)] to LBA[%llu(%llu)-%llu(%llu)]",
			__builtin_return_address(0), bytes, nBlocks, pos, pos * re->RealBlock * re->CalcBlock, pos + nBlocks, (pos + nBlocks) * re->RealBlock * re->CalcBlock);
#endif
	if(buf){
		memcpy(buf, data, bytes);
		fseek(re->file, pos * re->RealBlock * re->CalcBlock, SEEK_SET);
		fwrite(buf, re->RealBlock, nBlocks, re->file);
	}else{
#ifdef _DEBUG
        printf("    Failed to allocate Write-Buffer");
#endif
	}
}

void writebytes(rawenv re, void *data, uint64_t bytepos, uint64_t nbytes){
#ifdef _DEBUG
    printf("\n[Parent:%p] >> Writing [%u bytes(s)] to LBA[%llu-%llu]",
          __builtin_return_address(0), nbytes, __safediv(bytepos * re->CalcBlock, re->ConfBlock), __safediv((bytepos + nbytes) * re->CalcBlock, re->ConfBlock));
#endif
	void *rdata = readblocks(re, __safediv(bytepos * re->CalcBlock, re->ConfBlock), nbytes);
	uint64_t byteoffset = (bytepos * re->CalcBlock) % re->ConfBlock;
	memcpy(rdata + byteoffset, data, nbytes);
	writeblocks(re, rdata, __safediv(bytepos * re->CalcBlock, re->ConfBlock), nbytes);
}

void *readbytes(rawenv re, LBA pos, uint16_t offset, uint64_t nbytes){
#ifdef _DEBUG
    printf("\n[Parent:%p] >> Writing [%u bytes(s)] to LBA[%llu:%u-%llu]",
          __builtin_return_address(0), nbytes, __safediv(pos * re->CalcBlock, re->ConfBlock), offset, __safediv((pos + nbytes) * re->CalcBlock, re->ConfBlock));
#endif
	void *rdata = readblocks(re, __safediv((pos + __safediv(offset, re->ConfBlock)) * re->CalcBlock, re->ConfBlock), nbytes);
	memcpy(rdata, rdata + __safediv(offset, re->ConfBlock) + (offset % re->ConfBlock), nbytes);
	memset(
		rdata + __safediv(offset, re->ConfBlock) + (offset % re->ConfBlock) + nbytes, 
		0, ((nbytes + (re->RealBlock * re->CalcBlock) - 1) * re->RealBlock) - nbytes
	);
	return rdata;
}

void dispose(rawenv re){
#ifdef _DEBUG
	printf("\nClosing Disk Interface >> {%s}", re->path);
#endif
	fflush(re->file);
	fclose(re->file);
	free(re->path);
	free(re);
}