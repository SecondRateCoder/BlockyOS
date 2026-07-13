#include "convert.h"

BeSectionFlags64 ConvertPeFlagsBe(PeSectionCharacteristics Characteristics){
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
			BeSectionFlags64 out = 0;
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

// void ConvertPeSection(char *in, char *out){
// 	void *pheader = ReadPeExecutableHeader(in);
// 	DecodePeExecutableHeader(pheader);
// 	for(uint32_t sectioncc = 0; sectioncc < (POH->mMagic == Pe32? POH->mNumberOfRvaAndSizes: POHPlus->mNumberOfRvaAndSizes); ++sectioncc){
// 		// Foreach Section
// 		switch(STR8_TO_UINT64(PISHs[sectioncc].mName)){
// 			case STR8_TO_UINT64(SpecialSectionN(0)): {
				
// 			}
// 		}
// 	}
// }

// //	Convert the Pe-Executable Import Sections into Blocky-Executable Style Imports.
void InitImportSection(
	char *path, ExpandedPeExecutable *Image, 
	SectionNameBe OutName, uint32_t BaseRelocationTableOffset
){
	uint64_t nbytes = sizeof(BeRelImportHeader);
	uint32_t *nImportSymbols = calloc(Image->Format.imports.nImports, sizeof(uint32_t));
	char **DLLPaths = calloc(Image->Format.imports.nImports, sizeof(char *)), ***ImportSymbols = calloc(Image->Format.imports.nImports, sizeof(char *));

	for(uint32_t cc = 0; cc < Image->Format.imports.nImports; ++cc){
		char *DLLName = GetAtRVAFromSectionDataPe(Image->Format.imports.imports[cc].NameRVA, ".idata", Image->Format.imports.imports, Image->Raw);
		DLLPaths[cc] = strdup(PoolGetPath(DLLName));
		nbytes += sizeof(BeRelImportDllRef) + strlen(DLLPaths[cc]) + 1;
		ExpandedPeExecutable *DLLHeader = ExpandPeExecutableFormat(DLLPaths[cc]);
		//*	Foreach Lookup/Address.
		for(uint32_t cc_ = 0; (Image->Format.Optional.Pe32->mMagic == Pe32? 
			Image->Format.imports.perImport.lookups.ImportLookups32[cc][cc_].Raw: 
			Image->Format.imports.perImport.lookups.ImportLookups64[cc][cc_].Raw
		) != 0; ++cc_){
			if((cc_ % 10) == 0){
				if(ImportSymbols[cc]){ImportSymbols[cc] = realloc(ImportSymbols[cc], (cc_ + 10) * sizeof(char **));
				}else{ImportSymbols[cc] = calloc(10, sizeof(char **));}
			}
			uint32_t TargetOrdinal = 0;
			if(Image->Format.Optional.Pe32->mMagic == Pe32? 
				Image->Format.imports.perImport.lookups.ImportLookups32[cc][cc_].Bits.ImportByOrdinal: 
				Image->Format.imports.perImport.lookups.ImportLookups64[cc][cc_].Bits.ImportByOrdinal
			){
				uint16_t Ordinal = (uint16_t)((Image->Format.Optional.Pe32->mMagic == Pe32? 
					Image->Format.imports.perImport.lookups.ImportLookups32[cc][cc_].Bits.OrdinalNumberOrNameRVA: 
					Image->Format.imports.perImport.lookups.ImportLookups64[cc][cc_].Bits.OrdinalNumberOrNameRVA
				) & 0xFFFF);
				//	Search for the Ordinal ID in Exports.
				// We always Normalise the Ordinal IDs so this should all work Fine.
				//	Foreach Export recover the Original Imported Name.
				uint32_t OrdinalIndex;
				for(uint32_t exportI = 0; exportI < DLLHeader->Format.exports.nExports; ++exportI){
					OrdinalIndex = 0;
					for(; OrdinalIndex < DLLHeader->Format.exports.exportEntries[exportI].mNNamePointers; ++OrdinalIndex){
						if(DLLHeader->Format.exports.NormalisedOrdinalPointerRVAs[OrdinalIndex] == (Ordinal - DLLHeader->Format.exports.exportEntries[exportI].OrdinalBase)){
							break;
						}
					}
				}
				ImportSymbols[cc][cc_] = strdup(GetAtRVAFromSectionDataPe(DLLHeader->Format.exports.NamePointerRVAs[OrdinalIndex], ".edata", 
					DLLHeader->Format.exports.exportEntries, DLLHeader->Raw
				));
			}else{
				uint8_t *HintNameBlock = (uint8_t *)GetAtRVAFromSectionDataPe(
					(Image->Format.Optional.Pe32->mMagic == Pe32? 
						Image->Format.imports.perImport.lookups.ImportLookups32[cc][cc_].Bits.OrdinalNumberOrNameRVA: 
						Image->Format.imports.perImport.lookups.ImportLookups64[cc][cc_].Bits.OrdinalNumberOrNameRVA
					), ".idata", Image->Format.imports.imports, Image->Raw
				);
				if(HintNameBlock){ImportSymbols[cc][cc_] = strdup((char *)(HintNameBlock + 2));}
			}
			nbytes += strlen(ImportSymbols[cc][cc_]);
			nImportSymbols[cc]++;
		}
		free(DLLHeader);
	}
	//*	Generate the Import Section
	CreateImportSectionBe(
		path, OutName, DLLPaths, ImportSymbols, nImportSymbols, 
		Image->Format.imports.nImports, BaseRelocationTableOffset);
}

// void *InitRelocSection(char *path, ExpandedPeExecutable *Image, void *BHeader, uint64_t *RawFPointer){
// 	//*	Export the Reloc Section.
// 	DecodeBeExecutableHeader(BHeader);
// 	size_t nbytes = sizeof(BeRelocationHeader) + (Image->Format.reloc.nRelocationBlocks * sizeof(BeRelocationDirectory));
// 	void *out = calloc(1, nbytes);
// 	size_t peOffset = 0, beOffset = 0;
// 	BeSectionDescriptor *relocSection = FindSectionBe(BHeader, ".reloc");
// 	for(uint32_t cc = 0; cc < Image->Format.reloc.nRelocationBlocks; ++cc){
// 		PeBaseRelocationBlock *relocations = (PeBaseRelocationBlock *)(Image->Format.reloc.data.Raw + peOffset);
// 		RelativeVirtualOffset PageOffset = relocSection->bVirtualAddress + (*RawFPointer) + 
// 			RvoToFileOffsetBe(relocations->PageRVA, relocSection);
// 		((BeRelocationDirectory *)(out + sizeof(BeRelocationHeader)))[cc] = (BeRelocationDirectory){
// 			.bNRelocationEntries = Image->Format.reloc.nRelocationEntriesPerBlock[cc], 
// 			.bOffset = PageOffset, 
// 			.bRelocationEntryTableRVO = relocSection->bVirtualAddress + 
// 				RvoToFileOffsetBe((*RawFPointer) + nbytes, relocSection), 
// 			.bSection = (((size_t)relocSection) - ((size_t)BSDs)) / sizeof(BeSectionDescriptor)
// 		};
// 		nbytes += (sizeof(BeRelocationDirectoryEntry) * Image->Format.reloc.nRelocationEntriesPerBlock[cc]);
// 		out = realloc(out, nbytes);
// 		for(uint32_t cc_ = 0; cc_ < Image->Format.reloc.nRelocationEntriesPerBlock[cc]; ++cc_){
// 			((BeRelocationDirectoryEntry *)(out + nbytes))[cc] = (BeRelocationDirectoryEntry){
// 				.Bits = {
// 					.bOffset = PageOffset > (relocations->relocations[cc_].Offset + relocations->PageRVA)? 
// 						PageOffset - (relocations->relocations[cc_].Offset + relocations->PageRVA):	
// 						(relocations->relocations[cc_].Offset + relocations->PageRVA) - PageOffset,
// 					.bType = (flagcheck(relocations->relocations[cc_].Type, PE_REL_ABSOLUTE)? BRETAbsolute: 0)	| 
// 							// (flagcheck(relocations->relocations[cc_].Type, PE_REL_ABSOLUTE)? BRET16: 0)
// 							(flagcheck(relocations->relocations[cc_].Type, PE_REL_HIGHLOW)? BRET32: 0)			|
// 							(flagcheck(relocations->relocations[cc_].Type, PE_REL_HIGH)? BRET16High: 0)			|
// 							(flagcheck(relocations->relocations[cc_].Type, PE_REL_LOW)? BRET16Low: 0)			|
// 							(flagcheck(relocations->relocations[cc_].Type, PE_REL_DIR64)? BRET64: 0)
// 				}
// 			};
// 		}
// 		peOffset += relocations->BlockSize;
// 	}
// 	(*RawFPointer) += nbytes;
// 	return out;
// }