#include "standard.h"

void libinit(EFI_HANDLE Image, EFI_SYSTEM_TABLE *Table){
#ifdef __DEBUG__
	Print(L"\nIntitialising GNU-EFI");
#endif
	InitializeLib(Image, Table);
    if(!ST){ST = Table;}
    if(!BS){BS = Table->BootServices;}
    if(!RT){RT = Table->RuntimeServices;}
#ifdef __DEBUG__
	Print(L"\nPost GNU-EFI Init");
#endif
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE Image, EFI_SYSTEM_TABLE *Table){
#ifdef __DEBUG__
	Print(L"Sanity Check[0]\nDEBUG: %p; %p", Image, Table);
	Print(L"\nGUID:");	prGUID((EFI_GUID)EFI_ZERO_GUID);
#endif
	// Initialise GNU-EFI
	libinit(Image, Table);
	// Initialise Hardware Device Tree
	UINTN nNodes = 0;	UINT32 ntries = 3;
	__efiDevNode **dNodes = loadDNodes(&nNodes);
	// Open FrAT Socket
	socket_t *fs = NULL;
	do{
		socket_ret socket = socketopen(0, sizeof(UINT32) + (sizeof(EFI_GUID) * 2), (UINT32)0, rootDesc.guid, rootDesc.uGuid);
		if(socket.errout == __noerr){
			// We have a Mounted FRaT Socket
			socket_ret *tmp = (socket_ret *)socket.data;
			if(tmp->errout == __noerr){
#ifdef __DEBUG__
				Print(L"\nOpened FS Socket");
#endif
				fs = (socket_t *)tmp->data;
				Print(L"\nMounted FRaT Socket!");
			}else{
				__free(socket.data);
#ifdef __DEBUG__
				Print(L"\nFailed to open FS Socket");
#endif
			}
		}else{
#ifdef __DEBUG__
			Print(L"\nFailed to open Socket");
#endif
		}
		GPTeNSTR *str = makeGPTeNSTR(rootDesc.name);
		formatpart(rootDesc.guid, rootDesc.uGuid, *str, __FS_DEFAULTBLOCKSIZE, 5, 1, 0);
		__free(str);
		ntries--;
	}while(fs == NULL && ntries);
	if(fs == NULL){Print(L"Failed to retrieve FS Socket");}
#ifdef __DEBUG__
	Print(L"\nRetrieved Expanded Node Tree");
#endif
	
	return EFI_SUCCESS;
}