#include "exec.h"

void *ReadBeHeader(const socket_t *socket){
	GenericLengthType nbytes = sizeof(BeHeader) + sizeof(RelativeVirtualOffset);
	void *out = __calloc(1, nbytes);
	socket_ret ret = socket->read(socket, 0, sizeof(RelativeVirtualOffset), 0);
	if(!socketreterr(ret, sizeof(RelativeVirtualOffset))){
		CopyMemC(out, ret.data, sizeof(RelativeVirtualOffset));
		__free(ret.data);
		ret = socket->read(socket, *((RelativeVirtualOffset *)out) + __offsetof(BeHeader, bRawSize), sizeof(BeHeader), 0);
		if(!socketreterr(ret, sizeof(BeHeader))){
			nbytes = *((GenericLengthType *)ret.data);      __free(ret.data);
			out = __realloc(out, sizeof(BeHeader) + sizeof(RelativeVirtualOffset), nbytes);
			ret = socket->read(socket, *((RelativeVirtualOffset *)out), nbytes, 0);
			if(!socketreterr(ret, nbytes)){
				CopyMemC(out, ret.data, nbytes);
				return out;
			}
		}
	}
	__free(out);
	return NULL;
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
		if(name[7]){if(!strncmpa(BSDs[cc].bName, name, sizeof(name))){return (BSDs + cc);}
		}else{if(!strcmpa(BSDs[cc].bName, name)){return (BSDs + cc);}}
	}
	return NULL;
}

void *GetAtRVOFromSectionDataBe(RelativeVirtualOffset RVO, SectionNameBe name, void *data, void *bheader){
	BeSectionDescriptor *Section = FindSectionBe(bheader, name);
	uint64_t Address = RvoToFileOffsetBe(RVO, Section);
	return Address? data + (Address - Section->bRawPointer): data;
}
void *ReadAtRVOFromSectionBe(RelativeVirtualOffset RVO, uint32_t Size, 
	SectionNameBe name, const socket_t *socket, void *bheader
){
	BeSectionDescriptor *Section = FindSectionBe(bheader, name);
	uint64_t Address = RvoToFileOffsetBe(RVO, Section);
	if(Address){
		socket_ret ret = socket->read(socket, Address, Size, 0);
		if(!socketreterr(ret, Size)){
			return ret.data;
		}
	}
	return NULL;
}
void *ReadSectionBe(const socket_t *socket, void *bheader, SectionNameBe name){
	BeSectionDescriptor *This = FindSectionBe(bheader, name);
	if(This){
		socket_ret out = socket->read(socket, This->bRawPointer, This->bRawSize, 0);
		return out.data;
	}
}

uint64_t FindRelocationBe(const socket_t *socket, uint64_t VirtualAddress){
	void *bheader = ReadBeHeader(socket);
	DecodeBeExecutableHeader(bheader);
	// void *Data = ReadSectionFromManifestBe(path, JsonRelocSectionNamePath);
	void *Data = ReadSectionBe(socket, bheader, DefRelocationSectionName);
	BeRelocationHeader *Header = Data;
	// SectionNameBe ThisName = {0};
	// ReadResourceBe(path, JsonRelocSectionNamePath, &ThisName, sizeof(ThisName), NULL);
	BeRelocationDirectory *Dir = GetAtRVOFromSectionDataBe(Header->bRelocationDirTableRVO, DefRelocationSectionName, Data, bheader);
	for(GenericLengthType cc = 0; cc < Header->bNDirectories; ++cc){
		GenericLengthType nEntries = (Dir->bDirectorySize - sizeof(BeRelocationDirectory)) / sizeof(BeRelocationEntry);
		for(GenericLengthType cc_ = 0; cc_ < nEntries; ++cc_){
			if(VirtualAddress == (Dir->bAddress + ((BeRelocationEntry *)((void *)Dir + sizeof(BeRelocationDirectory)))[cc].bOffset)){
				__free(bheader);	__free(Data);
				return (((uint64_t)cc) >> 32) | cc_;
			}
		}
		Dir = ((void *)Dir) + Dir->bDirectorySize;
	}
	return 0;
}

bool MatchResourceName(BeResourceString *nameObj, const char *snippet){
    if(!nameObj || !snippet){return false;}
    size_t snippetLen = __strlen(snippet);
    return (nameObj->bStringLength == snippetLen) && 
           (strncmpa(nameObj->String, snippet, snippetLen) == 0);
}
bool ReadResourceBe(const socket_t *socket, const char *target, 
    void *out, GenericLengthType NBytes, GenericLengthType *Remaining
){
    if(!socket || !target || !out){return false;}
    void *bheader = ReadBeHeader(socket);
    if(!bheader){return false;}
    
    DecodeBeExecutableHeader(bheader);
    void *rsrcData = ReadSectionBe(socket, bheader, BH->bSystemSection);
    if(!rsrcData){__free(bheader);	return false;}

    BeResourceHeader *rheader = (BeResourceHeader *)rsrcData;
    void *Entry = GetAtRVOFromSectionDataBe(rheader->bRootDirectoryRVO, BH->bSystemSection, rsrcData, bheader);
	strtok_t *tokstate =	strtok_i(target, "\\/", 0x0);
    char *saveptr = NULL, *snippet = /*strtok_r(targetCopy, &saveptr);*/strtok_k(tokstate);
    bool found = false;
    while(Entry && snippet){
        BeResourceType type = *((BeResourceType *)Entry);
		switch(type){
			case BRTFile: {
				BeResourceFile *File = (BeResourceFile *)Entry;
				BeResourceString *Name = GetAtRVOFromSectionDataBe(File->bFileNameRVO, BH->bSystemSection, rsrcData, bheader);
				if(MatchResourceName(Name, snippet)){
					char *nextSnippet = strtok_k(tokstate);
					if(nextSnippet == NULL){ // Target reached
						void *OutData = ReadAtRVOFromSectionBe(File->bDataOffsetRVO, File->bDataSize, BH->bSystemSection, socket, bheader);
						if(OutData){
							GenericLengthType copySize = __min(File->bDataSize, NBytes);
							__memcpy(out, OutData, copySize);
							if(Remaining){ *Remaining = (File->bDataSize > NBytes) ? (File->bDataSize - NBytes) : 0; }
							__free(OutData);
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
					snippet = strtok_k(tokstate);
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
	strtok_d(tokstate);
    __free(rsrcData);
    __free(bheader);
    return found;
}