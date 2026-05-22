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
		__free(C16);
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
	__efiDevNode **dnodes = NULL;	(*nNodes) = 0;
#ifdef __DEBUG__
	Print(L"\nLoading Device Tree");
#endif
	if(!EFI_ERROR(uefi_call_wrapper(gBS->LocateHandleBuffer, 5, AllHandles, NULL, NULL, &nHandles, &handles))){
		dnodes = AllocatePool(sizeof(__efiDevNode *) * nHandles);
		EFI_STATUS status;
#ifdef __DEBUG__
		Print(L"\n");
#endif
		for(UINTN cc = 0; cc < nHandles; ++cc){
			EFI_DEVICE_PATH *dPath = NULL;
			status = uefi_call_wrapper(gBS->HandleProtocol, 3, handles[cc], &dPathGUID, (void **)&dPath);
			if(!EFI_ERROR(status)){
				// DebugDevicePath(dPath);
				dnodes[*nNodes] = BuildDeviceTree(dPath);
				if(dnodes[*nNodes]){
					Print(L"Call #%llu    \"%s\"\n", cc, dnodes[*nNodes]->nodeName);
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
	__free(handles);
	dnodes = ReallocatePool(sizeof(__efiDevNode *) * nHandles, sizeof(__efiDevNode *) * (*nNodes), dnodes);
#ifdef __DEBUG__
		Print(L"\nReturning Expanded Node Tree");
#endif
	return dnodes;
}

EFI_MEMORY_DESCRIPTOR *GetMemoryMap(UINTN *mapSize, UINTN *mapKey, UINTN *descSize, UINT32 *descVersion){
#ifdef __DEBUG__
	Print(L"\nGetting the Memory Map");
#endif
	EFI_STATUS status;
	EFI_MEMORY_DESCRIPTOR *map = NULL;
	*mapSize = 0;

	// First call: get required size
	status = uefi_call_wrapper(gBS->GetMemoryMap, 5, mapSize, map, mapKey, descSize, descVersion);
	if(status != EFI_BUFFER_TOO_SMALL){return NULL;}
	
	// Allocate extra space (UEFI spec recommends padding)
	*mapSize += 2 * (*descSize);
	status = uefi_call_wrapper(gBS->AllocatePool, 3, EfiLoaderData, *mapSize, (void **)&map);
	if(EFI_ERROR(status)){return NULL;}

	// Second call: actual memory map
	status = uefi_call_wrapper(gBS->GetMemoryMap, 5, mapSize, map, mapKey, descSize, descVersion);
	if(EFI_ERROR(status)){
		gBS->FreePool(map);
		return NULL;
	}
#ifdef __DEBUG__
	Print(L"=== UEFI Memory Map (%u entries) ===\n", (*mapSize) / (*descSize));
	Print(L"Descriptor Size: %llu, #n Items: %llu  Version: %u\n\n", *mapSize, (*mapSize) / (*descSize), *descVersion);
	for(UINTN cc = 0; cc < ((*mapSize) / (*descSize)); ++cc){
		Print(
			L"[%llu] %s  Start: 0x%lx  Pages: %llu  Size: %llu KB\n",
			cc,
			EfiMemoryTypeToStr(map[cc].Type),
			map[cc].PhysicalStart,
			map[cc].NumberOfPages,
			map[cc].NumberOfPages * 4
		);
	}
#endif
	return map;
}

static CHAR16 *EfiMemoryTypeToStr(UINT32 type){
	switch(type){
		case EfiReservedMemoryType:      return L"EfiReservedMemoryType";
		case EfiLoaderCode:              return L"EfiLoaderCode";
		case EfiLoaderData:              return L"EfiLoaderData";
		case EfiBootServicesCode:        return L"EfiBootServicesCode";
		case EfiBootServicesData:        return L"EfiBootServicesData";
		case EfiRuntimeServicesCode:     return L"EfiRuntimeServicesCode";
		case EfiRuntimeServicesData:     return L"EfiRuntimeServicesData";
		case EfiConventionalMemory:      return L"EfiConventionalMemory";
		case EfiUnusableMemory:          return L"EfiUnusableMemory";
		case EfiACPIReclaimMemory:       return L"EfiACPIReclaimMemory";
		case EfiACPIMemoryNVS:           return L"EfiACPIMemoryNVS";
		case EfiMemoryMappedIO:          return L"EfiMemoryMappedIO";
		case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
		case EfiPalCode:                 return L"EfiPalCode";
		case EfiPersistentMemory:        return L"EfiPersistentMemory";
		default:                         return L"Unknown";
	}
}

__bootinfo *gatherbootinfo(){
	__bootinfo *out = __calloc(1, sizeof(__bootinfo));
	out->devices.devices = loadDNodes(&out->devices.nnodes);
	out->memory.desc = GetMemoryMap(&out->memory.descSize, &out->memory.mapKey, &out->memory.descItemSize, &out->memory.descVersion);
	out->memory.nDescs = out->memory.descSize / out->memory.descItemSize;
	__memset(out->bootentry.BootEntryName, 0, sizeof(out->bootentry.BootEntryName));
	out->bootentry.BootEntryCode = CreateBootEntry(&rootDesc.guid, &rootDesc.uGuid, (CHAR16 *)out->bootentry.BootEntryName);
	if(out->bootentry.BootEntryCode != 1){RestartSystem();}
	return out;
}

// Return codes:
// 0 = Error
// 1 = Already Exists
// 2 = Added
UINT8 CreateBootEntry(EFI_GUID *BootGuid, EFI_GUID *AltGuid, CHAR16 *OutBootVarName){
	EFI_STATUS Status;
	UINTN BootIndex = 0;
	CHAR16 BootVar[12];
	UINT8 *Existing = NULL;
	UINTN ExistingSize = 0;

	// 1. Scan Boot0000 → BootFFFF for existing entry
	for (BootIndex = 0; BootIndex < 0xFFFF; BootIndex++){
		SPrint(BootVar, sizeof(BootVar), L"Boot%04X", BootIndex);

		ExistingSize = 0;
		Existing = NULL;

		Status = uefi_call_wrapper(
			RT->GetVariable, 5, BootVar, BootGuid,
			NULL, &ExistingSize, NULL
		);

		if(Status == EFI_BUFFER_TOO_SMALL){
			// Entry exists → return "already exists"
			StrCpy(OutBootVarName, BootVar);
			return 1;
		}
	}

	// Find first free Boot#### index
	for(BootIndex = 0; BootIndex < 0xFFFF; BootIndex++){
		SPrint(BootVar, sizeof(BootVar), L"Boot%04X", BootIndex);
		ExistingSize = 0;
		Status = uefi_call_wrapper(
			RT->GetVariable, 5, BootVar, BootGuid,
			NULL, &ExistingSize, NULL
		);
		if(Status == EFI_NOT_FOUND){break;}
	}
	if(BootIndex >= 0xFFFF){return 0;}
	StrCpy(OutBootVarName, BootVar);

	// Build the EFI_LOAD_OPTION buffer manually
	CHAR16 Description[] = BOOTDESC16;
	UINTN DescLen = (StrLen(Description) + 1) * sizeof(CHAR16);

	// Build a simple FilePath: a vendor device path with AltGuid
	struct{
		EFI_DEVICE_PATH_PROTOCOL Header;
		EFI_GUID Guid;
		UINT8 EndNode[4];
	}__attribute__((packed)) DevPath = {
		.Header = { 0x04, 0x0C, sizeof(EFI_DEVICE_PATH_PROTOCOL) + sizeof(EFI_GUID), 0 },
		.Guid = *AltGuid,
		.EndNode = { 0x7F, 0xFF, 0x04, 0x00 }
	};

	UINT16 FilePathLen = sizeof(DevPath);
	UINTN TotalSize = sizeof(MY_LOAD_OPTION) + DescLen + FilePathLen;
	UINT8 *Buffer = AllocatePool(TotalSize);
	if(!Buffer){return 0;}

	MY_LOAD_OPTION *Opt = (MY_LOAD_OPTION*)Buffer;
	Opt->Attributes = 0x00000001; // ACTIVE
	Opt->FilePathListLength = FilePathLen;

	UINT8 *Ptr = Buffer + sizeof(MY_LOAD_OPTION);

	// Copy Description
	__memcpy(Ptr, Description, DescLen);
	Ptr += DescLen;

	// Copy Device Path
	__memcpy(Ptr, &DevPath, FilePathLen);

	// Write Boot#### variable
	Status = uefi_call_wrapper(
		RT->SetVariable, 5, BootVar, BootGuid,
		EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
		TotalSize, Buffer
	);
	FreePool(Buffer);
	if(EFI_ERROR(Status)){return 0;}
	return 2;
}