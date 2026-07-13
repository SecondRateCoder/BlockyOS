#include "pe.h"
char *GenericError(char *str, uint32_t val);

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
	if(!out){
		fclose(f);
		return NULL;
	}
	if(fread(out, sizeof(PeHeader), 1, f) != 1){
		free(out);
		fclose(f); // Clean up file
		GenericError("Could not read enough Bytes from Executable", 1);
		return NULL;
	}
	if((((PeHeader *)out)->mMagic[0] != 'P') || (((PeHeader *)out)->mMagic[1] != 'E') || 
		(((PeHeader *)out)->mMagic[2] != '\0') || (((PeHeader *)out)->mMagic[3] != '\0')
	){
		free(out);
		fclose(f); // Clean up file
		GenericError("Executable has a Malformed PE Magic", 2);
		return NULL;
	}
	bool switchSuccess = false;
	switch(((PeHeader *)out)->mSizeOfOptionalHeader){
		case sizeof(Pe32OptionalHeader) + (sizeof(PeRVAnSize) * PeDefaultNDataDirectories): {
			if(fread(out + sizeof(PeHeader), sizeof(Pe32OptionalHeader), 1, f) != 1){
				GenericError("Failed to read Pe32 Optional Header", 3);
			}else if(((Pe32OptionalHeader *)(out + sizeof(PeHeader)))->mMagic != Pe32){
				GenericError("Malformed Pe32 Optional Header Magic", 5);
			}else{
				outputSize = sizeof(PeHeader) + sizeof(Pe32OptionalHeader) + 
					(((Pe32OptionalHeader *)(out + sizeof(PeHeader)))->mNumberOfRvaAndSizes * sizeof(PeRVAnSize));
				out = realloc(out, outputSize);
				if (fread(out + sizeof(PeHeader) + sizeof(Pe32OptionalHeader), 
					sizeof(PeRVAnSize), ((Pe32OptionalHeader *)(out + sizeof(PeHeader)))->mNumberOfRvaAndSizes, f
					) != ((Pe32OptionalHeader *)(out + sizeof(PeHeader)))->mNumberOfRvaAndSizes
				){GenericError("Could not read Pe32 Data Directories", 4);}else{switchSuccess = true;}
			}
			break; // <--- Properly exits the PE32 switch case
		} case sizeof(Pe32PlusOptionalHeader) + (sizeof(PeRVAnSize) * PeDefaultNDataDirectories): {
			if(fread(out + sizeof(PeHeader), sizeof(Pe32PlusOptionalHeader), 1, f) != 1){
				GenericError("Failed to read Pe32 Plus Optional Header", 6);
			}else if(((Pe32PlusOptionalHeader *)(out + sizeof(PeHeader)))->mMagic != Pe32Plus){
				GenericError("Malformed Pe32 Plus Optional Header Magic", 8);
			}else{
				outputSize = sizeof(PeHeader) + sizeof(Pe32PlusOptionalHeader) + 
					(((Pe32PlusOptionalHeader *)(out + sizeof(PeHeader)))->mNumberOfRvaAndSizes * sizeof(PeRVAnSize));
				out = realloc(out, outputSize);
				if(fread(out + sizeof(PeHeader) + sizeof(Pe32PlusOptionalHeader), 
					sizeof(PeRVAnSize), ((Pe32PlusOptionalHeader *)(out + sizeof(PeHeader)))->mNumberOfRvaAndSizes, f
					) != ((Pe32PlusOptionalHeader *)(out + sizeof(PeHeader)))->mNumberOfRvaAndSizes
				){GenericError("Could not read Pe32 Plus Data Directories", 7);}else{switchSuccess = true;}
			}
			break; // <--- Properly exits the PE32+ switch case
		} default: {
			GenericError("Unsupported or unexpected Optional Header size", 10);
			break;
		}
	}
	if(!switchSuccess){
		if(out){free(out);}
		fclose(f);
		return NULL;
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
		GenericError("Failed to Read Pe32 Section Headers", 9);
		return NULL;
	}
	fclose(f);
	return out;
}

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
	for(uint32_t cc = 0; cc < (POH->mMagic == Pe32? POH->mNumberOfRvaAndSizes: POHPlus->mNumberOfRvaAndSizes); ++cc){
		if(!strncmp(PISHs[cc].mName, name, strlen(PISHs[cc].mName))){return (PISHs + cc);}
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

void *ReadSectionPe(char *path, void *header, char name[8]){
	FILE *f = NULL;
	if((f = fopen(path, "rb"))){
		DecodePeExecutableHeader(header);
		PeImageSectionHeader *section = FindSectionPe(header, name);
		if(section){
			// uint32_t Address = RvaToFileOffsetPe(0, section);
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
					GenericError("Failed to Read Section, Failed to read all bytes", 10);
					return NULL;
				}
			}else{
				fclose(f);
				GenericError("Failed to Read Section, Section lacks Size on Disk", 10);
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

	PeExportDirectoryEntry *exports = ReadSectionPe(path, header, ".edata");
	uint32_t nExports = 0;
	if(exports){while(memcmp(exports + nExports, (uint8_t[sizeof(PeExportDirectoryEntry)]){0}, sizeof(PeExportDirectoryEntry))){nExports++;}	nExports++;}
	// These are RVA Pointers to Various Names in the Export Name Table.
	uint32_t *NamePointerRVAs = exports? GetAtRVAFromSectionDataPe(exports->mNamePointerRVA, ".edata", exports, header): NULL;
	//	Indices of Various Addresses Exported in the Export Directory Table.
	//	Subtract the Ordinal Base from the Digit.
	uint16_t *OrdinalPointerRVAs = exports? GetAtRVAFromSectionDataPe(exports->OrdinalPointerRVA, ".edata", exports, header): NULL;
	if(exports){for(uint32_t cc = 0; cc < exports->mNNamePointers; ++cc){OrdinalPointerRVAs[cc] -= exports->OrdinalBase;}}
	PeExportAddressEntry *exportAddressTable = exports? GetAtRVAFromSectionDataPe(exports->mExportTableRVA, ".edata", exports, header): NULL;

	PeImportDirectoryEntry *imports = ReadSectionPe(path, header, ".idata");
	uint32_t nImports = 0;
	if(imports){while(memcmp(imports + nImports, (uint8_t[sizeof(PeImportDirectoryEntry)]){0}, sizeof(PeImportDirectoryEntry))){++nImports;}}
	char **importName = imports? calloc(sizeof(char *), nImports): NULL;
	PeImportLookupEntry32 **ImportLookups32 = imports? calloc(sizeof(PeImportLookupEntry32 *), nImports): NULL;
	PeImportAddressEntry32 **ImportAddresses32 = imports? calloc(sizeof(PeImportLookupEntry32 *), nImports): NULL;
	PeImportLookupEntry64 **ImportLookups64 = (PeImportLookupEntry64 **)ImportLookups32;
	PeImportAddressEntry64 **ImportAddresses64 = (PeImportAddressEntry64 **)ImportAddresses32;
	if(imports){
		for(uint32_t cc = 0; cc < nImports; ++cc){
			ImportLookups32[cc] = GetAtRVAFromSectionDataPe(imports[cc].ImportLookupTableRVA, ".idata", imports, header);
			ImportAddresses64[cc] = GetAtRVAFromSectionDataPe(imports[cc].ImportAddressTableRVA, ".idata", imports, header);
			importName[cc] = GetAtRVAFromSectionDataPe(imports[cc].NameRVA, ".idata", imports, header);
		}
	}

	// We dont care for .pdata Information.

	// .reloc Info
	void *relocblock = ReadSectionPe(path, header, ".reloc");
	PeImageSectionHeader *relocHeader = relocblock? FindSectionPe(header, ".reloc"): NULL;
	uint32_t nRelocationBlocks = 0, byteOffset = 0, *nPerBlock = relocblock? calloc(sizeof(uint32_t), 5): NULL;
	PeBaseRelocationBlock *relocations = (PeBaseRelocationBlock *)(relocblock + byteOffset);
	if(relocblock){
		while((relocations->BlockSize != 0) && (relocations->PageRVA != 0)){
			if((nRelocationBlocks % 5) == 0){nPerBlock = realloc(nPerBlock, sizeof(uint32_t) * (nRelocationBlocks + 5));}
			relocations = (PeBaseRelocationBlock *)(relocblock + byteOffset);
			nPerBlock[nRelocationBlocks] = ((relocations->BlockSize - sizeof(PeBaseRelocationBlock)) / sizeof(PeRelocationEntry));
			byteOffset += relocations->BlockSize;
			nRelocationBlocks++;
		}
		nRelocationBlocks++;
	}

	*out = (ExpandedPeExecutable){
		.Raw = header,
		.Format = {
			.Header = PH,
			.Optional.Pe32Plus = POHPlus,
			.RVAs = RVAs,
			.SectionTable = PISHs,
			.exports = {
				.nExports = nExports,
				.exportEntries = exports,
				.NamePointerRVAs = NamePointerRVAs,
				.NormalisedOrdinalPointerRVAs = OrdinalPointerRVAs,
				.RawExportAddresses = exportAddressTable
			}, 
			.imports = {
				.imports = imports,
				.nImports = nImports,
				.perImport = {
					.Names = importName,
					.lookups.ImportLookups64 = ImportLookups64,
					.addresses.ImportAddresses64 = ImportAddresses64
				}
			},
			.reloc = {
				.data = relocblock,
				.nRelocationBlocks = nRelocationBlocks,
				.nRelocationEntriesPerBlock = nPerBlock
			}
		}
	};
	return out;
}