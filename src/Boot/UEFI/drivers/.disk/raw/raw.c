#include "raw.h"

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

rawenv startup(UINT32 MediaID, UINT32 configuredBlockSize){
	EFI_STATUS status = 0;
	EFI_HANDLE *handles = NULL;
	UINTN handlecount = 0;
	EFI_GUID BlockIoGuid = EFI_BLOCK_IO_PROTOCOL_GUID,
			 DiskIoGuid = EFI_DISK_IO_PROTOCOL_GUID,
			 DevPathGuid = EFI_DEVICE_PATH_PROTOCOL_GUID;
	rawenv re = AllocatePool(sizeof(rawenv_t));
#ifdef __DEBUG__
	re->EnableVerbose = TRUE;
#endif
	// Check for Disk Protocols
	status = uefi_call_wrapper(BS->LocateHandleBuffer, 5, ByProtocol, &BlockIoGuid, &handlecount, NULL);
	if(!EFI_ERROR(status)){
		handles = AllocatePool(handlecount * sizeof(EFI_HANDLE));
		status = uefi_call_wrapper(BS->LocateHandleBuffer, 5, ByProtocol, &BlockIoGuid, &handlecount, handles);
		if(EFI_ERROR(status)){
			FreePool(re);
			FreePool(handles);
			return NULL;
		}
	}
	for(UINTN cc = 0; cc < handlecount; ++cc){
		EFI_DEVICE_PATH *Dp;
		status = uefi_call_wrapper(BS->HandleProtocol, 3, handles[cc], &DevPathGuid, (void **)(&Dp));
		if(EFI_ERROR(status)){FreePool(handles);		FreePool(re);		return NULL;}
		EFI_BLOCK_IO *Blk = NULL;
		status = uefi_call_wrapper(BS->HandleProtocol, 3, handles[cc], &BlockIoGuid, (void **)(&Blk));
		if(!EFI_ERROR(status)){
			if(Blk->Media->MediaId == MediaID){
				*re = (rawenv_t){
					.Blk = Blk,
					.isPart = IsPartition(Dp),
					.MediaID = MediaID,
					.RealBlock = Blk->Media->BlockSize,
					.CalcBlock = (configuredBlockSize + Blk->Media->BlockSize - 1) / Blk->Media->BlockSize,
					.ConfBlock = configuredBlockSize,
					.Dev = GetDevicePath(handles[cc])
				};
				return re;
			}else{FreePool(handles);		FreePool(re);		return NULL;}
		}else{FreePool(handles);		FreePool(re);		return NULL;}
	}
}

void *readblock(rawenv re, UINTN pos, UINTN blocks){
#ifdef __DEBUG__
	Print(L"\n[%a:%u:  Parent:%p] >> Reading [%llu:%llu] from [%llu:%llu]", 
		__FILE__, __LINE__, __builtin_return_address(0),
		blocks, (blocks * re->RealBlock) + ((re->ConfBlock / re->RealBlock) != 0), pos, pos * re->CalcBlock
	);
#endif
	EFI_STATUS status = -1;
	void *out = AllocatePool((blocks * re->RealBlock) + ((re->ConfBlock / re->RealBlock) != 0));
	if(out){
		status = uefi_call_wrapper(
			re->Blk->ReadBlocks, 5, re->Blk,
			pos * re->CalcBlock, (blocks * re->RealBlock) + ((re->ConfBlock / re->RealBlock) != 0), 
			out
		);
		if(EFI_ERROR(status)){
			FreePool(out);
#ifdef __DEBUG__
			Print(L"Read Error");
#endif
			return NULL;
		}
#ifdef __DEBUG__
		Print(L"Read %a", (EFI_ERROR(status)? "FAILURE": "SUCCESS"));
#endif
		return out;
	}
	return NULL;
}

void writeblock(rawenv re, void *buffer, UINTN pos, UINTN blocks){
	#ifdef __DEBUG__
	Print(L"\n[%a:%u:  Parent:%p] >> Reading [%llu:%llu] from [%llu:%llu]", 
		__FILE__, __LINE__, __builtin_return_address(0),
		blocks, (blocks * re->RealBlock) + ((re->ConfBlock / re->RealBlock) != 0),  pos, pos * re->CalcBlock
	);
#endif
	EFI_STATUS status = 0;
	status = uefi_call_wrapper(
		re->Blk->WriteBlocks, 5, re->Blk,
		pos * re->CalcBlock, (blocks * re->RealBlock) + ((re->ConfBlock / re->RealBlock) != 0), 
		buffer
	);
#ifdef __DEBUG__
		Print(L"\nWrite %a", (EFI_ERROR(status)? "FAILURE": "SUCCESS"));
#endif
}

void dispose(rawenv re){
	FreePool(re);
}