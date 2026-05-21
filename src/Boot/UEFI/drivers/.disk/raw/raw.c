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
#ifdef __DEBUG__
	Print(L"\nnHandles:	%llu", nHandles);
#endif
	if(EFI_ERROR(status)){return NULL;}
	for(UINTN i = 0; i < nHandles; i++){
		EFI_BLOCK_IO_PROTOCOL *blk = NULL;
		EFI_DISK_IO_PROTOCOL  *dsk = NULL;
		EFI_STATUS status = uefi_call_wrapper(gBS->HandleProtocol, 3, handles[i], &BlkIoGuid, (void**)&blk);
		status |= uefi_call_wrapper(gBS->HandleProtocol, 3, handles[i], &DskIoGuid,  (void**)&dsk);
#ifdef __DEBUG__
		Print(
			L"\nHandle %u: BlockSize=%u,    LastBlock=%u,    LogicalPartition? = %a,    MediaId=%u,    Status=%llu", 
			i, blk->Media->BlockSize, blk->Media->LastBlock, blk->Media->LogicalPartition? "TRUE": "FALSE", blk->Media->MediaId, (status & ~0xF000000000000000)
		);
#endif
		if(!blk || !dsk || !blk->Media->MediaPresent){continue;}
		UINTN blockSize = blk->Media->BlockSize;
		miniGPT *hdr = __calloc(blockSize, 1);
		status = uefi_call_wrapper(
			dsk->ReadDisk, 5, 
			dsk, blk->Media->MediaId,
			blockSize * 1, blockSize, hdr
		);
		if(EFI_ERROR(status) || (__memcmp(hdr->sig, GPTsig, 8) != 0)){
#ifdef __DEBUG__
			Print(L"\nInvalid Sig");
#endif
			__free(hdr);
			continue;
		}
#ifdef __DEBUG__
		Print(L"\n    Target-GUID={");prGUID(TargetGUID);Print(L"}    Disk-GUID={");prGUID(hdr->dGUID);Print(L"}");
#endif
		if(__memcmp(&hdr->dGUID, &TargetGUID, sizeof(EFI_GUID)) || __memcmp(&hdr->dGUID, &AltGUID, sizeof(EFI_GUID))){
#ifdef __DEBUG__
			Print(L"\nFound Handle");
#endif
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
#ifdef __DEBUG__
			Print(L"\n    Target-GUID={");prGUID(TargetGUID);Print(L"}    Partition-Unique-GUID={");prGUID(p->uGUID);Print(L"}    Partition-GUID={");prGUID(p->GUID);Print(L"}");
#endif
			if(__memcmp(&p->uGUID, &TargetGUID, sizeof(EFI_GUID)) || __memcmp(&p->GUID, &TargetGUID, sizeof(EFI_GUID)) || 
				__memcmp(&p->uGUID, &AltGUID, sizeof(EFI_GUID)) || __memcmp(&p->GUID, &AltGUID, sizeof(EFI_GUID))){
#ifdef __DEBUG__
				Print(L"\nFound Handle");
#endif
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

void DisableVerbose(rawenv re){
#ifdef __DEBUG__
	re->EnableVerbose = false;
#endif
}

void EnableVerbose(rawenv re){
#ifdef __DEBUG__
	re->EnableVerbose = true;
#endif
}

UINT32 getblocksize(rawenv re){return re->ConfBlock;}
void setblocksize(rawenv re, UINT32 new){
	re->ConfBlock = new;
	re->CalcBlock = safediv__((new + re->RealBlock - 1), re->RealBlock);
}

rawenv startup(EFI_GUID GUID, EFI_GUID altGUID, UINT32 configuredBlockSize){
#ifdef __DEBUG__
	Print(L"\n[Parent:%p] >>   ConfBlockSize: %u    Starting Disk Env ID: ", 
		__builtin_return_address(0), configuredBlockSize
	);
	prGUID(GUID);
#endif
	EFI_STATUS status = 0;
	rawenv re = __calloc(1, sizeof(rawenv_t));
#ifdef __DEBUG__
	re->EnableVerbose = TRUE;
#endif
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
				.CalcBlock = safediv__((configuredBlockSize + Blk->Media->BlockSize - 1), Blk->Media->BlockSize),
				.RealBlock = Blk->Media->BlockSize,
				.ConfBlock = configuredBlockSize,
				.handle = handle
			};
			if(re->CalcBlock == 0){re->CalcBlock = 1;}
#ifdef __DEBUG__
			Print(
				L"\nRawenv Dump"
				L"\n    IsPart: %a"
				L"\n    Real-Block Size: %u"
				L"\n    Calculated-Block Size: %u",
				(re->isPart? "TRUE": "FALSE"), re->RealBlock, re->CalcBlock
			);
#endif
			return re;
		}else{
#ifdef __DEBUG__
			Print(L"\nBlock IO Protocol Error    %llu", status & 0xF000000000000000);
#endif
			__free(re);
		}
	}else{
#ifdef __DEBUG__
		Print(L"\nHandle Not Found Error");
#endif
		__free(re);
	}
	return NULL;
}

void *readblocks(rawenv re, LBA pos, UINTN bytes){
    if(!re){
#ifdef __DEBUG__
		Print(L"\nDisk Interface does not exist");
#endif
		return NULL;
	}

    UINTN blockBytes = re->RealBlock * re->CalcBlock;
    UINTN nBlocks    = safediv__((bytes + blockBytes - 1), re->RealBlock);
    UINTN allocSize  = nBlocks * re->RealBlock;
#ifdef __DEBUG__
	Print(L"\nReading Bytes\n[Parent:%p] >> Reading [%u bytes(s)->%u block(s)] to LBA[%llu(%llu)-%llu(%llu)]",
		__builtin_return_address(0), bytes, nBlocks, pos, pos * re->RealBlock * re->CalcBlock, pos + nBlocks, (pos + nBlocks) * re->RealBlock * re->CalcBlock);
#endif
    void *data = __calloc(1, allocSize);
    if(!data){return NULL;}

    EFI_STATUS status = uefi_call_wrapper(
        re->Blk->ReadBlocks, 5, re->Blk,
        re->Blk->Media->MediaId,
        pos * re->CalcBlock, allocSize, data
    );
#ifdef __DEBUG__
    	Print(L"    Read %a:%u:%r", (EFI_ERROR(status)? "ERROR": "..."), status & ~0x8000000000000000, status);
#endif
    if(EFI_ERROR(status)){
        __free(data);
        return NULL;
    }
    return data;
}
// void *readblocks(rawenv re, LBA pos, UINTN bytes){
// 	if(!re){
// #ifdef __DEBUG__
// 		Print(L"\nDisk Interface does not exist");
// #endif
// 		return NULL;
// 	}
// 	UINTN nBlocks = safediv__((bytes + (re->RealBlock * re->CalcBlock) - 1), re->RealBlock);
// 	void *data = __calloc(1, bytes);
// #ifdef __DEBUG__
// 	Print(L"\nReading Bytes\n[Parent:%p] >> Reading [%u bytes(s)->%u block(s)] to LBA[%llu(%llu)-%llu(%llu)]",
// 		__builtin_return_address(0), bytes, nBlocks, pos, pos * re->RealBlock * re->CalcBlock, pos + nBlocks, (pos + nBlocks) * re->RealBlock * re->CalcBlock);
// #endif
// 	if(data){
// 		EFI_STATUS status = uefi_call_wrapper(
// 			re->Blk->ReadBlocks, 5, re->Blk, 
// 			re->Blk->Media->MediaId, 
// 			pos * re->CalcBlock, nBlocks * re->RealBlock, data
// 		);
// 		if(EFI_ERROR(status)){__free(data);	data = NULL;}
// #ifdef __DEBUG__
//     	Print(L"    Read %a:%u:%r", (EFI_ERROR(status)? "ERROR": "..."), status & ~0x8000000000000000, status);
// #endif
// 		return data;
// 	}else{
// #ifdef __DEBUG__
// 		Print(L"    ERROR: Failed to Allocate Read Buffer");
// #endif
// 	}
// 	return NULL;
// }
void *writeblocks(rawenv re, void *data, LBA pos, UINTN bytes){
	if(!re){
#ifdef __DEBUG__
		Print(L"\nDisk Interface does not exist");
#endif
		return NULL;
	}
	EFI_STATUS status;
	UINTN nBlocks = safediv__((bytes + (re->RealBlock * re->CalcBlock) - 1), re->RealBlock);
	void *buf = __calloc(nBlocks, re->RealBlock);
#ifdef __DEBUG__
	Print(L"\nWriting Bytes\n[Parent:%p] >> Writing [%u bytes(s)->%u block(s)] to LBA[%llu(%llu)-%llu(%llu)]",
			__builtin_return_address(0), bytes, nBlocks, pos, pos * re->RealBlock * re->CalcBlock, pos + nBlocks, (pos + nBlocks) * re->RealBlock * re->CalcBlock);
#endif
	if(buf){
		__memcpy(buf, data, bytes);
		status = uefi_call_wrapper(
			re->Blk->WriteBlocks, 5, re->Blk, 
			re->Blk->Media->MediaId, 
			pos * re->CalcBlock, nBlocks * re->RealBlock, buf
		);
	}else{
#ifdef __DEBUG__
        Print(L"    Failed to allocate Write-Buffer");
#endif
	}
#ifdef __DEBUG__
    Print(L"    Write %a:%u:%r", (EFI_ERROR(status)? "ERROR": "..."), status & ~0x8000000000000000, status);
#endif
}

void writebytes(rawenv re, void *data, UINTN bytepos, UINTN nbytes){
#ifdef __DEBUG__
    Print(L"\n[Parent:%p] >> Writing [%u bytes(s)] to LBA[%llu-%llu]",
          __builtin_return_address(0), nbytes, safediv__(bytepos * re->CalcBlock, re->ConfBlock), safediv__((bytepos + nbytes) * re->CalcBlock, re->ConfBlock));
#endif
	void *rdata = readblocks(re, safediv__(bytepos * re->CalcBlock, re->ConfBlock), nbytes);
	UINTN byteoffset = (bytepos * re->CalcBlock) % re->ConfBlock;
	__memcpy(rdata + byteoffset, data, nbytes);
	writeblocks(re, rdata, safediv__(bytepos * re->CalcBlock, re->ConfBlock), nbytes);
}

void *readbytes(rawenv re, LBA pos, UINT16 offset, UINTN nbytes){
#ifdef __DEBUG__
    Print(L"\n[Parent:%p] >> Writing [%u bytes(s)] to LBA[%llu:%u-%llu]",
          __builtin_return_address(0), nbytes, safediv__(pos * re->CalcBlock, re->ConfBlock), offset, safediv__((pos + nbytes) * re->CalcBlock, re->ConfBlock));
#endif
	void *rdata = readblocks(re, safediv__((pos + safediv__(offset, re->ConfBlock)) * re->CalcBlock, re->ConfBlock), nbytes);
	__safecopy(rdata, rdata + safediv__(offset, re->ConfBlock) + (offset % re->ConfBlock), nbytes);
	__memset(
		rdata + safediv__(offset, re->ConfBlock) + (offset % re->ConfBlock) + nbytes, 
		0, ((nbytes + (re->RealBlock * re->CalcBlock) - 1) * re->RealBlock) - nbytes
	);
	return rdata;
}

void dispose(rawenv re){
#ifdef __DEBUG__
	Print(L"\nClosing Disk Interface >> {");prGUID(re->GUID);Print(L"}");
#endif
	EFI_GUID BlkIoGuid = EFI_BLOCK_IO_PROTOCOL_GUID;
	uefi_call_wrapper(gBS->CloseProtocol, 3, &BlkIoGuid, re->handle, NULL);
	__free(re);
}