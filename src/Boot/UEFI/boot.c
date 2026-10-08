#include "standard.h"
#include "drivers/socket/socket.h"
#include "tools/tools.h"

#include "drivers/executable/pe.h"
#include "drivers/.disk/fs/frat.h"
#include "drivers/gif/gif.h"

// void TestVideo(gifDescriptionSpace_t *GIF, void *FB, EFI_GRAPHICS_PIXEL_FORMAT PixelFormat, UINT32 N, UINT32 x, UINT32 y, UINT32 w){
// 	if(GIF && GIF->nFrames > 0 && FB && N){
// 		UINT32 *Frame = NULL, Width = 0, Height = 0;
// 		while(N-- && (Frame = GIF->frames[N % GIF->nFrames].fb)){BltFrame(Frame, Width, Height, FB, x, y, w);}
// 	}
// }

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

	// Initialise GNU-EFI
	libinit(Image, Table);
	
	// Open FrAT Socket
	socket_ret rt = socketopen(0, sizeof(UINT32) + (sizeof(EFI_GUID) * 2), (UINT32)0, rootDesc.guid, rootDesc.uGuid);
	socket_ret diskrt = *((socket_ret *)(rt.data));
	if(socketreterr(diskrt, sizeof(socket_t))){
		DEBUGPRINT(L"\nError opening Disk [%llu:%llu:%p\t:\t%llu:%llu:%p]", 
			(UINT64)rt.errout, (UINT64)rt.nData, (UINT64)rt.data, (UINT64)diskrt.errout, (UINT64)diskrt.nData, (UINT64)diskrt.data);
		Exit(EFI_ABORTED, 0, NULL);
	}
	__free(rt.data);
	
	socket_t *disk = diskrt.data;
	
	
	
	
	LoadedPeExecutable *KBOOT = LoadExecutable(disk, FALSE, 32, "SYSD/kboot.exe");
	if(!KBOOT){DEBUGPRINT(L"\nError Loading Executable");		Exit(EFI_ABORTED, 0, NULL);}
	
	EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	UINTN SizeOfInfo;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
	EFI_STATUS status = uefi_call_wrapper(BS->LocateProtocol, 0, &gopGuid, NULL, (void**)&gop);
	status = uefi_call_wrapper(gop->QueryMode, 0, gop, (gop->Mode == NULL? 0: gop->Mode->Mode), &SizeOfInfo, &info);
	if(status == EFI_NOT_STARTED){status = uefi_call_wrapper(gop->SetMode, 0, gop, 0);}
	rt = socketfcall(disk, open, "SYSD/icon.gif", "f");
	socket_t *gif = rt.data;
	gifDescriptionSpace_t *GIF = OpenGIF(gif, gop->Mode->Info->PixelFormat);
	DEBUGPRINT(L"\nPrinting GIF");
	
	GifShow(GIF, gop);
	socketfcall(gif, close, 0);

	__bootinfo *bootout = gatherbootinfo(Image);
	// bochs_breakpoint();
	

	kernelmain KernelBoot = (kernelmain)KBOOT->EntryPoint;
	KernelBoot(bootout);
	DEBUGPRINT(L"\nFatal Error");
	Exit(EFI_ABORTED, 0, NULL);
}
