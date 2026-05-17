#include "standard.h"

__efiDevNode *ProcessNode(EFI_DEVICE_PATH *Node, __efiDevNode *Parent){
#ifdef __DEBUG__
		Print(L"\nLoading Node");
#endif
	__efiDevNode *New = AllocatePool(sizeof(__efiDevNode));
	if(Parent){
		Parent->local__.children = ReallocatePool(
			(sizeof(__efiDevNode) * Parent->local__.nChildren),
			(sizeof(__efiDevNode) * (Parent->local__.nChildren + 1)),
			Parent->local__.children
		);
		Parent->local__.children[Parent->local__.nChildren] = New;
		Parent->local__.nChildren++;
	}
	*New = (__efiDevNode){
		.local = Node,
		.local__ = {
			.children = NULL,
			.nChildren = 0,
			.parent = Parent
		},
		.protocolData = {0},
		.nodeName = DescribeDeviceNode(Node)
	};
	CopyMem(New->protocolData, ((void *)Node) +  sizeof(EFI_DEVICE_PATH), DevicePathNodeLength(Node) - sizeof(EFI_DEVICE_PATH));
#ifdef __DEBUG__
	Print(L"    ??    [%s]", New->nodeName);
#endif
	return New;
}

__efiDevNode *BuildDeviceTree(EFI_DEVICE_PATH *Path){
	__efiDevNode *Root = NULL;
	__efiDevNode *Parent = NULL;
	EFI_DEVICE_PATH *Node = Path;
	while(!IsDevicePathEnd(Node)){
		if(DevicePathNodeLength(Node) == 0){
			Print(L"\n[WARNING] Zero-length device path node, aborting walk");
			break;
		}
		__efiDevNode *Current = ProcessNode(Node, Parent);
		if(!Root){Root = Current;}
		Parent = Current;
		Node = NextDevicePathNode(Node);
	}
	return Root;
}

void DebugDevicePath(EFI_DEVICE_PATH *ROOT){
	Print(L"\n    [  ");
	while(!IsDevicePathEnd(ROOT)){
		CHAR16 *C16 = DescribeDeviceNode(ROOT);
		Print(L"%s  ", C16);
		FreePool(C16);
		ROOT = NextDevicePathNode(ROOT);
	}
	Print(L"  ]");
}

CHAR16 *DescribeDeviceNode(EFI_DEVICE_PATH *Node){
	static UINT8 bufLEN = 128;
	CHAR16 *Buffer = AllocatePool(bufLEN);
	ZeroMem(Buffer, bufLEN);
	switch(Node->Type){
		case ACPI_DEVICE_PATH: {
			ACPI_HID_DEVICE_PATH *Acpi = (ACPI_HID_DEVICE_PATH*)Node;
			UnicodeSPrint(Buffer, bufLEN, L"ACPI(HID=%08x    UID=%08x)", Acpi->HID, Acpi->UID);
			break;
		} case HARDWARE_DEVICE_PATH: {
			switch (Node->SubType) {
				case HW_PCI_DP: {
					PCI_DEVICE_PATH *Pci = (PCI_DEVICE_PATH*)Node;
					UnicodeSPrint(Buffer, bufLEN, L"PCI(%d,%d)", Pci->Device, Pci->Function);
					break;
				} case HW_MEMMAP_DP: {
					MEMMAP_DEVICE_PATH *Mm = (MEMMAP_DEVICE_PATH*)Node;
					UnicodeSPrint(Buffer, bufLEN, L"MemMap(%lx-%lx)", Mm->StartingAddress, Mm->EndingAddress);
					break;
				} default: {StrnCpy(Buffer, L"Hardware(Unknown)", 17);}
			}
			break;
		} case MESSAGING_DEVICE_PATH: {
			switch(Node->SubType){
				case MSG_USB_DP: {
					USB_DEVICE_PATH *Usb = (USB_DEVICE_PATH*)Node;
					UnicodeSPrint(Buffer, bufLEN, L"USB(Port %d)", Usb->Port);
					break;
				} case MSG_SATA_DP: {
					SATA_DEVICE_PATH *Sata = (SATA_DEVICE_PATH*)Node;
					UnicodeSPrint(Buffer, bufLEN, L"SATA(Port %d    PM %d    Lun %d)",
								Sata->HBAPortNumber, Sata->PortMultiplierPortNumber, Sata->Lun);
					break;
				} case MSG_SCSI_DP: {
					SCSI_DEVICE_PATH *Scsi = (SCSI_DEVICE_PATH*)Node;
					UnicodeSPrint(Buffer, bufLEN, L"SCSI(Pun %d    Lun %d)", Scsi->Pun, Scsi->Lun);
					break;
				} default: {StrnCpy(Buffer, L"Messaging(Unknown)", 18);}
			}
			break;
		} case MEDIA_DEVICE_PATH: {
			switch(Node->SubType){
				case MEDIA_FILEPATH_DP: {
					FILEPATH_DEVICE_PATH *Fp = (FILEPATH_DEVICE_PATH*)Node;
					UnicodeSPrint(Buffer, bufLEN, L"FilePath(%s)", Fp->PathName);
					break;
				} case MEDIA_HARDDRIVE_DP: {
					HARDDRIVE_DEVICE_PATH *Hd = (HARDDRIVE_DEVICE_PATH*)Node;
					UnicodeSPrint(Buffer, bufLEN, L"HD(Part %d    Start %lx    End %lx)",
								Hd->PartitionNumber, Hd->PartitionStart, Hd->PartitionSize);
					break;
				} default: {StrnCpy(Buffer, L"Media(Unknown)", 14);}
			}
			break;
		} default:{StrnCpy(Buffer, L"Unknown(Unknown)", 16);}
	}
	return Buffer;
}

EFI_DEVICE_PATH *getDevPath(EFI_DEVICE_PATH *dPath, UINT32 dType, UINT32 sType){
	EFI_DEVICE_PATH *dPath__ = dPath;
	while(!IsDevicePathEnd(dPath__)){
		if(((dPath__->Type & dType) == dPath__->Type) && ((dPath__->SubType & sType) == dPath__->SubType)){return dPath__;}
		dPath__ = NextDevicePathNode(dPath__);
	}
	return NULL;
}

__efiDevNode **loadDNodes(UINTN *nNodes){
	EFI_GUID dPathGUID = EFI_DEVICE_PATH_PROTOCOL_GUID;
	EFI_HANDLE *handles = NULL;		UINTN nHandles = 0;
	__efiDevNode **dNodes = NULL;	(*nNodes) = 0;
#ifdef __DEBUG__
	Print(L"\nLoading Device Tree");
#endif
	if(!EFI_ERROR(uefi_call_wrapper(gBS->LocateHandleBuffer, 5, AllHandles, NULL, NULL, &nHandles, &handles))){
		dNodes = AllocatePool(sizeof(__efiDevNode *) * nHandles);
		EFI_STATUS status;
#ifdef __DEBUG__
		Print(L"\n");
#endif
		for(UINTN cc = 0; cc < nHandles; ++cc){
			EFI_DEVICE_PATH *dPath = NULL;
			status = uefi_call_wrapper(gBS->HandleProtocol, 3, handles[cc], &dPathGUID, (void **)&dPath);
			if(!EFI_ERROR(status)){
				// DebugDevicePath(dPath);
				dNodes[*nNodes] = BuildDeviceTree(dPath);
				if(dNodes[*nNodes]){
					Print(L"Call #%llu    \"%s\"\n", cc, dNodes[*nNodes]->nodeName);
					(*nNodes)++;
				}else{
#ifdef __DEBUG__
					Print(L"Call Error #%llu    ", cc);
#endif
				}
			}else{
#ifdef __DEBUG__
				Print(L"Call Error#%u: %llu    ", cc, status & ~((UINTN)0xF000000000000000));
#endif
			}
		}
	}else{
#ifdef __DEBUG__
		Print(L"\nError Getting Handles");
#endif
	}
	FreePool(handles);
	dNodes = ReallocatePool(sizeof(__efiDevNode *) * nHandles, sizeof(__efiDevNode *) * (*nNodes), dNodes);
#ifdef __DEBUG__
		Print(L"\nReturning Expanded Node Tree");
#endif
	return dNodes;
}