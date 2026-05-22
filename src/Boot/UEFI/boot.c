#include "standard.h"

const EFI_PHYSICAL_ADDRESS DebugPort = 0x402;

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
	UINT32 ntries = 3;
	__bootinfo *bootout = gatherbootinfo();
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
	if(fs == NULL){Print(L"\nFailed to retrieve FS Socket");}
	else{Print(L"\nGot FS Socket");}

	// Load the Executable
#ifdef __DEBUG__
	Print(L"\nfs->open ptr = %p", fs->open);
	UINT8 *p = (UINT8 *)fs->open;
	// BUFDEFPRINT(p, 64, cc);
#endif
	socket_ret ret = socketfunc(fs->open)(fs, sizeof(char *) * 2, KERNELEXE, "f");
	if(flagcheck(ret.errout, __noerr)){
		socket_t *exe = ret.data;
		meta_fsblock *metadata = _freadinfo((fhandle *)exe->persistent);
		void *loadin = __calloc(1, __fsize((fhandle *)exe->persistent));
		kernelmain main = (kernelmain)__resolve((socket_t *)ret.data, loadin);
		uefi_call_wrapper(gBS->ExitBootServices, 2, Image, bootout->memory.mapKey);
		main(bootout);
	}

#ifdef __DEBUG__
	Print(L"\nRetrieved Expanded Node Tree");
#endif
	
	return EFI_ABORTED;
}