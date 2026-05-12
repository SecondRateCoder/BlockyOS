#include "standard.h"

EFI_STATUS EFIAPI efi_main(EFI_HANDLE Image, EFI_SYSTEM_TABLE *Table){
#ifdef __DEBUG__
	Print(L"Sanity Check[0]\nDEBUG: %p; %p; %p; %p", Image, Table, Table->BootServices, BS);
#endif
	InitializeLib(Image, Table);
    if(!ST){ST = Table;}
    if(!BS){BS = Table->BootServices;}
    if(!RT){RT = Table->RuntimeServices;}
#ifdef __DEBUG__
	Print(L"\nSanity Check[1]\nDEBUG: %p; %p; %p; %p", Image, Table, Table->BootServices, BS);
	EFI_STATUS S = ValidateImageHandle(Image);
	Print(L"\nImage Validity: %r (%u)\n", S, S);
	if(EFI_ERROR(S)){return EFI_ABORTED;}
#endif
	conf_fsroot *root = NULL;
	UINT32 mID;
	if(!EFI_ERROR(getDriveMediaID(Image, &mID))){
		if(!(root = fmount(Image, mID))){
			GPTeNSTR *str = makeGPTeNSTR("Root");
			formatpart(Image, mID, *str);
			FreePool(str);
			if(!(root = fmount(Image, mID))){
				Print(L"\nError! Couldnt Mount Drive %u\n", mID);
				return EFI_SUCCESS;
			}
		}
	}
	return EFI_ABORTED;
}