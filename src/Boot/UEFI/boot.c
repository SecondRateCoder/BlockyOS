#include "standard.h"

const EFI_PHYSICAL_ADDRESS DebugPort = 0x402;

void libinit(EFI_HANDLE Image, EFI_SYSTEM_TABLE *Table){
	DEBUGPRINT(L"\nIntitialising GNU-EFI");
	InitializeLib(Image, Table);
    if(!ST){ST = Table;}
    if(!BS){BS = Table->BootServices;}
    if(!RT){RT = Table->RuntimeServices;}
	sysbase(Image);
	DEBUGPRINT(L"\nPost GNU-EFI Init");
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE Image, EFI_SYSTEM_TABLE *Table){
	DEBUGPRINT(L"Sanity Check[0]\nDEBUG: %p; %p", Image, Table);
	DEBUGDO{DEBUGPRINT(L"\nGUID:");	prGUID((EFI_GUID)EFI_ZERO_GUID);}
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
			DEBUGPRINT(L"\nOpened FS Socket");
				fs = (socket_t *)tmp->data;
				Print(L"\nMounted FRaT Socket!");
			}else{
				__free(socket.data);
				DEBUGPRINT(L"\nFailed to open FS Socket");
			}
		}else{DEBUGPRINT(L"\nFailed to open Socket");}
		GPTeNSTR *str = makeGPTeNSTR(rootDesc.name);
		formatpart(rootDesc.guid, rootDesc.uGuid, *str, __FS_DEFAULTBLOCKSIZE, 5, 1, 0);
		__free(str);
		ntries--;
	}while(fs == NULL && ntries);
	if(fs == NULL){Print(L"\nFailed to retrieve FS Socket");}
	else{Print(L"\nGot FS Socket");}

	// Load the Executable
	char *path = KERNELEXE, *loadargs = "f";
	DEBUGPRINT(L"\n\nPath: %p:%a\nLoad-Args: %p:%a", path, path, loadargs, loadargs);
	socket_ret ret = socketfunc(fs->open)(fs, sizeof(char *) * 2, path, loadargs);
	if(flagcheck(ret.errout, __noerr)){
		DEBUGPRINT(L"\nGot Executable File");
		socket_t *exe = ret.data;
		void *loadin = __calloc(1, __fsize(((unhandle *)exe->persistent)->fhandle_));
		DEBUGPRINT(L"\nResolving to Memory");
		kernelmain main = (kernelmain)resolve(fs, (socket_t *)ret.data, NULL, 0, NULL);
		DEBUGPRINT(L"\nExiting Boot Servies?");
		uefi_call_wrapper(gBS->ExitBootServices, 2, Image, bootout->memory.mapKey);
		main(bootout);
	}else{DEBUGPRINT(L"\nError opening Executable");}
	return EFI_ABORTED;
}