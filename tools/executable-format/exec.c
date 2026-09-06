#include "exec.h"
#include "convert.h"

uint32_t GetSectionNameLenBe(SectionNameBe Name){
	register uint32_t out = 0;
	while(out < sizeof(SectionNameBe)){if(!Name[out]){break;}		out++;}
	return out;
}

void GenerateBeHeader(const char *path, SectionNameBe SystemSecton, GenericLengthType RawDataSize, uint64_t EntryPoint){
	RawDataSize = RoundUp(__max((sizeof(BeSectionDescriptor) * BeDefaultNSections) + sizeof(BeHeader), RawDataSize), BeStandardAlign);
	uint64_t nbytes = sizeof(RelativeVirtualOffset) + RawDataSize;
	void *out = calloc(nbytes, 1);
	*((RelativeVirtualOffset *)out) = sizeof(RelativeVirtualOffset);
	DecodeBeExecutableHeader(out);
	*BH = (BeHeader){
		.bMagic = BeHeaderMagic, 
		.bNSections = 1, 	//	For the System Section
		.bRawSize = RawDataSize, 
		.bSectionTableOffset = sizeof(RelativeVirtualOffset) + sizeof(BeHeader), 
		.bEntryPoint = (RelativeVirtualOffset)EntryPoint
	};
	memcpy(BH->bSystemSection, SystemSecton, GetSectionNameLenBe(SystemSecton));
	{FILE *f = fopen(path, "w");		fclose(f);}
	DumpBeHeader(path, out);	
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
		fseek(f, 0, SEEK_SET);		fread(&HeaderRVO, sizeof(RelativeVirtualOffset), 1, f);
		if(!HeaderRVO){HeaderRVO = sizeof(RelativeVirtualOffset);}
		RDecodeBeExecutableHeader(bheader);
		//	Make sure we dont exceed the Original bRawSize, or if the Data is uninitialised, dont mind it.
		if(Temp.bRawSize >= BH->bRawSize || strcmp(Temp.bMagic.Magic, BeHeaderMagic)){
			if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
				fprintf(f, "\n[%s:%u:DumpBeHeader]\tWriting %llu bytes to %llu", 0, __FILE__, (uint32_t)__LINE__, BH->bRawSize, HeaderRVO);
				fclose(f);
			}
			fseek(f, 0, SEEK_SET);	fwrite(&HeaderRVO, sizeof(RelativeVirtualOffset), 1, f);	fseek(f, HeaderRVO, SEEK_SET);
			fwrite(bheader + sizeof(RelativeVirtualOffset), BH->bRawSize, 1, f);
			fclose(f);
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
	for(register uint32_t cc = 0; cc < BH->bNSections; ++cc){
		if(name[7]){if(!strncmp(BSDs[cc].bName, name, sizeof(name))){return (BSDs + cc);}
		}else{if(!strcmp(BSDs[cc].bName, name)){return (BSDs + cc);}}
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
			fseek(f, section->bRawPointer, SEEK_SET);
			if(section->bRawSize){
				void *out = calloc(section->bRawSize, sizeof(char));
				if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
					fprintf(f, "\n[%s:%u:ReadSection]\tReading %llu from %llu", __FILE__, (uint32_t)__LINE__, (uint64_t)section->bRawSize, (uint64_t)section->bRawPointer);
					fclose(f);
				}printf("\n[%s:%u:ReadSection]\tReading %llu from %llu", __FILE__, (uint32_t)__LINE__, (uint64_t)section->bRawSize, (uint64_t)section->bRawPointer);
				if(fread(out, sizeof(char), section->bRawSize, f)){
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

bool WriteSectionBe(const char *path, void *bheader, SectionNameBe name, void *Data, RelativeVirtualOffset RVO, GenericLengthType NBytes){
	DecodeBeExecutableHeader(bheader);
	BeSectionDescriptor *sdesc = FindSectionBe(bheader, name);
	uint64_t Address = RvoToFileOffsetBe(RVO, sdesc);
	// if(Address && ((Address + NBytes) <= (sdesc->bRawSize + sdesc->bRawPointer))){
	if((sdesc && Address >= sdesc->bRawPointer) && (Address + NBytes) <= (sdesc->bRawPointer + sdesc->bRawSize)){
		FILE *f = NULL;
		if((f = fopen(path, "rb+"))){
			fseek(f, Address, SEEK_SET);
			if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
				fprintf(f, "\n[%s:%u:WriteSection]\tWriting %llu to %llu", __FILE__, (uint32_t)__LINE__, (uint64_t)NBytes, (uint64_t)Address);
			}printf("\n[%s:%u:WriteSection]\tWriting %llu to %llu", __FILE__, (uint32_t)__LINE__, (uint64_t)NBytes, (uint64_t)Address);
			fwrite(Data, NBytes, 1, f);
			fclose(f);
			return true;
		}
	}
	return false;
}

bool MatchResourceName(BeResourceString *nameObj, const char *snippet){
    if(!nameObj || !snippet){return false;}
    size_t snippetLen = strlen(snippet);
    return (nameObj->bStringLength == snippetLen) && 
           (strncmp(nameObj->String, snippet, snippetLen) == 0);
}
bool ReadResourceBe(const char *path, const char *target, 
    void *out, GenericLengthType NBytes, GenericLengthType *Remaining
){
    if(!path || !target || !out){return false;}
    void *bheader = ReadBeHeader(path);
    if(!bheader){return false;}
    
    DecodeBeExecutableHeader(bheader);
    void *rsrcData = ReadSectionBe(path, bheader, BH->bSystemSection);
    if(!rsrcData){free(bheader);	return false;}

    BeResourceHeader *rheader = (BeResourceHeader *)rsrcData;
    void *Entry = GetAtRVOFromSectionDataBe(rheader->bRootDirectoryRVO, BH->bSystemSection, rsrcData, bheader);
    char *targetCopy = strdup(target);
    if(!targetCopy){free(rsrcData);		free(bheader);		return false;}
    char *saveptr = NULL, *snippet = strtok_r(targetCopy, "\\/", &saveptr);
    bool found = false;
    while(Entry && snippet){
        BeResourceType type = *((BeResourceType *)Entry);
		switch(type){
			case BRTFile: {
				BeResourceFile *File = (BeResourceFile *)Entry;
				BeResourceString *Name = GetAtRVOFromSectionDataBe(File->bFileNameRVO, BH->bSystemSection, rsrcData, bheader);
				if(MatchResourceName(Name, snippet)){
					char *nextSnippet = strtok_r(NULL, "\\/", &saveptr);
					if(nextSnippet == NULL){ // Target reached
						void *OutData = ReadAtRVOFromSectionBe(File->bDataOffsetRVO, File->bDataSize, BH->bSystemSection, path, bheader);
						if(OutData){
							GenericLengthType copySize = __min(File->bDataSize, NBytes);
							memcpy(out, OutData, copySize);
							if(Remaining){ *Remaining = (File->bDataSize > NBytes) ? (File->bDataSize - NBytes) : 0; }
							free(OutData);
							found = true;
						}
						break;
					}
				}
				Entry = GetAtRVOFromSectionDataBe(File->bNextFileRVO, BH->bSystemSection, rsrcData, bheader);
				break;
			} case BRTDirectory: {
				BeResourceDirectory *Dir = (BeResourceDirectory *)Entry;
				BeResourceString *Name = GetAtRVOFromSectionDataBe(Dir->bDirectoryNameRVO, BH->bSystemSection, rsrcData, bheader);
				if(MatchResourceName(Name, snippet)){
					// Descend into directory contents
					Entry = GetAtRVOFromSectionDataBe(Dir->bNextFileRVO, BH->bSystemSection, rsrcData, bheader);
					snippet = strtok_r(NULL, "\\/", &saveptr);
				}else{
					// Search across all 4 potential directory sibling branches
					void *nextDirEntry = NULL;
					for(int i = 0; i < 4; i++){
						if(Dir->bNextDirectoriesRVO[i] != 0){
							void *cand = GetAtRVOFromSectionDataBe(Dir->bNextDirectoriesRVO[i], BH->bSystemSection, rsrcData, bheader);
							if(cand){
								BeResourceString *candName = GetAtRVOFromSectionDataBe(((BeResourceDirectory *)cand)->bDirectoryNameRVO, BH->bSystemSection, rsrcData, bheader);
								if(MatchResourceName(candName, snippet)){
									nextDirEntry = cand;
									break;
								}
							}
						}
					}
					// Fallback to primary sibling slot [0] if no direct match was resolved in the branch
					if(!nextDirEntry && Dir->bNextDirectoriesRVO[0] != 0){nextDirEntry = GetAtRVOFromSectionDataBe(Dir->bNextDirectoriesRVO[0], BH->bSystemSection, rsrcData, bheader);}
					Entry = nextDirEntry;
				}
				break;
			} default: {snippet = NULL;		break;}
		}
    }
    free(targetCopy);
    free(rsrcData);
    free(bheader);
    return found;
}
bool ModifyResourceBe(const char *path, const char *target, const char *name, BeResourceType Type, ...){
    #include <stdarg.h>
    if(!path || !target || !name){return false;}
    char *tduplicate = strdup(target);
    if(!tduplicate){return false;}

    va_list ls;
    va_start(ls, Type);
    void *bheader = ReadBeHeader(path);
    if(!bheader){free(tduplicate);		va_end(ls);		return false;}
    DecodeBeExecutableHeader(bheader);

    BeSectionDescriptor *This = FindSectionBe(bheader, BH->bSystemSection);
    void *rsrcData = ReadSectionBe(path, bheader, BH->bSystemSection);
    if(!rsrcData){free(tduplicate);		free(bheader);		va_end(ls);		return false;}

    BeResourceHeader *rheader = (BeResourceHeader *)rsrcData;
    void *Entry = GetAtRVOFromSectionDataBe(rheader->bRootDirectoryRVO, BH->bSystemSection, rsrcData, bheader);
    char *saveptr = NULL, *snippet = strtok_r(tduplicate, "\\/", &saveptr);
    if(!snippet){snippet = tduplicate;}

    while(Entry && snippet){
        BeResourceType entryType = *((BeResourceType *)Entry);
		switch(entryType){
			case BRTFile: {
				BeResourceFile *File = (BeResourceFile *)Entry;
				BeResourceString *Name = GetAtRVOFromSectionDataBe(File->bFileNameRVO, BH->bSystemSection, rsrcData, bheader);
				if(MatchResourceName(Name, snippet)){
					snippet = strtok_r(NULL, "\\/", &saveptr);
					if(!snippet){break;}
				}
				Entry = GetAtRVOFromSectionDataBe(File->bNextFileRVO, BH->bSystemSection, rsrcData, bheader);
				break;
			} case BRTDirectory: {
				BeResourceDirectory *Dir = (BeResourceDirectory *)Entry;
				BeResourceString *Name = GetAtRVOFromSectionDataBe(Dir->bDirectoryNameRVO, BH->bSystemSection, rsrcData, bheader);
				if(MatchResourceName(Name, snippet)){
					snippet = strtok_r(NULL, "\\/", &saveptr);
					if (!snippet) break;
					Entry = GetAtRVOFromSectionDataBe(Dir->bNextFileRVO, BH->bSystemSection, rsrcData, bheader);
				}else{
					// Fallback traversal across directory array slots
					void *nextEntry = NULL;
					for(int i = 0; i < 4; i++){
						if(Dir->bNextDirectoriesRVO[i] != 0){
							nextEntry = GetAtRVOFromSectionDataBe(Dir->bNextDirectoriesRVO[i], BH->bSystemSection, rsrcData, bheader);
							if(nextEntry) break;
						}
					}
					Entry = nextEntry;
				}
				break;
			} default: {snippet = NULL;		break;}
		}
    }
    size_t nameLen = strlen(name);
	switch(Type){
		case BRTFile: {
			if(Entry){
				if(*((BeResourceType *)Entry) == BRTFile){((BeResourceFile *)Entry)->bNextFileRVO = rheader->bUnusedBytesRVO;}else
				if(*((BeResourceType *)Entry) == BRTDirectory){((BeResourceDirectory *)Entry)->bNextFileRVO = rheader->bUnusedBytesRVO;}
			}
			BeResourceFileType fType = va_arg(ls, BeResourceFileType);
			GenericLengthType fSize  = va_arg(ls, GenericLengthType);
			void *fData              = va_arg(ls, void *);
			if(fData){
				size_t stringStructSize = sizeof(BeResourceString) + nameLen + 1;
				size_t totalSize = sizeof(BeResourceFile) + stringStructSize + fSize;
		
				if(rheader->bUnusedBytesRVO + totalSize > This->bRawSize){
					void *temp = realloc(rsrcData, rheader->bUnusedBytesRVO + totalSize);
					if(!temp){ free(tduplicate); free(rsrcData); free(bheader); va_end(ls); return false; }
					rsrcData = temp;
					rheader = (BeResourceHeader *)rsrcData;
					This->bRawSize = (uint32_t)(rheader->bUnusedBytesRVO + totalSize);
				}
		
				uint8_t *Unused = (uint8_t *)rsrcData + rheader->bUnusedBytesRVO;
				uint64_t dataOffset = sizeof(BeResourceFile) + stringStructSize;
		
				BeResourceFile *File = (BeResourceFile *)Unused;
				*File = (BeResourceFile){
					.bFileMagic = BRTFile, .bFileType = fType, .bNextFileRVO = 0,
					.bFileNameRVO = rheader->bUnusedBytesRVO + sizeof(BeResourceFile),
					.bDataOffsetRVO = rheader->bUnusedBytesRVO + (uint32_t)dataOffset,
					.bDataSize = fSize
				};
		
				BeResourceString *String = (BeResourceString *)(Unused + sizeof(BeResourceFile));
				String->bStringMagic = BRTNameString;
				String->bStringLength = (uint16_t)nameLen;
				memcpy(String->String, name, nameLen);
				String->String[nameLen] = '\0';
		
				memcpy(Unused + dataOffset, fData, fSize);
				rheader->bUnusedBytesRVO += (uint32_t)(dataOffset + fSize);
			}
			break;
		} case BRTDirectory: {
			if(Entry){
				if(*((BeResourceType *)Entry) == BRTDirectory){
					BeResourceDirectory *pDir = (BeResourceDirectory *)Entry;
					bool slotFound = false;
					// Link to the first available directory slot in the array
					for(int i = 0; i < 4; i++){
						if(pDir->bNextDirectoriesRVO[i] == 0){
							pDir->bNextDirectoriesRVO[i] = rheader->bUnusedBytesRVO;
							slotFound = true;
							break;
						}
					}
					if(!slotFound){pDir->bNextDirectoriesRVO[0] = rheader->bUnusedBytesRVO;} // Fallback overwrite
				}
			}
			BeResourceDirectoryType dType = va_arg(ls, BeResourceDirectoryType);
			size_t stringStructSize = sizeof(BeResourceString) + nameLen + 1;
			size_t totalSize = sizeof(BeResourceDirectory) + stringStructSize;
		
			if(rheader->bUnusedBytesRVO + totalSize > This->bRawSize){
				void *temp = realloc(rsrcData, rheader->bUnusedBytesRVO + totalSize);
				if(!temp){ free(tduplicate); free(rsrcData); free(bheader); va_end(ls); return false; }
				rsrcData = temp;
				rheader = (BeResourceHeader *)rsrcData;
				This->bRawSize = (uint32_t)(rheader->bUnusedBytesRVO + totalSize);
			}
			uint8_t *Unused = (uint8_t *)rsrcData + RvoToFileOffsetBe(rheader->bUnusedBytesRVO, This) - This->bRawPointer;
			BeResourceDirectory *Dir = (BeResourceDirectory *)Unused;
			memset(Dir, 0, sizeof(BeResourceDirectory));
			*Dir = (BeResourceDirectory){
				.bDirectoryMagic = BRTDirectory, .bDirectoryType = dType, 
				.bDirectoryNameRVO = rheader->bUnusedBytesRVO + sizeof(BeResourceDirectory)
			};
		
			BeResourceString *String = (BeResourceString *)(Unused + sizeof(BeResourceDirectory));
			String->bStringMagic = BRTNameString;
			String->bStringLength = (uint16_t)nameLen;
			memcpy(String->String, name, nameLen);
			String->String[nameLen] = '\0';
			rheader->bUnusedBytesRVO += (uint32_t)totalSize;
			break;
		}
	}

    va_end(ls);
    bool Ret = WriteSectionBe(path, bheader, BH->bSystemSection, rsrcData, This->bRawPointer, This->bRawSize);
    free(tduplicate);
    free(rsrcData);
    free(bheader);
    return Ret;
}

bool UpdateJsonSchemaBe(const char *path, const char *target, JsonType Type, ...){
	va_list ls;		va_start(ls, target);
	void *bheader = ReadBeHeader(path);
	DecodeBeExecutableHeader(bheader);
	if(!(FindSectionBe(bheader, BH->bSystemSection))){free(bheader);	return false;}

	char *Manifest = NULL;
	GenericLengthType Remaining = 0;
	do{
		uint32_t Temp = RoundUp(Remaining + sizeof(JsonManifest), sizeof(JsonManifest));
		Manifest = Manifest? realloc(Manifest, Temp): calloc(1, Temp);		memset(Manifest, 0, Temp);
		ReadResourceBe(path, BeResourceManifestPath, Manifest, Temp, &Remaining);
	}while(Remaining);
	

	JsonValue *Root = JsonParse(Manifest);
	if(!Root){strcpy(Manifest, "{}");		Root = JsonParse(Manifest);}
	switch(Type){
		case JTYPE_STRING:	{JsonSetString(Root, target, va_arg(ls, char *));	break;}
		case JTYPE_NUMBER:	{JsonSetNumber(Root, target, va_arg(ls, double));	break;}
		case JTYPE_BOOL:	{JsonSetBool(Root, target, va_arg(ls, bool));		break;}
		case JTYPE_NULL:	{JsonSetNull(Root, target);							break;}
		case JTYPE_OBJECT:	{JsonSetObject(Root, target);						break;}
	}
	char *NewManifest = JsonSerialize(Root);	JsonFree(Root, NULL);
	NewManifest = realloc(NewManifest, sizeof(JsonManifest));
	memset(NewManifest + strlen(NewManifest), 0, sizeof(JsonManifest) - strlen(NewManifest));
	
	bool Ret = ModifyResourceBe(path, BeResourceManifestPath, BeResourceManifestFile, 
		BRTFile, BRFTManifest, sizeof(JsonManifest), NewManifest);
	free(NewManifest);
	free(Manifest);
	free(bheader);
	return Ret;
}

void *ReadSectionFromManifestBe(const char *path, char *manifestpath){
	void *bheader = ReadBeHeader(path), *Out = NULL;
	DecodeBeExecutableHeader(bheader);
	BeSectionDescriptor *system = FindSectionBe(bheader, BH->bSystemSection);
	char *Manifest = calloc(2048, sizeof(char));
	ReadResourceBe(path, BeResourceManifestPath, Manifest, 2048, NULL);
	JsonValue *JsonRoot = JsonreadValue((const char **)(&Manifest));
	JsonValue *Value = JsongetValue(JsonRoot, manifestpath);
	if(Value){Out = ReadSectionBe(path, bheader, Value->stringValue);}
	
	free(bheader);	free(Manifest);
	if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
		fprintf(f, "\n[%s:%u:ReadSectionFromManifest]", __FILE__, (uint32_t)__LINE__);
		fclose(f);
	}printf("\n[%s:%u:ReadSectionFromManifest]", __FILE__, (uint32_t)__LINE__);
	DumpBeHeader(path, bheader);
	JsonFree(JsonRoot, NULL);
	return Out;
}

bool AddSectionBe(const char *path, SectionNameBe Name, BeSectionFlags Flags, void *RawData, 
	uint64_t NBytes, uint64_t VirtualAddress, uint64_t VirtualSize, Alignment Align
){
    VirtualAddress = RoundUp(VirtualAddress, Align);
    VirtualSize = RoundUp(VirtualSize, Align);
    NBytes = RoundUp(NBytes, Align);

    void *bheader = ReadBeHeader(path);
    DecodeBeExecutableHeader(bheader);

    // Assuming BH and BSDs map inside bheader:
    // BeHeader *BH = (BeHeader*)bheader;
    // BeSectionDescriptor *BSDs = (BeSectionDescriptor*)(BH + 1);

    uint32_t RawPointer = 0;
    if(FindSectionBe(bheader, Name)){
        BeSectionDescriptor *existing = FindSectionBe(bheader, Name);
        GenericLengthType OriginalSize = existing->bRawSize;
        void *Temp = ReadSectionBe(path, bheader, Name);
        RemoveSectionBe(path, bheader, Name);

        // Refresh header reference after modification
        free(bheader);
        bheader = ReadBeHeader(path);
        Temp = realloc(Temp, NBytes);

        // Standard C compliant pointer arithmetic
        memset((uint8_t*)Temp + OriginalSize, 0, NBytes - OriginalSize);

        // Correct function arguments passed to recursive call
        bool res = AddSectionBe(path, Name, Flags, Temp, NBytes, VirtualAddress, VirtualSize, Align);
        free(Temp);
        free(bheader);
        return res; // Return early; recursive call handled section writing
    } 
    
    // Check section header space bounds BEFORE incrementing
    if((BH->bNSections + 1) * sizeof(BeSectionDescriptor) > BH->bRawSize){
        free(bheader);
        return false;
    }
    RawPointer = BH->bRawSize + *((RelativeVirtualOffset *)bheader);

    for(uint32_t cc = 0; cc < BH->bNSections; ++cc){
        if(flagcheck(Flags, SFAllocatable) && VirtualAddress >= BSDs[cc].bVirtualAddress && 
            VirtualAddress < (BSDs[cc].bVirtualAddress + BSDs[cc].bVirtualSize)
		){free(bheader);    return false;}

        // Search for gaps between consecutive sections
        if(cc > 0){
            if(BSDs[cc - 1].bRawPointer && (BSDs[cc].bRawPointer - (BSDs[cc - 1].bRawPointer + BSDs[cc - 1].bRawSize)) >= NBytes){
                RawPointer = BSDs[cc - 1].bRawPointer + BSDs[cc - 1].bRawSize;
                break;
            }
        }
        if((BSDs[cc].bRawPointer + BSDs[cc].bRawSize) > RawPointer){RawPointer = BSDs[cc].bRawPointer + BSDs[cc].bRawSize;}
    }

    // Align file raw pointer
    RawPointer = RoundUp(RawPointer, Align);
    BH->bNSections++;
    BSDs[BH->bNSections - 1] = (BeSectionDescriptor){
        .bAlignment = Align, .bFlags = Flags,
        .bName = {0}, .bRawPointer = RawPointer,
        .bRawSize = NBytes, .bVirtualAddress = VirtualAddress,
        .bVirtualSize = VirtualSize, .bSectionMagic = {.Magic = BeSectionMagic}
    };
    memcpy(BSDs[BH->bNSections - 1].bName, Name, GetSectionNameLenBe(Name));
    DumpBeHeader(path, bheader);
    WriteSectionBe(path, bheader, Name, RawData, flagcheck(Flags, SFAllocatable) ? VirtualAddress : RawPointer, NBytes);
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
		if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
			fprintf(f, "\n[%s:%u:RemoveSection]", __FILE__, (uint32_t)__LINE__);
			fclose(f);
		}printf("\n[%s:%u:RemoveSection]", __FILE__, (uint32_t)__LINE__);
		DumpBeHeader(path, bheader);
	}
	return true;
}

bool CreateResourceSectionBe(char *path, SectionNameBe Name, uint64_t NTotalBytes){
	//	Allocate 1 Mb of Data.
	void *TempData = calloc(1, NTotalBytes);
	if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
		fprintf(f, "\n[%s:%u:CreateResourceSection]", __FILE__, (uint32_t)__LINE__);
		fclose(f);
	}printf("\n[%s:%u:CreateResourceSection]", __FILE__, (uint32_t)__LINE__);
	bool Ret = AddSectionBe(path, Name, SDDiscardableReadOnlyData, TempData, BeResourceSectionSize, 0x0, 0x0, BeStandardAlign);

	void *bheader = ReadBeHeader(path);
	DecodeBeExecutableHeader(bheader);
	memcpy(BH->bSystemSection, Name, sizeof(SectionNameBe));
	if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
		fprintf(f, "\n[%s:%u:CreateResourceSection]", __FILE__, (uint32_t)__LINE__);
		fclose(f);
	}printf("\n[%s:%u:CreateResourceSection]", __FILE__, (uint32_t)__LINE__);
	DumpBeHeader(path, bheader);

	BeSectionDescriptor *This = FindSectionBe(bheader, Name);
	BeResourceHeader *RHeader = TempData;	*RHeader = (BeResourceHeader){
		.bRootDirectoryRVO = This->bRawPointer + sizeof(BeResourceHeader), 
		.bUnusedBytesRVO = This->bRawPointer + sizeof(BeResourceHeader), 
	};
	if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
		fprintf(f, "\n[%s:%u:CreateResourceSection]", __FILE__, (uint32_t)__LINE__);
		fclose(f);
	}printf("\n[%s:%u:CreateResourceSection]", __FILE__, (uint32_t)__LINE__);
	Ret &= WriteSectionBe(path, bheader, Name, TempData, This->bRawPointer, NTotalBytes);
	free(TempData);
	free(bheader);
	return Ret;
}

uint64_t AddRelocationsBe(const char *path, uint64_t VirtualBase, RelativeVirtualOffset *Addresses, BeRelocationType *Types, GenericLengthType N){
	void *bheader = ReadBeHeader(path);
	DecodeBeExecutableHeader(bheader);
	
	void *Data = ReadSectionBe(path, bheader, DefRelocationSectionName);
	BeRelocationHeader *Header = Data;
	BeSectionDescriptor *This = FindSectionBe(bheader, DefRelocationSectionName);
	void *Unused = GetAtRVOFromSectionDataBe(Header->bUnusedRVO, DefRelocationSectionName, Data, bheader);
	//	Just add a Directory with all the Entries
	Header->bUnusedRVO += sizeof(BeRelocationDirectory) + (N * sizeof(BeRelocationEntry));
	BeRelocationDirectory *Dir = Unused;	*Dir = (BeRelocationDirectory){.bAddress = VirtualBase, 
		.bDirectorySize = sizeof(BeRelocationDirectory) + (N * sizeof(BeRelocationEntry)), 
	};
	uint64_t Out = (uint64_t)(Header->bNDirectories++) << 32;
	while(N--){((BeRelocationEntry *)((void *)Dir + sizeof(BeRelocationDirectory)))[N] = (BeRelocationEntry){.bOffset = Addresses[N] - VirtualBase, .bType = Types[N]};}
	if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
		fprintf(f, "\n\t[%s:%u]", __FILE__, (uint32_t)__LINE__);
		fclose(f);
	}printf("\n\t[%s:%u]", __FILE__, (uint32_t)__LINE__);
	WriteSectionBe(path, bheader, DefRelocationSectionName, Data, This->bRawPointer, This->bRawSize);
	free(bheader);	free(Data);	
	return Out;
}
uint64_t FindRelocationBe(const char *path, uint64_t VirtualAddress){
	void *bheader = ReadBeHeader(path);
	DecodeBeExecutableHeader(bheader);
	// void *Data = ReadSectionFromManifestBe(path, JsonRelocSectionNamePath);
	void *Data = ReadSectionBe(path, bheader, DefRelocationSectionName);
	BeRelocationHeader *Header = Data;
	// SectionNameBe ThisName = {0};
	// ReadResourceBe(path, JsonRelocSectionNamePath, &ThisName, sizeof(ThisName), NULL);
	BeRelocationDirectory *Dir = GetAtRVOFromSectionDataBe(Header->bRelocationDirTableRVO, DefRelocationSectionName, Data, bheader);
	for(GenericLengthType cc = 0; cc < Header->bNDirectories; ++cc){
		GenericLengthType nEntries = (Dir->bDirectorySize - sizeof(BeRelocationDirectory)) / sizeof(BeRelocationEntry);
		for(GenericLengthType cc_ = 0; cc_ < nEntries; ++cc_){
			if(VirtualAddress == (Dir->bAddress + ((BeRelocationEntry *)((void *)Dir + sizeof(BeRelocationDirectory)))[cc].bOffset)){
				free(bheader);	free(Data);
				return (((uint64_t)cc) >> 32) | cc_;
			}
		}
		Dir = ((void *)Dir) + Dir->bDirectorySize;
	}
	return 0;
}
bool CreateRelocationSectionBe(const char *path, SectionNameBe Name, 
	RelativeVirtualOffset *AddressPerDirectory, uint16_t **OffsetsPerDirectory, 
	BeRelocationType **TypesPerDirectory, GenericLengthType nDirectories, 
	GenericLengthType *nPerDirectory, GenericLengthType Additional
){
	uint64_t totalNOffsets = 0, ByteOffset = 0;
	for(GenericLengthType cc = 0; cc < nDirectories; ++cc){totalNOffsets += nPerDirectory[cc];}

	uint64_t NBytes = sizeof(BeRelocationHeader) + (sizeof(BeRelocationDirectory) * nDirectories) + (sizeof(BeRelocationEntry) * totalNOffsets) + Additional;
	void *Out = calloc(1, NBytes);
	bool Ret = AddSectionBe(path, Name, SDDiscardableReadOnlyData, Out, NBytes, 0x0, NBytes, BeStandardAlign);
	void *bheader = ReadBeHeader(path);
	DecodeBeExecutableHeader(bheader);
	BeSectionDescriptor *This = FindSectionBe(bheader, Name);
	
	BeRelocationHeader *Header = Out;	*Header = (BeRelocationHeader){
		.bNDirectories = nDirectories, .bUnusedRVO = This->bRawPointer + (NBytes - Additional), 
		.bRelocationDirTableRVO = This->bRawPointer + sizeof(BeRelocationHeader)
	};

	for(register GenericLengthType cc = 0; cc < nDirectories; ++cc){
		BeRelocationDirectory *Directory = Out + sizeof(BeRelocationHeader) + ByteOffset;
		BeRelocationEntry *Table = (void *)Directory + sizeof(BeRelocationDirectory);
		*Directory = (BeRelocationDirectory){
			//	We Temporarily only Generate a Offset=0 per Section
			.bDirectorySize = sizeof(BeRelocationDirectory) + (sizeof(BeRelocationEntry) * nPerDirectory[cc]), 
			.bAddress = AddressPerDirectory[cc]
		};
		for(register GenericLengthType cc_ = 0; cc_ < nPerDirectory[cc]; ++cc_){
			Table[cc_] = (BeRelocationEntry){.bOffset = OffsetsPerDirectory[cc][cc_], .bType = TypesPerDirectory[cc][cc_]};
		}
		ByteOffset += Directory->bDirectorySize;
	}
	Ret &= WriteSectionBe(path, bheader, Name, Out, This->bRawPointer, NBytes);

	if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
		fprintf(f, "\n[%s:%u:CreateResourceSection]", __FILE__, (uint32_t)__LINE__);
		fclose(f);
	}printf("\n[%s:%u:CreateResourceSection]", __FILE__, (uint32_t)__LINE__);
		// Ret &= UpdateJsonSchemaBe(path, JsonRelocSectionNamePath, JTYPE_STRING, Name);
	free(bheader);
	free(Out);
	return Ret;
}

bool CreateImportSectionBe(
	const char *path, SectionNameBe Name, char **DLLPaths, char ***imports, 
	GenericLengthType *nImportsPerDll, GenericLengthType nDlls, 
	RelativeVirtualOffset *VirtualPerEntry, BeRelocationType *TypePerEntry
){
	uint64_t SectionSize = sizeof(BeImportHeader) + (sizeof(BeImportDll) * nDlls), totalNImports = 0;
	for(register GenericLengthType cc = 0; cc < nDlls; ++cc){
		totalNImports += nImportsPerDll[cc]/* + ((cc % 8) == 0)*/;
		SectionSize += strlen(DLLPaths[cc]) + 1;
		for(register GenericLengthType cc_ = 0; cc_ < nImportsPerDll[cc]; ++cc_){SectionSize += (strlen(imports[cc][cc_]) + 1);}
	}	SectionSize += totalNImports * sizeof(BeImportEntry);
	
	uint32_t importCounter = 0;
	void *Out = calloc(1, SectionSize);
	bool Ret = AddSectionBe(path, Name, SDDiscardableReadOnlyData, Out, SectionSize, 0x0, 0x0, BeStandardAlign);
	void *bheader = ReadBeHeader(path);
	DecodeBeExecutableHeader(bheader);
	BeSectionDescriptor *This = FindSectionBe(bheader, Name);
	BeImportHeader *Header = Out;	*Header = (BeImportHeader){.bNDllReferences = nDlls, .bImportTableRVO = This->bRawPointer + sizeof(BeImportHeader), 
		.bImportPathRVO = This->bRawPointer + sizeof(BeImportHeader) + (sizeof(BeImportDll) * nDlls) + (sizeof(BeImportEntry) * totalNImports)};
	for(register GenericLengthType cc = 0, Temp = sizeof(BeImportHeader), offset = 0; cc < nDlls; ++cc){
		uint64_t RelBaseN = AddRelocationsBe(path, VirtualPerEntry[importCounter], VirtualPerEntry + importCounter, TypePerEntry + importCounter, nImportsPerDll[cc]);
		*((BeImportDll *)(Out + Temp)) = (BeImportDll){.bImportPathIndex = cc, .bNImports = nImportsPerDll[cc]};
		for(register GenericLengthType cc_ = 0; cc_ < nImportsPerDll[cc]; ++cc_){
			((BeImportDll *)(Out + Temp))->Table[cc_] = (BeImportEntry){.bHash = {0}, .bRelocation = RelBaseN++};
			blake2b_state bstate;	blake2b_init(&bstate, sizeof(GenericHashType));
			blake2b_update(&bstate, imports[cc][cc_], strlen(imports[cc][cc_]));
			blake2b_final(&bstate, ((BeImportEntry *)(Out + Temp + sizeof(BeImportDll)))[cc_].bHash, sizeof(GenericHashType));
		}
		Temp += sizeof(BeImportDll) + (nImportsPerDll[cc] * sizeof(BeImportEntry));
		strcpy(Out + (Header->bImportPathRVO - This->bRawPointer) + offset, DLLPaths[cc]);
		offset += (strlen(DLLPaths[cc]) + 1);
	}
	if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
		fprintf(f, "\n[%s:%u:CreateResourceSection]", __FILE__, (uint32_t)__LINE__);
		fclose(f);
	}printf("\n[%s:%u:CreateResourceSection]", __FILE__, (uint32_t)__LINE__);
	Ret &= WriteSectionBe(path, bheader, Name, Out, This->bRawPointer, SectionSize);
		// Ret &= UpdateJsonSchemaBe(path, JsonImportSectionNamePath, JTYPE_STRING, Name);
	free(bheader);
	free(Out);
	return Ret;
}

bool CreateExceptionSectionBe(const char *path, SectionNameBe Name, 
	RelativeVirtualOffset *VirtualAddresses, GenericLengthType *VirtualLengths, 
	RelativeVirtualOffset *VirtualHandlers, GenericLengthType N, bool FunctionLengthsKnown
){
	BeExceptionHandler *Data = calloc(N, sizeof(BeExceptionHandler));
	for(register GenericLengthType cc = 0; cc < N; ++cc){Data[cc] = (BeExceptionHandler){
		.bHandlerRVO = VirtualHandlers[cc], .bVirtualAddressRVO = VirtualAddresses[cc], 
		.End = {
			.bVirtualEndRVO = FunctionLengthsKnown? VirtualAddresses[cc] + VirtualLengths[cc]: VirtualLengths[cc], 
			.bUnknownFunctionSize = !FunctionLengthsKnown
		}
	};}
	if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
		fprintf(f, "\n[%s:%u:CreateResourceSection]", __FILE__, (uint32_t)__LINE__);
		fclose(f);
	}printf("\n[%s:%u:CreateResourceSection]", __FILE__, (uint32_t)__LINE__);
	bool Ret = AddSectionBe(path, Name, SDDiscardableReadOnlyData, Data, N * sizeof(BeExceptionHandler), 0x0, 0x0, BeStandardAlign);
		// Ret &= UpdateJsonSchemaBe(path, JsonExceptionSectionNamePath, JTYPE_STRING, Name);
	free(Data);
	return Ret;
}

BeResourceFileType MapPeResourceType(PeResourceType id){
    switch(id){
        case RT_MANIFEST: return BRFTManifest;
        case RT_STRING:   return BRFTString;
        case RT_BITMAP:   return BRFTBitmap;
        case RT_ICON:     return BRFTIcon;
        case RT_FONT:     return BRFTFont;
        default:          return BRFTRawData;
    }
}

char *ExtractResourceName(void *rsrcBase, uint32_t nameOffsetOrId, bool isString){
    char nameBuffer[256] = {0};
    if(isString){
        // NameOffset points to a uint16_t length followed by UTF-16LE characters
        uint16_t *strPtr = (uint16_t *)((uint8_t *)rsrcBase + (nameOffsetOrId & 0x7FFFFFFF)), 
				*utf16Str = &strPtr[1], 
				len = strPtr[0];
        for(register uint16_t i = 0; i < len && i < 255; i++){
            nameBuffer[i] = (char)(utf16Str[i] & 0xFF);
        }
    }else{snprintf(nameBuffer, sizeof(nameBuffer), "#%u", nameOffsetOrId);}
    return strdup(nameBuffer);
}

// Internal recursive worker passing rsrcBase context to prevent repeated I/O reads
static BeResourceConfigurator ParseDirectoryNodeInternal(
    PeResourceDirectoryEntry *Entry, ExpandedPeExecutable *Image, 
    void *rsrcBase, GenericLengthType *TotalBytes
){
    BeResourceConfigurator Out = {0};
    uint32_t offset = Entry->mOffset & 0x7FFFFFFF;
    Out.Name = ExtractResourceName(rsrcBase, Entry->mNameOffset, Entry->mNameIsString);
    if(Out.Name){(*TotalBytes) += strlen(Out.Name);}

    if(Entry->mIsDirectory){
        Out.Type = BRTDirectory;
        Out.Directory.Type = BRDTNoType;
        Out.Directory.nFiles = 0;
        Out.Directory.Files = NULL;
        (*TotalBytes) += sizeof(BeResourceDirectory);

        //	Treat offset as a subdirectory entry pointer
        PeResourceDirectoryEntry *ChildEntry = (PeResourceDirectoryEntry *)((uint8_t *)rsrcBase + offset);
        BeResourceConfigurator NextDir = ParseDirectoryNodeInternal(ChildEntry, Image, rsrcBase, TotalBytes);
        
        Out.Directory.NextDirectory = malloc(sizeof(BeResourceConfigurator));
        if(Out.Directory.NextDirectory){
            memcpy(Out.Directory.NextDirectory, &NextDir, sizeof(BeResourceConfigurator));
        }
    }else{
        // Processing a leaf File Node
        Out.Type = BRTDirectory; // Container Node
        Out.Directory.Type = BRDTNoType;
        Out.Directory.nFiles = 1;
        Out.Directory.NextDirectory = NULL;
        Out.Directory.Files = calloc(1, sizeof(BeResourceConfigurator));
        if(Out.Directory.Files){
            PeResourceDataEntry *File = (PeResourceDataEntry *)((uint8_t *)rsrcBase + offset);
            //	Generate clean filename
            size_t nameLen = Out.Name ? strlen(Out.Name) : 0;
            char *fileName = malloc(nameLen + 3);
            if(fileName){snprintf(fileName, nameLen + 3, "%s_F", Out.Name ? Out.Name : "Res");}
            Out.Directory.Files->Type = BRTFile;
            Out.Directory.Files->Name = fileName;
            Out.Directory.Files->File.Data = GetAtRVAFromSectionDataPe(
                File->mDataRVA, PeResourceSection, 
                Image->Fmt.rsrc.Raw, Image->Raw
            );
            Out.Directory.Files->File.Length = File->mDataSize;
            Out.Directory.Files->File.Type = MapPeResourceType(Entry->mID);
            (*TotalBytes) += File->mDataSize + sizeof(BeResourceFile);
            if(fileName){(*TotalBytes) += strlen(fileName);}
        }
    }
    return Out;
}

BeResourceConfigurator ParseDirectoryNode(PeResourceDirectoryEntry *Directory, 
    ExpandedPeExecutable *Image, GenericLengthType *TotalBytes
){
    void *RSRC = ReadSectionPe(Image->Path, Image->Raw, PeResourceSection);
    if(!RSRC){return (BeResourceConfigurator){0};}
    BeResourceConfigurator Out = ParseDirectoryNodeInternal(Directory, Image, RSRC, TotalBytes);
    free(RSRC);
    return Out;
}
static BeResourceConfigurator *GetOrCreateDirNode(BeResourceConfigurator *parentDir, const char *dirName){
    if(!parentDir){return NULL;}
    // Search existing subdirectories hanging off parentDir
    BeResourceConfigurator *curr = parentDir->Directory.NextDirectory;
    BeResourceConfigurator *prev = NULL;

    while(curr){
		if(curr->Type == BRTDirectory && curr->Name && strcmp(curr->Name, dirName) == 0){return curr;}
        prev = curr;
        curr = curr->Directory.NextDirectory;
    }

    // Allocate new directory node if not found
    BeResourceConfigurator *newDir = calloc(1, sizeof(BeResourceConfigurator));
    if(!newDir){return NULL;}

    newDir->Type = BRTDirectory;
    newDir->Name = strdup(dirName);
    newDir->Directory.Type = BRDTNoType;

    if(prev){prev->Directory.NextDirectory = newDir;
    }else{parentDir->Directory.NextDirectory = newDir;}
    return newDir;
}

BeResourceConfigurator *ParseResourceDirectoryTree(ExpandedPeExecutable *Image, GenericLengthType *totalN, GenericLengthType *TotalBytes){
    uint32_t totalEntries = Image->Fmt.rsrc.RootDirectory->mNNameEntries + Image->Fmt.rsrc.RootDirectory->mNIDEntres;

    // Allocate unifying Root Node "##"
    BeResourceConfigurator *root = calloc(1, sizeof(BeResourceConfigurator));	*root = (BeResourceConfigurator){
		.Type = BRTDirectory, .Name = strdup(BeResourceRootName), 
		.Directory = {.Type = BRDTNoType}
	};

    *TotalBytes = sizeof(BeResourceHeader) + sizeof(BeResourceDirectory) + sizeof(BeResourceRootName);
    if(totalN){*totalN = totalEntries;}

    void *rsrcBase = ReadSectionPe(Image->Path, Image->Raw, PeResourceSection);
    if(!rsrcBase){return root;}

    BeResourceConfigurator *lastSibling = NULL;
    for(register uint32_t cc = 0; cc < totalEntries; ++cc){
        PeResourceDirectoryEntry *entry = Image->Fmt.rsrc.REntries.ResourceEntries + cc;
        
        // Parse node dynamically
        BeResourceConfigurator childNode = ParseDirectoryNodeInternal(entry, Image, rsrcBase, TotalBytes);
        BeResourceConfigurator *heapNode = malloc(sizeof(BeResourceConfigurator));
        if(heapNode){
            memcpy(heapNode, &childNode, sizeof(BeResourceConfigurator));
            heapNode->Directory.NextDirectory = NULL;

            // Link as child sibling of "##"
            if(!root->Directory.NextDirectory){root->Directory.NextDirectory = heapNode;}else
			if(lastSibling){lastSibling->Directory.NextDirectory = heapNode;}
            lastSibling = heapNode;
        }
    }

    free(rsrcBase);
    return root;
}

void FreeBeResourceConfigurator(BeResourceConfigurator *node){
    if(!node){return;}
    if(node->Name){
        free(node->Name);
        node->Name = NULL;
    }
    if(node->Type == BRTDirectory){
        //	Recursively free linked sibling directories
        if(node->Directory.NextDirectory){
            FreeBeResourceConfigurator(node->Directory.NextDirectory);
            node->Directory.NextDirectory = NULL;
        }

        //	Free child files array
        if(node->Directory.Files){
            for(GenericLengthType i = 0; i < node->Directory.nFiles; i++){
                BeResourceConfigurator *fileNode = &node->Directory.Files[i];
                if(fileNode->Name){free(fileNode->Name);}
                if(fileNode->File.Data){free(fileNode->File.Data);}
            }
            free(node->Directory.Files);
            node->Directory.Files = NULL;
        }
        free(node);
    }else if(node->Type == BRTFile){
        if(node->File.Data){
            free(node->File.Data);
            node->File.Data = NULL;
        }
        free(node);
    }
}

BeResourceConfigurator *CreateResourceTreeFromPaths(size_t numFiles, const char **paths, ...){
    BeResourceConfigurator *root = calloc(1, sizeof(BeResourceConfigurator));

    // Enforce "##" as the root name
    root->Type = BRTDirectory;
    root->Name = strdup("##");
    root->Directory.Type = BRDTNoType;

    va_list args;		va_start(args, paths);

    for(register size_t i = 0; i < numFiles; i++){
        BeResourceConfigurator *currentDir = root;
        BeResourceFileType fType = va_arg(args, BeResourceFileType);
        GenericLengthType fLen   = va_arg(args, GenericLengthType);
        const void *fData        = va_arg(args, const void *);

        char *pathCopy = strdup(paths[i]), 
			*saveptr = NULL, 
			*token = strtok_r(pathCopy, "\\/", &saveptr);

        // Strip leading "##" if present in path input string
        if(token && strcmp(token, BeResourceRootName) == 0){token = strtok_r(NULL, "\\/", &saveptr);}

        while(token != NULL){
            char *nextToken = strtok_r(NULL, "\\/", &saveptr);
            if(nextToken != NULL){
				if(!(currentDir = GetOrCreateDirNode(currentDir, token))){break;}
            }else{
                size_t nFiles = currentDir->Directory.nFiles;
                BeResourceConfigurator *reallocatedFiles = realloc(
                    currentDir->Directory.Files, 
                    sizeof(BeResourceConfigurator) * (nFiles + 1)
                );
                if(reallocatedFiles){
                    currentDir->Directory.Files = reallocatedFiles;
                    BeResourceConfigurator *fileNode = &currentDir->Directory.Files[nFiles];
					memset(fileNode, 0, sizeof(BeResourceConfigurator));
					*fileNode = (BeResourceConfigurator){
						.Type = BRTFile, .Name = strdup(token), 
						.File = {
							.Type = fType, .Length = fLen, 
							.Data = fData && fLen > 0? malloc(fLen): NULL
						}
					};
					if(fileNode->File.Data){memcpy(fileNode->File.Data, fData, fLen);}
                    currentDir->Directory.nFiles++;
                }
            }
            token = nextToken;
        }
        free(pathCopy);
    }
    va_end(args);
    return root;
}

bool UpdateResourceTreeBe(BeResourceConfigurator *root, size_t numFiles, const char **paths, ...){
    va_list args;	va_start(args, paths);
    for(register size_t i = 0; i < numFiles; i++){
        BeResourceFileType fType = va_arg(args, BeResourceFileType);
        GenericLengthType fLen   = va_arg(args, GenericLengthType);
        const void *fData        = va_arg(args, const void *);

        char *pathCopy = strdup(paths[i]),
			*saveptr = NULL,
			*token = strtok_r(pathCopy, "\\/", &saveptr);

        // Standardize root lookup
        BeResourceConfigurator *currentDir = root;
        if(token && strcmp(token, BeResourceRootName) == 0){token = strtok_r(NULL, "\\/", &saveptr);}

        while(token != NULL){
            char *nextToken = strtok_r(NULL, "\\/", &saveptr);
            if(nextToken != NULL){currentDir = GetOrCreateDirNode(currentDir, token);}else{
                BeResourceConfigurator *existingFile = NULL;
                for(register GenericLengthType f = 0; f < currentDir->Directory.nFiles; f++){
                    if(currentDir->Directory.Files[f].Name && strcmp(currentDir->Directory.Files[f].Name, token) == 0){
                        existingFile = &currentDir->Directory.Files[f];
                        break;
                    }
                }
                if(existingFile){
                    if(existingFile->File.Data){
                        free(existingFile->File.Data);
                        existingFile->File.Data = NULL;
                    }
                    existingFile->File.Type = fType;
                    existingFile->File.Length = fLen;
                    if(fData && fLen > 0){
                        existingFile->File.Data = malloc(fLen);
                        if(existingFile->File.Data){memcpy(existingFile->File.Data, fData, fLen);}
                    }
                }else{
                    size_t nFiles = currentDir->Directory.nFiles;
                    BeResourceConfigurator *reallocatedFiles = realloc(
                        currentDir->Directory.Files,
                        sizeof(BeResourceConfigurator) * (nFiles + 1)
                    );

                    if(reallocatedFiles){
                        currentDir->Directory.Files = reallocatedFiles;
                        BeResourceConfigurator *fileNode = &currentDir->Directory.Files[nFiles];
						memset(fileNode, 0, sizeof(BeResourceConfigurator));
						*fileNode = (BeResourceConfigurator){
							.Type = BRTFile, .Name = strdup(token), 
							.File = {
								.Data = fData && fLen > 0? malloc(fLen): NULL, 
								.Length = fLen, .Type = fType
							}
						};
                        if(fileNode->File.Data){memcpy(fileNode->File.Data, fData, fLen);}
                        currentDir->Directory.nFiles++;
                    }
                }
            }
            token = nextToken;
        }
        free(pathCopy);
    }
    va_end(args);
    return true;
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

char *BeRelocationTypeToString(BeRelocationType Type){
	switch(Type){
		default:
		case BRETAbsolute:	return MAKESTR(BRETAbsolute);
		case BRET16:		return MAKESTR(BRET16);
		case BRET32:		return MAKESTR(BRET32);
		case BRET64:		return MAKESTR(BRET64);
	}
}

char *BeSectionFlagsToString(BeSectionFlags Flags){
    char *outStr = NULL;
    size_t currentLen = 0;

	APPEND_FLAG(Flags, SFAllocatable);
	APPEND_FLAG(Flags, (SFPersistent));
	APPEND_FLAG(Flags, SFRelocatable);
	APPEND_FLAG(Flags, SFInitData);
	APPEND_FLAG(Flags, SFUInitData);
	APPEND_FLAG(Flags, SFExecutable);
	APPEND_FLAG(Flags, SFReadable);
	APPEND_FLAG(Flags, SFWritable);
	APPEND_FLAG(Flags, SFSharable);

	return outStr;
}

void DumpBe(const char *path){
	void *bh = ReadBeHeader(path);
	DecodeBeExecutableHeader(bh);
	if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
		fprintf(f, "\n[%s]"
			"\nHeader: {"
			"\n  .Magic\t%llu"
			"\n  .SystemSection\t%.16s"
			"\n  .NSections\t%llu"
			"\n  .RawHeaderSize\t%llu"
			"\n  .SectionTabeOffset\t%llu", path, (uint64_t)BH->bMagic.uMagic, 
			BH->bSystemSection, (uint64_t)BH->bNSections, 
			(uint64_t)BH->bRawSize, (uint64_t)BH->bSectionTableOffset);
		fclose(f);
	}printf("\n[%s]"
		"\nHeader: {"
		"\n  .Magic\t%llu"
		"\n  .SystemSection\t%.16s"
		"\n  .NSections\t%llu"
		"\n  .RawHeaderSize\t%llu"
		"\n  .SectionTabeOffset\t%llu", path, (uint64_t)BH->bMagic.uMagic, 
		BH->bSystemSection, (uint64_t)BH->bNSections, 
		(uint64_t)BH->bRawSize, (uint64_t)BH->bSectionTableOffset);
	for(uint32_t cc = 0; cc < BH->bNSections; ++cc){
		char *Flags = BeSectionFlagsToString(BSDs[cc].bFlags);
		if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
			fprintf(f, "\n    [%llu]: {"
				"\n      .Magic\t%llu"
				"\n      .Name\t%.16s"
				"\n      .Flags\t%s"
				"\n      .Alignment\t%llu"
				"\n      .VirtualAddress\t%llu"
				"\n      .VirtualSize\t%llu"
				"\n      .RawPointer\t%llu"
				"\n      .RawSize\t%llu", (uint64_t)cc, BSDs[cc].bSectionMagic, 
			BSDs[cc].bName, Flags, BSDs[cc].bAlignment, BSDs[cc].bVirtualAddress, 
			BSDs[cc].bVirtualSize, BSDs[cc].bRawPointer, BSDs[cc].bRawSize);
			fclose(f);
		}printf("\n    [%llu]: {"
			"\n      .Magic\t%llu"
			"\n      .Name\t%.16s"
			"\n      .Flags\t%s"
			"\n      .Alignment\t%llu"
			"\n      .VirtualAddress\t%llu"
			"\n      .VirtualSize\t%llu"
			"\n      .RawPointer\t%llu"
			"\n      .RawSize\t%llu", (uint64_t)cc, BSDs[cc].bSectionMagic, 
			BSDs[cc].bName, Flags, BSDs[cc].bAlignment, BSDs[cc].bVirtualAddress, 
			BSDs[cc].bVirtualSize, BSDs[cc].bRawPointer, BSDs[cc].bRawSize);
		if(!strncmp(BSDs[cc].bName, DefExceptionSectionName, sizeof(SectionNameBe))){
			BeExceptionHandler *Table = ReadSectionBe(path, bh, DefExceptionSectionName);
			uint64_t N = __min(BeDumpVolumeLine, (FindSectionBe(bh, DefExceptionSectionName))->bRawSize / sizeof(BeExceptionHandler));
			if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
				fprintf(f, "\n      {");
				fclose(f);
			}printf("\n      {");
			do{
				if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
					fprintf(f, "\n        [%llu]: {"
						"\n          .VirtualAddress\t%llu"
						"\n          .HandlerAddress\t%llu"
						"\n          .%s\t%llu"
						"\n        }", (uint64_t)N, (uint64_t)Table[N].bVirtualAddressRVO, (uint64_t)Table[N].bHandlerRVO, 
						(Table[N].End.bUnknownFunctionSize? "NInstructions": "NBytes"), (uint64_t)Table[N].End.bUnknownFunctionSize);
					fclose(f);
				}printf("\n        [%llu]: {"
					"\n          .VirtualAddress\t%llu"
					"\n          .HandlerAddress\t%llu"
					"\n          .%s\t%llu"
					"\n        }", (uint64_t)N, (uint64_t)Table[N].bVirtualAddressRVO, (uint64_t)Table[N].bHandlerRVO, 
					(Table[N].End.bUnknownFunctionSize? "NInstructions": "NBytes"), (uint64_t)Table[N].End.bUnknownFunctionSize);
			}while(N--);
			if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
				fprintf(f, "\n      }");
				fclose(f);
			}printf("\n      }");
			free(Table);
		}else if(!strncmp(BSDs[cc].bName, DefRelocationSectionName, sizeof(SectionNameBe))){
			const BeSectionDescriptor *This = FindSectionBe(bh, DefRelocationSectionName);
			BeRelocationHeader *Rel = ReadSectionBe(path, bh, DefRelocationSectionName);
			if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
				fprintf(f, "\n      Header: {"
					"\n        .DirectoryTable\t%llu"
					"\n        .UnusedBytes\t%llu"
					"\n        .NRelocationDirectories\t%llu", 
					Rel->bRelocationDirTableRVO, Rel->bUnusedRVO, Rel->bNDirectories);
				fclose(f);
			}printf("\n      Header: {"
				"\n        .DirectoryTable\t%llu"
				"\n        .UnusedBytes\t%llu"
				"\n        .NRelocationDirectories\t%llu", 
				Rel->bRelocationDirTableRVO, Rel->bUnusedRVO, Rel->bNDirectories);
			uint64_t Offset = 0;
			const BeRelocationDirectory *Root = GetAtRVOFromSectionDataBe(Rel->bRelocationDirTableRVO, DefRelocationSectionName, Rel, bh);
			for(uint32_t cc_ = 0; cc_ < __min(BeDumpVolume, Rel->bNDirectories); ++cc_){
				const BeRelocationDirectory *Dir = (void *)Root + Offset;
				const BeRelocationEntry *Table = (void *)Dir + sizeof(BeRelocationDirectory);
				const uint32_t N = (Dir->bDirectorySize - sizeof(BeRelocationDirectory)) / sizeof(BeRelocationEntry);
				if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
					fprintf(f, "\n        [%llu]: {"
						"\n          .BaseAddress\t%llu"
						"\n          .DirectorySize\t%llu(%llu Entries)\n            ", (uint64_t)cc_, 
						(uint64_t)Dir->bAddress, (uint64_t)Dir->bDirectorySize, (uint64_t)N);
					fclose(f);
				}printf("\n        [%llu]: {"
					"\n          .BaseAddress\t%llu"
					"\n          .DirectorySize\t%llu(%llu Entries)\n            ", (uint64_t)cc_, 
					(uint64_t)Dir->bAddress, (uint64_t)Dir->bDirectorySize, (uint64_t)N);
				for(uint32_t cc__ = 0; cc__ < __min(BeDumpVolumeLine, N); cc__++){
					if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
						fprintf(f, " [%llu]: {"
							"\n            .Type\t%s"
							"\n            .Offset\t%llu(0x%X)"
							"\n          }", (uint64_t)cc__, BeRelocationTypeToString(Table[cc__].bType), 
							(uint64_t)Table[cc__].bOffset, (uint64_t)(Dir->bAddress + (uint64_t)Table[cc__].bOffset));
						fclose(f);
					}printf(" [%llu]: {"
						"\n            .Type\t%s"
						"\n            .Offset\t%llu(0x%X)"
						"\n          }", (uint64_t)cc__, BeRelocationTypeToString(Table[cc__].bType), 
						(uint64_t)Table[cc__].bOffset, (uint64_t)(Dir->bAddress + (uint64_t)Table[cc__].bOffset)
					);
				}
				if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
					fprintf(f, "\n        }");
					fclose(f);
				}printf("\n        }");
				Offset += Dir->bDirectorySize;
			}
			if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
				fprintf(f, "\n      }");
				fclose(f);
			}printf("\n      }");
			free(Rel);
		}else if(!strncmp(BSDs[cc].bName, DefImportSectionName, sizeof(SectionNameBe))){
			BeImportHeader *Imp = ReadSectionBe(path, bh, DefImportSectionName);
			BeSectionDescriptor *This = FindSectionBe(bh, DefImportSectionName);
			char *PathTable = GetAtRVOFromSectionDataBe(Imp->bImportPathRVO, DefImportSectionName, Imp, bh);
			if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
				fprintf(f, "\n      Header: {"
					"\n        .NDlls\t%llu"
					"\n        .DllImportPathTableRVO\t%llu"
					"\n        .DllImportTableRVO\t%llu", (uint64_t)Imp->bNDllReferences, 
					(uint64_t)Imp->bImportPathRVO, (uint64_t)Imp->bImportTableRVO);
				fclose(f);
			}printf("\n      Header: {"
				"\n        .NDlls\t%llu"
				"\n        .DllImportPathTableRVO\t%llu"
				"\n        .DllImportTableRVO\t%llu", (uint64_t)Imp->bNDllReferences, 
				(uint64_t)Imp->bImportPathRVO, (uint64_t)Imp->bImportTableRVO
			);
			for(uint32_t cc_ = 0, ByteOffset = 0; cc_ < __min(BeDumpVolume, Imp->bNDllReferences); ++cc_){
				BeImportDll *Table = (void *)Imp + (RvoToFileOffsetBe(Imp->bImportTableRVO, This) - This->bRawPointer) + ByteOffset;
				char *Path = PathTable;		{
					uint32_t N = Table->bImportPathIndex;
					if(N){while(*Path && N--){Path += (strlen(Path) + 1);}}
				};
				if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
					fprintf(f, "\n        [%llu]: {"
						"\n          .ImportPathIndex\t%llu(%s)"
						"\n          .NHashes\t%llu", (uint64_t)cc_, 
						(uint64_t)Table->bImportPathIndex, *Path? Path: "null", Table->bNImports);
					fclose(f);
				}printf("\n        [%llu]: {"
					"\n          .ImportPathIndex\t%llu(%s)"
					"\n          .NHashes\t%llu", (uint64_t)cc_, 
					(uint64_t)Table->bImportPathIndex, *Path? Path: "null", Table->bNImports);
				for(uint32_t cc__ = 0; cc__ < __min(BeDumpVolumeLine, Table->bNImports); ++cc__){
					if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
						fprintf(f, "\n          [%llu]: {"
							"\n            .Hash\t{%X:%X}"
							"\n            .Relocation\t%llu"
							"\n          }", (uint64_t)cc__, (uint64_t)Table->Table[cc__].bHash[0], 
							(uint64_t)Table->Table[cc__].bHash[1], (uint64_t)Table->Table[cc__].bRelocation);
						fclose(f);
					}printf("\n          [%llu]: {"
						"\n            .Hash\t{%X:%X}"
						"\n            .Relocation\t%llu"
						"\n          }", (uint64_t)cc__, (uint64_t)Table->Table[cc__].bHash[0], 
						(uint64_t)Table->Table[cc__].bHash[1], (uint64_t)Table->Table[cc__].bRelocation);
				}
				if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
					fprintf(f, "\n        }");
					fclose(f);
				}printf("\n        }");
				ByteOffset += (Table->bNImports * sizeof(BeImportEntry)) + sizeof(BeImportDll);
			}
			if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
				fprintf(f, "\n      }");
				fclose(f);
			}printf("\n      }");
			free(Imp);
		}else if(!strncmp(BSDs[cc].bName, DefExportSectionName, sizeof(SectionNameBe))){
			BeExportHeader *Exp = ReadSectionBe(path, bh, DefExportSectionName);
			BeExportEntry *Table = GetAtRVOFromSectionDataBe(Exp->bExportTableRVO, DefExportSectionName, Exp, bh);
			char *SymTable = GetAtRVOFromSectionDataBe(Exp->bExportNameRVO, DefExportSectionName, Exp, bh);
			if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
				fprintf(f, "\n      Header: {"
					"\n        .NExported\t%llu"
					"\n        .ExportTableRVO\t%llu"
					"\n        .ExportNameRVO\t%llu"
					"\n        .NExportAddresses\t%llu", (uint64_t)Exp->bNExported, 
					(uint64_t)Exp->bExportTableRVO, (uint64_t)Exp->bExportNameRVO, (uint64_t)Exp->bNExportAddresses);
				fclose(f);
			}printf("\n      Header: {"
				"\n        .NExported\t%llu"
				"\n        .ExportTableRVO\t%llu"
				"\n        .ExportNameRVO\t%llu"
				"\n        .NExportAddresses\t%llu", (uint64_t)Exp->bNExported, 
				(uint64_t)Exp->bExportTableRVO, (uint64_t)Exp->bExportNameRVO, (uint64_t)Exp->bNExportAddresses
			);
			for(uint32_t cc_ = 0; cc_ < __min(BeDumpVolume, (Exp->bNExported + Exp->bNExportAddresses) / 2); ++cc_){
				char *Path = SymTable;		{
					uint32_t N = Table->bNameIndex;
					while(Path && N--){Path += strlen(Path);}
				};
				if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
					fprintf(f, "\n        [%llu]: {"
						"\n          .SymbolHash\t[%llu:%llu]"
						"\n          .VirtualAddress\t%llu"
						"\n          .Flags\t%s"
						"\n          .NameIndex\t%llu(%s)"
						"\n        }", (uint64_t)cc_, (uint64_t)Table->bSymbolHash, 
						(uint64_t)Table->bVirtualAddress, Table->bNameIndex, Path);
					fclose(f);
				}printf("\n        [%llu]: {"
					"\n          .SymbolHash\t[%llu:%llu]"
					"\n          .VirtualAddress\t%llu"
					"\n          .Flags\t%s"
					"\n          .NameIndex\t%llu(%s)"
					"\n        }", (uint64_t)cc_, (uint64_t)Table->bSymbolHash, 
					(uint64_t)Table->bVirtualAddress, Table->bNameIndex, Path
				);
			}
			if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
				fprintf(f, "\n      }");
				fclose(f);
			}printf("\n      }");
			free(Exp);
		}else{
			void *Data = ReadSectionBe(path, bh, BSDs[cc].bName);
			if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
				fprintf(f, "\n      {");
				fclose(f);
			}printf("\n      {");
			for(uint32_t cc_ = 0; cc_ < (BeDumpVolume / BeDumpVolumeLine); ++cc_){
				if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
					fprintf(f, "\n        ");
					fclose(f);
				}printf("\n        ");
				for(uint32_t cc__ = 0; cc__ < BeDumpVolumeLine && BSDs[cc].bRawSize > ((cc_ * (BeDumpVolume / BeDumpVolumeLine)) + cc__); ++cc__){
					if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
						fprintf(f, "%02x ", ((uint8_t *)Data)[cc_ + (cc__ * (PeDumpVolume / PeDumpVolumeLine))]);
						fclose(f);
					}printf("%02x ", ((uint8_t *)Data)[cc_ + (cc__ * (PeDumpVolume / PeDumpVolumeLine))]);}
				}
			if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
				fprintf(f, "\n      }");
				fclose(f);
			}printf("\n      }");
			free(Data);
		}
		if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
		fprintf(f, "\n    ");
		fclose(f);
	}printf("\n    ");
	}
	if(logfile){FILE *f = fopen(logfile, "rb+");    fseek(f, 0, SEEK_END);
		fprintf(f, "}");
		fclose(f);
	}printf("}");
}