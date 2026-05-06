#include "raw.h"

void configureBlockSize(rawenv *re, UINT32 configBlockSize){re->configBlockSize = configBlockSize;}

#ifdef __DEBUG__
    void EnableVerbose(rawenv *re){re->EnableVerbose = true;}
	void DisableVerbose(rawenv *re){re->EnableVerbose = false;}
#else
	void EnableVerbose(rawenv *re){return;}
	void DisableVerbose(rawenv *re){return;}
#endif

rawenv *startup_me(EFI_HANDLE Image, UINT32 configblocksize){
	EFI_LOADED_IMAGE_PROTOCOL *lImage;
	EFI_GUID LoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
	if(!EFI_ERROR(BS->HandleProtocol(Image, &LoadedImageProtocolGuid, (void**)&lImage))){
		EFI_BLOCK_IO_PROTOCOL *Blk;
		EFI_GUID BlockIoGuid = EFI_BLOCK_IO_PROTOCOL_GUID;
		if(!EFI_ERROR(BS->HandleProtocol(lImage->DeviceHandle, &BlockIoGuid, (void **)&Blk))){
			return startup(Blk->Media->MediaId, configblocksize);
		}
	}
}

rawenv *startup(UINT32 MediaID, UINT32 configblocksize){
#ifdef __DEBUG__
	Print(L"\n\nStarting File Interface[%a:%u]: %u", __FILE__, __LINE__, MediaID);
#endif
	rawenv *re = AllocatePool(sizeof(rawenv));
#ifdef __DEBUG__
	re->EnableVerbose = true;
#endif
	UINTN count;	EFI_HANDLE *handles;
	EFI_GUID BlockIOGuid = EFI_BLOCK_IO_PROTOCOL_GUID;
	BS->LocateHandleBuffer(ByProtocol, &BlockIOGuid, NULL, &count, &handles);
	EFI_GUID DiskIOGuid = EFI_DISK_IO_PROTOCOL_GUID;
	EFI_BLOCK_IO_PROTOCOL *Blk = NULL;
	EFI_DISK_IO_PROTOCOL  *Dsk = NULL;
	EFI_STATUS BlkStatus = (EFI_STATUS)-1, DskStatus = (EFI_STATUS)-1;
	for(UINTN cc = 0; cc < count; cc++){
		if(EFI_ERROR(BlkStatus)){BlkStatus = BS->HandleProtocol(handles[cc], &BlockIOGuid, (void **)&Blk);}
		if(EFI_ERROR(DskStatus)){DskStatus = BS->HandleProtocol(handles[cc], &DiskIOGuid, (void **)&Dsk);}
		if(EFI_ERROR(BlkStatus) && EFI_ERROR(DskStatus)){continue;
		}else{
			if(Blk->Media->MediaId == MediaID){
				re->configBlockSize = configblocksize;
				re->B = Blk;
				re->D = Dsk;
				return re;
			}else{continue;}
		}
	}
	FreePool(re);
	return NULL;
}

void *readblock(rawenv *re, UINTN pos, UINTN blocks){
#ifdef __DEBUG__
	if(re->EnableVerbose){Print(L"\nReading [%a:%u]: Addr:%llu Size:%llu", __FILE__, __LINE__, pos, blocks);}
#endif
	void *out = AllocatePool(blocks * re->B->Media->BlockSize);
	EFI_STATUS status = re->D->ReadDisk(
		re->D, re->B->Media->MediaId, 
		(pos * re->configBlockSize), (blocks * re->configBlockSize),
		out
	);
#ifdef __DEBUG__
	Print(L"\nRead Res: %a", ((!EFI_ERROR(status)? "TRUE": "FALSE")));
#endif
	if(EFI_ERROR(status)){
		FreePool(out);		out = NULL;
	}
#ifdef __DEEP_DEBUG__ && __DEBUG__
	for(UINT32 cc = 0; cc < (blocks * re->configBlockSize); ++cc){Print(L"%u", ((UINT8 *)out)[cc]);}
#endif
	return out;
}

BOOLEAN writeblock(rawenv *re, void *buffer, size_t pos, size_t blocks){
#ifdef __DEBUG__
	if(re->EnableVerbose){Print(L"\nReading [%a:%u]: Addr:%llu Size:%llu", __FILE__, __LINE__, pos, blocks);}
	#ifdef __DEEP_DEBUG__
		Print(L"\n\n");
		for(UINT32 cc = 0; cc < (blocks * re->configBlockSize); ++cc){Print(L"%u", ((UINT8 *)buffer)[cc]);}
		Print(L"\n\n");
	#endif
#endif
	EFI_STATUS status = re->D->WriteDisk(
		re->D, re->B->Media->MediaId, 
		pos * re->configBlockSize, blocks * re->configBlockSize,
		buffer
	);
#ifdef __DEBUG__
	Print(L"\nWrite Res: %a", ((!EFI_ERROR(status)? "TRUE": "FALSE")));
#endif
	return EFI_ERROR(status);
}

void dispose(rawenv *re){
#ifdef __DEBUG__
	if(re->EnableVerbose){Print(L"\nDisposing File Interface");}
#endif
	FreePool(re);
}