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
			L"\nHandle %u: BlockSize=%u, LastBlock=%u, LogicalPartition? = %a, MediaId=%u:    Status=%llu", 
			i, blk->Media->BlockSize, blk->Media->LastBlock, blk->Media->LogicalPartition? "TRUE": "FALSE", blk->Media->MediaId, (status & ~0xF000000000000000)
		);
#endif
		if(!blk || !dsk || !blk->Media->MediaPresent){continue;}
        UINTN blockSize = blk->Media->BlockSize;
        miniGPT *hdr = AllocateZeroPool(blockSize);
        status = uefi_call_wrapper(
			dsk->ReadDisk, 5, 
            dsk, blk->Media->MediaId,
            blockSize * 1, blockSize, hdr
        );
        if(EFI_ERROR(status) || (CompareMem(hdr->sig, GPTsig, 8) != 0)){
#ifdef __DEBUG__
			Print(L"\nInvalid Sig");
#endif
            FreePool(hdr);
            continue;
        }
#ifdef __DEBUG__
		Print(L"\n    Target-GUID={");prGUID(TargetGUID);Print(L"}    Disk-GUID={");prGUID(hdr->dGUID);Print(L"}");
#endif
		if(CompareMem(&hdr->dGUID, &TargetGUID, sizeof(EFI_GUID)) || CompareMem(&hdr->dGUID, &AltGUID, sizeof(EFI_GUID))){
#ifdef __DEBUG__
			Print(L"\nFound Handle");
#endif
			EFI_HANDLE found = handles[i];
			FreePool(hdr);
			FreePool(handles);
			return found;
		}
        UINTN entriesSize = hdr->nPartEntries * hdr->partEntrySize;
        GPTentry *entries = AllocateZeroPool(entriesSize);
        status = uefi_call_wrapper(
			dsk->ReadDisk, 5, 
            dsk, blk->Media->MediaId,
            hdr->partEntryLoc * blockSize,
            entriesSize, entries
        );
        if(EFI_ERROR(status)){
            FreePool(entries);
            FreePool(hdr);
            continue;
        }
        for(UINTN e = 0; e < hdr->nPartEntries; e++){
            GPTentry *p = (GPTentry *)(((UINT8 *)entries) + (e * hdr->partEntrySize));
#ifdef __DEBUG__
			Print(L"\n    Target-GUID={");prGUID(TargetGUID);Print(L"}    Partition-Unique-GUID={");prGUID(p->uGUID);Print(L"}    Partition-GUID={");prGUID(p->GUID);Print(L"}");
#endif
            if(CompareMem(&p->uGUID, &TargetGUID, sizeof(EFI_GUID)) || CompareMem(&p->GUID, &TargetGUID, sizeof(EFI_GUID)) || 
				CompareMem(&p->uGUID, &AltGUID, sizeof(EFI_GUID)) || CompareMem(&p->GUID, &AltGUID, sizeof(EFI_GUID))){
#ifdef __DEBUG__
				Print(L"\nFound Handle");
#endif
                EFI_HANDLE found = handles[i];
                FreePool(entries);
                FreePool(hdr);
                FreePool(handles);
                return found;
            }
        }
        FreePool(entries);
        FreePool(hdr);
    }
    FreePool(handles);
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
	re->CalcBlock = (new + re->RealBlock - 1) / re->RealBlock;
}

rawenv startup(EFI_GUID GUID, EFI_GUID altGUID, UINT32 configuredBlockSize){
#ifdef __DEBUG__
	Print(L"\n[%a:%u : Parent:%p] >>   ConfBlockSize: %u    Starting Disk Env ID: ", 
		__FILE__, __LINE__, __builtin_return_address(0), configuredBlockSize
	);
	prGUID(GUID);
#endif
	EFI_STATUS status = 0;
	rawenv re = AllocatePool(sizeof(rawenv_t));
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
				.CalcBlock = ((configuredBlockSize + Blk->Media->BlockSize - 1) / Blk->Media->BlockSize),
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
			FreePool(re);
		}
	}else{
#ifdef __DEBUG__
		Print(L"\nHandle Not Found Error");
#endif
		FreePool(re);
	}
	return NULL;
}

void *readblocks(rawenv re, LBA pos, UINTN bytes){
	if(re){
		UINTN nAccBlocks = ((bytes * re->CalcBlock) / re->ConfBlock) + (((bytes * re->CalcBlock * re->RealBlock) / re->ConfBlock) != 0);
#ifdef __DEBUG__
		Print(L"\n[Parent:%p] >> Reading [%u bytes(s)->%u block(s)] from LBA[%u->(%u)]", 
			__builtin_return_address(0), bytes, nAccBlocks, pos, pos * re->CalcBlock
		);
#endif
		EFI_STATUS status = -1;
		void *out = AllocatePool(nAccBlocks * re->RealBlock);
		if(out){
			status = uefi_call_wrapper(
				re->Blk->ReadBlocks, 5, re->Blk, re->Blk->Media->MediaId, 
				pos, nAccBlocks * re->RealBlock, out
			);
			if(EFI_ERROR(status)){
#ifdef __DEBUG__
				Print(L"    Read ERROR %u:%r", status & ~0x8000000000000000, status);
#endif
				FreePool(out);
				return NULL;
			}
#ifdef __DEBUG__
			Print(L"    Read Success");
#endif
			return out;
		}else{
#ifdef __DEBUG__
			Print(L"    Read ERROR: Could Not allocate Buffer");
#endif
		}
	}
	return NULL;
}

void writeblocks(rawenv re, void *buffer, LBA pos, UINTN bytes){
	if(re){
		UINTN nAccBlocks = ((bytes * re->CalcBlock) / re->ConfBlock) + (((bytes * re->CalcBlock * re->RealBlock) / re->ConfBlock) != 0);
#ifdef __DEBUG__
		Print(L"\n[Parent:%p] >> Writing [%u bytes(s)->%u block(s)] to LBA[%u->(%u)]", 
			__builtin_return_address(0), bytes, nAccBlocks, pos, pos * re->CalcBlock
		);
#endif
		EFI_STATUS status = EFI_ERROR_MASK;
		void *dup = AllocatePool(nAccBlocks * re->RealBlock);
		Print(L"    Allocate");
		if(dup){
			__memcpy(dup, buffer, bytes);
			Print(L"    Memcpy");
			status = uefi_call_wrapper(
				re->Blk->WriteBlocks, 5, re->Blk, re->Blk->Media->MediaId, 
				pos, nAccBlocks * re->RealBlock, dup
			);
			Print(L"    Write");
			FreePool(dup);
		}else{Print(L"    Failed to allocate Write-Buffer");}
		// status = uefi_call_wrapper(
		// 	re->Blk->WriteBlocks, 5, re->Blk, re->Blk->Media->MediaId, 
		// 	pos, nAccBlocks * re->RealBlock, buffer
		// );
#ifdef __DEBUG__
		Print(L"    Write %a:%u:%r", (EFI_ERROR(status)? "ERROR": "..."), status & ~0x8000000000000000, status);
#endif
	}
}

void *readbytes(rawenv re, UINT64 byteOffset, UINTN byteCount){
    if(!re || byteCount == 0){return NULL;}
    UINT32 block = re->RealBlock;
	
    UINT64 startBlock = byteOffset / block;
    UINT64 endBlock   = (byteOffset + byteCount - 1) / block;
    UINTN  nBlocks    = (endBlock - startBlock) + 1;

    void *blkData = readblocks(re, startBlock, nBlocks);
    if(!blkData){return NULL;}

    UINT64 offsetInBlock = byteOffset % block;
    void *out = AllocatePool(byteCount);
    if(!out){FreePool(blkData);    return NULL;}

    CopyMem(out, (UINT8*)blkData + offsetInBlock, byteCount);
    FreePool(blkData);
    return out;
}

EFI_STATUS writebytes(rawenv re, UINT64 byteOffset, void *buffer, UINTN byteCount){
    if(!re || !buffer || byteCount == 0){return EFI_INVALID_PARAMETER;}
    UINT32 block = re->RealBlock;

    UINT64 startBlock = byteOffset / block;
    UINT64 endBlock   = (byteOffset + byteCount - 1) / block;
    UINTN  nBlocks    = (endBlock - startBlock) + 1;

    void *blkData = readblocks(re, startBlock, nBlocks);
    if(!blkData){return EFI_DEVICE_ERROR;}

    UINT64 offsetInBlock = byteOffset % block;
    CopyMem((UINT8*)blkData + offsetInBlock, buffer, byteCount);

    EFI_STATUS status = EFI_SUCCESS;
    writeblocks(re, blkData, startBlock, nBlocks);

    FreePool(blkData);
    return status;
}

void dispose(rawenv re){
#ifdef __DEBUG__
	Print(L"\nClosing Disk Interface >> {");prGUID(re->GUID);Print(L"}");
#endif
	EFI_GUID BlkIoGuid = EFI_BLOCK_IO_PROTOCOL_GUID;
	uefi_call_wrapper(gBS->CloseProtocol, 3, &BlkIoGuid, re->handle, NULL);
	FreePool(re);
}