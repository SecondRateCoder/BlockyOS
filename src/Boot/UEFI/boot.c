#include "standard.h"

void libinit(EFI_HANDLE img, EFI_SYSTEM_TABLE *table){
	InitializeLib(img, table);
	gST = table;
	gRT = table->RuntimeServices;
	gBS = table->BootServices;
}

EFI_BLOCK_IO_MEDIA *getParentDevice(EFI_HANDLE Image){
    EFI_STATUS status = -1;
    EFI_LOADED_IMAGE *ImageProc;
    EFI_GUID ImageProcGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    status = uefi_call_wrapper(BS->HandleProtocol, 3, Image, &gEfiLoadedImageProtocolGuid, (void **)(&ImageProc));
    if(!EFI_ERROR(status)){
        EFI_DEVICE_PATH *Dp;
        status = uefi_call_wrapper(BS->HandleProtocol, 2, ImageProc->DeviceHandle, &gEfiDevicePathProtocolGuid, (void **)(&Dp));
        if(!EFI_ERROR(status)){
            EFI_DEVICE_PATH *disk = Dp;
            while(!IsDevicePathEnd(disk)){
                if((disk->Type == MEDIA_DEVICE_PATH) && (disk->SubType == MEDIA_HARDDRIVE_DP)){
                    break;
                }
                disk = NextDevicePathNode(disk);
            }
            UINTN Len = (UINTN)disk - (UINTN)Dp;
            EFI_DEVICE_PATH *ParentPath = __memdup(Dp, Len + sizeof(EFI_DEVICE_PATH_PROTOCOL));
            SetDevicePathEndNode((EFI_DEVICE_PATH*)((UINT8*)ParentPath + Len));
            EFI_HANDLE Dh;
            status = uefi_call_wrapper(BS->LocateDevicePath, 3, &gEfiBlockIoProtocolGuid, ParentPath, &Dh);
            EFI_BLOCK_IO *DiskBlk;
            status = uefi_call_wrapper(BS->HandleProtocol, 3, Dh, &gEfiBlockIoProtocolGuid, (void**)&DiskBlk);
            return DiskBlk->Media;
        }
    }
    return NULL;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE Image, EFI_SYSTEM_TABLE *Table){
	libinit(Image, Table);
	if(EFI_ERROR(ValidateImageHandle(Image))){
#ifdef __DEBUG__
    // Print(L"\nError with Image Handle, Re-starting Image from Boot Menu");
#endif
        // CreateBootEntryLoadAndReboot(Image, L"BlockyOS Boot Device", L"FS0:\\EFI\\BOOT\\BOOTX64.efi");
    }
#ifdef __DEBUG__
    Print(L"\nStarting Boot Device");
#endif
	conf_fsroot *root = NULL;
    // Get Parent MediaID
    EFI_BLOCK_IO_MEDIA *IOMedia = getParentDevice(Image);
	if(IOMedia){
		if(!(root = fmount(Image, IOMedia->MediaId))){
			GPTeNSTR *str = makeGPTeNSTR("Root");
			formatpart(Image, IOMedia->MediaId, *str);
			FreePool(str);
			if(!(root = fmount(Image, IOMedia->MediaId))){
				Print(L"\nError! Couldnt Mount Drive %u\n", IOMedia->MediaId);
				return EFI_SUCCESS;
			}
		}
	}else{
#ifdef __DEBUG__
    Print(L"\nImage Failed");
#endif
    }
	return EFI_ABORTED;
}