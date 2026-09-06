#include "convert.h"

void *memdup(void *mem, size_t n){
	void *out = calloc(1, n);
	memcpy(out, mem, n);
	return out;
}

BeSectionFlags ConvertPeSectionFlagsBe(PeSectionCharacteristics Characteristics){
	switch(Characteristics){
		textSectionCharacteristics:		return SDExecutableCode;
		dataSectionCharacteristics:		return SDReadWritableData;
		sdataSectionCharacteristics:
		sbssSectionCharacteristics:
		bssSectionCharacteristics:		return SDRandomStaticData;
		edataSectionCharacteristics:	return SDRelocExport;
		idataSectionCharacteristics:	return SDRelocImport;
		xdataSectionCharacteristics:
		pdataSectionCharacteristics:
		rsrcSectionCharacteristics:	
		rdataSectionCharacteristics:	return SDReadonlyData;
		relocSectionCharacteristics:	return SDDiscardableReadOnlyData;
		srdataSectionCharacteristics:	return SDRandomStaticReadonlyData;
		default: {
			BeSectionFlags out = 0;
			if(flagcheck(Characteristics, PeSectionCharacteristics_CODE) || 
				flagcheck(Characteristics, PeSectionCharacteristics_MEXECUTABLE)){flagset(out, SFExecutable);}
			if(flagcheck(Characteristics, PeSectionCharacteristics_INITDATA)){flagset(out, SFInitData);}
			if(flagcheck(Characteristics, PeSectionCharacteristics_UINITDATA)){flagset(out, SFUInitData);}
			if(flagcheck(Characteristics, PeSectionCharacteristics_DISCARDABLE)){flaguset(out, SFPersistent);}
			if(flagcheck(Characteristics, PeSectionCharacteristics_MSHARED)){flagset(out, SFSharable);}
			if(flagcheck(Characteristics, PeSectionCharacteristics_MREADABLE)){flagset(out, SFReadable);}
			if(flagcheck(Characteristics, PeSectionCharacteristics_MWRITABLE)){flagset(out, SFWritable);}
			return out;
		}
	}
}

uint32_t GetAlignmentBe(uint64_t Number){
	if((Number % 10) == 0){return 10;}else
	if((Number % 9) == 0){return 9;}else
	if((Number % 8) == 0){return 8;}else
	if((Number % 7) == 0){return 7;}else
	if((Number % 6) == 0){return 6;}else
	if((Number % 5) == 0){return 5;}else
	if((Number % 4) == 0){return 4;}else
	if((Number % 3) == 0){return 3;}else
	if((Number % 2) == 0){return 2;}else
	if((Number % 1) == 0){return 1;}else
	{return 0;}
}

#define AddRVA32	AddRVA64
static void AddRVA64(EATReferenceList *list, RelativeVirtualOffset rva){
	if (list->Count >= list->Capacity) {
		list->Capacity = (list->Capacity == 0) ? 16 : list->Capacity * 2;
		list->RVAs = realloc(list->RVAs, list->Capacity * sizeof(RelativeVirtualOffset));
	}
	list->RVAs[list->Count++] = rva;
}

/**
 * Finds code references to the Export Address Table (EAT) in a 32-bit PE binary.
 *
 * @param codeBytes         Pointer to raw bytes of the executable section (.text).
 * @param codeSize          Length of the code section in bytes.
 * @param codeSectionRVA    RVA where the code section starts in memory.
 * @param imageBase         Base load address of the PE image (e.g., 0x00400000).
 * @param eatRVA            RVA of the Export Address Table (AddressOfFunctions).
 * @return EATReferenceList List of RVAs inside the code section pointing to the EAT.
 */
EATReferenceList FindEATReferences32(
	const uint8_t *codeBytes, 
	uint64_t codeSize, 
	RelativeVirtualOffset codeSectionRVA, 
	uint32_t imageBase,
	RelativeVirtualOffset eatRVA
){
	EATReferenceList refs = {NULL, 0, 0};
	uint32_t targetAbsoluteAddress = imageBase + eatRVA;
	size_t offset = 0;

	while(offset < codeSize){
		// Look ahead to check if a 32-bit immediate matches our target address
		if(offset + 5 <= codeSize){
			uint8_t opcode = codeBytes[offset];
			bool isDirectMatch = false;
			size_t immOffset = 0;
			if(opcode == 0xA1 || opcode == 0x68){
				immOffset = offset + 1;
				isDirectMatch = true;
			}else if(opcode >= 0xB8 && opcode <= 0xBF){
				immOffset = offset + 1;
				isDirectMatch = true;
			}else if((opcode == 0x8B || opcode == 0x8D || opcode == 0x39 || opcode == 0x3B) && (offset + 6 <= codeSize)){
				uint8_t modrm = codeBytes[offset + 1];
				if((modrm & 0xC7) == 0x05){
					immOffset = offset + 2;
					isDirectMatch = true;
				}
			}
			if(isDirectMatch && (immOffset + 4 <= codeSize)){
				uint32_t candidateAddress = *(uint32_t *)&codeBytes[immOffset];
				if(candidateAddress == targetAbsoluteAddress){
					RelativeVirtualOffset currentInstrRVA = codeSectionRVA + (RelativeVirtualOffset)offset;
					AddRVA32(&refs, currentInstrRVA);
				}
			}
		}
		offset++;
	}
	return refs;
}
/**
 * Finds code references to the Export Address Table (EAT) in an x86_64 PE32+ image.
 *
 * @param codeBytes         Pointer to raw bytes of the executable section (e.g., .text).
 * @param codeSize          Length of the code section in bytes.
 * @param codeSectionRVA    RVA where the code section starts in memory.
 * @param eatRVA            RVA of the Export Address Table (AddressOfFunctions).
 * @return EATReferenceList List of RVAs inside the code section pointing to the EAT.
 */
EATReferenceList FindEATReferences64(
	const uint8_t *codeBytes, size_t codeSize, 
	RelativeVirtualOffset codeSectionRVA, 
	RelativeVirtualOffset eatRVA
){
	EATReferenceList refs = { NULL, 0, 0 };
	size_t offset = 0;
	while(offset < codeSize){
		// x86_64 REX Prefixes range from 0x40 to 0x4F
		uint8_t currentByte = codeBytes[offset];
		size_t prefixLen = 0;
		if(currentByte >= 0x40 && currentByte <= 0x4F){prefixLen = 1;}

		// Check for 3-byte instruction pattern: [REX] [Opcode: LEA (0x8D) / MOV (0x8B)] [ModR/M]
		if(offset + prefixLen + 5 < codeSize){
			uint8_t opcode = codeBytes[offset + prefixLen];
			uint8_t modrm = codeBytes[offset + prefixLen + 1];

			// 0x8D = LEA, 0x8B = MOV
			// ModR/M byte with Mod=00 and RM=101 indicates RIP-relative addressing in x86_64
			if((opcode == 0x8D || opcode == 0x8B) && (modrm & 0xC7) == 0x05){
				int32_t disp = *(int32_t *)&codeBytes[offset + prefixLen + 2];
				
				// Total length of instruction = Prefix + Opcode (1) + ModRM (1) + Disp32 (4)
				size_t instrLen = prefixLen + 6;
				RelativeVirtualOffset currentInstrRVA = codeSectionRVA + (RelativeVirtualOffset)offset;
				RelativeVirtualOffset nextInstrRVA = currentInstrRVA + (RelativeVirtualOffset)instrLen;
				RelativeVirtualOffset targetRVA = (RelativeVirtualOffset)((int64_t)nextInstrRVA + disp);
				if(targetRVA == eatRVA){AddRVA64(&refs, currentInstrRVA);}
				offset += instrLen;
				continue;
			}
		}
		offset++;
	}
	return refs;
}

#define AddIATRef	AddIATRef64
static void AddIATRef64(IATReferenceList *list, RelativeVirtualOffset rva){
	if(list->Count >= list->Capacity){
		list->Capacity = (list->Capacity == 0) ? 16 : list->Capacity * 2;
		list->RVAs = realloc(list->RVAs, list->Capacity * sizeof(RelativeVirtualOffset));
	}
	list->RVAs[list->Count++] = rva;
}

/**
 * Finds code references (CALL / JMP / MOV) to the Import Address Table (IAT) in x86_64 PE binaries.
 *
 * @param codeBytes         Pointer to the raw executable section (.text).
 * @param codeSize          Size of the code section in bytes.
 * @param codeSectionRVA    RVA where the code section starts.
 * @param iatStartRVA       RVA of the start of the IAT.
 * @param iatSize           Size of the IAT in bytes.
 * @return IATReferenceList List of instruction RVAs targeting the IAT.
 */
IATReferenceList FindIATReferences64(
	const uint8_t *codeBytes, 
	size_t codeSize, 
	RelativeVirtualOffset codeSectionRVA, 
	RelativeVirtualOffset iatStartRVA,
	uint32_t iatSize
){
	IATReferenceList refs = {NULL, 0, 0};
	size_t offset = 0;
	RelativeVirtualOffset iatEndRVA = iatStartRVA + iatSize;

	while(offset < codeSize){
		size_t prefixLen = (codeBytes[offset] >= 0x40 && codeBytes[offset] <= 0x4F) ? 1 : 0;
		if(offset + prefixLen + 5 < codeSize){
			uint8_t opcode = codeBytes[offset + prefixLen];
			uint8_t modrm  = codeBytes[offset + prefixLen + 1];
			bool isIndirectCallJmp = (opcode == 0xFF && (modrm == 0x15 || modrm == 0x25)), 
				isIndirectMov     = (opcode == 0x8B && (modrm & 0xC7) == 0x05);

			if(isIndirectCallJmp || isIndirectMov){
				int32_t disp = *(int32_t *)&codeBytes[offset + prefixLen + 2];
				size_t instrLen = prefixLen + 6;
				RelativeVirtualOffset currentInstrRVA = codeSectionRVA + (RelativeVirtualOffset)offset, 
										nextInstrRVA = currentInstrRVA + (RelativeVirtualOffset)instrLen, 
										targetRVA = (RelativeVirtualOffset)((int64_t)nextInstrRVA + disp);
				if(targetRVA >= iatStartRVA && targetRVA < iatEndRVA){AddIATRef64(&refs, currentInstrRVA);}
				offset += instrLen;
				continue;
			}
		}
		offset++;
	}
	return refs;
}
/**
 * Finds code references to the Import Address Table (IAT) in 32-bit PE binaries.
 *
 * @param codeBytes         Pointer to raw executable section bytes (.text).
 * @param codeSize          Size of the code section in bytes.
 * @param codeSectionRVA    RVA where the code section starts.
 * @param imageBase         Base load address of the PE image (e.g., 0x00400000).
 * @param iatStartRVA       RVA where the IAT begins.
 * @param iatSize           Total byte size of the IAT.
 * @return IATReferenceList List of instruction RVAs pointing into the IAT.
 */
IATReferenceList FindIATReferences32(
	const uint8_t *codeBytes, 
	size_t codeSize, 
	RelativeVirtualOffset codeSectionRVA, 
	uint32_t imageBase,
	RelativeVirtualOffset iatStartRVA,
	uint32_t iatSize
){
	IATReferenceList refs = { NULL, 0, 0 };
	uint32_t iatStartAbs = imageBase + iatStartRVA, 
			iatEndAbs   = iatStartAbs + iatSize;
	size_t offset = 0;
	while(offset < codeSize){
		if(offset + 5 <= codeSize){
			uint8_t opcode = codeBytes[offset];
			if(opcode == 0xFF && (offset + 6 <= codeSize)){
				uint8_t modrm = codeBytes[offset + 1];
				if(modrm == 0x15 || modrm == 0x25){
					uint32_t candidateAbs = *(uint32_t *)&codeBytes[offset + 2];
					if(candidateAbs >= iatStartAbs && candidateAbs < iatEndAbs){
						AddIATRef(&refs, codeSectionRVA + (RelativeVirtualOffset)offset);
					}
				}
			}else if(opcode == 0x8B && (offset + 6 <= codeSize)){
				uint8_t modrm = codeBytes[offset + 1];
				if((modrm & 0xC7) == 0x05){
					uint32_t candidateAbs = *(uint32_t *)&codeBytes[offset + 2];
					if(candidateAbs >= iatStartAbs && candidateAbs < iatEndAbs){
						AddIATRef(&refs, codeSectionRVA + (RelativeVirtualOffset)offset);
					}
				}
			}
		}
		offset++;
	}
	return refs;
}

// //	Convert the Pe-Executable Import Sections into Blocky-Executable Style Imports.
bool InitImportSection(
	char *path, ExpandedPeExecutable *Image, 
	SectionNameBe Name
){
	char **DLLPaths = calloc(Image->Fmt.imp.nImports, sizeof(char *)), ***ImportSymbols = calloc(Image->Fmt.imp.nImports, sizeof(char *));
	for(register uint32_t cc = 0; cc < Image->Fmt.imp.nImports; ++cc){
		DLLPaths[cc] = strdup(PoolGetPath(GetAtRVAFromSectionDataPe(Image->Fmt.imp.imports[cc].NameRVA, ".idata", Image->Fmt.imp.imports, Image->Raw)));
		ExpandedPeExecutable *DLLHeader = ExpandPeExecutableFormat(DLLPaths[cc]);
		//*	Foreach Lookup/Address.
		for(register uint32_t cc_ = 0; (Image->Fmt.Opt.Pe32->mMagic == Pe32? 
			Image->Fmt.imp.perImport.lookups.ImportLookups32[cc][cc_].Raw: 
			Image->Fmt.imp.perImport.lookups.ImportLookups64[cc][cc_].Raw
		) != 0; ++cc_){
			if((cc_ % 10) == 0){ImportSymbols[cc] = ImportSymbols[cc]? realloc(ImportSymbols[cc], (cc_ + 10) * sizeof(char **)): calloc(10, sizeof(char **));}
			uint32_t TargetOrdinal = 0;
			if(Image->Fmt.Opt.Pe32->mMagic == Pe32? 
				Image->Fmt.imp.perImport.lookups.ImportLookups32[cc][cc_].Bits.ImportByOrdinal: 
				Image->Fmt.imp.perImport.lookups.ImportLookups64[cc][cc_].Bits.ImportByOrdinal
			){
				uint16_t Ordinal = (uint16_t)((Image->Fmt.Opt.Pe32->mMagic == Pe32? 
					Image->Fmt.imp.perImport.lookups.ImportLookups32[cc][cc_].Bits.OrdinalNumberOrNameRVA: 
					Image->Fmt.imp.perImport.lookups.ImportLookups64[cc][cc_].Bits.OrdinalNumberOrNameRVA
				) & 0xFFFF);
				//	Search for the Ordinal ID in Exports.
				// We always Normalise the Ordinal IDs so this should all work Fine.
				//	Foreach Export recover the Original Imported Name.
				uint32_t OrdinalIndex;
				for(uint32_t exportI = 0; exportI < DLLHeader->Fmt.exp.nExports; ++exportI){
					OrdinalIndex = 0;
					for(; OrdinalIndex < DLLHeader->Fmt.exp.exportEntries[exportI].mNNamePointers; ++OrdinalIndex){
						if(DLLHeader->Fmt.exp.NormalisedOrdinals[OrdinalIndex] == (Ordinal - DLLHeader->Fmt.exp.exportEntries[exportI].mOrdinalBase)){
							break;
						}
					}
				}
				ImportSymbols[cc][cc_] = strdup(GetAtRVAFromSectionDataPe(DLLHeader->Fmt.exp.NamePointerRVAs[OrdinalIndex], PeExportSection, 
					DLLHeader->Fmt.exp.exportEntries, DLLHeader->Raw
				));
			}else{
				PeImportNameEntry *HintNameBlock = (PeImportNameEntry *)GetAtRVAFromSectionDataPe(
					(Image->Fmt.Opt.Pe32->mMagic == Pe32? 
						Image->Fmt.imp.perImport.lookups.ImportLookups32[cc][cc_].Bits.OrdinalNumberOrNameRVA: 
						Image->Fmt.imp.perImport.lookups.ImportLookups64[cc][cc_].Bits.OrdinalNumberOrNameRVA
					), PeImportSection, Image->Fmt.imp.imports, Image->Raw
				);
				if(HintNameBlock){ImportSymbols[cc][cc_] = strdup(HintNameBlock->Name);}
			}
		}
		free(DLLHeader);
	}
	//*	Generate the Imports, we need to create one for each Item, then Update the List to accomodate it
	void *Text = ReadSectionPe(Image->Path, Image->Raw, PeCodeSection);
	PeImageSectionHeader *SH = FindSectionPe(Image->Raw, PeCodeSection);
	IATReferenceList List = {0};
	for(register uint32_t cc = 0; cc < Image->Fmt.imp.nImports; ++cc){
		IATReferenceList Temp = Image->Fmt.Opt.Pe32->mMagic == Pe32? 
			FindIATReferences32(Text, SH->mSizeOfRawData, SH->mVirtualAddress, Image->Fmt.Opt.Pe32->mImageBase, 
				Image->Fmt.imp.imports[cc].ImportAddressTableRVA, Image->Fmt.imp.nEntries[cc] * sizeof(PeImportLookupEntry32)): 
			FindIATReferences64(Text, SH->mSizeOfRawData, SH->mVirtualAddress, Image->Fmt.imp.imports[cc].ImportAddressTableRVA, 
				Image->Fmt.imp.nEntries[cc] * sizeof(PeImportLookupEntry64));
		List.RVAs = realloc(List.RVAs, sizeof(RelativeVirtualOffset) * (List.Count + Temp.Count));
		memcpy(List.RVAs + List.Count, Temp.RVAs, Temp.Count * sizeof(RelativeVirtualOffset));
		List.Count += Temp.Count;
		free(Temp.RVAs);
	}
	free(Text);
	//  We Process each Flag
	uint64_t totalNImports = 0;
	for(register uint32_t cc = 0; cc < Image->Fmt.imp.nImports; ++cc){totalNImports += Image->Fmt.imp.nEntries[cc];}
	BeRelocationType Types[totalNImports];
	while(totalNImports--){Types[totalNImports] = Image->Fmt.Opt.Pe32->mMagic == Pe32? BRET32: BRET64;}
	bool Ret = CreateImportSectionBe(path, Name, DLLPaths, ImportSymbols, Image->Fmt.imp.nEntries, Image->Fmt.imp.nImports, List.RVAs, Types);
	for(register uint32_t cc = 0; cc < Image->Fmt.imp.nImports; ++cc){
		free(DLLPaths[cc]);
		for(register uint32_t cc_ = 0; (Image->Fmt.Opt.Pe32->mMagic == Pe32? 
			Image->Fmt.imp.perImport.lookups.ImportLookups32[cc][cc_].Raw: 
			Image->Fmt.imp.perImport.lookups.ImportLookups64[cc][cc_].Raw
		) != 0; ++cc_){free(ImportSymbols[cc][cc_]);}
	}
	return Ret;
} 

#define MAX_PATH_LENGTH 2048
bool InitResourceSection(const char *path, SectionNameBe Name, 
	uint64_t SectionSize, BeResourceConfigurator *Root
){
	bool Ret = CreateResourceSectionBe(path, Name, SectionSize);

	// Initialize stack for Depth-First Search (DFS)
	char CurrentPath[MAX_PATH_LENGTH] = {0};
	uint64_t PathLength = 0, stackCapacity = 16, stackSize = 0;
	TraversalFrame *stack = calloc(stackCapacity, sizeof(TraversalFrame));
	stack[stackSize++] = (TraversalFrame){ .Config = Root, .SavedPathLength = 0 };
	while(stackSize > 0){
		// Pop the current node
		TraversalFrame currentFrame = stack[--stackSize];
		BeResourceConfigurator *Current = currentFrame.Config;
		if(!Current){continue;}

		PathLength = currentFrame.SavedPathLength;
		CurrentPath[PathLength] = '\0';
		switch(Current->Type){
			case BRTFile: {
				Ret &= ModifyResourceBe(path, CurrentPath, Current->Name, BRTFile, 
					Current->File.Type, Current->File.Length, Current->File.Data
				);
				break;
			} case BRTDirectory: {
				size_t nameLen = Current->Name ? strlen(Current->Name) : 0;
				Ret &= ModifyResourceBe(path, CurrentPath, Current->Name, 
					BRTDirectory, Current->Directory.Type
				);
				if(PathLength + nameLen + 2 < MAX_PATH_LENGTH){
					memcpy(CurrentPath + PathLength, Current->Name, nameLen);
					PathLength += nameLen;
					CurrentPath[PathLength++] = '\\';
					CurrentPath[PathLength] = '\0';
				}
				if(Current->Directory.NextDirectory){
					if(stackSize >= stackCapacity){
						stackCapacity *= 2;
						stack = realloc(stack, stackCapacity * sizeof(TraversalFrame));
					}
					stack[stackSize++] = (TraversalFrame){
						.Config = Current->Directory.NextDirectory,
						.SavedPathLength = currentFrame.SavedPathLength // Siblings share the parent path length
					};
				}
				if(Current->Directory.Files){
					for(GenericLengthType i = 0; i < Current->Directory.nFiles; i++){
						if(stackSize >= stackCapacity){
							stackCapacity *= 2;
							stack = realloc(stack, stackCapacity * sizeof(TraversalFrame));
						}
						// Array of files under this directory
						stack[stackSize++] = (TraversalFrame){
							.Config = &Current->Directory.Files[i],
							.SavedPathLength = PathLength // Children inherit updated path length
						};
					}
				}
				break;
			} default: {break;}
		}
	}
	free(stack);
	return Ret;
}

bool InitExportSection(const char *path, ExpandedPeExecutable *Image, SectionNameBe Name){
	if(!Image->Fmt.exp.nExports){return false;}
	char **Symbols = calloc(Image->Fmt.exp.nExports, sizeof(char *));
	BeExportEntryFlags *FlagsPerEntry = calloc(Image->Fmt.exp.nExports, sizeof(BeExportEntryFlags));
	EATReferenceList List = {0};	{
		const uint8_t *Text = ReadSectionPe(Image->Path, Image->Raw, PeCodeSection);
		PeImageSectionHeader *TextH = FindSectionPe(Image->Raw, PeCodeSection);
		List = Image->Fmt.Opt.Pe32->mMagic == Pe32? FindEATReferences32(Text, TextH->mSizeOfRawData, TextH->mVirtualAddress, 
				Image->Fmt.Opt.Pe32->mImageBase, Image->Fmt.exp.exportEntries->mExportTableRVA): 
			FindEATReferences64(Text, TextH->mSizeOfRawData, TextH->mVirtualAddress, Image->Fmt.exp.exportEntries->mExportTableRVA);
		free(Text);
	}
	//	We need to re-sort the Name Table according to the Ordinal Table Indices, so they are accurate to the Export Address Table.
	uint16_t Diff = 0;
	uint32_t CurrentIndex = 0;
	for(register uint32_t cc = 0; cc < Image->Fmt.exp.nExports; ++cc){
		Diff = UINT16_MAX;
		for(register uint32_t cc_ = 0; cc_ < Image->Fmt.exp.nExports; ++cc_){
			uint16_t Temp = Image->Fmt.exp.NormalisedOrdinals[cc_] > 
				Image->Fmt.exp.NormalisedOrdinals[cc]? 
					(Image->Fmt.exp.NormalisedOrdinals[cc_] - Image->Fmt.exp.NormalisedOrdinals[cc]): 
					(Image->Fmt.exp.NormalisedOrdinals[cc] - Image->Fmt.exp.NormalisedOrdinals[cc_]);
			if(Image->Fmt.exp.NormalisedOrdinals[CurrentIndex] != Image->Fmt.exp.NormalisedOrdinals[cc_]){
				if(Temp < Diff){CurrentIndex = cc_;		Diff = Temp;}
			}
		}
		Symbols[cc] = ReadStringAtRVAFromSectionPe(
			RvaToFileOffsetPe((Image->Fmt.exp.NormalisedOrdinals[CurrentIndex] * sizeof(uint16_t)) + 
				Image->Fmt.exp.exportEntries->OrdinalPointerRVA, FindSectionPe(Image->Raw, PeExportSection)), 
			PeExportSection, Image->Path, Image->Raw
		);
	}

	uint64_t TotalNBytes = sizeof(BeExportHeader) + (Image->Fmt.exp.nExports * sizeof(BeExportEntry));
	for(register GenericLengthType cc = 0; cc < Image->Fmt.exp.nExports; ++cc){TotalNBytes += strlen(Symbols[cc]);}
	void *Data = calloc(1, TotalNBytes);
	
	//	Discardable, as such resolves with the Raw Pointer.
	bool Ret = AddSectionBe(path, Name, SDDiscardableReadOnlyData, Data, TotalNBytes, 0x0, 0x0, BeStandardAlign);
	void *bheader = ReadBeHeader(path);
	BeSectionDescriptor This = *(FindSectionBe(bheader, Name));
	BeExportHeader *Header = Data;	*Header = (BeExportHeader){
		.bExportNameRVO = This.bRawPointer + sizeof(BeExportHeader) + (Image->Fmt.exp.nExports * sizeof(BeExportEntry)), 
		.bExportTableRVO = This.bRawPointer + sizeof(BeExportHeader), .bNExportAddresses = Image->Fmt.exp.nExports, 
		.bNExported = Image->Fmt.exp.nExports
	};
	for(register GenericLengthType cc = 0; cc < Image->Fmt.exp.nExports; ++cc){
		*((BeExportEntry *)(Data + sizeof(BeExportHeader) + (cc * sizeof(BeExportEntry)))) = (BeExportEntry){
			.bFlags = FlagsPerEntry[cc], .bNameIndex = cc, .bSymbolHash = {0}, .bVirtualAddress = List.RVAs[cc]
		};
		strcpy(Data + sizeof(BeExportHeader) + (Image->Fmt.exp.nExports * sizeof(BeExportEntry)), Symbols[cc]);
		blake2b_state bstate;
		blake2b_init(&bstate, sizeof(GenericHashType));
		blake2b_update(&bstate, Symbols[cc], strlen(Symbols[cc]));
		blake2b_final(&bstate, 
			((BeExportEntry *)(Data + sizeof(BeExportHeader) + (cc * sizeof(BeExportEntry))))->bSymbolHash, 
			sizeof(GenericHashType)
		);
	}
	Ret &= WriteSectionBe(path, bheader, Name, Data, This.bRawPointer, TotalNBytes);
	free(bheader);
	free(Data);
	return Ret;
}

bool InitExceptionSection(const char *path, ExpandedPeExecutable *Image, SectionNameBe Name){
	void *Raw = ReadSectionPe(Image->Path, Image->Raw, PeExceptionInfoSection);
	PeImageSectionHeader *This = FindSectionPe(Image->Raw, PeExceptionInfoSection);
	RelativeVirtualOffset *Addresses = NULL, *Handlers = NULL;
	GenericLengthType *Lengths = NULL, N;
	bool FunctionSizeKnown = true;
	switch(Image->Fmt.Header->mMachine){
		case PeMachineType_R3000BE:
		case PeMachineType_R3000: {
			Pe32MIPSExceptionDataEntry *Data = (Pe32MIPSExceptionDataEntry *)Raw;
			N = This->mSizeOfRawData / sizeof(Pe32MIPSExceptionDataEntry);
			Addresses = calloc(N, sizeof(RelativeVirtualOffset));
			Handlers = calloc(N, sizeof(RelativeVirtualOffset));
			Lengths = calloc(N, sizeof(GenericLengthType));
			for(GenericLengthType cc = 0; cc < N; ++cc){
				Addresses[cc] = Data[cc].mVirtualAddress;
				Lengths[cc] = Data[cc].mVirtualEnd - Data[cc].mVirtualAddress;
				Handlers[cc] = Data[cc].mHandler;
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
			PeARMExceptionDataEntry *Data = (PeARMExceptionDataEntry *)Raw;
			N = This->mSizeOfRawData / sizeof(PeARMExceptionDataEntry);
			Addresses = calloc(N, sizeof(RelativeVirtualOffset));
			Handlers = calloc(N, sizeof(RelativeVirtualOffset));
			Lengths = calloc(N, sizeof(GenericLengthType));
			for(GenericLengthType cc = 0; cc < N; ++cc){
				Addresses[cc] = Data[cc].mVirtualAddress;
				Lengths[cc] = Data[cc].mPrologLength + Data[cc].mFunctionLength;
				Handlers[cc] = Data[cc].mHasHandler;
				FunctionSizeKnown = false;
			}
		}
		case PeMachineType_ALPHA64:
		case PeMachineType_AMD64:
		case PeMachineType_IA64:
		case PeMachineType_LOONGARCH64:
		case PeMachineType_R4000:
		case PeMachineType_R10000:
		case PeMachineType_RISCV64: {
			Pe32PlusExceptionDataEntry *Data = (Pe32PlusExceptionDataEntry *)Raw;
			N = This->mSizeOfRawData / sizeof(Pe32PlusExceptionDataEntry);
			Addresses = calloc(N, sizeof(RelativeVirtualOffset));
			Handlers = calloc(N, sizeof(RelativeVirtualOffset));
			Lengths = calloc(N, sizeof(GenericLengthType));
			for(GenericLengthType cc = 0; cc < N; ++cc){
				Addresses[cc] = Data[cc].mAddressRVA;
				Lengths[cc] = Data[cc].mEndRVA - Data[cc].mAddressRVA;
				Handlers[cc] = Data[cc].mUnwindRVA;
			}
		}
	}
	bool Ret = CreateExceptionSectionBe(path, Name, Addresses, Lengths, Handlers, N, FunctionSizeKnown);
	free(Addresses);	free(Lengths);		free(Handlers);		free(Raw);
	return Ret;
}

bool EndianCheck(){
	unsigned int i = 1;
    char *c = (char *)&i;
    if(*c == 1){return true;
    }else{return false;}
}
#define IsBigEndian		(!(EndianCheck()))
#define IsLittleEndian	(EndianCheck())

bool InitRelocationSection(const char *path, ExpandedPeExecutable *Image, SectionNameBe Name){
	uint32_t NDirectories = Image->Fmt.reloc.nRelocationBlocks, 
			*NPerDirectory = Image->Fmt.reloc.nRelocationEntriesPerBlock;
	RelativeVirtualOffset *Directories = calloc(NDirectories, sizeof(RelativeVirtualOffset *));
	uint16_t **OPerDirectory = calloc(NDirectories, sizeof(uint16_t *));
	BeRelocationType **TPerDirectory = calloc(NDirectories, sizeof(BeRelocationType *));

	uint64_t Offset = 0;
	for(uint32_t cc = 0; cc < NDirectories; ++cc){
		PeBaseRelocationBlock *Base = (PeBaseRelocationBlock *)(Image->Fmt.reloc.data.Raw + Offset);
		PeRelocationEntry *Entries = ((PeRelocationEntry *)(Image->Fmt.reloc.data.Raw + Offset + sizeof(PeBaseRelocationBlock)));
		Directories[cc] = Base->PageRVA;
		OPerDirectory[cc] = calloc(NPerDirectory[cc], sizeof(GenericIndexType));
		TPerDirectory[cc] = calloc(NPerDirectory[cc], sizeof(BeRelocationType));
		for(uint32_t cc_ = 0; cc_ < NPerDirectory[cc]; ++cc_){
			OPerDirectory[cc][cc_] = Entries[cc_].Offset;
			switch(Entries[cc_].Type){
				case PE_REL_ABSOLUTE:	{TPerDirectory[cc][cc_] = BRETAbsolute;	break;}
				case PE_REL_HIGH:		if(IsBigEndian){OPerDirectory[cc][cc_] += sizeof(uint16_t);}
										else if(IsLittleEndian){OPerDirectory[cc][cc_] -= sizeof(uint16_t);}
				case PE_REL_LOW:		{TPerDirectory[cc][cc_] = BRET32;		break;}
				case PE_REL_HIGHADJ:	{
					memmove_s(OPerDirectory[cc] + cc_, sizeof(GenericIndexType) * (NPerDirectory[cc] - cc_), 
						OPerDirectory[cc] + cc_ + 1, sizeof(GenericIndexType) * (NPerDirectory[cc] - (cc_ + 1)));	// Remove the Entry
					TPerDirectory[cc][cc_] = BRET32;
					NPerDirectory[cc]--;
					break;
				}
				case PE_REL_HIGHLOW:	{TPerDirectory[cc][cc_] = BRET32;		break;}
				case PE_REL_DIR64:		{TPerDirectory[cc][cc_] = BRET64;		break;}
			}
		}
		Offset += Base->BlockSize;
	}

	uint64_t totalRelocations = 32;	{
		void *Text = ReadSectionPe(Image->Path, Image->Raw, PeCodeSection);
		PeImageSectionHeader *TextH = FindSectionPe(Image->Raw, PeCodeSection);
		for(uint32_t cc = 0; cc < Image->Fmt.imp.nImports; ++cc){
			IATReferenceList List = Image->Fmt.Opt.Pe32->mMagic == Pe32? 
				FindIATReferences32(Text, TextH->mSizeOfRawData, TextH->mVirtualAddress, Image->Fmt.Opt.Pe32->mImageBase, 
					Image->Fmt.imp.imports[cc].ImportAddressTableRVA, Image->Fmt.imp.nEntries[cc] * sizeof(uint32_t)): 
				FindIATReferences64(Text, TextH->mSizeOfRawData, TextH->mVirtualAddress, 
					Image->Fmt.imp.imports[cc].ImportAddressTableRVA, Image->Fmt.imp.nEntries[cc] * sizeof(uint32_t));
			totalRelocations += List.Count;
			free(List.RVAs);
		}
		free(Text);
	}
	bool Ret = CreateRelocationSectionBe(path, Name, Directories, OPerDirectory, TPerDirectory, NDirectories, NPerDirectory, 
		(totalRelocations * sizeof(BeRelocationEntry)) + ((Image->Fmt.imp.nImports + 32) * sizeof(BeRelocationDirectory)));
	free(Directories);	free(OPerDirectory);	free(TPerDirectory);
	return Ret;
}