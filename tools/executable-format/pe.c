#include "pe.h"

void *ReadPeExecutableHeader(const char *path){
	FILE *f = NULL;
	if(!(f = fopen(path, "rb"))){return NULL;}
	fseek(f, PeHeaderOffsetAddress, SEEK_SET);
	uint32_t PeHeaderOffset;
	uint64_t outputSize = sizeof(PeHeader) + __max(sizeof(Pe32OptionalHeader), sizeof(Pe32PlusOptionalHeader));
	if(fread(&PeHeaderOffset, sizeof(uint32_t), 1, f) != 1){
		fclose(f);
		return NULL;
	}
	fseek(f, PeHeaderOffset, SEEK_SET);
	void *out = calloc(1, outputSize);
	if(fread(out, sizeof(PeHeader), 1, f) != 1){
		free(out);
		fclose(f); // Clean up file
		return NULL;
	}
	if(strncmp(((PeHeader *)out)->mMagic, PeHeaderMagicStr, 4)){free(out);	fclose(f);	return NULL;}
	switch(((PeHeader *)out)->mSizeOfOptionalHeader){
		case sizeof(Pe32OptionalHeader) + (sizeof(PeRVAnSize) * PeDefaultNDataDirectories): {
			pe32label:
			fread(out + sizeof(PeHeader), sizeof(Pe32OptionalHeader), 1, f);
			if(((Pe32OptionalHeader *)(out + sizeof(PeHeader)))->mMagic == Pe32){
			}else{
				outputSize = sizeof(PeHeader) + sizeof(Pe32OptionalHeader) + 
					(((Pe32OptionalHeader *)(out + sizeof(PeHeader)))->mNumberOfRvaAndSizes * sizeof(PeRVAnSize));
				out = realloc(out, outputSize);
				fread(out + sizeof(PeHeader) + sizeof(Pe32OptionalHeader), sizeof(PeRVAnSize), 
					((Pe32OptionalHeader *)(out + sizeof(PeHeader)))->mNumberOfRvaAndSizes, f);
			}
			break;
		} case sizeof(Pe32PlusOptionalHeader) + (sizeof(PeRVAnSize) * PeDefaultNDataDirectories): {
			pe32pluslabel:
			fread(out + sizeof(PeHeader), sizeof(Pe32PlusOptionalHeader), 1, f);
			if(((Pe32PlusOptionalHeader *)(out + sizeof(PeHeader)))->mMagic == Pe32Plus){
				outputSize = sizeof(PeHeader) + sizeof(Pe32PlusOptionalHeader) + 
					(((Pe32PlusOptionalHeader *)(out + sizeof(PeHeader)))->mNumberOfRvaAndSizes * sizeof(PeRVAnSize));
				out = realloc(out, outputSize);
				fread(out + sizeof(PeHeader) + sizeof(Pe32PlusOptionalHeader), sizeof(PeRVAnSize), 
					((Pe32PlusOptionalHeader *)(out + sizeof(PeHeader)))->mNumberOfRvaAndSizes, f);
			}
			break;
		} default: {
			PeOHeaderType HType = 0;
			const uint64_t Curr = ftell(f);
			fread(&HType, sizeof(PeOHeaderType), 1, f);
			switch(HType){
				case Pe32:		{fseek(f, Curr, SEEK_SET);		goto pe32label;}
				case Pe32Plus:	{fseek(f, Curr, SEEK_SET);		goto pe32pluslabel;}
				default:		{free(out);		fclose(f);		return NULL;}
			}
		}
	}
	DecodePeExecutableHeader(out);
	// Get Section Table
	outputSize = sizeof(PeHeader) + (POH->mMagic == Pe32? sizeof(Pe32OptionalHeader): sizeof(Pe32PlusOptionalHeader)) + (POH->mMagic == Pe32 ? 
		(POH->mNumberOfRvaAndSizes * (sizeof(PeRVAnSize) + sizeof(PeImageSectionHeader))) : 
		(POHPlus->mNumberOfRvaAndSizes * (sizeof(PeRVAnSize) + sizeof(PeImageSectionHeader))));
	out = realloc(out, outputSize);
	RDecodePeExecutableHeader(out);
	uint32_t numSectionHeaders = (POH->mMagic == Pe32 ? POH->mNumberOfRvaAndSizes : POHPlus->mNumberOfRvaAndSizes);
	if(fread(PISHs, sizeof(PeImageSectionHeader), numSectionHeaders, f) != numSectionHeaders){
		free(out);
		fclose(f);
		return NULL;
	}
	fclose(f);
	return out;
}
// void *ReadPeExecutableHeader(const char *path){
//     FILE *f = fopen(path, "rb");
//     if(!f){return NULL;}

//     fseek(f, PeHeaderOffsetAddress, SEEK_SET);
//     uint32_t PeHeaderOffset = 0;
//     if(fread(&PeHeaderOffset, sizeof(uint32_t), 1, f) != 1){
//         fclose(f);
//         return NULL;
//     }

//     fseek(f, PeHeaderOffset, SEEK_SET);
//     PeHeader peHeader;
//     if(fread(&peHeader, sizeof(PeHeader), 1, f) != 1){
//         fclose(f);
//         return NULL;
//     }

//     // Validate "PE\0\0" signature
//     if(peHeader.mMagic[0] != 'P' || peHeader.mMagic[1] != 'E' ||
//     peHeader.mMagic[2] != '\0' || peHeader.mMagic[3] != '\0'){
//         fclose(f);
//         return NULL;
//     }

//     uint16_t optMagic = 0;
//     long optHeaderStartPos = ftell(f);
//     if (fread(&optMagic, sizeof(uint16_t), 1, f) != 1) {
//         fclose(f);
//         return NULL;
//     }
//     fseek(f, optHeaderStartPos, SEEK_SET);

//     bool isPe32Plus = (optMagic == Pe32Plus);
//     size_t optHeaderSize = isPe32Plus ? sizeof(Pe32PlusOptionalHeader) : sizeof(Pe32OptionalHeader);

//     uint8_t optBuffer[sizeof(Pe32PlusOptionalHeader)] = {0};
//     if(fread(optBuffer, optHeaderSize, 1, f) != 1){
//         fclose(f);
//         return NULL;
//     }

//     uint32_t numRvaAndSizes = isPe32Plus 
//         ? ((Pe32PlusOptionalHeader *)optBuffer)->mNumberOfRvaAndSizes 
//         : ((Pe32OptionalHeader *)optBuffer)->mNumberOfRvaAndSizes;

//     size_t totalDataDirSize = numRvaAndSizes * sizeof(PeRVAnSize), 
// 			totalSectionHeadersSize = peHeader.mNumberOfSections * sizeof(PeImageSectionHeader), 
// 			totalBufferSize = sizeof(PeHeader) + optHeaderSize + totalDataDirSize + totalSectionHeadersSize;

//     uint8_t *out = calloc(1, totalBufferSize);
//     if(!out){
//         fclose(f);
//         return NULL;
//     }

//     memcpy(out, &peHeader, sizeof(PeHeader));
//     memcpy(out + sizeof(PeHeader), optBuffer, optHeaderSize);

//     if(numRvaAndSizes > 0){
//         size_t dataDirOffset = sizeof(PeHeader) + optHeaderSize;
//         if(fread(out + dataDirOffset, sizeof(PeRVAnSize), numRvaAndSizes, f) != numRvaAndSizes){
//             free(out);
//             fclose(f);
//             return NULL;
//         }
//     }

//     DecodePeExecutableHeader(out);

//     size_t sectionHeaderOffset = sizeof(PeHeader) + optHeaderSize + totalDataDirSize;
//     if(peHeader.mNumberOfSections > 0){
//         if(fread(out + sectionHeaderOffset, sizeof(PeImageSectionHeader), peHeader.mNumberOfSections, f) != peHeader.mNumberOfSections){
//             free(out);
//             fclose(f);
//             return NULL;
//         }
//     }
//     fclose(f);
//     return out;
// }

uint32_t RvaToFileOffsetPe(uint32_t rva, PeImageSectionHeader *Section){
	uint32_t startRVA = Section->mVirtualAddress;
	uint32_t endRVA = startRVA + Section->mVirtualSize;
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
	for(uint32_t cc = 0; cc < PH->mNumberOfSections; ++cc){
		if(!strncmp(PISHs[cc].mName, name, 8)){return (PISHs + cc);}
	}
	return NULL;
}

void *GetAtRVAFromSectionDataPe(uint32_t RVA, char name[8], void *data, void *header){
    PeImageSectionHeader *Section = FindSectionPe(header, name);
    if(!Section){return NULL;}
    // Verify the RVA actually lands within this section's virtual boundaries
    if((RVA >= Section->mVirtualAddress) && RVA < (Section->mVirtualAddress + Section->mVirtualSize)){
        // Calculate the byte offset from the start of this section's memory space
        uint32_t sectionOffset = RVA - Section->mVirtualAddress;
        // Return the pointer shifted by that offset
        return (void *)((uint8_t *)data + sectionOffset);
    }
    
    return NULL;
}

void *ReadAtRVAFromSectionPe(uint32_t RVA, uint32_t Size, char name[8], char *path, void *header){
	FILE *f = NULL;
	if(f = fopen(path, "rb")){
		PeImageSectionHeader *Section = FindSectionPe(header, name);
		if(Section){
			uint32_t Address = RvaToFileOffsetPe(RVA, Section);
			if(Address){
				fseek(f, Address, SEEK_SET);
				void *out = calloc(Size, sizeof(char));
				if(fread(out, sizeof(char), Size, f) != Size){
					fclose(f);
					return out;
				}else{
					fclose(f);
					free(out);
					return NULL;
				}
			}
		}
	}
	return NULL;
}

char *ReadStringAtRVAFromSectionPe(uint32_t RVA, char name[8], char *path, void *header){
	FILE *f = NULL;
	if(f = fopen(path, "rb")){
		PeImageSectionHeader *Section = FindSectionPe(header, name);
		if(Section){
			uint32_t Address = RvaToFileOffsetPe(RVA, Section);
			if(Address){
				fseek(f, Address, SEEK_SET);
				uint32_t len = 0;
				char *out = calloc(8, sizeof(char));
				while((out[len++] = getc(f))){
					if((len % 8) == 0){out = realloc(out, len + 8);	
						memset(out + len, 0, 8);}
				}
			}
		}
	}
	return NULL;
}

void *ReadSectionPe(char *path, void *header, char name[8]){
	FILE *f = NULL;
	if((f = fopen(path, "rb"))){
		DecodePeExecutableHeader(header);
		PeImageSectionHeader *section = FindSectionPe(header, name);
		if(section){
			uint32_t Address = section->mPointerToRawData;
			if(Address){fseek(f, Address, SEEK_SET);
			}else{fclose(f);	return NULL;}
			if(section->mSizeOfRawData){
				void *out = calloc(section->mSizeOfRawData, sizeof(char));
				if(fread(out, sizeof(char), section->mSizeOfRawData, f) == section->mSizeOfRawData){
					fclose(f);
					return out;
				}else{
					fclose(f);
					free(out);
					return NULL;
				}
			}else{
				fclose(f);
				return NULL;
			}
		}
		fclose(f);
	}
	return NULL;
}

ExpandedPeExecutable *ExpandPeExecutableFormat(const char *path){
	void *header = ReadPeExecutableHeader(path);
	DecodePeExecutableHeader(header);

	ExpandedPeExecutable *out = calloc(sizeof(ExpandedPeExecutable), 1);

	PeExportDirectoryEntry *exports = ReadSectionPe(path, header, PeExportSection);
	uint32_t nExports = 0;
	if(exports){while(memcmp(exports + nExports, (uint8_t[sizeof(PeExportDirectoryEntry)]){0}, sizeof(PeExportDirectoryEntry))){nExports++;}	nExports++;}
	// These are RVA Pointers to Various Names in the Export Name Table.
	uint32_t *NamePointerRVAs = exports? GetAtRVAFromSectionDataPe(exports->mNamePointerRVA, PeExportSection, exports, header): NULL;
	//	Indices of Various Addresses Exported in the Export Directory Table.
	//	Subtract the Ordinal Base from the Digit.
	uint16_t *OrdinalPointerRVAs = exports? GetAtRVAFromSectionDataPe(exports->OrdinalPointerRVA, PeExportSection, exports, header): NULL;
	if(exports){for(uint32_t cc = 0; cc < exports->mNNamePointers; ++cc){OrdinalPointerRVAs[cc] -= exports->mOrdinalBase;}}
	PeExportAddressEntry *exportAddressTable = exports? GetAtRVAFromSectionDataPe(exports->mExportTableRVA, PeExportSection, exports, header): NULL;

	PeImportDirectoryEntry *imports = ReadSectionPe(path, header, PeImportSection);
	uint32_t nImports = 0;
	if(imports){while(memcmp(imports + nImports, (uint8_t[sizeof(PeImportDirectoryEntry)]){0}, sizeof(PeImportDirectoryEntry))){++nImports;}}
	uint32_t *nEntries = calloc(nImports, sizeof(uint32_t));
	char **importName = imports? calloc(sizeof(char *), nImports): NULL;
	PeImportLookupEntry32 **ImportLookups32 = imports? calloc(POH->mMagic == Pe32? 
		sizeof(PeImportLookupEntry32 *): sizeof(PeImportLookupEntry64 *), nImports): NULL;
	PeImportLookupEntry64 **ImportLookups64 = (PeImportLookupEntry64 **)ImportLookups32;
	if(imports){
		for(uint32_t cc = 0; cc < nImports; ++cc){
			ImportLookups32[cc] = GetAtRVAFromSectionDataPe(imports[cc].ImportLookupTableRVA, PeImportSection, imports, header);
			importName[cc] = GetAtRVAFromSectionDataPe(imports[cc].NameRVA, PeImportSection, imports, header);
			uint32_t cc_ = 0;
			if(POH->mMagic == Pe32){
				while(memcmp(ImportLookups32[cc] + cc_, (uint8_t[sizeof(PeImportLookupEntry32)]){0}, sizeof(PeImportLookupEntry32))){++cc_;}
			}else{while(memcmp(ImportLookups64[cc] + cc_, (uint8_t[sizeof(PeImportLookupEntry64)]){0}, sizeof(PeImportLookupEntry64))){++cc_;}}
			nEntries[cc] = cc_;
		}
	}

	//	.pdata Info
	void *pdataBlock = ReadSectionPe(path, header, PeExceptionInfoSection);

	//	.rsrc Info
	void *rsrcBlock = ReadSectionPe(path, header, PeResourceSection);
	PeResourceRootDirectory *rsrcRoot = (PeResourceRootDirectory *)rsrcBlock;
	PeResourceDirectoryEntry *rsrcEntries = rsrcBlock + sizeof(PeResourceRootDirectory);
	

	// .reloc Info
	void *relocblock = ReadSectionPe(path, header, PeRelocDataSection);
	PeImageSectionHeader *relocHeader = relocblock? FindSectionPe(header, PeRelocDataSection): NULL;
	uint32_t nRelocationBlocks = 0, byteOffset = 0, *nPerBlock = relocblock? calloc(sizeof(uint32_t), 5): NULL;
	PeBaseRelocationBlock *relocations = (PeBaseRelocationBlock *)(relocblock + byteOffset);
	if(relocblock){
		while((relocations->BlockSize != 0) && (relocations->PageRVA != 0)){
			if((nRelocationBlocks % 5) == 0){nPerBlock = realloc(nPerBlock, sizeof(uint32_t) * (nRelocationBlocks + 5));}
			nPerBlock[nRelocationBlocks] = ((relocations->BlockSize - sizeof(PeBaseRelocationBlock)) / sizeof(PeRelocationEntry));
			byteOffset += relocations->BlockSize;			nRelocationBlocks++;
			relocations = (PeBaseRelocationBlock *)(relocblock + byteOffset);
		}
		nRelocationBlocks++;
	}

	*out = (ExpandedPeExecutable){
		.Raw = header, .Path = strdup(path), 
		.Fmt = {
			.Header = PH,
			.Opt = {.HeaderType = POH->mMagic == Pe32? Pe32: Pe32Plus, .Pe32 = POH},
			.RVAs = RVAs,
			.SectionTable = PISHs,
			.exp = {
				.Raw = (void *)exports, 
				.nExports = nExports,
				.exportEntries = exports,
				.NamePointerRVAs = NamePointerRVAs,
				.NormalisedOrdinals = OrdinalPointerRVAs,
				.RawExportAddresses = exportAddressTable
			}, .imp = {
				.Raw = (void *)imports, 
				.imports = imports,
				.nImports = nImports,
				.nEntries = nEntries,
				.perImport = {
					.Names = importName,
					.lookups.ImportLookups64 = ImportLookups64
				}
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
#define APPEND_FLAG(value, flag_enum)	do{							\
	if(flagcheck((value), flag_enum)){								\
		const char *name = MAKESTR(flag_enum);						\
		size_t nameLen = strlen(name);								\
		char *nextBuf = realloc(outStr, currentLen + nameLen + 2);	\
		if(nextBuf){												\
			outStr = nextBuf;										\
			memcpy(outStr + currentLen, name, nameLen);				\
			currentLen += nameLen;									\
			outStr[currentLen++] = ' ';								\
			outStr[currentLen] = '\0';								\
		}															\
	}																\
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

    if(!outStr){outStr = strdup("");}else
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

    if(!outStr){outStr = strdup("");}else
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

    if(!outStr){outStr = strdup("");}else
	if(currentLen > 0 && outStr[currentLen - 1] == ' '){outStr[currentLen - 1] = '\0';}
    return outStr;
}

void DumpPe(ExpandedPeExecutable *EXE){
	char *CharacteristicsString = PeHeaderCharacteristicsToString(EXE->Fmt.Header->mCharacteristics);
	if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
		fprintf(f, "\n[%s]"
			"\nHeader: {"
			"\n  .Magic\t%.4s"
			"\n  .MachineType\t%s"
			"\n  .NSections\t%llu"
			"\n  .Time/Date-Stamp\t%llu"
			"\n  .SymbolTablePtr\t0x%p"
			"\n  .NSymbols\t%llu"
			"\n  .OptionalHeaderSize\t%llu"
			"\n  .Characteristics\t%s", 
			EXE->Path, EXE->Fmt.Header->mMagic, PeMachineTypeToString(EXE->Fmt.Header->mMachine), 
			(uint64_t)EXE->Fmt.Header->mNumberOfSections, (uint64_t)EXE->Fmt.Header->mTimeDateStamp, 
			(uint64_t)EXE->Fmt.Header->mPointerToSymbolTable, (uint64_t)EXE->Fmt.Header->mNumberOfSymbols, 
			(uint64_t)EXE->Fmt.Header->mSizeOfOptionalHeader, CharacteristicsString);
		fclose(f);
	}printf(
		"\n[%s]"
		"\nHeader: {"
		"\n  .Magic\t%.4s"
		"\n  .MachineType\t%s"
		"\n  .NSections\t%llu"
		"\n  .Time/Date-Stamp\t%llu"
		"\n  .SymbolTablePtr\t0x%p"
		"\n  .NSymbols\t%llu"
		"\n  .OptionalHeaderSize\t%llu"
		"\n  .Characteristics\t%s", 
		EXE->Path, EXE->Fmt.Header->mMagic, PeMachineTypeToString(EXE->Fmt.Header->mMachine), 
		(uint64_t)EXE->Fmt.Header->mNumberOfSections, (uint64_t)EXE->Fmt.Header->mTimeDateStamp, 
		(uint64_t)EXE->Fmt.Header->mPointerToSymbolTable, (uint64_t)EXE->Fmt.Header->mNumberOfSymbols, 
		(uint64_t)EXE->Fmt.Header->mSizeOfOptionalHeader, CharacteristicsString);
	if(EXE->Fmt.Opt.Pe32->mMagic == Pe32){
		char *DllCharacteristicsString = PeDllCharacteristicsToString((uint64_t)EXE->Fmt.Opt.Pe32->mDllCharacteristics), 
			*SubsystemString = PeSubsystemToString((uint64_t)EXE->Fmt.Opt.Pe32->mSubsystem);
		if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
			fprintf(f, "\n\tOptional(32-bit): {"
				"\n    .Magic\t%s"
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
				"\n    .Subsystem\t%s"
				"\n    .Characteristics\t%s"
				"\n    .ReservedStackSize\t%llu"
				"\n    .CommitStackSize\t%llu"
				"\n    .ReservedHeapSize\t%llu"
				"\n    .CommitHeapSize\t%llu"
				"\n    .LoaderFlags\t%llu"
				"\n    .NRVAsAndSize\t%llu", MAKESTR(Pe32), (uint64_t)EXE->Fmt.Opt.Pe32->mMajorLinkerVersion, 
				(uint64_t)EXE->Fmt.Opt.Pe32->mMinorLinkerVersion, (uint64_t)EXE->Fmt.Opt.Pe32->mSizeOfCode, 
				(uint64_t)EXE->Fmt.Opt.Pe32->mSizeOfUninitializedData, (uint64_t)EXE->Fmt.Opt.Pe32->mAddressOfEntryPoint, 
				(uint64_t)EXE->Fmt.Opt.Pe32->mBaseOfCode, (uint64_t)EXE->Fmt.Opt.Pe32->mBaseOfData, 
				(uint64_t)EXE->Fmt.Opt.Pe32->mImageBase, (uint64_t)EXE->Fmt.Opt.Pe32->mSectionAlignment, 
				(uint64_t)EXE->Fmt.Opt.Pe32->mFileAlignment, (uint64_t)EXE->Fmt.Opt.Pe32->mMajorOperatingSystemVersion, 
				(uint64_t)EXE->Fmt.Opt.Pe32->mMinorOperatingSystemVersion, (uint64_t)EXE->Fmt.Opt.Pe32->mMajorImageVersion, 
				(uint64_t)EXE->Fmt.Opt.Pe32->mMinorImageVersion, (uint64_t)EXE->Fmt.Opt.Pe32->mMajorSubsystemVersion, 
				(uint64_t)EXE->Fmt.Opt.Pe32->mMinorSubsystemVersion, (uint64_t)EXE->Fmt.Opt.Pe32->mWin32VersionValue, 
				(uint64_t)EXE->Fmt.Opt.Pe32->mSizeOfImage, (uint64_t)EXE->Fmt.Opt.Pe32->mSizeOfHeaders, 
				(uint64_t)EXE->Fmt.Opt.Pe32->mCheckSum, SubsystemString, DllCharacteristicsString, 
				(uint64_t)EXE->Fmt.Opt.Pe32->mSizeOfStackReserve, (uint64_t)EXE->Fmt.Opt.Pe32->mSizeOfStackCommit, 
				(uint64_t)EXE->Fmt.Opt.Pe32->mSizeOfHeapReserve, (uint64_t)EXE->Fmt.Opt.Pe32->mSizeOfHeapCommit, 
				(uint64_t)EXE->Fmt.Opt.Pe32->mLoaderFlags,(uint64_t)EXE->Fmt.Opt.Pe32->mNumberOfRvaAndSizes);
			fclose(f);
		}printf(
			"\n\tOptional(32-bit): {"
			"\n    .Magic\t%s"
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
			"\n    .Subsystem\t%s"
			"\n    .Characteristics\t%s"
			"\n    .ReservedStackSize\t%llu"
			"\n    .CommitStackSize\t%llu"
			"\n    .ReservedHeapSize\t%llu"
			"\n    .CommitHeapSize\t%llu"
			"\n    .LoaderFlags\t%llu"
			"\n    .NRVAsAndSize\t%llu", MAKESTR(Pe32), (uint64_t)EXE->Fmt.Opt.Pe32->mMajorLinkerVersion, 
			(uint64_t)EXE->Fmt.Opt.Pe32->mMinorLinkerVersion, (uint64_t)EXE->Fmt.Opt.Pe32->mSizeOfCode, 
			(uint64_t)EXE->Fmt.Opt.Pe32->mSizeOfUninitializedData, (uint64_t)EXE->Fmt.Opt.Pe32->mAddressOfEntryPoint, 
			(uint64_t)EXE->Fmt.Opt.Pe32->mBaseOfCode, (uint64_t)EXE->Fmt.Opt.Pe32->mBaseOfData, 
			(uint64_t)EXE->Fmt.Opt.Pe32->mImageBase, (uint64_t)EXE->Fmt.Opt.Pe32->mSectionAlignment, 
			(uint64_t)EXE->Fmt.Opt.Pe32->mFileAlignment, (uint64_t)EXE->Fmt.Opt.Pe32->mMajorOperatingSystemVersion, 
			(uint64_t)EXE->Fmt.Opt.Pe32->mMinorOperatingSystemVersion, (uint64_t)EXE->Fmt.Opt.Pe32->mMajorImageVersion, 
			(uint64_t)EXE->Fmt.Opt.Pe32->mMinorImageVersion, (uint64_t)EXE->Fmt.Opt.Pe32->mMajorSubsystemVersion, 
			(uint64_t)EXE->Fmt.Opt.Pe32->mMinorSubsystemVersion, (uint64_t)EXE->Fmt.Opt.Pe32->mWin32VersionValue, 
			(uint64_t)EXE->Fmt.Opt.Pe32->mSizeOfImage, (uint64_t)EXE->Fmt.Opt.Pe32->mSizeOfHeaders, 
			(uint64_t)EXE->Fmt.Opt.Pe32->mCheckSum, SubsystemString, DllCharacteristicsString, 
			(uint64_t)EXE->Fmt.Opt.Pe32->mSizeOfStackReserve, (uint64_t)EXE->Fmt.Opt.Pe32->mSizeOfStackCommit, 
			(uint64_t)EXE->Fmt.Opt.Pe32->mSizeOfHeapReserve, (uint64_t)EXE->Fmt.Opt.Pe32->mSizeOfHeapCommit, 
			(uint64_t)EXE->Fmt.Opt.Pe32->mLoaderFlags,(uint64_t)EXE->Fmt.Opt.Pe32->mNumberOfRvaAndSizes);
	}else if(EXE->Fmt.Opt.Pe32->mMagic == Pe32Plus){
		char *DllCharacteristicsString = PeDllCharacteristicsToString((uint64_t)EXE->Fmt.Opt.Pe32Plus->mDllCharacteristics), 
			*SubsystemString = PeSubsystemToString((uint64_t)EXE->Fmt.Opt.Pe32Plus->mSubsystem);
		if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
			fprintf(f, "\n\tOptional(32-bit): {"
				"\n    .Magic\t%s"
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
				"\n    .Subsystem\t%s"
				"\n    .Characteristics\t%s"
				"\n    .ReservedStackSize\t%llu"
				"\n    .CommitStackSize\t%llu"
				"\n    .ReservedHeapSize\t%llu"
				"\n    .CommitHeapSize\t%llu"
				"\n    .LoaderFlags\t%llu"
				"\n    .NRVAsAndSize\t%llu", MAKESTR(Pe32Plus), (uint64_t)EXE->Fmt.Opt.Pe32Plus->mMajorLinkerVersion, 
				(uint64_t)EXE->Fmt.Opt.Pe32Plus->mMinorLinkerVersion, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mSizeOfCode, 
				(uint64_t)EXE->Fmt.Opt.Pe32Plus->mSizeOfInitializedData, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mSizeOfUninitializedData, 
				(uint64_t)EXE->Fmt.Opt.Pe32Plus->mAddressOfEntryPoint, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mBaseOfCode, 
				(uint64_t)EXE->Fmt.Opt.Pe32Plus->mImageBase, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mSectionAlignment, 
				(uint64_t)EXE->Fmt.Opt.Pe32Plus->mFileAlignment, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mMajorOperatingSystemVersion, 
				(uint64_t)EXE->Fmt.Opt.Pe32Plus->mMinorOperatingSystemVersion, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mMajorImageVersion, 
				(uint64_t)EXE->Fmt.Opt.Pe32Plus->mMinorImageVersion, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mMajorSubsystemVersion, 
				(uint64_t)EXE->Fmt.Opt.Pe32Plus->mMinorSubsystemVersion, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mWin32VersionValue, 
				(uint64_t)EXE->Fmt.Opt.Pe32Plus->mSizeOfImage, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mSizeOfHeaders, 
				(uint64_t)EXE->Fmt.Opt.Pe32Plus->mCheckSum, SubsystemString, DllCharacteristicsString, 
				(uint64_t)EXE->Fmt.Opt.Pe32Plus->mSizeOfStackReserve, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mSizeOfStackCommit, 
				(uint64_t)EXE->Fmt.Opt.Pe32Plus->mSizeOfHeapReserve, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mSizeOfHeapCommit, 
				(uint64_t)EXE->Fmt.Opt.Pe32Plus->mLoaderFlags,(uint64_t)EXE->Fmt.Opt.Pe32Plus->mNumberOfRvaAndSizes);
			fclose(f);
		}printf(
			"\n\tOptional(32-bit): {"
			"\n    .Magic\t%s"
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
			"\n    .Subsystem\t%s"
			"\n    .Characteristics\t%s"
			"\n    .ReservedStackSize\t%llu"
			"\n    .CommitStackSize\t%llu"
			"\n    .ReservedHeapSize\t%llu"
			"\n    .CommitHeapSize\t%llu"
			"\n    .LoaderFlags\t%llu"
			"\n    .NRVAsAndSize\t%llu", MAKESTR(Pe32Plus), (uint64_t)EXE->Fmt.Opt.Pe32Plus->mMajorLinkerVersion, 
			(uint64_t)EXE->Fmt.Opt.Pe32Plus->mMinorLinkerVersion, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mSizeOfCode, 
			(uint64_t)EXE->Fmt.Opt.Pe32Plus->mSizeOfInitializedData, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mSizeOfUninitializedData, 
			(uint64_t)EXE->Fmt.Opt.Pe32Plus->mAddressOfEntryPoint, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mBaseOfCode, 
			(uint64_t)EXE->Fmt.Opt.Pe32Plus->mImageBase, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mSectionAlignment, 
			(uint64_t)EXE->Fmt.Opt.Pe32Plus->mFileAlignment, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mMajorOperatingSystemVersion, 
			(uint64_t)EXE->Fmt.Opt.Pe32Plus->mMinorOperatingSystemVersion, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mMajorImageVersion, 
			(uint64_t)EXE->Fmt.Opt.Pe32Plus->mMinorImageVersion, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mMajorSubsystemVersion, 
			(uint64_t)EXE->Fmt.Opt.Pe32Plus->mMinorSubsystemVersion, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mWin32VersionValue, 
			(uint64_t)EXE->Fmt.Opt.Pe32Plus->mSizeOfImage, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mSizeOfHeaders, 
			(uint64_t)EXE->Fmt.Opt.Pe32Plus->mCheckSum, SubsystemString, DllCharacteristicsString, 
			(uint64_t)EXE->Fmt.Opt.Pe32Plus->mSizeOfStackReserve, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mSizeOfStackCommit, 
			(uint64_t)EXE->Fmt.Opt.Pe32Plus->mSizeOfHeapReserve, (uint64_t)EXE->Fmt.Opt.Pe32Plus->mSizeOfHeapCommit, 
			(uint64_t)EXE->Fmt.Opt.Pe32Plus->mLoaderFlags,(uint64_t)EXE->Fmt.Opt.Pe32Plus->mNumberOfRvaAndSizes);
	}
	for(uint32_t cc = 0; cc < (EXE->Fmt.Opt.Pe32->mMagic == Pe32? EXE->Fmt.Opt.Pe32->mNumberOfRvaAndSizes: EXE->Fmt.Opt.Pe32Plus->mNumberOfRvaAndSizes); ++cc){
		char *SectionCharacteristicsString = PeSectionCharacteristicsToString(EXE->Fmt.SectionTable[cc].mCharacteristics);
		if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
			fprintf(f, "\n    [%llu]: {"
				"\n      .RVA\t%llu"
				"\n      .Size\t%llu"
				"\n      {"
				"\n        .Name\t%.8s"
				"\n        .VirtualSize\t%llu"
				"\n        .VirtualAddress\t%llu"
				"\n        .RawSize\t%llu"
				"\n        .RawPointer\t0x%p"
				"\n        .RelocationTablePointer\t0x%p"
				"\n        .LineNumberPointer\t0x%p"
				"\n        .Characteristics\t%s"
				"\n      }", (uint64_t)cc, 
				(uint64_t)EXE->Fmt.RVAs[cc].RVA, (uint64_t)EXE->Fmt.RVAs[cc].Size, 
				EXE->Fmt.SectionTable[cc].mName, (uint64_t)EXE->Fmt.SectionTable[cc].mVirtualSize, 
				(uint64_t)EXE->Fmt.SectionTable[cc].mVirtualAddress, (uint64_t)EXE->Fmt.SectionTable[cc].mSizeOfRawData, 
				(uint64_t)EXE->Fmt.SectionTable[cc].mPointerToRawData, (uint64_t)EXE->Fmt.SectionTable[cc].mPointerToRelocations, 
				(uint64_t)EXE->Fmt.SectionTable[cc].mPointerToLinenumbers, SectionCharacteristicsString);
			fclose(f);
		}printf(
			"\n    [%llu]: {"
			"\n      .RVA\t%llu"
			"\n      .Size\t%llu"
			"\n      {"
			"\n        .Name\t%.8s"
			"\n        .VirtualSize\t%llu"
			"\n        .VirtualAddress\t%llu"
			"\n        .RawSize\t%llu"
			"\n        .RawPointer\t0x%p"
			"\n        .RelocationTablePointer\t0x%p"
			"\n        .LineNumberPointer\t0x%p"
			"\n        .Characteristics\t%s"
			"\n      }", (uint64_t)cc, 
			(uint64_t)EXE->Fmt.RVAs[cc].RVA, (uint64_t)EXE->Fmt.RVAs[cc].Size, 
			EXE->Fmt.SectionTable[cc].mName, (uint64_t)EXE->Fmt.SectionTable[cc].mVirtualSize, 
			(uint64_t)EXE->Fmt.SectionTable[cc].mVirtualAddress, (uint64_t)EXE->Fmt.SectionTable[cc].mSizeOfRawData, 
			(uint64_t)EXE->Fmt.SectionTable[cc].mPointerToRawData, (uint64_t)EXE->Fmt.SectionTable[cc].mPointerToRelocations, 
			(uint64_t)EXE->Fmt.SectionTable[cc].mPointerToLinenumbers, SectionCharacteristicsString);
		free(SectionCharacteristicsString);
		if(!strncmp(EXE->Fmt.SectionTable[cc].mName, PeExportSection, 8)){
			if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
				fprintf(f, "\n      {\n        .NExports\t%llu", (uint64_t)EXE->Fmt.exp.nExports);
				fclose(f);
			}printf("\n      {\n        .NExports\t%llu", (uint64_t)EXE->Fmt.exp.nExports);
			for(uint32_t cc_ = 0; cc_ < EXE->Fmt.exp.nExports; ++cc_){
				if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
					fprintf(f, "\n        [%llu]: {"
						"\n          .Flags\t%llu"
						"\n          .Time/DateStamp\t%llu"
						"\n          .Version  [%llu:%llu]"
						"\n          .NameRVA\t%llu(%s)"
						"\n          .OrdinalBase\t%llu"
						"\n          .NTableEntries\t%llu"
						"\n          .NNamePointers\t%llu"
						"\n          .ExportTableRVA\t%llu"
						"\n          .NamePointerRVA\t%llu"
						"\n          .OrdinalPointerRVA\t%llu"
						"\n          {", (uint64_t)cc_, 
						(uint64_t)EXE->Fmt.exp.exportEntries[cc_].mExportFlags, (uint64_t)EXE->Fmt.exp.exportEntries[cc_].mVersionMajor, 
						(uint64_t)EXE->Fmt.exp.exportEntries[cc_].mVersionMinor, (uint64_t)EXE->Fmt.exp.exportEntries[cc_].NameRVA, 
						GetAtRVAFromSectionDataPe(EXE->Fmt.exp.exportEntries[cc_].NameRVA, PeExportSection, EXE->Fmt.exp.Raw, EXE->Raw), 
						(uint64_t)EXE->Fmt.exp.exportEntries[cc_].mOrdinalBase, (uint64_t)EXE->Fmt.exp.exportEntries[cc_].mNTableEntries, 
						(uint64_t)EXE->Fmt.exp.exportEntries[cc_].mNNamePointers, (uint64_t)EXE->Fmt.exp.exportEntries[cc_].mExportTableRVA, 
						(uint64_t)EXE->Fmt.exp.exportEntries[cc_].mNamePointerRVA, (uint64_t)EXE->Fmt.exp.exportEntries[cc_].OrdinalPointerRVA);
					fclose(f);
				}printf(
					"\n        [%llu]: {"
					"\n          .Flags\t%llu"
					"\n          .Time/DateStamp\t%llu"
					"\n          .Version  [%llu:%llu]"
					"\n          .NameRVA\t%llu(%s)"
					"\n          .OrdinalBase\t%llu"
					"\n          .NTableEntries\t%llu"
					"\n          .NNamePointers\t%llu"
					"\n          .ExportTableRVA\t%llu"
					"\n          .NamePointerRVA\t%llu"
					"\n          .OrdinalPointerRVA\t%llu"
					"\n          {", (uint64_t)cc_, 
					(uint64_t)EXE->Fmt.exp.exportEntries[cc_].mExportFlags, (uint64_t)EXE->Fmt.exp.exportEntries[cc_].mVersionMajor, 
					(uint64_t)EXE->Fmt.exp.exportEntries[cc_].mVersionMinor, (uint64_t)EXE->Fmt.exp.exportEntries[cc_].NameRVA, 
					GetAtRVAFromSectionDataPe(EXE->Fmt.exp.exportEntries[cc_].NameRVA, PeExportSection, EXE->Fmt.exp.Raw, EXE->Raw), 
					(uint64_t)EXE->Fmt.exp.exportEntries[cc_].mOrdinalBase, (uint64_t)EXE->Fmt.exp.exportEntries[cc_].mNTableEntries, 
					(uint64_t)EXE->Fmt.exp.exportEntries[cc_].mNNamePointers, (uint64_t)EXE->Fmt.exp.exportEntries[cc_].mExportTableRVA, 
					(uint64_t)EXE->Fmt.exp.exportEntries[cc_].mNamePointerRVA, (uint64_t)EXE->Fmt.exp.exportEntries[cc_].OrdinalPointerRVA);
				uint32_t *NamePointers = GetAtRVAFromSectionDataPe(EXE->Fmt.exp.exportEntries[cc_].mNamePointerRVA, PeExportSection, EXE->Fmt.exp.Raw, EXE->Raw);
				for(uint32_t cc__ = 0; cc__ < __min(PeDumpVolume, EXE->Fmt.exp.exportEntries[cc_].mNNamePointers); ++cc__){
					if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
						fprintf(f, "\n            \"%s\"", GetAtRVAFromSectionDataPe(NamePointers[cc__], PeExportSection, EXE->Fmt.exp.Raw, EXE->Raw));
						fclose(f);
					}printf("\n            \"%s\"", GetAtRVAFromSectionDataPe(NamePointers[cc__], PeExportSection, EXE->Fmt.exp.Raw, EXE->Raw));
				}
				if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
					fprintf(f, "\n          }\n        }");
					fclose(f);
				}printf("\n          }\n        }");
			}
		}else if(!strncmp(EXE->Fmt.SectionTable[cc].mName, PeImportSection, 8)){
			if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
				fprintf(f, "\n      {\n        .NImports\t%llu", (uint64_t)EXE->Fmt.imp.nImports);
				fclose(f);
			}printf("\n      {\n        .NImports\t%llu", (uint64_t)EXE->Fmt.imp.nImports);
			for(uint32_t cc_ = 0; cc_ < EXE->Fmt.imp.nImports; ++cc_){
				if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
					fprintf(f, "\n        [%llu]: {"
						"\n          .NEntries\t%llu"
						"\n          .ImportLookupTableRVA\t%llu"
						"\n          .Time/DateStamp\t%llu"

						"\n          .ForwarderChainFirstIndex\t%llu"
						"\n          .NameRVA\t%llu(%s)"
						"\n          .ImportAddressTableRVA\t%llu", 
						(uint64_t)cc_, (uint64_t)EXE->Fmt.imp.nEntries[cc_], (uint64_t)EXE->Fmt.imp.imports[cc_].ImportLookupTableRVA, 
						(uint64_t)EXE->Fmt.imp.imports[cc_].TimeDateStamp, (uint64_t)EXE->Fmt.imp.imports[cc_].ForwarderChain, 
						(uint64_t)EXE->Fmt.imp.imports[cc_].NameRVA, 
						GetAtRVAFromSectionDataPe(EXE->Fmt.imp.imports[cc_].NameRVA, PeImportSection, EXE->Fmt.imp.Raw, EXE->Raw), 
						(uint64_t)EXE->Fmt.imp.imports[cc_].ImportAddressTableRVA);
					fclose(f);
				}printf(
					"\n        [%llu]: {"
					"\n          .NEntries\t%llu"
					"\n          .ImportLookupTableRVA\t%llu"
					"\n          .Time/DateStamp\t%llu"

					"\n          .ForwarderChainFirstIndex\t%llu"
					"\n          .NameRVA\t%llu(%s)"
					"\n          .ImportAddressTableRVA\t%llu", 
					(uint64_t)cc_, (uint64_t)EXE->Fmt.imp.nEntries[cc_], (uint64_t)EXE->Fmt.imp.imports[cc_].ImportLookupTableRVA, 
					(uint64_t)EXE->Fmt.imp.imports[cc_].TimeDateStamp, (uint64_t)EXE->Fmt.imp.imports[cc_].ForwarderChain, 
					(uint64_t)EXE->Fmt.imp.imports[cc_].NameRVA, 
					GetAtRVAFromSectionDataPe(EXE->Fmt.imp.imports[cc_].NameRVA, PeImportSection, EXE->Fmt.imp.Raw, EXE->Raw), 
					(uint64_t)EXE->Fmt.imp.imports[cc_].ImportAddressTableRVA);
				if(EXE->Fmt.Opt.Pe32->mMagic == Pe32){
					PeImportLookupEntry32 *Table = GetAtRVAFromSectionDataPe(EXE->Fmt.imp.imports[cc_].ImportLookupTableRVA, PeImportSection, EXE->Fmt.imp.Raw, EXE->Raw);
					for(uint32_t cc__ = 0; cc__ < __min(EXE->Fmt.imp.nEntries[cc_], PeDumpVolume); ++cc__){
						if(Table[cc__].Bits.ImportByOrdinal){
							if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
								fprintf(f, "\n            \"#%llu\"", (uint64_t)Table[cc__].Bits.OrdinalNumberOrNameRVA);
								fclose(f);
							}printf("\n            \"#%llu\"", (uint64_t)Table[cc__].Bits.OrdinalNumberOrNameRVA);
						}else{
							PeImportHintEntry *Name = GetAtRVAFromSectionDataPe(Table[cc__].Bits.OrdinalNumberOrNameRVA, PeImportSection, EXE->Fmt.imp.Raw, EXE->Raw);
							if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
								fprintf(f, "\n            \"{%llu:%s}\"", (uint64_t)Name->Hint, Name->Name);
								fclose(f);
							}printf("\n            \"{%llu:%s}\"", (uint64_t)Name->Hint, Name->Name);
						}
					}
				}else if(EXE->Fmt.Opt.Pe32->mMagic == Pe32Plus){
					PeImportLookupEntry64 *Table = GetAtRVAFromSectionDataPe(EXE->Fmt.imp.imports[cc_].ImportLookupTableRVA, PeImportSection, EXE->Fmt.imp.Raw, EXE->Raw);
					for(uint32_t cc__ = 0; cc__ < __min(EXE->Fmt.imp.nEntries[cc_], PeDumpVolume); ++cc__){
						if(Table[cc__].Bits.ImportByOrdinal){
							if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
								fprintf(f, "\n            \"#%llu\"", (uint64_t)Table[cc__].Bits.OrdinalNumberOrNameRVA);
								fclose(f);
							}printf("\n            \"#%llu\"", (uint64_t)Table[cc__].Bits.OrdinalNumberOrNameRVA);
						}else{
							PeImportHintEntry *Name = GetAtRVAFromSectionDataPe(Table[cc__].Bits.OrdinalNumberOrNameRVA, PeImportSection, EXE->Fmt.imp.Raw, EXE->Raw);
							if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
								fprintf(f, "\n            \"{%llu:%s}\"", (uint64_t)Name->Hint, Name->Name);
								fclose(f);
							}printf("\n            \"{%llu:%s}\"", (uint64_t)Name->Hint, Name->Name);
						}
					}
				}
			}
		}else if(!strncmp(EXE->Fmt.SectionTable[cc].mName, PeExceptionInfoSection, 8)){
			void *Data = ReadSectionPe(EXE->Path, EXE->Raw, PeExceptionInfoSection);
			PeImageSectionHeader *This = FindSectionPe(EXE->Raw, PeExceptionInfoSection);
			uint64_t N = 0;
			switch(EXE->Fmt.Header->mMachine){
				case PeMachineType_R3000BE:
				case PeMachineType_R3000: {
					Pe32MIPSExceptionDataEntry *Table = (Pe32MIPSExceptionDataEntry *)Data;
					N = This->mSizeOfRawData / sizeof(Pe32MIPSExceptionDataEntry);
					if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
						fprintf(f, "\n      {\n        .N\t%llu\n        MachineType\t%s", N, PeMachineTypeToString(EXE->Fmt.Header->mMachine));
						fclose(f);
					}printf("\n      {\n        .N\t%llu\n        MachineType\t%s", N, PeMachineTypeToString(EXE->Fmt.Header->mMachine));
					for(GenericLengthType cc_ = 0; cc_ < __min(N, PeDumpVolume); ++cc_){
						if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
							fprintf(f, "\n        [%llu]: {"
								"\n          .VirtualAddress\t%llu"
								"\n          .VirtualEnd\t%llu"
								"\n          .VirtualHandler\t%llu", 
								"\n          .HandlerDataPointer\t%llu"
								"\n          .VirtualPrologAddress\t%llu", (uint64_t)cc_, 
								(uint64_t)(Table[cc_].mVirtualAddress), (uint64_t)(Table[cc_].mVirtualEnd), 
								(uint64_t)(Table[cc_].mHandler), (uint64_t)(Table[cc_].mHandlerData), 
								(uint64_t)(Table[cc_].mVirtualPrologAddress));
							fclose(f);
						}printf(
							"\n        [%llu]: {"
							"\n          .VirtualAddress\t%llu"
							"\n          .VirtualEnd\t%llu"
							"\n          .VirtualHandler\t%llu", 
							"\n          .HandlerDataPointer\t%llu"
							"\n          .VirtualPrologAddress\t%llu", (uint64_t)cc_, 
							(uint64_t)(Table[cc_].mVirtualAddress), (uint64_t)(Table[cc_].mVirtualEnd), 
							(uint64_t)(Table[cc_].mHandler), (uint64_t)(Table[cc_].mHandlerData), 
							(uint64_t)(Table[cc_].mVirtualPrologAddress));
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
					if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
						fprintf(f, "\n      {\n        .N\t%llu\n        MachineType\t%s", N, PeMachineTypeToString(EXE->Fmt.Header->mMachine));
						fclose(f);
					}printf("\n      {\n        .N\t%llu\n        MachineType\t%s", N, PeMachineTypeToString(EXE->Fmt.Header->mMachine));
					for(GenericLengthType cc_ = 0; cc_ < __min(N, PeDumpVolume); ++cc_){
						if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
							fprintf(f, "\n        [%llu]  (32-Bit? %s\tHasHandler? %s): {"
								"\n          .VirtualAddress\t%llu"
								"\n          .PrologLength\t%llu"
								"\n          .FunctionLength\t%llu", (uint64_t)cc_, 
								(Table[cc_].mIs32Bit? "TRUE": "FALSE"), (Table[cc_].mHasHandler? "TRUE": "FALSE"), 
								(uint64_t)(Table[cc_].mVirtualAddress), (uint64_t)(Table[cc_].mPrologLength), 
								(uint64_t)(Table[cc_].mFunctionLength));
							fclose(f);
						}printf(
							"\n        [%llu]  (32-Bit? %s\tHasHandler? %s): {"
							"\n          .VirtualAddress\t%llu"
							"\n          .PrologLength\t%llu"
							"\n          .FunctionLength\t%llu", (uint64_t)cc_, 
							(Table[cc_].mIs32Bit? "TRUE": "FALSE"), (Table[cc_].mHasHandler? "TRUE": "FALSE"), 
							(uint64_t)(Table[cc_].mVirtualAddress), (uint64_t)(Table[cc_].mPrologLength), 
							(uint64_t)(Table[cc_].mFunctionLength));
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
					if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
						fprintf(f, "\n      {\n        .N\t%llu\n        MachineType\t%s", N, PeMachineTypeToString(EXE->Fmt.Header->mMachine));
						fclose(f);
					}printf("\n      {\n        .N\t%llu\n        MachineType\t%s", N, PeMachineTypeToString(EXE->Fmt.Header->mMachine));
					for(GenericLengthType cc_ = 0; cc_ < __min(N, PeDumpVolume); ++cc_){
						if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
							fprintf(f, "\n        [%llu]: {"
								"\n          .AddressRVA\t%llu"
								"\n          .EndRVA\t%llu"
								"\n          .UnwindRVA\t%llu", (uint64_t)cc_, 
								(uint64_t)(Table[cc_].mAddressRVA), (uint64_t)(Table[cc_].mEndRVA), 
								(uint64_t)(Table[cc_].mUnwindRVA));
							fclose(f);
						}printf(
							"\n        [%llu]: {"
							"\n          .AddressRVA\t%llu"
							"\n          .EndRVA\t%llu"
							"\n          .UnwindRVA\t%llu", (uint64_t)cc_, 
							(uint64_t)(Table[cc_].mAddressRVA), (uint64_t)(Table[cc_].mEndRVA), 
							(uint64_t)(Table[cc_].mUnwindRVA));
					}
				}
			}
			if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
				fprintf(f, "\n          }\n        }");
				fclose(f);
			}printf("\n          }\n        }");
			free(Data);
		}else{
			void *Data = ReadSectionPe(EXE->Path, EXE->Raw, EXE->Fmt.SectionTable[cc].mName);
			PeImageSectionHeader *This = FindSectionPe(EXE->Raw, EXE->Fmt.SectionTable[cc].mName);
			if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
				fprintf(f, "\n      {");
				fclose(f);
			}printf("\n      {");
			if(Data){
				for(uint32_t cc_ = 0; cc_ < (PeDumpVolume / PeDumpVolumeLine); ++cc_){
					if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
						fprintf(f, "\n        ");
						fclose(f);
					}printf("\n        ");
					for(uint32_t cc__ = 0; cc__ < PeDumpVolumeLine && ((cc_ * PeDumpVolumeLine) + cc__) < This->mSizeOfRawData; ++cc__){
						if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
							fprintf(f, "%02x ", ((uint8_t *)Data)[cc_ + (cc__ * (PeDumpVolume / PeDumpVolumeLine))]);
							fclose(f);
						}printf("%02x ", ((uint8_t *)Data)[cc_ + (cc__ * (PeDumpVolume / PeDumpVolumeLine))]);}
				}
			}
			if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
				fprintf(f, "\n      }");
				fclose(f);
			}printf("\n      }");
			free(Data);
		}
		if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
			fprintf(f, "\n    }");
			fclose(f);
		}printf("\n    }");
	}
}