#include "standard.h"
#include "drivers/socket/socket.h"
#include "tools/tools.h"
#include "drivers/executable/pe.h"
#include "drivers/.disk/fs/frat.h"

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
	// DEBUGPRINT(L"Sanity Check[0]\nDEBUG: %p; %p", Image, Table);
	// DEBUGDO{DEBUGPRINT(L"\nGUID:");	prGUID((EFI_GUID)EFI_ZERO_GUID);}

	// Initialise GNU-EFI
	libinit(Image, Table);

	// Initialise Hardware Device Tree
	__bootinfo *bootout = gatherbootinfo();
	// Open FrAT Socket
	socket_ret rt = socketopen(0, sizeof(UINT32) + (sizeof(EFI_GUID) * 2), (UINT32)0, rootDesc.guid, rootDesc.uGuid);
	socket_ret diskrt = *((socket_ret *)(rt.data));
	
	if(socketreterr(diskrt, sizeof(socket_t))){
		DEBUGPRINT(L"\nError opening Disk [%llu:%llu:%llu:%llu]", (UINT64)diskrt.errout, (UINT64)diskrt.nData, (UINT64)diskrt.data, (UINT64)((conf_fsroot *)((socket_t *)diskrt.data))->clusterbuffer.nClusterSectors);
		Exit(EFI_ABORTED, 0, NULL);
	}else{__free(rt.data);}
	
	socket_t *disk = diskrt.data;
	LoadedPeExecutable *KBOOT = LoadExecutable(disk, true, "SYSD/kboot.exe");
	if(KBOOT){DEBUGPRINT(L"[%a]:\t%llu\t%llu", KBOOT->Name, (UINT64)KBOOT->NDependencies, (UINT64)KBOOT->NSections);
	}else{DEBUGPRINT(L"\nError Loading Executable");	Exit(EFI_ABORTED, 0, NULL);}
	return EFI_SUCCESS;
}