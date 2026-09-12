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

rawenv startup(char *path, LBA PartitionBase, uint32_t configuredBlockSize){
#ifdef _DEBUG
	printf("\n[Parent:%p] >>  %s  Base: %u    ConfBlockSize: %u    Starting Disk Env ID: ", 
		__builtin_return_address(0), path, PartitionBase, configuredBlockSize
	);
#endif
	rawenv re = calloc(1, sizeof(rawenv_t));
	if(!re){return NULL;}
#ifdef _DEBUG
	re->EnableVerbose = true;
#endif
	*re = (rawenv_t){
		.file = fopen(path, "rb+"),
		.path = strdup(path),
		.Partition = PartitionBase,
		.CalcBlock = (configuredBlockSize / DefaultRawDiskSize) + ((configuredBlockSize % DefaultRawDiskSize) != 0),
		.RealBlock = DefaultRawDiskSize,
		.ConfBlock = configuredBlockSize
	};
	if(!re->file){free(re->path); free(re); return NULL;}
	if(!re->CalcBlock){re->CalcBlock = 1;}
	return re;
}

void *readblocks(rawenv re, LBA pos, uint64_t bytes){
    if(!re){
#ifdef _DEBUG
		printf("\nDisk Interface does not exist");
#endif
		return NULL;
	}
	if(pos < re->Partition){printf("\nInvalid Read Position %llu", (uint64_t)pos);	return NULL;}
    uint64_t blockBytes = re->RealBlock * re->CalcBlock;
	uint64_t nBlocks    = __safediv((bytes + blockBytes - 1), blockBytes) * re->CalcBlock;
    uint64_t allocSize  = nBlocks * re->RealBlock;
#ifdef _DEBUG
	printf("\nReading Bytes\n[Parent:%p] >> Reading [%u bytes(s)->%u block(s)] from LBA[%llu(%llu)-%llu(%llu)]",
		__builtin_return_address(0), bytes, nBlocks, pos, 
		(re->Partition * re->RealBlock) + ((pos - re->Partition) * re->RealBlock * re->CalcBlock), pos + nBlocks, 
		(re->Partition * re->RealBlock) + (((pos + nBlocks) - re->Partition) * re->RealBlock * re->CalcBlock));
#endif
    void *data = calloc(1, allocSize);
    if(!data){return NULL;}
	if(fseek(re->file, (re->Partition * re->RealBlock) + ((pos - re->Partition) * re->RealBlock * re->CalcBlock), SEEK_SET) != 0 ||
		fread(data, 1, allocSize, re->file) != allocSize){
		free(data);
		return NULL;
	}
    return data;
}
void writeblocks(rawenv re, void *data, LBA pos, uint64_t bytes){
	if(!re){
#ifdef _DEBUG
		printf("\nDisk Interface does not exist");
#endif
		return;
	}
	if(!data || pos < re->Partition){printf("\nInvalid Write Position %llu", (uint64_t)pos);	return;}
	uint64_t blockBytes = re->RealBlock * re->CalcBlock;
	uint64_t nBlocks = __safediv((bytes + blockBytes - 1), blockBytes) * re->CalcBlock;
	void *buf = calloc(nBlocks, re->RealBlock);
#ifdef _DEBUG
	printf("\nWriting Bytes\n[Parent:%p] >> Wrtiting [%u bytes(s)->%u block(s)] to LBA[%llu(%llu)-%llu(%llu)]",
		__builtin_return_address(0), bytes, nBlocks, pos, 
		(re->Partition * re->RealBlock) + ((pos - re->Partition) * re->RealBlock * re->CalcBlock), pos + nBlocks, 
		(re->Partition * re->RealBlock) + (((pos + nBlocks) - re->Partition) * re->RealBlock * re->CalcBlock));
#endif
	if(buf){
		memcpy(buf, data, bytes);
		if(fseek(re->file, (re->Partition * re->RealBlock) + ((pos - re->Partition) * re->RealBlock * re->CalcBlock), SEEK_SET) == 0){
			fwrite(buf, re->RealBlock, nBlocks, re->file);
		}
		free(buf);
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
	if(!re){return;}
#ifdef _DEBUG
	printf("\nClosing Disk Interface >> {%s}", re->path ? re->path : "");
#endif
	if(re->file){fflush(re->file); fclose(re->file);}
	free(re->path);
	free(re);
}