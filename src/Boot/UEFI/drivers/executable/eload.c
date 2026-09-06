#include "eload.h"
#include "exec.h"
#include "src/Boot/UEFI/drivers/json/minijson.h"

void ResolveRelS(const socket_t *file, void *bheader, UINT32 Allocated, UINT32 Ordinal[], void *Addresses[]);

ExecutableSection *LoadExecutable(const socket_t *drive, const socket_t *file, UINT32 *Length){
	void *bheader = ReadBeHeader(file);
	DecodeBeExecutableHeader(bheader);
	UINT8 *Addresses[BH->bNSections];
	UINT32 Allocated = 0, Ordinal[BH->bNSections];
	EFI_STATUS status = 0;
	for(register UINT32 cc = 0; cc < BH->bNSections; ++cc){
		//  Try to allocate for each section
		if(__check(BSDs[cc].bFlags, SFAllocatable)){
			status = uefi_call_wrapper(gBS->AllocatePages, 0, AllocateAddress, EfiConventionalMemory, 
				__roundup(BSDs[cc].bVirtualSize, EFI_PAGE_SIZE) / EFI_PAGE_SIZE, Addresses + Allocated);
			__memset(Addresses[Allocated], 0, BSDs[cc].bVirtualSize);
			Ordinal[Allocated] = cc;
			void *Temp = ReadSectionBe(file, bheader, BSDs[cc].bName);
			CopyMemC(Addresses[Allocated], Temp, BSDs[cc].bRawSize);
			__free(Temp);
			Allocated++;
		}
	}
	ExecutableSection *Out = __calloc(Allocated, sizeof(ExecutableSection));
	(*Length) = Allocated;

	//  For now we only resolve .code, .data and .RelS
	//	First we resolve Relocations.
	ResolveRelS(file, bheader, Allocated, Ordinal, (void **)Addresses);
	for(UINT32 cc = 0; cc < Allocated; ++cc){
		char *name = __calloc(1, sizeof(SectionNameBe) + 1);
		Out[cc] = (ExecutableSection){.Base = Addresses[cc], .Entry = NULL, .Name = name};
		__memcpy(Out[cc].Name, BSDs[Ordinal[cc]].bName, sizeof(SectionNameBe));
		if(BSDs[Ordinal[cc]].bVirtualAddress <= BH->bEntryPoint && 
		(BSDs[Ordinal[cc]].bVirtualAddress + BSDs[Ordinal[cc]].bVirtualSize) > BH->bEntryPoint){
			Out[cc].Entry = (void *)((UINT64)Addresses[cc] + BSDs[Ordinal[cc]].bVirtualAddress - BH->bEntryPoint);
			break;
		}
	}
	__free(bheader);
	return Out;
}

#define ServiceExecutablePath		"Main/Exe"
LoadedService LoadService(const socket_t *drive, const socket_t *manifest){
	LoadedService out = {0};
	socket_ret ret = manifest->info(manifest, 0, 8);
	if(socketreterr(ret, sizeof(uint64_t))){
		ret = manifest->read(manifest, 0, *((uint64_t *)ret.data), 0x0);
		if(socketreterr(ret, sizeof(uint64_t))){
			JsonValue *Manifest = JsonParse(ret.data);
			JsonValue *Path = JsongetValue(Manifest, ServiceExecutablePath);
			if(Path){
				ret = drive->open(drive, sizeof(char *) * 2, Manifest->stringValue, "f");
				if(socketreterr(ret, sizeof(socket_t))){
					out.Sections = LoadExecutable(drive, (socket_t *)ret.data, &(out.NLoadedSections));
					void *bheader = ReadBeHeader((socket_t *)ret.data);
					BeExportHeader *ExpHeader = ReadSectionBe((socket_t *)ret.data, bheader, DefExportSectionName);
					BeExportEntry *ExpTable = GetAtRVOFromSectionDataBe(ExpHeader->bExportTableRVO, 
						DefExportSectionName, ExpHeader, bheader);
					char *PathTable = GetAtRVOFromSectionDataBe(ExpHeader->bExportNameRVO, 
						DefExportSectionName, ExpHeader, bheader);
					BeSectionDescriptor *This = FindSectionBe(bheader, DefExportSectionName);
					out.NLoadedServices = ExpHeader->bNExportAddresses;
					out.Table = __calloc(ExpHeader->bNExportAddresses, sizeof(LoadedServiceEntry));
					for(uint32_t cc = 0; cc < ExpHeader->bNExportAddresses; ++cc){
						char *curr = PathTable;
						while(ExpTable[cc].bNameIndex && *curr){
							curr = PathTable + strlena(PathTable);
							ExpTable[cc].bNameIndex--;
						}
						out.Table[cc] = (LoadedServiceEntry){
							.Base = GetAtRVOFromSectionDataBe(ExpTable[cc].bVirtualAddress, 
								DefExportSectionName, ExpHeader, bheader), 
							.Name = __memdup(curr, strlena(curr))
						};
					}
					__free(ExpHeader);
				}
			}
			JsonFree(Manifest);
		}
	}
	return out;
}

void ResolveRelS(const socket_t *file, void *bheader, UINT32 Allocated, UINT32 Ordinal[], void *Addresses[]){
	DecodeBeExecutableHeader(bheader);
	//	Resolve .RelS
	BeRelocationHeader *RelH = ReadSectionBe(file, bheader, DefRelocationSectionName);
	uint64_t Offset = 0;
	for(register UINT32 cc = 0; cc < RelH->bNDirectories; ++cc){
		BeRelocationDirectory *Directory = 
			GetAtRVOFromSectionDataBe(RelH->bRelocationDirTableRVO, DefRelocationSectionName, RelH, bheader) + Offset;
		BeRelocationEntry *Table = (void *)Directory + sizeof(BeRelocationDirectory);
		//	We find the Section the Directory and Entry's Address refer to then compute the Offset.
		for(UINT32 cc_ = 0; cc_ < (Directory->bDirectorySize - sizeof(BeRelocationDirectory)) / sizeof(BeRelocationEntry); ++cc_){
			BeSectionDescriptor *Referred = NULL;
			UINT32 cc__ = 0;
			for(; cc__ < Allocated; ++cc__){
				if(BSDs[Ordinal[cc__]].bVirtualAddress <= (Directory->bAddress + Table[cc_].bOffset) && 
				(BSDs[Ordinal[cc__]].bVirtualAddress + BSDs[Ordinal[cc__]].bVirtualSize) > (Directory->bAddress + Table[cc_].bOffset)){
					Referred = BSDs + Ordinal[cc__];	break;
				}
			}
			if(Referred){
				//	The Virtual precedes the Actual Location.
				if(Referred->bVirtualAddress > Addresses[cc__]){
					switch(Table[cc_].bType){
						case BRETAbsolute:	{continue;}
						case BRET16:		{
							//	Calculate the Offset within the Data
							UINT16 *Ptr = Addresses[cc__] + Referred->bVirtualAddress - (Directory->bAddress + Table[cc_].bOffset);
							*Ptr -= Referred->bVirtualAddress;		break;
						} case BRET32:		{
							//	Calculate the Offset within the Data
							UINT32 *Ptr = Addresses[cc__] + Referred->bVirtualAddress - (Directory->bAddress + Table[cc_].bOffset);
							*Ptr -= Referred->bVirtualAddress;		break;
						} case BRET64:		{
							//	Calculate the Offset within the Data
							UINT64 *Ptr = Addresses[cc__] + Referred->bVirtualAddress - (Directory->bAddress + Table[cc_].bOffset);
							*Ptr -= Referred->bVirtualAddress;		break;
						}
					}
				}else if(Referred->bVirtualAddress < Addresses[cc__]){
					switch(Table[cc_].bType){
						case BRETAbsolute:	{continue;}
						case BRET16:		{
							//	Calculate the Offset within the Data
							UINT16 *Ptr = Addresses[cc__] + Referred->bVirtualAddress - (Directory->bAddress + Table[cc_].bOffset);
							*Ptr += Referred->bVirtualAddress;		break;
						} case BRET32:		{
							//	Calculate the Offset within the Data
							UINT32 *Ptr = Addresses[cc__] + Referred->bVirtualAddress - (Directory->bAddress + Table[cc_].bOffset);
							*Ptr += Referred->bVirtualAddress;		break;
						} case BRET64:		{
							//	Calculate the Offset within the Data
							UINT64 *Ptr = Addresses[cc__] + Referred->bVirtualAddress - (Directory->bAddress + Table[cc_].bOffset);
							*Ptr += Referred->bVirtualAddress;		break;
						}
					}
				}
			}
		}
		Offset += Directory->bDirectorySize;
	}
	__free(RelH);
}