#include "standard.h"

EFI_STATUS EFIAPI efi_main(EFI_HANDLE Image, EFI_SYSTEM_TABLE *Table){
	InitializeLib(Image, Table);
	Print(L"Hi");
	return EFI_SUCCESS;
}