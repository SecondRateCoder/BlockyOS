#include "raw.h"

EFI_HANDLE FindDiskHandleByGUID(EFI_GUID TargetGUID, EFI_GUID AltGUID){
	EFI_STATUS status;
	EFI_HANDLE *handles = NULL;
	UINTN nHandles = 0;
	EFI_GUID BlkIoGuid = EFI_BLOCK_IO_PROTOCOL_GUID, DskIoGuid = EFI_DISK_IO_PROTOCOL_GUID;
	status = uefi_call_wrapper(
		gBS->LocateHandleBuffer, 5,
		ByProtocol, &BlkIoGuid,
		NULL, &nHandles, &handles
	);
	DEBUGPRINT(L"\nnHandles:	%llu", nHandles);
	if(EFI_ERROR(status)){return NULL;}
	for(UINTN i = 0; i < nHandles; i++){
		EFI_BLOCK_IO_PROTOCOL *blk = NULL;
		EFI_DISK_IO_PROTOCOL  *dsk = NULL;
		EFI_STATUS status = uefi_call_wrapper(gBS->HandleProtocol, 3, handles[i], &BlkIoGuid, (void**)&blk);
		status |= uefi_call_wrapper(gBS->HandleProtocol, 3, handles[i], &DskIoGuid,  (void**)&dsk);
		DEBUGPRINT(
			L"\nHandle %u: BlockSize=%u,    LastBlock=%u,    LogicalPartition? = %a,    MediaId=%u,    Status=%llu", 
			i, blk->Media->BlockSize, blk->Media->LastBlock, blk->Media->LogicalPartition? "TRUE": "FALSE", blk->Media->MediaId, (status & ~0xF000000000000000)
		);
		if(!blk || !dsk || !blk->Media->MediaPresent){continue;}
		UINTN blockSize = blk->Media->BlockSize;
		miniGPT *hdr = __calloc(blockSize, 1);
		status = uefi_call_wrapper(
			dsk->ReadDisk, 5, 
			dsk, blk->Media->MediaId,
			blockSize * 1, blockSize, hdr
		);
		if(EFI_ERROR(status) || (__memcmp(hdr->sig, GPTsig, 8) != 0)){
			DEBUGPRINT(L"\nInvalid Sig");
			__free(hdr);
			continue;
		}
		DEBUGDO{DEBUGPRINT(L"\n    Target-GUID={");prGUID(TargetGUID);Print(L"}    Disk-GUID={");prGUID(hdr->dGUID);Print(L"}");}
		if(__memcmp(&hdr->dGUID, &TargetGUID, sizeof(EFI_GUID)) || __memcmp(&hdr->dGUID, &AltGUID, sizeof(EFI_GUID))){
			DEBUGPRINT(L"\nFound Handle");
			EFI_HANDLE found = handles[i];
			__free(hdr);
			__free(handles);
			return found;
		}
		UINTN entriesSize = hdr->nPartEntries * hdr->partEntrySize;
		GPTentry *entries = __calloc(entriesSize, 1);
		status = uefi_call_wrapper(
			dsk->ReadDisk, 5, 
			dsk, blk->Media->MediaId,
			hdr->partEntryLoc * blockSize,
			entriesSize, entries
		);
		if(EFI_ERROR(status)){
			__free(entries);
			__free(hdr);
			continue;
		}
		for(UINTN e = 0; e < hdr->nPartEntries; e++){
			GPTentry *p = (GPTentry *)(((UINT8 *)entries) + (e * hdr->partEntrySize));
			DEBUGDO{DEBUGPRINT(L"\n    Target-GUID={");prGUID(TargetGUID);Print(L"}    Partition-Unique-GUID={");prGUID(p->uGUID);Print(L"}    Partition-GUID={");prGUID(p->GUID);Print(L"}");}
			if(__memcmp(&p->uGUID, &TargetGUID, sizeof(EFI_GUID)) || __memcmp(&p->GUID, &TargetGUID, sizeof(EFI_GUID)) || 
				__memcmp(&p->uGUID, &AltGUID, sizeof(EFI_GUID)) || __memcmp(&p->GUID, &AltGUID, sizeof(EFI_GUID))){
				DEBUGPRINT(L"\nFound Handle");
				EFI_HANDLE found = handles[i];
				__free(entries);
				__free(hdr);
				__free(handles);
				return found;
			}
		}
		__free(entries);
		__free(hdr);
	}
	__free(handles);
	return NULL;
}

void DisableVerbose(rawenv re){DEBUGDO{re->EnableVerbose = false;}}

void EnableVerbose(rawenv re){DEBUGDO{re->EnableVerbose = true;}}

UINT32 getblocksize(rawenv re){return re->ConfBlock;}
void setblocksize(rawenv re, UINT32 new){
	re->ConfBlock = new;
	re->CalcBlock = __safediv((new + re->RealBlock - 1), re->RealBlock);
}

rawenv startup(EFI_GUID GUID, EFI_GUID altGUID, UINT32 configuredBlockSize){
	DEBUGDO{
		Print(L"\n[Parent:%p] >>   ConfBlockSize: %u    Starting Disk Env ID: ", 
			__builtin_return_address(0), configuredBlockSize
		);
		prGUID(GUID);
	}
	EFI_STATUS status = 0;
	rawenv re = __calloc(1, sizeof(rawenv_t));
	DEBUGDO{re->EnableVerbose = TRUE;}
	EFI_GUID BlockIoGuid = EFI_BLOCK_IO_PROTOCOL_GUID;
	EFI_BLOCK_IO_PROTOCOL *Blk;
	EFI_HANDLE handle = FindDiskHandleByGUID(GUID, altGUID);
	if(handle){
		status = uefi_call_wrapper(BS->HandleProtocol, 3, handle, &BlockIoGuid, (void **)&Blk);
		if(!EFI_ERROR(status)){
			*re = (rawenv_t){
				.Blk = Blk,
				.isPart = Blk->Media->LogicalPartition,
				.GUID = GUID,
				.CalcBlock = __safediv((configuredBlockSize + Blk->Media->BlockSize - 1), Blk->Media->BlockSize),
				.RealBlock = Blk->Media->BlockSize,
				.ConfBlock = configuredBlockSize,
				.handle = handle
			};
			if(re->CalcBlock == 0){re->CalcBlock = 1;}
			DEBUGPRINT(
				L"\nRawenv Dump"
				L"\n    IsPart: %a"
				L"\n    Real-Block Size: %u"
				L"\n    Calculated-Block Size: %u",
				(re->isPart? "TRUE": "FALSE"), re->RealBlock, re->CalcBlock
			);
			return re;
		}else{
			DEBUGPRINT(L"\nBlock IO Protocol Error    %llu", status & 0xF000000000000000);
			__free(re);
		}
	}else{
		DEBUGPRINT(L"\nHandle Not Found Error");
		__free(re);
	}
	return NULL;
}

void *readblocks(rawenv re, LBA pos, UINTN bytes){
    if(!re){
		DEBUGPRINT(L"\nDisk Interface does not exist");
		return NULL;
	}

    UINTN blockBytes = re->RealBlock * re->CalcBlock;
    UINTN nBlocks    = __safediv((bytes + blockBytes - 1), re->RealBlock);
    UINTN allocSize  = nBlocks * re->RealBlock;
	DEBUGPRINT(L"\nReading Bytes\n[Parent:%p] >> Reading [%u bytes(s)->%u block(s)] to LBA[%llu(%llu)-%llu(%llu)]",
		__builtin_return_address(0), bytes, nBlocks, pos, pos * re->RealBlock * re->CalcBlock, pos + nBlocks, (pos + nBlocks) * re->RealBlock * re->CalcBlock);
    void *data = __calloc(1, allocSize);
    if(!data){return NULL;}

    EFI_STATUS status = uefi_call_wrapper(
        re->Blk->ReadBlocks, 5, re->Blk,
        re->Blk->Media->MediaId,
        pos * re->CalcBlock, allocSize, data
    );
	DEBUGPRINT(L"    Read %a:%u:%r", (EFI_ERROR(status)? "ERROR": "..."), status & ~0x8000000000000000, status);
    if(EFI_ERROR(status)){
        __free(data);
        return NULL;
    }
    return data;
}
void writeblocks(rawenv re, void *data, LBA pos, UINTN bytes){
	if(!re){DEBUGPRINT(L"\nDisk Interface does not exist");}
	EFI_STATUS status;
	UINTN nBlocks = __safediv((bytes + (re->RealBlock * re->CalcBlock) - 1), re->RealBlock);
	void *buf = __calloc(nBlocks, re->RealBlock);
	DEBUGPRINT(L"\nWriting Bytes\n[Parent:%p] >> Writing [%u bytes(s)->%u block(s)] to LBA[%llu(%llu)-%llu(%llu)]",
			__builtin_return_address(0), bytes, nBlocks, pos, pos * re->RealBlock * re->CalcBlock, pos + nBlocks, (pos + nBlocks) * re->RealBlock * re->CalcBlock);
	if(buf){
		__memcpy(buf, data, bytes);
		status = uefi_call_wrapper(
			re->Blk->WriteBlocks, 5, re->Blk, 
			re->Blk->Media->MediaId, 
			pos * re->CalcBlock, nBlocks * re->RealBlock, buf
		);
	}else{DEBUGPRINT(L"    Failed to allocate Write-Buffer");}
    DEBUGPRINT(L"    Write %a:%u:%r", (EFI_ERROR(status)? "ERROR": "..."), status & ~0x8000000000000000, status);
	return;
}

void writebytes(rawenv re, void *data, UINTN bytepos, UINTN nbytes){
    DEBUGPRINT(L"\n[Parent:%p] >> Writing [%u bytes(s)] to LBA[%llu-%llu]",
          __builtin_return_address(0), nbytes, __safediv(bytepos * re->CalcBlock, re->ConfBlock), __safediv((bytepos + nbytes) * re->CalcBlock, re->ConfBlock));
	void *rdata = readblocks(re, __safediv(bytepos * re->CalcBlock, re->ConfBlock), nbytes);
	UINTN byteoffset = (bytepos * re->CalcBlock) % re->ConfBlock;
	__memcpy(rdata + byteoffset, data, nbytes);
	writeblocks(re, rdata, __safediv(bytepos * re->CalcBlock, re->ConfBlock), nbytes);
	return;
}

void *readbytes(rawenv re, LBA pos, UINT16 offset, UINTN nbytes){
    DEBUGPRINT(L"\n[Parent:%p] >> Writing [%u bytes(s)] to LBA[%llu:%u-%llu]",
          __builtin_return_address(0), nbytes, __safediv(pos * re->CalcBlock, re->ConfBlock), offset, __safediv((pos + nbytes) * re->CalcBlock, re->ConfBlock));
	void *rdata = readblocks(re, __safediv((pos + __safediv(offset, re->ConfBlock)) * re->CalcBlock, re->ConfBlock), nbytes);
	__safecopy(rdata, rdata + __safediv(offset, re->ConfBlock) + (offset % re->ConfBlock), nbytes);
	__memset(
		rdata + __safediv(offset, re->ConfBlock) + (offset % re->ConfBlock) + nbytes, 
		0, ((nbytes + (re->RealBlock * re->CalcBlock) - 1) * re->RealBlock) - nbytes
	);
	return rdata;
}

void dispose(rawenv re){
	DEBUGDO{DEBUGPRINT(L"\nClosing Disk Interface >> {");prGUID(re->GUID);Print(L"}");}
	__free(re);
	return;
}