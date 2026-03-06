#include "efi.h"
#include "efilib.h"


EFI_STATUS EFIAPI main(EFI_HANDLE handle, EFI_SYSTEM_TABLE *sysTable){
	InitializeLib(handle, sysTable);
	EFI_STATUS s = uefi_call_wrapper(
		sysTable->ConOut->OutputString, 
		2, sysTable->ConOut,
		L"Hello World!\n");
	return EFI_SUCCESS;
}
