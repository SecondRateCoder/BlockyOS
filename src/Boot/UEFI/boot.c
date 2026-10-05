#include "standard.h"
#include "drivers/socket/socket.h"
#include "tools/tools.h"

#include "drivers/executable/pe.h"
#include "drivers/.disk/fs/frat.h"
#include "drivers/gif/gif.h"

void TestVideo(socket_t *disk, UINT32 N);

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
	if(!rt.data){
		DEBUGPRINT(L"\nError opening Disk: socket driver returned no result");
		Exit(EFI_ABORTED, 0, NULL);
	}
	socket_ret diskrt = *((socket_ret *)(rt.data));
	__free(rt.data);
	if(socketreterr(diskrt, sizeof(socket_t))){
		DEBUGPRINT(L"\nError opening Disk [%llu:%llu:%llu]", (UINT64)diskrt.errout, (UINT64)diskrt.nData, (UINT64)diskrt.data);
		Exit(EFI_ABORTED, 0, NULL);
	}

	socket_t *disk = diskrt.data;
	LoadedPeExecutable *KBOOT = LoadExecutable(disk, false, 200, "SYSD/kboot.exe");
	if(!KBOOT){DEBUGPRINT(L"\nError Loading Executable");		Exit(EFI_ABORTED, 0, NULL);}
	// TestVideo(disk, 32);
	
	__bootinfo *bootout = gatherbootinfo(Image);
	kernelmain KernelBoot = (kernelmain)KBOOT->EntryPoint;
	KernelBoot(bootout);
	DEBUGPRINT(L"\nFatal Error");
	Exit(EFI_ABORTED, 0, NULL);
}

void TestVideo(socket_t *disk, UINT32 N){
	EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;

	//	Query GOP
	EFI_STATUS status = uefi_call_wrapper(BS->LocateProtocol, 0, &gopGuid, NULL, (void**)&gop);
	if(EFI_ERROR(status)){Print(L"Unable to locate GOP");}
	socket_t *GIFFile = (socketfcall(disk, open, "SYSD/icon.gif", "f")).data;
	UINT32 **Frames = NULL, NFrames = 0, *Width, *Height;
	if(GIFFile){
		gifDescriptionSpace_t *GIF = OpenGIF(GIFFile);
		Frames = __calloc(GIF->nFrames, sizeof(UINT32 *));
		Height = __calloc(GIF->nFrames, sizeof(UINT32));
		Width = __calloc(GIF->nFrames, sizeof(UINT32));
		while((Frames[NFrames] = GetGIFFrame(GIF, NFrames, Width + NFrames, Height + NFrames, gop->Mode->Info->PixelFormat))){NFrames++;}
		socketfcall(GIFFile, close, 0);
		while(N--){
			for(UINT32 framecc = 0; framecc < NFrames; ++framecc){
				BltGIFFrame(Frames[framecc], Width[framecc], Height[framecc], 
					(void *)gop->Mode->FrameBufferBase, 0, 0, gop->Mode->Info->HorizontalResolution, 
					gop->Mode->Info->VerticalResolution, gop->Mode->Info->PixelsPerScanLine);}
		}
	}
}