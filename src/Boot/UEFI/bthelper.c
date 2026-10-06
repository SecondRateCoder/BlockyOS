#include "standard.h"
#include "tools/tools.h"

EFI_MEMORY_DESCRIPTOR *GetMemoryMap(UINTN *mapSize, UINTN *mapKey, UINTN *descSize, UINT32 *descVersion){
    DEBUGPRINT(L"\nGetting the Memory Map");
    EFI_STATUS status;
    EFI_MEMORY_DESCRIPTOR *map = NULL;
    *mapSize = 0;

    //	First call: Pass argument count 5 and NULL for buffer to query required size
    status = uefi_call_wrapper(gBS->GetMemoryMap, 0, mapSize, NULL, mapKey, descSize, descVersion);

    //	Add padding to handle descriptor splits caused by AllocatePool
    *mapSize += 8 * (*descSize);

    //	Allocate memory pool
    status = uefi_call_wrapper(gBS->AllocatePool, 0, EfiLoaderData, *mapSize, (void **)&map);
    if(EFI_ERROR(status)){
        DEBUGPRINT(L"\n%s Error", EfiStatusToString(status));
        return NULL;
    }

    //	Fetch memory map
    status = uefi_call_wrapper(gBS->GetMemoryMap, 0, mapSize, map, mapKey, descSize, descVersion);
    if(EFI_ERROR(status)){
        DEBUGPRINT(L"\n%s Error", EfiStatusToString(status));
        uefi_call_wrapper(gBS->FreePool, 1, map);
        return NULL;
    }

    UINTN numItems = *mapSize / *descSize;
    DEBUGPRINT(L"\n=== UEFI Memory Map (%llu entries) ===", numItems);
    DEBUGPRINT(L"\nMap Size: %llu, Descriptor Size: %llu, Map Key: %llu,\tVersion: %u", *mapSize, *descSize, *mapKey, numItems, *descVersion);

    //	Stride using byte pointer and descSize to prevent alignment drift
    UINT8 *mapBytePtr = (UINT8 *)map;
    for(UINTN cc = 0; cc < numItems; ++cc){
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)(mapBytePtr + (cc * (*descSize)));
        DEBUGPRINT(L"\n[%llu] %s:%u  Start: 0x%p  Pages: %llu  Size: %llu KB",
            cc, EfiMemoryTypeToStr(desc->Type), desc->Type, desc->PhysicalStart, 
			desc->NumberOfPages, desc->NumberOfPages * 4
        );
    }
    status = uefi_call_wrapper(gBS->GetMemoryMap, 0, mapSize, map, mapKey, descSize, descVersion);
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

EFI_GRAPHICS_OUTPUT_MODE_INFORMATION InitialiseVideoMemory(void **VideoMemory, UINT64 *PixelSize, UINT64 *Width, UINT64 *Height){
	EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
	UINTN SizeOfInfo, numModes;

	//	Query GOP
	EFI_STATUS status = uefi_call_wrapper(BS->LocateProtocol, 0, &gopGuid, NULL, (void**)&gop);
	if(EFI_ERROR(status)){Print(L"Unable to locate GOP");}

	//	Get the current Mode
	status = uefi_call_wrapper(gop->QueryMode, 0, gop, gop->Mode == NULL? 0: gop->Mode->Mode, &SizeOfInfo, &info);
	// this is needed to get the current video mode
	if(status == EFI_NOT_STARTED){status = uefi_call_wrapper(gop->SetMode, 0, gop, 0);}
	if(EFI_ERROR(status)){Print(L"Unable to get native mode");}else{numModes = gop->Mode->MaxMode;}

	//	Query all available Modes and set the Current Mode to that which is Largest and of the RGB Mode.
	for(UINT32 i = 0; i < numModes; i++){
		status = uefi_call_wrapper(gop->QueryMode, 0, gop, i, &SizeOfInfo, &info);
		if((info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor) && 
			((gop->Mode->Info->HorizontalResolution < info->HorizontalResolution) && (gop->Mode->Info->VerticalResolution < info->VerticalResolution))
		){status = uefi_call_wrapper(gop->SetMode, 3, gop, i);}
	}

	*VideoMemory = (void *)gop->Mode->FrameBufferBase;
	char *Fmt = NULL;
	switch(gop->Mode->Info->PixelFormat){
		case PixelRedGreenBlueReserved8BitPerColor:	{Fmt = "R:G:B:Reserved";break;}
		case PixelBlueGreenRedReserved8BitPerColor:	{Fmt = "B:G:R:Reserved";break;}
		case PixelBitMask:							{Fmt = "BitMask";		break;}
		case PixelBltOnly:							{Fmt = "BltOnly";		break;}
		default:
		case PixelFormatMax:						{Fmt = "Unknown";		break;}
	}
	Print(L"\nwidth %u height %u format %u[%a]", (UINT32)gop->Mode->Info->HorizontalResolution, 
		(UINT32)gop->Mode->Info->VerticalResolution, (UINT32)gop->Mode->Info->PixelFormat, Fmt);
	*Width = gop->Mode->Info->HorizontalResolution;
	*Height = gop->Mode->Info->VerticalResolution;
	*PixelSize = sizeof(UINT32);

	return *(gop->Mode->Info);
}

__bootinfo *gatherbootinfo(EFI_HANDLE Image){
	__bootinfo *out = __calloc(1, sizeof(__bootinfo));
	StrnCpy(out->bootentry.BootEntryName, BOOTOPTION16, __min(sizeof(BOOTOPTION16) / sizeof(CHAR16), sizeof(out->bootentry.BootEntryName)));

	*out = (__bootinfo){
		.Stack = {
			.Stack = NULL, 
			.StackSize = EFI_PAGE_SIZE * EFI_PAGE_SIZE
		}, .devices = {
			.devices = NULL, // loadDNodes(&out->devices.nnodes), .CTableLength = ST->NumberOfTableEntries, 
			.CTable = __memdup(ST->ConfigurationTable, sizeof(EFI_CONFIGURATION_TABLE) * ST->NumberOfTableEntries)
		}, .memory = {0}, .Video = {0}, 
		//	Temporary disable in order to Speed up Bochs.
		.bootentry.BootEntryCode = 0//	CreateBootEntry(&rootDesc.guid, &rootDesc.uGuid, (CHAR16 *)out->bootentry.BootEntryName), 
	};
	out->Video.CurrentVideoMode = InitialiseVideoMemory(&(out->Video.videomemory), &(out->Video.PixelSize), &(out->Video.PixelWidth), &(out->Video.PixelHeight));
	DEBUGPRINT(L"\nVideo Memory: %p\tPixel Size: %llu\tWidth: %llu\tHeight: %llu", out->Video.videomemory, out->Video.PixelSize, out->Video.PixelWidth, out->Video.PixelHeight);
	out->memory.MemoryDescriptors = GetMemoryMap(&(out->memory.MemoryDescriptorBufferSize), &(out->memory.MemoryDescriptorMapKey), 
		&(out->memory.MemoryDescriptorStructSize), &(out->memory.MDescriptorsVersion));
	DEBUGPRINT(L"\n\n\n");

	EFI_STATUS St = uefi_call_wrapper(gBS->AllocatePages, 0, AllocateAnyPages, 
		EfiRuntimeServicesData, out->Stack.StackSize / EFI_PAGE_SIZE, &(out->Stack.Stack));
	
	St = uefi_call_wrapper(gBS->ExitBootServices, 2, Image, out->memory.MemoryDescriptorMapKey);
	if(EFI_ERROR(St)){DEBUGPRINT(L"\nFailed to exit Boot Services.\t[%a:%llu]", EfiStatusToString(St), (UINT64)St);}

	for(UINTN cc = 0 ; cc < (out->memory.MemoryDescriptorBufferSize / out->memory.MemoryDescriptorStructSize); ++cc){
		EFI_MEMORY_DESCRIPTOR *emd = (void *)out->memory.MemoryDescriptors + (cc * out->memory.MemoryDescriptorStructSize);
		if(emd->PhysicalStart > out->memory.TotalMemorySize){
			out->memory.TotalMemorySize = emd->PhysicalStart + (emd->NumberOfPages * EFI_PAGE_SIZE);}
	}
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

		Status = uefi_call_wrapper(RT->GetVariable, 0, BootVar, BootGuid, NULL, &ExistingSize, NULL);

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
		Status = uefi_call_wrapper(RT->GetVariable, 0, BootVar, BootGuid, NULL, &ExistingSize, NULL);
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
	Status = uefi_call_wrapper(RT->SetVariable, 0, BootVar, BootGuid,
		EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
		TotalSize, Buffer
	);
	FreePool(Buffer);
	if(EFI_ERROR(Status)){return 0;}
	return 2;
}

const char *EfiStatusToString(EFI_STATUS Status){
    switch (Status) {
        case EFI_SUCCESS:				return "EFI_SUCCESS";
        case EFI_LOAD_ERROR:			return "EFI_LOAD_ERROR";
        case EFI_INVALID_PARAMETER:		return "EFI_INVALID_PARAMETER";
        case EFI_UNSUPPORTED:			return "EFI_UNSUPPORTED";
        case EFI_BAD_BUFFER_SIZE:		return "EFI_BAD_BUFFER_SIZE";
        case EFI_BUFFER_TOO_SMALL:		return "EFI_BUFFER_TOO_SMALL";
        case EFI_NOT_READY:				return "EFI_NOT_READY";
        case EFI_DEVICE_ERROR:			return "EFI_DEVICE_ERROR";
        case EFI_WRITE_PROTECTED:		return "EFI_WRITE_PROTECTED";
        case EFI_OUT_OF_RESOURCES:		return "EFI_OUT_OF_RESOURCES";
        case EFI_VOLUME_CORRUPTED:		return "EFI_VOLUME_CORRUPTED";
        case EFI_VOLUME_FULL:			return "EFI_VOLUME_FULL";
        case EFI_NO_MEDIA:				return "EFI_NO_MEDIA";
        case EFI_MEDIA_CHANGED:			return "EFI_MEDIA_CHANGED";
        case EFI_NOT_FOUND:				return "EFI_NOT_FOUND";
        case EFI_ACCESS_DENIED:			return "EFI_ACCESS_DENIED";
        case EFI_NO_RESPONSE:			return "EFI_NO_RESPONSE";
        case EFI_NO_MAPPING:			return "EFI_NO_MAPPING";
        case EFI_TIMEOUT:				return "EFI_TIMEOUT";
        case EFI_NOT_STARTED:			return "EFI_NOT_STARTED";
        case EFI_ALREADY_STARTED:		return "EFI_ALREADY_STARTED";
        case EFI_ABORTED:				return "EFI_ABORTED";
        case EFI_SECURITY_VIOLATION:	return "EFI_SECURITY_VIOLATION";
        case EFI_CRC_ERROR:				return "EFI_CRC_ERROR";
        case EFI_END_OF_FILE:			return "EFI_END_OF_FILE";
        default:						return (EFI_ERROR(Status)) ? "EFI_UNKNOWN_ERROR" : "EFI_UNKNOWN_WARNING";
    }
}