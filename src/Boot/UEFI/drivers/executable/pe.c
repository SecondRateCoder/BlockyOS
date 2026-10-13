#include "pe.h"

void *ReadPeExecutableHeader(socket_t *sck){
	if(!sck){return NULL;}
	UINT32 Offset = 0, Bytes = 0;
	socket_ret rt = {0};
	
	//	Read off the Header Offset.
	rt = socketfcall(sck, read, PeHeaderOffsetAddress, sizeof(Offset));
	if(socketreterr(rt, sizeof(Offset))){return NULL;}else{Offset = *((UINT32 *)rt.data);		__free(rt.data);}

	// Read the COFF header so the complete image-header size can be calculated.
	rt = socketfcall(sck, read, Offset, sizeof(PeHeader));
	if(socketreterr(rt, sizeof(PeHeader))){return NULL;}
	PeHeader *peHeader = rt.data;
	UINT16 nSections = peHeader->mNumberOfSections;
	UINT16 optionalHeaderSize = peHeader->mSizeOfOptionalHeader;
	__free(rt.data);

	//	Read off the Optional Header Magic.
	rt = socketfcall(sck, read, Offset + sizeof(PeHeader) + __offsetof(Pe32OptionalHeader, mMagic), sizeof(PeOptionalHeaderType));
	if(socketreterr(rt, sizeof(PeOptionalHeaderType))){return NULL;}
	switch(*((PeOptionalHeaderType *)rt.data)){
		case Pe32: {
			__free(rt.data);
			Bytes = sizeof(PeHeader) + optionalHeaderSize +
				nSections * sizeof(PeImageSectionHeader);
			return (socketfcall(sck, read, Offset, Bytes)).data;
		} case Pe32Plus: {
			__free(rt.data);
			Bytes = sizeof(PeHeader) + optionalHeaderSize +
				nSections * sizeof(PeImageSectionHeader);
			return (socketfcall(sck, read, Offset, Bytes)).data;
		}
	}
}

UINT32 RvaToFileOffsetPe(UINT32 rva, PeImageSectionHeader *Section){
	UINT32 startRVA = Section->mVirtualAddress;
	UINT32 endRVA = startRVA + Section->mVirtualSize;
	// Does the RVA live inside this section's virtual memory block?
	if(rva >= startRVA && rva < endRVA){
		// Check if the section actually contains data on disk
		if(Section->mPointerToRawData == 0){return 0;}
		return (rva - startRVA) + Section->mPointerToRawData;
	}
    return 0;
}

PeImageSectionHeader *FindSectionPe(void *header, char name[8]){
	DecodePeExecutableHeader(header);
	for(UINT32 cc = 0; cc < PH->mNumberOfSections; ++cc){
		if(!strncmpa(PISHs[cc].mName, name, 8)){return (PISHs + cc);}}
	return NULL;
}

void *GetAtRVAFromSectionDataPe(UINT32 RVA, char name[8], void *data, void *header){
    PeImageSectionHeader *Section = FindSectionPe(header, name);
    if(Section){
		if(RVA < Section->mVirtualAddress){return NULL;}
		UINT32 sectionOffset = RVA - Section->mVirtualAddress;
		if(sectionOffset >= Section->mSizeOfRawData){return NULL;}
		return (UINT8 *)data + sectionOffset;
	}else{return NULL;}
}

void *ReadAtRVAFromSectionPe(socket_t *sck, UINT32 RVA, UINT32 Size, char name[8], void *header){
	PeImageSectionHeader *Section = FindSectionPe(header, name);
	socket_ret rt = {0};
	if(Section){
		UINT32 Address = RvaToFileOffsetPe(RVA, Section);
		if(Address){
			void *out = NULL;
			rt = socketfcall(sck, read, Address, sizeof(char) * Size);
			if(!socketreterr(rt, sizeof(char) * Size)){return rt.data;}
		}
	}
	return NULL;
}

void *ReadSectionPe(socket_t *sck, void *header, char name[8]){
	DecodePeExecutableHeader(header);
	PeImageSectionHeader *section = FindSectionPe(header, name);
	if(section){
		UINT32 Address = section->mPointerToRawData;
		if(section->mSizeOfRawData){
			socket_ret rt = socketfcall(sck, read, Address, section->mSizeOfRawData);
			if(!socketreterr(rt, section->mSizeOfRawData)){return rt.data;}else{return NULL;}
		}else{return NULL;}
	}
	return NULL;
}

void *LoadSectionPe(socket_t *sck, void *header, char name[8]){
	DecodePeExecutableHeader(header);
	PeImageSectionHeader *section = FindSectionPe(header, name);
	if(section){
		UINT32 Address = section->mPointerToRawData;
		if(section->mVirtualSize){
			socket_ret rt = socketfcall(sck, read, Address, section->mSizeOfRawData);
			if(!socketreterr(rt, section->mSizeOfRawData)){
				void *Tmp = NULL;
				uefi_call_wrapper(gBS->AllocatePages, 0, AllocateAnyPages, EfiRuntimeServicesData, 
					__roundup(section->mVirtualSize, EFI_PAGE_SIZE) / EFI_PAGE_SIZE, &Tmp);
				__memset(Tmp, 0, __roundup(section->mVirtualSize, EFI_PAGE_SIZE));
				__memcpy(Tmp, rt.data, section->mSizeOfRawData);
				__free(rt.data);
				return Tmp;
			}else{return NULL;}
		}else{return NULL;}
	}
	return NULL;
}

ExpandedPeExecutable *ExpandPeExecutableFormat(socket_t *sck){
	void *header = ReadPeExecutableHeader(sck);
	DecodePeExecutableHeader(header);

	ExpandedPeExecutable *out = __calloc(sizeof(ExpandedPeExecutable), 1);

	PeExportDirectoryEntry *exports = ReadSectionPe(sck, header, PeExportSection);
	// These are RVA Pointers to Various Names in the Export Name Table.
	UINT32 *NamePointerRVAs = exports? GetAtRVAFromSectionDataPe(exports->mNamePointerRVA, PeExportSection, exports, header): NULL;
	//	Indices of Various Addresses Exported in the Export Directory Table.
	//	Subtract the Ordinal Base from the Digit.
	UINT16 *OrdinalPointerRVAs = exports? GetAtRVAFromSectionDataPe(exports->OrdinalPointerRVA, PeExportSection, exports, header): NULL;
	// if(exports){for(UINT32 cc = 0; cc < exports->mNNamePointers; ++cc){OrdinalPointerRVAs[cc] -= exports->mOrdinalBase;}}
	PeExportAddressEntry *exportAddressTable = exports? GetAtRVAFromSectionDataPe(exports->mExportTableRVA, PeExportSection, exports, header): NULL;

	PeImportDirectoryEntry *imports = ReadSectionPe(sck, header, PeImportSection);
	UINT32 nImports = 0;
	if(imports){while(__memcmp(imports + nImports, (UINT8[sizeof(PeImportDirectoryEntry)]){0}, sizeof(PeImportDirectoryEntry))){++nImports;}}
	UINT32 *nEntries = __calloc(nImports, sizeof(UINT32));
	PeImportLookupEntry32 **ImportLookups32 = NULL;
	if(imports){ImportLookups32 = __calloc(POH->mMagic == Pe32? sizeof(PeImportLookupEntry32 *): sizeof(PeImportLookupEntry64 *), nImports);}
	PeImportLookupEntry64 **ImportLookups64 = (PeImportLookupEntry64 **)ImportLookups32;
	if(imports){
		for(UINT32 cc = 0; cc < nImports; ++cc){
			UINT32 lookupRVA = imports[cc].ImportLookupTableRVA ?
				imports[cc].ImportLookupTableRVA : imports[cc].ImportAddressTableRVA;
			if(POH->mMagic == Pe32){
				ImportLookups32[cc] = GetAtRVAFromSectionDataPe(lookupRVA, PeImportSection, imports, header);
			}else{ImportLookups64[cc] = GetAtRVAFromSectionDataPe(lookupRVA, PeImportSection, imports, header);}
			if(!ImportLookups32[cc]){continue;}
			UINT32 cc_ = 0;
			if(POH->mMagic == Pe32){
				while(__memcmp(ImportLookups32[cc] + cc_, (UINT8[sizeof(PeImportLookupEntry32)]){0}, sizeof(PeImportLookupEntry32))){++cc_;}
			}else{while(__memcmp(ImportLookups64[cc] + cc_, (UINT8[sizeof(PeImportLookupEntry64)]){0}, sizeof(PeImportLookupEntry64))){++cc_;}}
			nEntries[cc] = cc_;
		}
	}

	//	.pdata Info
	void *pdataBlock = ReadSectionPe(sck, header, PeExceptionInfoSection);

	//	.rsrc Info
	void *rsrcBlock = ReadSectionPe(sck, header, PeResourceSection);
	PeResourceRootDirectory *rsrcRoot = (PeResourceRootDirectory *)rsrcBlock;
	PeResourceDirectoryEntry *rsrcEntries = rsrcBlock + sizeof(PeResourceRootDirectory);
	

	// .reloc Info
	void *relocblock = ReadSectionPe(sck, header, PeRelocDataSection);
	PeImageSectionHeader *relocHeader = relocblock? FindSectionPe(header, PeRelocDataSection): NULL;
	UINT32 nRelocationBlocks = 0, byteOffset = 0, *nPerBlock = NULL;
	if(relocblock){nPerBlock = __calloc(sizeof(UINT32), 5);}
	PeBaseRelocationBlock *relocations = (PeBaseRelocationBlock *)(relocblock + byteOffset);
	if(relocblock){
		while((relocations->BlockSize != 0) && (relocations->PageRVA != 0)){
			if((nRelocationBlocks % 5) == 0){nPerBlock = __realloc(nPerBlock, sizeof(UINT32) * nRelocationBlocks, sizeof(UINT32) * (nRelocationBlocks + 5));}
			nPerBlock[nRelocationBlocks] = ((relocations->BlockSize - sizeof(PeBaseRelocationBlock)) / sizeof(PeRelocationEntry));
			byteOffset += relocations->BlockSize;			nRelocationBlocks++;
			relocations = (PeBaseRelocationBlock *)(relocblock + byteOffset);
		}
		nRelocationBlocks++;
	}

	*out = (ExpandedPeExecutable){
		.Raw = header, .sck = sck, 
		.Fmt = {
			.Header = PH,
			.Opt = {.HeaderType = POH->mMagic == Pe32? Pe32: Pe32Plus, .Pe32 = POH},
			.RVAs = RVAs,
			.SectionTable = PISHs,
			.exp = {
				.Raw = (void *)exports, 
				.Export = exports,
				.NamePointerRVAs = NamePointerRVAs,
				.Ordinals = OrdinalPointerRVAs,
				.RawExportAddresses = exportAddressTable
			}, .imp = {
				.Raw = (void *)imports, 
				.imports = imports,
				.nImports = nImports,
				.nEntries = nEntries,
				.perImport = {.lookups.ImportLookups64 = ImportLookups64}
			}, .reloc = {
				.data = relocblock,
				.nRelocationBlocks = nRelocationBlocks,
				.nRelocationEntriesPerBlock = nPerBlock
			}, .rsrc = {
				.Raw = rsrcBlock, 
				.RootDirectory = rsrcRoot, 
				.REntries = {.ResourceEntries = rsrcEntries}
			}, .exception = {.Raw = pdataBlock}
		}
	};
	return out;
}

#define MAKESTR(S)	#S
#define APPEND_FLAG(value, flag_enum)	do{										\
	if(flagcheck((value), flag_enum)){											\
		const char *name = MAKESTR(flag_enum);									\
		size_t nameLen = strlena(name);											\
		char *nextBuf = __realloc(outStr, currentLen, currentLen + nameLen + 2);\
		if(nextBuf){															\
			outStr = nextBuf;													\
			__memcpy(outStr + currentLen, name, nameLen);						\
			currentLen += nameLen;												\
			outStr[currentLen++] = ' ';											\
			outStr[currentLen] = '\0';											\
		}																		\
	}																			\
}while(0)

const char *PeMachineTypeToString(PeMachineType Machine){
    switch (Machine){
        case PeMachineType_ALPHA:        return MAKESTR(PeMachineType_ALPHA);
        case PeMachineType_ALPHA64:      return MAKESTR(PeMachineType_ALPHA64);
        case PeMachineType_AM33:         return MAKESTR(PeMachineType_AM33);
        case PeMachineType_AMD64:        return MAKESTR(PeMachineType_AMD64);
        case PeMachineType_ARM:          return MAKESTR(PeMachineType_ARM);
        case PeMachineType_ARM64:        return MAKESTR(PeMachineType_ARM64);
        case PeMachineType_ARM64EC:      return MAKESTR(PeMachineType_ARM64EC);
        case PeMachineType_ARM64X:       return MAKESTR(PeMachineType_ARM64X);
        case PeMachineType_ARMNT:        return MAKESTR(PeMachineType_ARMNT);
        case PeMachineType_EBC:          return MAKESTR(PeMachineType_EBC);
        case PeMachineType_I386:         return MAKESTR(PeMachineType_I386);
        case PeMachineType_IA64:         return MAKESTR(PeMachineType_IA64);
        case PeMachineType_LOONGARCH32:  return MAKESTR(PeMachineType_LOONGARCH32);
        case PeMachineType_LOONGARCH64:  return MAKESTR(PeMachineType_LOONGARCH64);
        case PeMachineType_M32R:         return MAKESTR(PeMachineType_M32R);
        case PeMachineType_MIPS16:       return MAKESTR(PeMachineType_MIPS16);
        case PeMachineType_MIPSFPU:      return MAKESTR(PeMachineType_MIPSFPU);
        case PeMachineType_MIPSFPU16:    return MAKESTR(PeMachineType_MIPSFPU16);
        case PeMachineType_POWERPC:      return MAKESTR(PeMachineType_POWERPC);
        case PeMachineType_POWERPCFP:    return MAKESTR(PeMachineType_POWERPCFP);
        case PeMachineType_R3000BE:      return MAKESTR(PeMachineType_R3000BE);
        case PeMachineType_R3000:        return MAKESTR(PeMachineType_R3000);
        case PeMachineType_R4000:        return MAKESTR(PeMachineType_R4000);
        case PeMachineType_R10000:       return MAKESTR(PeMachineType_R10000);
        case PeMachineType_RISCV32:      return MAKESTR(PeMachineType_RISCV32);
        case PeMachineType_RISCV64:      return MAKESTR(PeMachineType_RISCV64);
        case PeMachineType_RISCV128:     return MAKESTR(PeMachineType_RISCV128);
        case PeMachineType_SH3:          return MAKESTR(PeMachineType_SH3);
        case PeMachineType_SH3DSP:       return MAKESTR(PeMachineType_SH3DSP);
        case PeMachineType_SH4:          return MAKESTR(PeMachineType_SH4);
        case PeMachineType_SH5:          return MAKESTR(PeMachineType_SH5);
        case PeMachineType_THUMB:        return MAKESTR(PeMachineType_THUMB);
        case PeMachineType_WCEMIPSV2:    return MAKESTR(PeMachineType_WCEMIPSV2);
        case PeMachineType_UNKNOWN:
        default:                         return MAKESTR(PeMachineType_UNKNOWN);
    }
}

const char *PeSubsystemToString(PeSubsystem subsystem){
    switch(subsystem){
        case PeSubsystem_NATIVE:                return MAKESTR(PeSubsystem_NATIVE);
        case PeSubsystem_WINGUI:                return MAKESTR(PeSubsystem_WINGUI);
        case PeSubsystem_WINCUI:                return MAKESTR(PeSubsystem_WINCUI);
        case PeSubsystem_0S2CUI:                return MAKESTR(PeSubsystem_0S2CUI);
        case PeSubsystem_POSIXCUI:              return MAKESTR(PeSubsystem_POSIXCUI);
        case PeSubsystem_WINNATIVE:             return MAKESTR(PeSubsystem_WINNATIVE);
        case PeSubsystem_WINCE_GUI:             return MAKESTR(PeSubsystem_WINCE_GUI);
        case PeSubsystem_EFI_APPLICATION:       return MAKESTR(PeSubsystem_EFI_APPLICATION);
        case PeSubsystem_EFIBOOT_SERVICEDRIVER: return MAKESTR(PeSubsystem_EFIBOOT_SERVICEDRIVER);
        case PeSubsystem_RUNTIME_DRIVER:        return MAKESTR(PeSubsystem_RUNTIME_DRIVER);
        case PeSubsystem_EFIROM:                return MAKESTR(PeSubsystem_EFIROM);
        case PeSubsystem_XBOX:                  return MAKESTR(PeSubsystem_XBOX);
        case PeSubsystem_WINBOOT_APPLICATION:   return MAKESTR(PeSubsystem_WINBOOT_APPLICATION);
        case PeSubsystem_UNKNOWN:
        default:                                return MAKESTR(PeSubsystem_UNKNOWN);
    }
}


char *PeSectionCharacteristicsToString(PeSectionCharacteristics Characteristics){
    char *outStr = NULL;
    size_t currentLen = 0;

    APPEND_FLAG(Characteristics, PeSectionCharacteristics_CODE);
    APPEND_FLAG(Characteristics, PeSectionCharacteristics_INITDATA);
    APPEND_FLAG(Characteristics, PeSectionCharacteristics_UINITDATA);
    APPEND_FLAG(Characteristics, PeSectionCharacteristics_LNK_OTHERS);
    APPEND_FLAG(Characteristics, PeSectionCharacteristics_GPREL);
    APPEND_FLAG(Characteristics, PeSectionCharacteristics_PURGABLEMEM);
    APPEND_FLAG(Characteristics, PeSectionCharacteristics_16BITMEM);
    APPEND_FLAG(Characteristics, PeSectionCharacteristics_MEMLOCKED);
    APPEND_FLAG(Characteristics, PeSectionCharacteristics_PRELOADMEM);
    APPEND_FLAG(Characteristics, PeSectionCharacteristics_NRELOC_OVFL);
    APPEND_FLAG(Characteristics, PeSectionCharacteristics_DISCARDABLE);
    APPEND_FLAG(Characteristics, PeSectionCharacteristics_NCACHABLE);
    APPEND_FLAG(Characteristics, PeSectionCharacteristics_NPAGABLE);
    APPEND_FLAG(Characteristics, PeSectionCharacteristics_MSHARED);
    APPEND_FLAG(Characteristics, PeSectionCharacteristics_MEXECUTABLE);
    APPEND_FLAG(Characteristics, PeSectionCharacteristics_MREADABLE);
    APPEND_FLAG(Characteristics, PeSectionCharacteristics_MWRITABLE);

    if(!outStr){outStr = __strdup(" ");}else
	if(currentLen > 0 && outStr[currentLen - 1] == ' '){outStr[currentLen - 1] = '\0';}

    return outStr;
}

char *PeHeaderCharacteristicsToString(PeCharacteristics Characteristics){
    char *outStr = NULL;
    size_t currentLen = 0;

    APPEND_FLAG(Characteristics, PeCharacteristics_NRELOCS);
    APPEND_FLAG(Characteristics, PeCharacteristics_EXECUTABLE);
    APPEND_FLAG(Characteristics, PeCharacteristics_NLINENUMS);
    APPEND_FLAG(Characteristics, PeCharacteristics_NLOCALSYM);
    APPEND_FLAG(Characteristics, PeCharacteristics_WSTRIM);
    APPEND_FLAG(Characteristics, PeCharacteristics_LADDRESS);
    APPEND_FLAG(Characteristics, PeCharacteristics_RBITS_LO);
    APPEND_FLAG(Characteristics, PeCharacteristics_32BIT);
    APPEND_FLAG(Characteristics, PeCharacteristics_NDEBUG);
    APPEND_FLAG(Characteristics, PeCharacteristics_LLOAD_ON_REMMEDIA);
    APPEND_FLAG(Characteristics, PeCharacteristics_LLOAD_ON_NETMEDIA);
    APPEND_FLAG(Characteristics, PeCharacteristics_SYSEXE);
    APPEND_FLAG(Characteristics, PeCharacteristics_DLL);
    APPEND_FLAG(Characteristics, PeCharacteristics_SYSONLY);
    APPEND_FLAG(Characteristics, PeCharacteristics_RBITS_HI);

    if(!outStr){outStr = __strdup("");}else
	if(currentLen > 0 && outStr[currentLen - 1] == ' '){outStr[currentLen - 1] = '\0';}
    return outStr;
}

char *PeDllCharacteristicsToString(PeDllCharacteristics Characteristics){
    char *outStr = NULL;
    size_t currentLen = 0;

    APPEND_FLAG(Characteristics, PeDllCharacteristics_HIGHENTROPY_VA);
    APPEND_FLAG(Characteristics, PeDllCharacteristics_DYNBASE);
    APPEND_FLAG(Characteristics, PeDllCharacteristics_FINTEGRITY);
    APPEND_FLAG(Characteristics, PeDllCharacteristics_NX);
    APPEND_FLAG(Characteristics, PeDllCharacteristics_NISOLATION);
    APPEND_FLAG(Characteristics, PeDllCharacteristics_NSEH);
    APPEND_FLAG(Characteristics, PeDllCharacteristics_NBIND);
    APPEND_FLAG(Characteristics, PeDllCharacteristics_CONTAINER);
    APPEND_FLAG(Characteristics, PeDllCharacteristics_WDMDRIVER);
    APPEND_FLAG(Characteristics, PeDllCharacteristics_GUARDCF);
    APPEND_FLAG(Characteristics, PeDllCharacteristics_TerminalServerAware);

    if(!outStr){outStr = __strdup("");}else
	if(currentLen > 0 && outStr[currentLen - 1] == ' '){outStr[currentLen - 1] = '\0';}
    return outStr;
}

void PrintPeExecutableFormat(ExpandedPeExecutable *EXE, UINT32 DataMax){
	char *CharacteristicsString = PeHeaderCharacteristicsToString(EXE->Fmt.Header->mCharacteristics);
	Print(L"\nHeader: {"
		"\n  .Magic\t%.4a"
		"\n  .MachineType\t%a"
		"\n  .NSections\t%llu"
		"\n  .Time/Date-Stamp\t%llu"
		"\n  .SymbolTablePtr\t0x%p"
		"\n  .NSymbols\t%llu"
		"\n  .OptionalHeaderSize\t%llu"
		"\n  .Characteristics\t%a", 
		EXE->Fmt.Header->mMagic, PeMachineTypeToString(EXE->Fmt.Header->mMachine), 
		(UINT64)EXE->Fmt.Header->mNumberOfSections, (UINT64)EXE->Fmt.Header->mTimeDateStamp, 
		(UINT64)EXE->Fmt.Header->mPointerToSymbolTable, (UINT64)EXE->Fmt.Header->mNumberOfSymbols, 
		(UINT64)EXE->Fmt.Header->mSizeOfOptionalHeader, CharacteristicsString);
	if(EXE->Fmt.Opt.Pe32->mMagic == Pe32){
		char *DllCharacteristicsString = PeDllCharacteristicsToString((UINT64)EXE->Fmt.Opt.Pe32->mDllCharacteristics), 
			*SubsystemString = PeSubsystemToString((UINT64)EXE->Fmt.Opt.Pe32->mSubsystem);
		Print(L"\n\tOptional(32-bit): {"
			"\n    .Magic\t%a"
			"\n    .LinkerVersion\t[%llu:%llu]"
			"\n    .SizeOfCode\t%llu"
			"\n    .SizeOfUninitialisedData\t%llu"
			"\n    .EntryPointAddress\t0x%p"
			"\n    .BaseOfCode\t%llu"
			"\n    .BaseOfData\t%llu"
			"\n    .ImageBase\t%llu"
			"\n    .SectionAlignment\t%llu"
			"\n    .FileAlignment\t%llu"
			"\n    .OSVersion\t[%llu:%llu]"
			"\n    .ImageVersion\t[%llu:%llu]"
			"\n    .SubsystemVersion\t[%llu:%llu]"
			"\n    .Win32Version\t%llu"
			"\n    .ImageSize\t%llu"
			"\n    .HeadersSize\t%llu"
			"\n    .Checksum\t%llu"
			"\n    .Subsystem\t%a"
			"\n    .Characteristics\t%a"
			"\n    .ReservedStackSize\t%llu"
			"\n    .CommitStackSize\t%llu"
			"\n    .ReservedHeapSize\t%llu"
			"\n    .CommitHeapSize\t%llu"
			"\n    .LoaderFlags\t%llu"
			"\n    .NRVAsAndSize\t%llu", MAKESTR(Pe32), (UINT64)EXE->Fmt.Opt.Pe32->mMajorLinkerVersion, 
			(UINT64)EXE->Fmt.Opt.Pe32->mMinorLinkerVersion, (UINT64)EXE->Fmt.Opt.Pe32->mSizeOfCode, 
			(UINT64)EXE->Fmt.Opt.Pe32->mSizeOfUninitializedData, (UINT64)EXE->Fmt.Opt.Pe32->mAddressOfEntryPoint, 
			(UINT64)EXE->Fmt.Opt.Pe32->mBaseOfCode, (UINT64)EXE->Fmt.Opt.Pe32->mBaseOfData, 
			(UINT64)EXE->Fmt.Opt.Pe32->mImageBase, (UINT64)EXE->Fmt.Opt.Pe32->mSectionAlignment, 
			(UINT64)EXE->Fmt.Opt.Pe32->mFileAlignment, (UINT64)EXE->Fmt.Opt.Pe32->mMajorOperatingSystemVersion, 
			(UINT64)EXE->Fmt.Opt.Pe32->mMinorOperatingSystemVersion, (UINT64)EXE->Fmt.Opt.Pe32->mMajorImageVersion, 
			(UINT64)EXE->Fmt.Opt.Pe32->mMinorImageVersion, (UINT64)EXE->Fmt.Opt.Pe32->mMajorSubsystemVersion, 
			(UINT64)EXE->Fmt.Opt.Pe32->mMinorSubsystemVersion, (UINT64)EXE->Fmt.Opt.Pe32->mWin32VersionValue, 
			(UINT64)EXE->Fmt.Opt.Pe32->mSizeOfImage, (UINT64)EXE->Fmt.Opt.Pe32->mSizeOfHeaders, 
			(UINT64)EXE->Fmt.Opt.Pe32->mCheckSum, SubsystemString, DllCharacteristicsString, 
			(UINT64)EXE->Fmt.Opt.Pe32->mSizeOfStackReserve, (UINT64)EXE->Fmt.Opt.Pe32->mSizeOfStackCommit, 
			(UINT64)EXE->Fmt.Opt.Pe32->mSizeOfHeapReserve, (UINT64)EXE->Fmt.Opt.Pe32->mSizeOfHeapCommit, 
			(UINT64)EXE->Fmt.Opt.Pe32->mLoaderFlags,(UINT64)EXE->Fmt.Opt.Pe32->mNumberOfRvaAndSizes);
	}else if(EXE->Fmt.Opt.Pe32->mMagic == Pe32Plus){
		char *DllCharacteristicsString = PeDllCharacteristicsToString((UINT64)EXE->Fmt.Opt.Pe32Plus->mDllCharacteristics), 
			*SubsystemString = PeSubsystemToString((UINT64)EXE->Fmt.Opt.Pe32Plus->mSubsystem);
		Print(L"\n\tOptional(32-bit): {"
			"\n    .Magic\t%a"
			"\n    .LinkerVersion\t[%llu:%llu]"
			"\n    .SizeOfCode\t%llu"
			"\n    .SizeOfInitialisedData\t%llu"
			"\n    .SizeOfUninitialisedData\t%llu"
			"\n    .EntryPointAddress\t0x%p"
			"\n    .BaseOfCode\t%llu"
			"\n    .ImageBase\t%llu"
			"\n    .SectionAlignment\t%llu"
			"\n    .FileAlignment\t%llu"
			"\n    .OSVersion  [%llu:%llu]"
			"\n    .ImageVersion  [%llu:%llu]"
			"\n    .SubsystemVersion  [%llu:%llu]"
			"\n    .Win32Version\t%llu"
			"\n    .ImageSize\t%llu"
			"\n    .HeadersSize\t%llu"
			"\n    .Checksum\t%llu"
			"\n    .Subsystem\t%a"
			"\n    .Characteristics\t%a"
			"\n    .ReservedStackSize\t%llu"
			"\n    .CommitStackSize\t%llu"
			"\n    .ReservedHeapSize\t%llu"
			"\n    .CommitHeapSize\t%llu"
			"\n    .LoaderFlags\t%llu"
			"\n    .NRVAsAndSize\t%llu", MAKESTR(Pe32Plus), (UINT64)EXE->Fmt.Opt.Pe32Plus->mMajorLinkerVersion, 
			(UINT64)EXE->Fmt.Opt.Pe32Plus->mMinorLinkerVersion, (UINT64)EXE->Fmt.Opt.Pe32Plus->mSizeOfCode, 
			(UINT64)EXE->Fmt.Opt.Pe32Plus->mSizeOfInitializedData, (UINT64)EXE->Fmt.Opt.Pe32Plus->mSizeOfUninitializedData, 
			(UINT64)EXE->Fmt.Opt.Pe32Plus->mAddressOfEntryPoint, (UINT64)EXE->Fmt.Opt.Pe32Plus->mBaseOfCode, 
			(UINT64)EXE->Fmt.Opt.Pe32Plus->mImageBase, (UINT64)EXE->Fmt.Opt.Pe32Plus->mSectionAlignment, 
			(UINT64)EXE->Fmt.Opt.Pe32Plus->mFileAlignment, (UINT64)EXE->Fmt.Opt.Pe32Plus->mMajorOperatingSystemVersion, 
			(UINT64)EXE->Fmt.Opt.Pe32Plus->mMinorOperatingSystemVersion, (UINT64)EXE->Fmt.Opt.Pe32Plus->mMajorImageVersion, 
			(UINT64)EXE->Fmt.Opt.Pe32Plus->mMinorImageVersion, (UINT64)EXE->Fmt.Opt.Pe32Plus->mMajorSubsystemVersion, 
			(UINT64)EXE->Fmt.Opt.Pe32Plus->mMinorSubsystemVersion, (UINT64)EXE->Fmt.Opt.Pe32Plus->mWin32VersionValue, 
			(UINT64)EXE->Fmt.Opt.Pe32Plus->mSizeOfImage, (UINT64)EXE->Fmt.Opt.Pe32Plus->mSizeOfHeaders, 
			(UINT64)EXE->Fmt.Opt.Pe32Plus->mCheckSum, SubsystemString, DllCharacteristicsString, 
			(UINT64)EXE->Fmt.Opt.Pe32Plus->mSizeOfStackReserve, (UINT64)EXE->Fmt.Opt.Pe32Plus->mSizeOfStackCommit, 
			(UINT64)EXE->Fmt.Opt.Pe32Plus->mSizeOfHeapReserve, (UINT64)EXE->Fmt.Opt.Pe32Plus->mSizeOfHeapCommit, 
			(UINT64)EXE->Fmt.Opt.Pe32Plus->mLoaderFlags,(UINT64)EXE->Fmt.Opt.Pe32Plus->mNumberOfRvaAndSizes);
	}
	for(UINT32 cc = 0; cc < EXE->Fmt.Header->mNumberOfSections; ++cc){
		char *SectionCharacteristicsString = PeSectionCharacteristicsToString(EXE->Fmt.SectionTable[cc].mCharacteristics);
		Print(L"\n    [%llu]: {"
			"\n      .RVA\t%llu"
			"\n      .Size\t%llu"
			"\n      {"
			"\n        .Name\t%.8a"
			"\n        .VirtualSize\t%llu"
			"\n        .VirtualAddress\t%llu"
			"\n        .RawSize\t%llu"
			"\n        .RawPointer\t0x%p"
			"\n        .RelocationTablePointer\t0x%p"
			"\n        .LineNumberPointer\t0x%p"
			"\n        .Characteristics\t%a"
			"\n      }", (UINT64)cc, 
			(UINT64)EXE->Fmt.RVAs[cc].RVA, (UINT64)EXE->Fmt.RVAs[cc].Size, 
			EXE->Fmt.SectionTable[cc].mName, (UINT64)EXE->Fmt.SectionTable[cc].mVirtualSize, 
			(UINT64)EXE->Fmt.SectionTable[cc].mVirtualAddress, (UINT64)EXE->Fmt.SectionTable[cc].mSizeOfRawData, 
			(UINT64)EXE->Fmt.SectionTable[cc].mPointerToRawData, (UINT64)EXE->Fmt.SectionTable[cc].mPointerToRelocations, 
			(UINT64)EXE->Fmt.SectionTable[cc].mPointerToLinenumbers, SectionCharacteristicsString);
		__free(SectionCharacteristicsString);
		if(!strncmpa(EXE->Fmt.SectionTable[cc].mName, PeExportSection, 8)){
			//	nExports describe the Exports in total within the DLL.
			Print(L"\n        {"
				"\n          .Flags\t%llu"
				"\n          .Time/DateStamp\t%llu"
				"\n          .Version  [%llu:%llu]"
				"\n          .NameRVA\t%llu(%a)"
				"\n          .OrdinalBase\t%llu"
				"\n          .NTableEntries\t%llu"
				"\n          .NNamePointers\t%llu"
				"\n          .ExportTableRVA\t%llu"
				"\n          .NamePointerRVA\t%llu"
				"\n          .OrdinalPointerRVA\t%llu"
				"\n          {", 
				(UINT64)EXE->Fmt.exp.Export->mExportFlags, (UINT64)EXE->Fmt.exp.Export->mVersionMajor, 
				(UINT64)EXE->Fmt.exp.Export->mVersionMinor, (UINT64)EXE->Fmt.exp.Export->NameRVA, 
				GetAtRVAFromSectionDataPe(EXE->Fmt.exp.Export->NameRVA, PeExportSection, EXE->Fmt.exp.Raw, EXE->Raw), 
				(UINT64)EXE->Fmt.exp.Export->mOrdinalBase, (UINT64)EXE->Fmt.exp.Export->mNTableEntries, 
				(UINT64)EXE->Fmt.exp.Export->mNNamePointers, (UINT64)EXE->Fmt.exp.Export->mExportTableRVA, 
				(UINT64)EXE->Fmt.exp.Export->mNamePointerRVA, (UINT64)EXE->Fmt.exp.Export->OrdinalPointerRVA);
			UINT32 *NamePointers = GetAtRVAFromSectionDataPe(EXE->Fmt.exp.Export->mNamePointerRVA, PeExportSection, EXE->Fmt.exp.Raw, EXE->Raw);
			for(UINT32 cc__ = 0; cc__ < __min(DataMax, EXE->Fmt.exp.Export->mNNamePointers); ++cc__){
				Print(L"\n            \"%a\"", GetAtRVAFromSectionDataPe(NamePointers[cc__], PeExportSection, EXE->Fmt.exp.Raw, EXE->Raw));}
		}else if(!strncmpa(EXE->Fmt.SectionTable[cc].mName, PeImportSection, 8)){
			Print(L"\n      {\n        .NImports\t%llu", (UINT64)EXE->Fmt.imp.nImports);
			for(UINT32 cc_ = 0; cc_ < EXE->Fmt.imp.nImports; ++cc_){
				Print(L"\n        [%llu]: {"
					"\n          .NEntries\t%llu"
					"\n          .ImportLookupTableRVA\t%llu"
					"\n          .Time/DateStamp\t%llu"

					"\n          .ForwarderChainFirstIndex\t%llu"
					"\n          .NameRVA\t%llu(%a)"
					"\n          .ImportAddressTableRVA\t%llu", 
					(UINT64)cc_, (UINT64)EXE->Fmt.imp.nEntries[cc_], (UINT64)EXE->Fmt.imp.imports[cc_].ImportLookupTableRVA, 
					(UINT64)EXE->Fmt.imp.imports[cc_].TimeDateStamp, (UINT64)EXE->Fmt.imp.imports[cc_].ForwarderChain, 
					(UINT64)EXE->Fmt.imp.imports[cc_].NameRVA, 
					GetAtRVAFromSectionDataPe(EXE->Fmt.imp.imports[cc_].NameRVA, PeImportSection, EXE->Fmt.imp.Raw, EXE->Raw), 
					(UINT64)EXE->Fmt.imp.imports[cc_].ImportAddressTableRVA);
				if(EXE->Fmt.Opt.Pe32->mMagic == Pe32){
					PeImportLookupEntry32 *Table = GetAtRVAFromSectionDataPe(EXE->Fmt.imp.imports[cc_].ImportLookupTableRVA, PeImportSection, EXE->Fmt.imp.Raw, EXE->Raw);
					for(UINT32 cc__ = 0; cc__ < __min(EXE->Fmt.imp.nEntries[cc_], DataMax); ++cc__){
						if(Table[cc__].Bits.ImportByOrdinal){Print(L"\n            \"#%llu\"", (UINT64)Table[cc__].Bits.OrdinalNumberOrNameRVA);}
						else{
							PeImportHintEntry *Name = GetAtRVAFromSectionDataPe(Table[cc__].Bits.OrdinalNumberOrNameRVA, PeImportSection, EXE->Fmt.imp.Raw, EXE->Raw);
							Print(L"\n            \"{%llu:%a}\"", (UINT64)Name->Hint, Name->Name);
						}
					}
				}else if(EXE->Fmt.Opt.Pe32->mMagic == Pe32Plus){
					PeImportLookupEntry64 *Table = GetAtRVAFromSectionDataPe(EXE->Fmt.imp.imports[cc_].ImportLookupTableRVA, PeImportSection, EXE->Fmt.imp.Raw, EXE->Raw);
					for(UINT32 cc__ = 0; cc__ < __min(EXE->Fmt.imp.nEntries[cc_], DataMax); ++cc__){
						if(Table[cc__].Bits.ImportByOrdinal){Print(L"\n            \"#%llu\"", (UINT64)Table[cc__].Bits.OrdinalNumberOrNameRVA);}
						else{
							PeImportHintEntry *Name = GetAtRVAFromSectionDataPe(Table[cc__].Bits.OrdinalNumberOrNameRVA, PeImportSection, EXE->Fmt.imp.Raw, EXE->Raw);
							Print(L"\n            \"{%llu:%a}\"", (UINT64)Name->Hint, Name->Name);
						}
					}
				}
			}
		}else if(!strncmpa(EXE->Fmt.SectionTable[cc].mName, PeExceptionInfoSection, 8)){
			void *Data = ReadSectionPe(EXE->sck, EXE->Raw, PeExceptionInfoSection);
			PeImageSectionHeader *This = FindSectionPe(EXE->Raw, PeExceptionInfoSection);
			UINT64 N = 0;
			switch(EXE->Fmt.Header->mMachine){
				case PeMachineType_R3000BE:
				case PeMachineType_R3000: {
					Pe32MIPSExceptionDataEntry *Table = (Pe32MIPSExceptionDataEntry *)Data;
					N = This->mSizeOfRawData / sizeof(Pe32MIPSExceptionDataEntry);
					Print(L"\n      {\n        .N\t%llu\n        MachineType\t%a", N, PeMachineTypeToString(EXE->Fmt.Header->mMachine));
					for(UINT64 cc_ = 0; cc_ < __min(N, PeDumpVolumeLine); ++cc_){
						Print(L"\n        [%llu]: {"
							"\n          .VirtualAddress\t%llu"
							"\n          .VirtualEnd\t%llu"
							"\n          .VirtualHandler\t%llu", 
							"\n          .HandlerDataPointer\t%llu"
							"\n          .VirtualPrologAddress\t%llu", (UINT64)cc_, 
							(UINT64)(Table[cc_].mVirtualAddress), (UINT64)(Table[cc_].mVirtualEnd), 
							(UINT64)(Table[cc_].mHandler), (UINT64)(Table[cc_].mHandlerData), 
							(UINT64)(Table[cc_].mVirtualPrologAddress));
					}
				}
				case PeMachineType_POWERPC:
				case PeMachineType_POWERPCFP:
				case PeMachineType_SH4:
				case PeMachineType_SH3DSP:
				case PeMachineType_SH3:
				case PeMachineType_ARM:
				case PeMachineType_ARM64:
				case PeMachineType_ARM64EC:
				case PeMachineType_ARM64X:
				case PeMachineType_ARMNT: {
					PeARMExceptionDataEntry *Table = (PeARMExceptionDataEntry *)Data;
					N = This->mSizeOfRawData / sizeof(PeARMExceptionDataEntry);
					Print(L"\n      {\n        .N\t%llu\n        MachineType\t%a", N, PeMachineTypeToString(EXE->Fmt.Header->mMachine));
					for(UINT64 cc_ = 0; cc_ < __min(N, PeDumpVolumeLine); ++cc_){
						Print(L"\n        [%llu]  (32-Bit? %a\tHasHandler? %a): {"
							"\n          .VirtualAddress\t%llu"
							"\n          .PrologLength\t%llu"
							"\n          .FunctionLength\t%llu", (UINT64)cc_, 
							(Table[cc_].mIs32Bit? "TRUE": "FALSE"), (Table[cc_].mHasHandler? "TRUE": "FALSE"), 
							(UINT64)(Table[cc_].mVirtualAddress), (UINT64)(Table[cc_].mPrologLength), 
							(UINT64)(Table[cc_].mFunctionLength));
					}
				}
				case PeMachineType_ALPHA64:
				case PeMachineType_AMD64:
				case PeMachineType_IA64:
				case PeMachineType_LOONGARCH64:
				case PeMachineType_R4000:
				case PeMachineType_R10000:
				case PeMachineType_RISCV64: {
					Pe32PlusExceptionDataEntry *Table = (Pe32PlusExceptionDataEntry *)Data;
					N = This->mSizeOfRawData / sizeof(Pe32PlusExceptionDataEntry);
					Print(L"\n      {\n        .N\t%llu\n        MachineType\t%a", N, PeMachineTypeToString(EXE->Fmt.Header->mMachine));
					for(UINT64 cc_ = 0; cc_ < __min(N, PeDumpVolumeLine); ++cc_){
						Print(L"\n        [%llu]: {"
							"\n          .AddressRVA\t%llu"
							"\n          .EndRVA\t%llu"
							"\n          .UnwindRVA\t%llu", (UINT64)cc_, 
							(UINT64)(Table[cc_].mAddressRVA), (UINT64)(Table[cc_].mEndRVA), 
							(UINT64)(Table[cc_].mUnwindRVA));
					}
				}
			}
			Print(L"\n          }\n        }");
			__free(Data);
		}else{
			void *Data = ReadSectionPe(EXE->sck, EXE->Raw, EXE->Fmt.SectionTable[cc].mName);
			PeImageSectionHeader *This = FindSectionPe(EXE->Raw, EXE->Fmt.SectionTable[cc].mName);
			Print(L"\n      {");
			if(Data){
				for(UINT32 cc_ = 0; cc_ < (DataMax / PeDumpVolumeLine); ++cc_){
					Print(L"\n        ");
					for(UINT32 cc__ = 0; cc__ < PeDumpVolumeLine && ((cc_ * PeDumpVolumeLine) + cc__) < This->mSizeOfRawData; ++cc__){
						Print(L"%02x ", ((UINT8 *)Data)[cc_ + (cc__ * (DataMax / PeDumpVolumeLine))]);}
				}
			}
			Print(L"\n      }");
			__free(Data);
		}
		Print(L"\n    }");
	}
}