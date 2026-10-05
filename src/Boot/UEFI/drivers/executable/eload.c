#include "pe.h"

static LoadedPeExecutable *SearchBinary(LoadedPeExecutable *Root, char *Binary){
    if(!Root || !Binary){return NULL;}
    LoadedPeExecutable **Binaries[32] = {0};Binaries[0] = (&Root);
    UINT32 Depth = 0,						Counter[32] = {0};

    while((Depth < 32) && Binaries[Depth]){
        if(strcmpa((*Binaries[Depth])->Name, Binary)){
            if((Counter[Depth] < (*Binaries[Depth])->NDependencies) && (*Binaries[Depth])->Dependencies){
                if((*Binaries[Depth])->Dependencies[Counter[Depth]]){
                    // Pass address of dependency pointer to match LoadedPeExecutable **
                    Binaries[Depth + 1] = &((*Binaries[Depth])->Dependencies[Counter[Depth]]);
                    Counter[Depth]++;
                    Depth++;
                }else{Counter[Depth]++;}
            }else{
                Binaries[Depth] = NULL;
                Counter[Depth] = 0;
                if(Depth > 0){Depth--;}else{break;}
            }
        }else{return *(Binaries[Depth]);}
    }
    return NULL;
}
static bool ResolveExport(LoadedPeExecutable *Import, bool OrdinalImport, UINT64 Ordinal, const char *Name, UINT64 *Address){
	PeExportDirectoryEntry *exports = Import->This->Fmt.exp.Export;
	if(!exports || !Import->This->Fmt.exp.RawExportAddresses){return false;}
	UINT32 exportIndex = UINT32_MAX;
	if(OrdinalImport){
		if(Ordinal < exports->mOrdinalBase){return false;}
		exportIndex = (UINT32)(Ordinal - exports->mOrdinalBase);
		if(exportIndex >= exports->mNTableEntries){return false;}
	}else{
		for(UINT32 i = 0; i < exports->mNNamePointers; ++i){
			UINT16 index = Import->This->Fmt.exp.Ordinals[i];
			if(index >= exports->mNTableEntries){continue;}
			char *exportName = GetAtRVAFromSectionDataPe(
				Import->This->Fmt.exp.NamePointerRVAs[i], PeExportSection,
				Import->This->Fmt.exp.Raw, Import->This->Raw);
			if(exportName && !strcmpa(exportName, (char *)Name)){
				exportIndex = index;
				break;
			}
		}
		if(exportIndex == UINT32_MAX){return false;}
	}
	UINT32 exportRVA = Import->This->Fmt.exp.RawExportAddresses[exportIndex].mExportRVA;
	PeRVAnSize exportDirectory = Import->This->Fmt.RVAs[0];
	if(!exportRVA || (exportRVA >= exportDirectory.RVA &&
		exportRVA - exportDirectory.RVA < exportDirectory.Size)){return false;}
	*Address = (UINT64)Import->Base + exportRVA;
	return true;
}
static LoadedPeExecutable *LoadBinary(const char *file, bool Info, uint32_t InfoMax, socket_t *drive, LoadedPeExecutable *Root){
	char *largs = "f";
	socket_ret rt = socketfcall(drive, open, file, largs);
	if(socketreterr(rt, sizeof(socket_t))){
		DEBUGPRINT(L"\nFailed to open File %a\tError Code: %llu[%llu:%llu]", file, (UINT64)rt.errout, (UINT64)rt.data, (UINT64)rt.nData);
		return NULL;
	}else{DEBUGPRINT(L"\nOpened File %a", file);}
	socket_t *this = rt.data;
	ExpandedPeExecutable *Exe = ExpandPeExecutableFormat(this);
	if(Info){PrintPeExecutableFormat(Exe, InfoMax);}
	void *ImageBase = NULL;
	uefi_call_wrapper(gBS->AllocatePages, 0, AllocateAnyPages, EfiRuntimeServicesData, 
		__roundup((Exe->Fmt.Opt.Pe32->mMagic == Pe32? Exe->Fmt.Opt.Pe32->mSizeOfImage: Exe->Fmt.Opt.Pe32Plus->mSizeOfImage), EFI_PAGE_SIZE) / EFI_PAGE_SIZE, &ImageBase);
	DEBUGPRINT(L"\nAllocated Executable %a at Address %llu", file, ImageBase);
	if((Root->NDependencies % 2) == 0){
		if(Root->Dependencies){
			Root->Dependencies = __realloc(Root->Dependencies, 
				Root->NDependencies * sizeof(LoadedPeExecutable *), (Root->NDependencies + 2) * sizeof(LoadedPeExecutable *));}
		else{Root->Dependencies = __calloc(2, sizeof(LoadedPeExecutable *));}
	}
	char *fname = NULL;		{
		for(UINT32 cc = __strlen(file); cc > 0; --cc){if((file[cc] == '\\') || (file[cc] == '/')){fname = (file + cc + 1);	break;}}
		if(!fname){fname = file;}
	}
	Root->NDependencies++;
	Root->Dependencies[Root->NDependencies - 1] = __calloc(1, sizeof(LoadedPeExecutable));		
	*(Root->Dependencies[Root->NDependencies - 1]) = (LoadedPeExecutable){
		.Dependencies = NULL, .Name = __strdup(fname), .NDependencies = 0, 
		.NSections = 0, .Sections = NULL, .This = Exe, .Base = ImageBase, 
		.EntryPoint = ImageBase + (Exe->Fmt.Opt.Pe32->mMagic == Pe32? Exe->Fmt.Opt.Pe32->mAddressOfEntryPoint: Exe->Fmt.Opt.Pe32Plus->mAddressOfEntryPoint)
	};
	//	Load all relevant Code/Data Sections.
	for(UINT32 cc = 0; cc < Exe->Fmt.Header->mNumberOfSections; ++cc){
		BOOLEAN SectionLoaded = false;
		if(!strncmpa(Exe->Fmt.SectionTable[cc].mName, PeCodeSection, 8) || 
			!strncmpa(Exe->Fmt.SectionTable[cc].mName, PeDefDataSection, 8) || 
			!strncmpa(Exe->Fmt.SectionTable[cc].mName, PeUDefDataSection, 8) || 
			!strncmpa(Exe->Fmt.SectionTable[cc].mName, PeRDefDataSection, 8)
		){
			const UINT64 Offset = __roundup(Exe->Fmt.SectionTable[cc].mVirtualAddress, EFI_PAGE_SIZE);
			void *Temp = LoadSectionPe(this, Exe->Raw, Exe->Fmt.SectionTable[cc].mName);
			if(Temp){
				SectionLoaded = true;
				__memcpy(ImageBase + Offset, Temp, __roundup(Exe->Fmt.SectionTable[cc].mVirtualSize, EFI_PAGE_SIZE));
				uefi_call_wrapper(gBS->FreePages, 0, Temp, __roundup(Exe->Fmt.SectionTable[cc].mVirtualSize, EFI_PAGE_SIZE) / EFI_PAGE_SIZE);
				DEBUGPRINT(L"\nLoaded Section %a at Offset %llu(%p)", Exe->Fmt.SectionTable[cc].mName, (UINT64)Offset, (UINT64)(Offset + ImageBase));
			}
		}else if(!strncmpa(Exe->Fmt.SectionTable[cc].mName, PeImportSection, 8)){
			//	Import Symbols.
			for(UINT32 ImportEntry = 0; ImportEntry < Exe->Fmt.imp.nImports; ++ImportEntry){
				const UINT64 IATOffset = Exe->Fmt.imp.imports[ImportEntry].ImportAddressTableRVA;
				UINT32 *IAT32 = ImageBase + IATOffset;
				UINT64 *IAT64 = ImageBase + IATOffset;
				
				char *LibName = GetAtRVAFromSectionDataPe(
					Exe->Fmt.imp.imports[ImportEntry].NameRVA, PeImportSection, Exe->Fmt.imp.Raw, Exe->Raw);
				if(!LibName){socketfcall(this, close, 0); return NULL;}
				LoadedPeExecutable *Import = NULL;
				if(!(Import = SearchBinary(Root, LibName))){
					//	Load the Dependency, Generate the Path.
					UINT16 tmplen = __strlen(file);
					char *tmp = __strdup(file);
					if(tmp){
						UINT64 tmplen = (UINT64)__strlen(tmp);
						while(tmplen > 0 && tmp[tmplen - 1] != '\\' && tmp[tmplen - 1] != '/'){tmplen--;}
						tmp[tmplen] = '\0';

						UINT32 newLen = tmplen + __strlen(LibName) + 1;
						char *fullPath = __calloc(newLen, sizeof(char));
						__memcpy(fullPath, tmp, tmplen);
						__memcpy(fullPath + tmplen, LibName, __strlen(LibName));

						DEBUGPRINT(L"\nLoading Import Library %a[%a]", fullPath, LibName);
						Import = LoadBinary(fullPath, Info, InfoMax, drive, Root->Dependencies[Root->NDependencies - 1]);
						__free(fullPath);
						__free(tmp);
					}
				}
				if(Import){DEBUGPRINT(L"\nLoaded Import Library %a at %llu(%p)", LibName, Import->Base, Import->Base);}
				else{socketfcall(this, close, 0);		return NULL;}

				SectionLoaded = true;
				//	Process Export Data.
				DEBUGPRINT(L"\nProcessing Import %llu", (UINT64)ImportEntry);
				if((Exe->Fmt.Opt.Pe32->mMagic == Pe32 && !Exe->Fmt.imp.perImport.lookups.ImportLookups32[ImportEntry]) ||
					(Exe->Fmt.Opt.Pe32->mMagic != Pe32 && !Exe->Fmt.imp.perImport.lookups.ImportLookups64[ImportEntry])){
					DEBUGPRINT(L"\nMissing import lookup table for %a", LibName);
					socketfcall(this, close, 0);
					return NULL;
				}
				for(UINT32 ImportCounter = 0; ImportCounter < Exe->Fmt.imp.nEntries[ImportEntry]; ++ImportCounter){
					bool OrdinalImport = Exe->Fmt.Opt.Pe32->mMagic == Pe32? 
						Exe->Fmt.imp.perImport.lookups.ImportLookups32[ImportEntry][ImportCounter].Bits.ImportByOrdinal: 
						Exe->Fmt.imp.perImport.lookups.ImportLookups64[ImportEntry][ImportCounter].Bits.ImportByOrdinal;
					UINT64 ImportSym = Exe->Fmt.Opt.Pe32->mMagic == Pe32?
						Exe->Fmt.imp.perImport.lookups.ImportLookups32[ImportEntry][ImportCounter].Bits.OrdinalNumberOrNameRVA:
						Exe->Fmt.imp.perImport.lookups.ImportLookups64[ImportEntry][ImportCounter].Bits.OrdinalNumberOrNameRVA;
					const char *ImportName = NULL;
					if(!OrdinalImport){
						PeImportNameEntry *NameEntry = GetAtRVAFromSectionDataPe(
							(UINT32)ImportSym, PeImportSection, Exe->Fmt.imp.Raw, Exe->Raw);
						if(NameEntry){ImportName = NameEntry->Name;}
					}
					UINT64 Address = 0;
					if(!ResolveExport(Import, OrdinalImport, ImportSym, ImportName, &Address)){
						if(OrdinalImport){DEBUGPRINT(L"\nUnresolved import in %a: ordinal %llu", LibName, ImportSym);}
						else{DEBUGPRINT(L"\nUnresolved import in %a: %a", LibName, ImportName ? ImportName : "invalid name RVA");}
						socketfcall(this, close, 0);
						return NULL;
					}
					if(OrdinalImport){DEBUGPRINT(L"\nLoaded Import #%llu", ImportSym);}
					else{DEBUGPRINT(L"\nLoaded Import \"%a\"", ImportName);}
					if(Exe->Fmt.Opt.Pe32->mMagic == Pe32){
						IAT32[ImportCounter] = (UINT32)(ImageBase + Address);
					}else{IAT64[ImportCounter] = (UINT64)(ImageBase + Address);}
				}
			}
		}
		if(SectionLoaded){
			if((Root->Dependencies[Root->NDependencies - 1]->NSections % 2) == 0){
				if(Root->Dependencies[Root->NDependencies - 1]->Sections){
					Root->Dependencies[Root->NDependencies - 1]->Sections = __realloc(Root->Dependencies[Root->NDependencies - 1]->Sections, 
						Root->Dependencies[Root->NDependencies - 1]->NSections * sizeof(LoadedPeSection), (Root->Dependencies[Root->NDependencies - 1]->NSections + 2) * sizeof(LoadedPeSection));}
				else{Root->Dependencies[Root->NDependencies - 1]->Sections = __calloc(2, sizeof(LoadedPeSection));}
			}
			Root->Dependencies[Root->NDependencies - 1]->Sections[Root->Dependencies[Root->NDependencies - 1]->NSections] = (LoadedPeSection){
				.Base = ImageBase + Exe->Fmt.SectionTable[cc].mVirtualAddress,
				.Limit = __max(Exe->Fmt.SectionTable[cc].mSizeOfRawData, Exe->Fmt.SectionTable[cc].mVirtualSize), .Name = {0}
			};
			__memcpy(Root->Dependencies[Root->NDependencies - 1]->Sections[Root->Dependencies[Root->NDependencies - 1]->NSections].Name, Exe->Fmt.SectionTable[cc].mName, 8);
			Root->Dependencies[Root->NDependencies - 1]->NSections++;
		}
	}
	EFI_MEMORY_ATTRIBUTE_PROTOCOL *MemAttrProtocol = NULL;
	EFI_GUID MemAttrGUID = {0xf4560cf6, 0x40ec, 0x4b4a, 0xa1, 0x92, 0xbf, 0x1d, 0x57, 0xd0, 0xb1, 0x89};
	EFI_STATUS MemAttrStatus = uefi_call_wrapper(gBS->LocateProtocol, 0, &MemAttrGUID, NULL, (VOID **)&MemAttrProtocol);
	if(!EFI_ERROR(MemAttrStatus)){
		for(UINT32 cc = 0; cc < Exe->Fmt.Header->mNumberOfSections; ++cc){
			PeImageSectionHeader *section = Exe->Fmt.SectionTable + cc;
			UINT64 Attributes = 0;
			if(!strncmpa(section->mName, PeCodeSection, 8)){Attributes = EFI_MEMORY_RO;}
			else if(!strncmpa(section->mName, PeRDefDataSection, 8)){Attributes = EFI_MEMORY_RO | EFI_MEMORY_XP;}
			if(Attributes){
				UINT64 Offset = __roundup(section->mVirtualAddress, EFI_PAGE_SIZE);
				UINT64 SectionSize = __roundup(section->mVirtualSize, EFI_PAGE_SIZE);
				MemAttrStatus = uefi_call_wrapper(MemAttrProtocol->SetMemoryAttributes, 0,
					MemAttrProtocol, ImageBase + Offset, SectionSize, Attributes);
				if(EFI_ERROR(MemAttrStatus)){
					DEBUGPRINT(L"\nUnable to update %a Memory Permissions: %r", section->mName, MemAttrStatus);
				}
			}
		}
	}else{DEBUGPRINT(L"\nUnable to find MemorySetAttribute Protocol");}
	socketfcall(this, close, 0);
	DEBUGPRINT(L"\n[%a]:\t[%p:%p]\t%llu\t%llu\n", (UINT64)Root->Dependencies[Root->NDependencies - 1]->Name, (UINT64)Root->Dependencies[Root->NDependencies - 1]->Base, 
		(UINT64)Root->Dependencies[Root->NDependencies - 1]->EntryPoint, (UINT64)Root->Dependencies[Root->NDependencies - 1]->NDependencies, 
		(UINT64)Root->Dependencies[Root->NDependencies - 1]->NSections);
	return Root->Dependencies[Root->NDependencies - 1];
}


LoadedPeExecutable *LoadExecutable(socket_t *drive, bool Info, UINT32 InfoMax, const char *file){
	LoadedPeExecutable Stub = {0};
	LoadedPeExecutable *Temp = LoadBinary(file, Info, InfoMax, drive, &Stub);
	__free(Stub.Dependencies);
	return Temp;
}