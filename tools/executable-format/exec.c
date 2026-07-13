#include "exec.h"
#include "convert.h"

char *GenericError(char *str, uint32_t val);

uint32_t GetSectionNameLenBe(SectionNameBe Name){
	register uint32_t out = 0;
	while(out < sizeof(SectionNameBe)){if(!Name[out]){break;}		out++;}
	return out;
}

void GenerateBeHeader(
	const char *path, SectionNameBe SystemSecton, 
	ExecIcon Icon, JsonManifest Manifest, 
	GenericLengthType RawDataSize
){
	RawDataSize = RoundUp(__max((sizeof(BeSectionDescriptor) * BeDefaultNSections) + sizeof(BeHeader), RawDataSize), BeStandardAlign);
	uint64_t nbytes = sizeof(RelativeVirtualOffset) + RawDataSize;
	void *out = calloc(nbytes, 1);
	*((RelativeVirtualOffset *)out) = sizeof(RelativeVirtualOffset);
	DecodeBeExecutableHeader(out);
	*BH = (BeHeader){
		.bMagic = BeHeaderMagic, 
		.bIconRVO = nbytes, 
		.bManifestRVO = nbytes + sizeof(ExecIcon), 
		.bNSections = 1, 	//	For the System Section
		.bRawSize = RawDataSize, 
		.bSectionTableOffset = sizeof(RelativeVirtualOffset) + sizeof(BeHeader), 
	};	memcpy(BH->bSystemSection, SystemSecton, GetSectionNameLenBe(SystemSecton));
	BSDs[0] = (BeSectionDescriptor){
		.bAlignment = BeStandardAlign, 
		.bFlags = SDMountableResource, 
		.bRawPointer = nbytes,  
		.bName = {0}, 
		.bRawSize = RoundUp(sizeof(ExecIcon) + strlen(Manifest), BeStandardAlign), 
		.bVirtualAddress = 0x0, .bVirtualSize = 0x0, .SectionMagic = BeSectionMagic
	};	memcpy(BSDs[0].bName, SystemSecton, GetSectionNameLenBe(SystemSecton));
	{FILE *f = fopen(path, "w");		fclose(f);}
	DumpBeHeader(path, out);
	
	void *SectionData = calloc(1, strlen(Manifest) + sizeof(ExecIcon));
	memcpy(SectionData, Icon, sizeof(ExecIcon));
	memcpy(SectionData + sizeof(ExecIcon), Manifest, strlen(Manifest));
	WriteSectionBe(path, out, SystemSecton, SectionData, nbytes, strlen(Manifest) + sizeof(ExecIcon));
	free(SectionData);
	free(out);
	return;
}

void *ReadBeHeader(const char *path){
	FILE *f = NULL;
	GenericLengthType nbytes = sizeof(BeHeader) + sizeof(RelativeVirtualOffset);
	void *out = calloc(1, nbytes);
	//	The Offset from the Start of the File that the BeHeader starts at.
	// RelativeVirtualOffset HeaderRVO;
	if((f = fopen(path, "rb"))){
		out = calloc(1, nbytes);
		fseek(f, 0, SEEK_SET);
		fread(out, sizeof(RelativeVirtualOffset), 1, f);
		//	Read off the RawSize
		fseek(f, *((RelativeVirtualOffset *)out) + offsetof(BeHeader, bRawSize), SEEK_SET);
		fread(&nbytes, sizeof(GenericLengthType), 1, f);

		out = realloc(out, nbytes + sizeof(RelativeVirtualOffset));	
		memset(out + sizeof(RelativeVirtualOffset), 0, nbytes);
		//	We seek back in order to read the Whole Header.
		fseek(f, *((RelativeVirtualOffset *)out), SEEK_SET);
		fread(out + sizeof(RelativeVirtualOffset), nbytes, 1, f);
		fclose(f);
	}
	return out;
}
bool DumpBeHeader(const char *path, void *bheader){
	FILE *f = NULL;
	void *diskheader = ReadBeHeader(path);
	DecodeBeExecutableHeader(diskheader);
	BeHeader Temp = *BH;
	free(diskheader);
	if((f = fopen(path, "rb+"))){
		RelativeVirtualOffset HeaderRVO = 0;
		fread(&HeaderRVO, sizeof(RelativeVirtualOffset), 1, f);

		RDecodeBeExecutableHeader(bheader);
		//	Make sure we dont exceed the Original bRawSize, or if the Data is uninitialised, dont mind it.
		if(Temp.bRawSize >= BH->bRawSize || strcmp(Temp.bMagic.Magic, BeHeaderMagic)){
			if(HeaderRVO){
				fseek(f, HeaderRVO, SEEK_SET);
				fwrite(bheader + sizeof(RelativeVirtualOffset), BH->bRawSize, 1, f);
			}else{fseek(f, 0, SEEK_SET);	fwrite(bheader, BH->bRawSize, 1, f);}
			fclose(f);
			// free(bheader);
			return true;
		}
		fclose(f);
	}
	return false;
}

uint64_t RvoToFileOffsetBe(RelativeVirtualOffset RVO, BeSectionDescriptor *Section){
	if(flagcheck(Section->bFlags, SFAllocatable)){
		if((RVO < (Section->bVirtualAddress + Section->bVirtualSize)) && 
			(RVO >= Section->bVirtualAddress)
		){return (RVO - Section->bVirtualAddress) + Section->bRawPointer;}
	}else if((RVO >= Section->bRawPointer) && 
		(RVO < (Section->bRawPointer + Section->bRawSize))){return RVO;}
	return 0;
}
RelativeVirtualOffset FileOffsetToRvoBe(uint64_t Offset, BeSectionDescriptor *Section){
	RelativeVirtualOffset Estimate = (Offset + Section->bVirtualAddress) - Section->bRawPointer;
	if(Offset == RvoToFileOffsetBe(Estimate, Section)){
		return Estimate;
	}
	return 0;
}

BeSectionDescriptor *FindSectionBe(void *bheader, SectionNameBe name){
	DecodeBeExecutableHeader(bheader);
	for(uint32_t cc = 0; cc < BH->bNSections; ++cc){
		if(name[7]){if(!strncmp(BSDs[cc].bName, name, sizeof(name))){return (BSDs + cc);}
		}else{if(strcmp(BSDs[cc].bName, name)){return (BSDs + cc);}}
	}
	return NULL;
}

void *GetAtRVOFromSectionDataBe(RelativeVirtualOffset RVO, SectionNameBe name, void *data, void *bheader){
    BeSectionDescriptor *Section = FindSectionBe(bheader, name);
    uint64_t Address = RvoToFileOffsetBe(RVO, Section);
    return Address? data + (Address - Section->bRawPointer): data;
}

void *ReadAtRVOFromSectionBe(RelativeVirtualOffset RVO, uint32_t Size, SectionNameBe name, char *path, void *bheader){
	FILE *f = NULL;
	if(f = fopen(path, "rb")){
		BeSectionDescriptor *Section = FindSectionBe(bheader, name);
		if(Section){
			uint64_t Address = RvoToFileOffsetBe(RVO, Section);
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

void *ReadSectionBe(const char *path, void *bheader, SectionNameBe name){
	FILE *f = NULL;
	if((f = fopen(path, "rb"))){
		DecodeBeExecutableHeader(bheader);
		BeSectionDescriptor *section = FindSectionBe(bheader, name);
		if(section){
			uint32_t Address = section->bRawPointer;
			if(Address){fseek(f, Address, SEEK_SET);
			}else{fclose(f);	return NULL;}
			if(section->bRawSize){
				void *out = calloc(section->bRawSize, sizeof(char));
				if(fread(out, sizeof(char), section->bRawSize, f) == section->bRawSize){
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

bool WriteSectionBe(const char *path, void *bheader, SectionNameBe name, void *Data, RelativeVirtualOffset RVO, GenericLengthType NBytes){
	DecodeBeExecutableHeader(bheader);
	BeSectionDescriptor *sdesc = FindSectionBe(bheader, name);
	uint64_t Address = RvoToFileOffsetBe(RVO, sdesc);
	if(Address && ((Address + NBytes) <= (sdesc->bRawSize + sdesc->bRawPointer))){
		FILE *f = NULL;
		if((f = fopen(path, "rb+"))){
			fseek(f, Address, SEEK_SET);
			fwrite(Data, NBytes, 1, f);
			fclose(f);
			return true;
		}
	}
	return false;
}

#include <stdarg.h>
bool UpdateJsonSchemaBe(const char *path, const char *target, JsonType Type, ...){
	va_list ls;		va_start(ls, target);
	void *bheader = ReadBeHeader(path);
	DecodeBeExecutableHeader(bheader);

	BeSectionDescriptor *SysDesc = FindSectionBe(bheader, BH->bSystemSection);
	void *SysData = ReadSectionBe(path, bheader, BH->bSystemSection);
	char *Manifest = GetAtRVOFromSectionDataBe(BH->bManifestRVO, BH->bSystemSection, SysData, bheader);
	//	Get The Data Up to the Manifest Offset.
	void *Prefix = SysData, *Suffix = GetAtRVOFromSectionDataBe(
		RoundUp(BH->bManifestRVO + strlen(Manifest), BeStandardAlign), 
		BH->bSystemSection, SysData, bheader
	);	//	Get the Suffix Following the Manifest.
	uint64_t PrefixLen = BH->bManifestRVO - SysDesc->bRawPointer, 
			SuffixLen = SysDesc->bRawSize - (strlen(Manifest) + PrefixLen), 
			ManifestLen = strlen(Manifest);

	JsonValue *Root = JsonParse(Manifest);
	JsonValue Temp = {.type = Type, .next = NULL, .parent = NULL, .stringValue = va_arg(ls, char *)};
	JsonSetValue(Root, target, &Temp);
	Manifest = JsonSerialize(Root);
	ManifestLen = strlen(Manifest);

	void *Final = calloc(sizeof(char), PrefixLen + strlen(Manifest) + SuffixLen);
		memcpy(Final, Prefix, PrefixLen);
		strcpy(Final + PrefixLen, Manifest);
		memcpy(Final + PrefixLen + ManifestLen, Suffix, SuffixLen);
	bool Ret = RemoveSectionBe(path, bheader, BH->bSystemSection);
	Ret &= AddSectionBe(path, BH->bSystemSection, SDDiscardableReadOnlyData, Final, PrefixLen + ManifestLen + SuffixLen, 
		SysDesc->bVirtualAddress, SysDesc->bVirtualSize, BeStandardAlign
	);
	RelativeVirtualOffset OldManifestRVODiff = BH->bManifestRVO - SysDesc->bRawPointer, 
							OldIconRVODiff = BH->bIconRVO - SysDesc->bRawPointer;
	// DumpBeHeader(path, bheader);
	free(bheader);
	free(SysData);
	free(Final);
	free(Manifest);

	//	Save Just the New RVOs
	bheader = ReadBeHeader(path);
	RDecodeBeExecutableHeader(bheader);
	BeSectionDescriptor OldSysDesc = *SysDesc;
	BeSectionDescriptor *NewSysDesc = FindSectionBe(bheader, BH->bSystemSection);
	BH->bManifestRVO = (OldSysDesc.bRawPointer > NewSysDesc->bRawPointer? 
		(OldSysDesc.bRawPointer - NewSysDesc->bRawPointer): 
		(NewSysDesc->bRawPointer - OldSysDesc.bRawPointer))/*Difference*/ + OldManifestRVODiff;
	BH->bIconRVO = (OldSysDesc.bRawPointer > NewSysDesc->bRawPointer? 
		(OldSysDesc.bRawPointer - NewSysDesc->bRawPointer): 
		(NewSysDesc->bRawPointer - OldSysDesc.bRawPointer))/*Difference*/ + OldIconRVODiff;
	DumpBeHeader(path, bheader);
	free(bheader);
	return Ret;
}

void *ReadSectionFromManifest(const char *path, char *manifestpath){
	void *bheader = ReadBeHeader(path);
	DecodeBeExecutableHeader(bheader);
	BeSectionDescriptor *system = FindSectionBe(bheader, BH->bSystemSection);
	uint64_t ManifestAddress = RvoToFileOffsetBe(BH->bManifestRVO, system);
	void *SystemSection = ReadSectionBe(path, bheader, BH->bSystemSection);

	char *JsonManifest = SystemSection + (ManifestAddress - system->bRawPointer);
	SectionNameBe SectionName = {0};
	JsonValue *JsonRoot = JsonreadValue((const char **)(&JsonManifest));
	JsonValue *RelocSectionNameJson = JsongetValue(JsonRoot, manifestpath);
	if(RelocSectionNameJson && RelocSectionNameJson->type == JTYPE_STRING){
		memcpy(SectionName, RelocSectionNameJson->stringValue, strlen(RelocSectionNameJson->stringValue));
	}
	
	void *Out = ReadSectionBe(path, bheader, SectionName);
	free(SystemSection);
	free(JsonManifest);
	DumpBeHeader(path, bheader);
	free(bheader);
	return Out;
}

//	R/W From/To Disk rather than stored Memory Ptr.
bool AddSectionBe(const char *path, SectionNameBe Name, BeSectionFlags64 Flags, 
	void *RawData, uint64_t NBytes, uint64_t VirtualAddress, uint64_t VirtualSize, 
	Alignment Align
){
	VirtualAddress = RoundUp(VirtualAddress, Align);
	VirtualSize = RoundUp(VirtualSize, Align);
	NBytes = RoundUp(NBytes, Align);
	void *bheader = ReadBeHeader(path);
	DecodeBeExecutableHeader(bheader);
	uint32_t RawPointer = BH->bRawSize + *((RelativeVirtualOffset *)bheader);
	if(((BH->bNSections++) * sizeof(BeSectionDescriptor)) > BH->bRawSize){return false;}
	for(uint32_t cc = 0; cc < (BH->bNSections - 1); ++cc){
		if(flagcheck(Flags, SFAllocatable) && VirtualAddress >= BSDs[cc].bVirtualAddress && 
			VirtualAddress < (BSDs[cc].bVirtualAddress + BSDs[cc].bVirtualSize)
		){free(bheader);		return false;}else 
		//	If the Raw Ptr of the Current Section - The Ending Ptr of the Previous Section is >= the NBytes, 
		//		then we can use that region for the Section.
		if(cc > 0){if((BSDs[cc].bRawPointer - (BSDs[cc - 1].bRawPointer + BSDs[cc - 1].bRawSize)) >= NBytes){break;}}
		RawPointer += BSDs[cc].bRawSize;
	}
	BSDs[BH->bNSections - 1] = (BeSectionDescriptor){
		.bAlignment = Align,
		.bFlags = Flags,
		.bName = {0},
		.bRawPointer = RawPointer,
		.bRawSize = NBytes,
		.bVirtualAddress = VirtualAddress,
		.bVirtualSize = VirtualSize,
		.SectionMagic.Magic = BeSectionMagic
	};	memcpy(BSDs[BH->bNSections - 1].bName, Name, GetSectionNameLenBe(Name));
	DumpBeHeader(path, bheader);
	// FILE *f = fopen(path, "rb+");
	// fseek(f, RawPointer, SEEK_SET);
	// fwrite(RawData, sizeof(char), NBytes, f);
	// fclose(f);
	WriteSectionBe(path, bheader, Name, RawData, flagcheck(Flags, SFAllocatable)? VirtualAddress: RawPointer, NBytes);
	free(bheader);
	return true;
}

bool RemoveSectionBe(const char *path, void *bheader, SectionNameBe name){
	BeSectionDescriptor *sdesc = FindSectionBe(bheader, name);
	if(sdesc){
		DecodeBeExecutableHeader(bheader);
		// Execute the safe bounded memory shift
		memmove_s(
			sdesc, (BH->bNSections - ((size_t)(sdesc - BSDs))) * sizeof(BeSectionDescriptor),
			sdesc + 1, (BH->bNSections - (((size_t)(sdesc - BSDs)) + 1)) * sizeof(BeSectionDescriptor)
		);
		BH->bNSections--;
		DumpBeHeader(path, bheader);
	}
	return true;
}

bool CreateRelocationSectionBe(char *path, SectionNameBe Name, 
	SectionNameBe *NamePerDirectory, uint16_t **OffsetPerDirectoryEntry, BeRelocationType **TypePerDirectoryEntry, 
	GenericLengthType nDirectories, GenericLengthType *nPerDirectory 
){
	void *bheader = ReadBeHeader(path);
	DecodeBeExecutableHeader(bheader);
	uint32_t RawPointer = BH->bRawSize + *((RelativeVirtualOffset *)bheader);
	for(uint32_t cc = 0; cc < BH->bNSections; ++cc){RawPointer += BSDs[cc].bRawSize;}

	uint64_t totalNOffsets = 0;
	for(GenericLengthType cc = 0; cc < nDirectories; ++cc){totalNOffsets += nPerDirectory[cc];}
	uint64_t NBytes = sizeof(BeRelocationHeader) + (sizeof(BeRelocationDirectory) * nDirectories) + (sizeof(BeRelocationDirectoryEntry) * totalNOffsets);
	void *Out = calloc(1, NBytes);
	uint64_t ByteOffset = 0;
	BeRelocationHeader *Header = Out;	*Header = (BeRelocationHeader){
		.bNDirectories = nDirectories,
		.bRelocationDirTableRVO = RawPointer + sizeof(BeRelocationHeader)
	};
	RawPointer += sizeof(BeRelocationHeader);

	for(GenericLengthType cc = 0; cc < nDirectories; ++cc){
		BeRelocationDirectory *Directory = Out + sizeof(BeRelocationHeader) + ByteOffset;
		*Directory = (BeRelocationDirectory){
			.bDirectorySize = sizeof(BeRelocationDirectory) + (sizeof(BeRelocationDirectoryEntry) * nPerDirectory[cc]),
			.bOffset = 0,	/*We Temporarily only Generate a Offset=0 per Section*/
			.bSection = (SectionReference)(((size_t)(FindSectionBe(bheader, NamePerDirectory[cc]))) - ((size_t)BSDs)) / sizeof(BeSectionDescriptor)
		};
		register uint64_t MappedSize = 0;
		uint16_t PreviousOffset = 0;
		for(GenericLengthType cc_ = 0; cc_ < nPerDirectory[cc]; ++cc_){
			//	We need to recover the Offset of the Previous Item.
			uint16_t Offset = 0;
			BeSectionDescriptor *Temp = FindSectionBe(bheader, NamePerDirectory[cc]);
			for(GenericLengthType sortcc = 0; sortcc < nPerDirectory[cc]; sortcc++){
				if((Offset = OffsetPerDirectoryEntry[cc][sortcc]) > PreviousOffset){
					while(Offset > Temp->bRawSize){Offset -= Temp->bRawSize;}
				}
			}
			BeRelocationDirectoryEntry *Entry = (Directory->bRelocationTable + cc_);	*Entry = (BeRelocationDirectoryEntry){
				.Bits.bOffset = Offset, 
				.Bits.bType = TypePerDirectoryEntry[cc][cc_]
			};
		}
		ByteOffset += Directory->bDirectorySize;
		RawPointer += Directory->bDirectorySize;
	}

	bool Ret = AddSectionBe(path, Name, SDDiscardableReadOnlyData, Out, NBytes, 0x0, NBytes, BeStandardAlign);
	UpdateJsonSchemaBe(path, JsonRelocSectionNamePath, JTYPE_STRING, Name);
	free(bheader);
	free(Out);
	return Ret;
}

bool CreateImportSectionBe(
	const char *path, SectionNameBe Name, char **DLLs, char ***imports, 
	GenericLengthType *nImportsPerDll, GenericLengthType nImports, 
	//	Info for Generating Relocations.
	uint32_t BaseRelocationTableOffset
){
	void *bheader = ReadBeHeader(path);
	DecodeBeExecutableHeader(bheader);
	uint32_t RawPointer = BH->bRawSize + *((RelativeVirtualOffset *)bheader);
	uint64_t totalNImports = 0, totalCharacters = 0;
	for(uint32_t cc = 0; cc < BH->bNSections; ++cc){RawPointer += BSDs[cc].bRawSize;}
	for(register GenericLengthType cc = 0; cc < nImports; ++cc){
		totalNImports += nImportsPerDll[cc];
		totalCharacters += strlen(DLLs[cc]);
		for(register GenericLengthType cc_ = 0; cc_ < nImportsPerDll[cc]; ++cc_){
			totalCharacters += (strlen(imports[cc][cc_]) + 1);
		}
	}

	uint64_t SectionSize = sizeof(BeRelImportHeader) + (sizeof(BeRelImportDllRef) * nImports) + 
		(sizeof(BeRelImportEntry) * totalNImports) + (sizeof(char) * totalCharacters);
	void *Out = calloc(1, SectionSize);
	BeRelImportHeader *Header = Out;	*Header = (BeRelImportHeader){
		.bNDllReferences = nImports, 
		.bImportTableRVO = RawPointer,
		//	Follows the Import Path Buffer.
		.bImportPathRVO = RawPointer + RoundUp(sizeof(BeRelImportDllRef) * nImports, BeStandardAlign)
	};
	RawPointer += sizeof(BeRelImportHeader) + RoundUp(sizeof(BeRelImportDllRef) * nImports, BeStandardAlign);
	for(GenericLengthType cc = 0, totalNImports = 0; cc < nImports; ++cc){
		const uint64_t Temp = sizeof(BeRelImportDllRef) + (sizeof(BeRelImportEntry) * nImportsPerDll[cc]);

		((BeRelImportDllRef *)(Out + sizeof(BeRelImportHeader)))[cc] = (BeRelImportDllRef){
			.bImportPathIndex = cc, 
			.bNHashes = nImportsPerDll[cc], 
		};
		Out = realloc(Out, SectionSize + Temp);
		memset(Out + SectionSize, 0, Temp);

		for(GenericLengthType cc_ = 0; cc_ < nImportsPerDll[cc]; ++cc_){
			((BeRelImportEntry *)(Out + SectionSize))[cc_] = (BeRelImportEntry){
				.bFlags = BRIFExternal,
				.bHash = {0},
				.bRelocation = BaseRelocationTableOffset + totalNImports,
			};
			blake2b_state bstate;
			blake2b_init(&bstate, sizeof(GenericHashType));
			blake2b_update(&bstate, imports[cc][cc_], strlen(imports[cc][cc_]));
			blake2b_final(&bstate, &(((BeRelImportEntry *)(Out + SectionSize))[cc_].bHash), sizeof(GenericHashType));
			memcpy(Out + SectionSize, DLLs[cc], strlen(DLLs[cc]));
			totalNImports++;
		}
		SectionSize += Temp;
	}

	bool Ret = AddSectionBe(path, Name, SDDiscardableReadOnlyData, Out, SectionSize, 0x0, 0x0, BeStandardAlign);
	UpdateJsonSchemaBe(path, JsonImportSectionNamePath, JTYPE_STRING, Name);
	free(bheader);
	free(Out);
	return Ret;
}