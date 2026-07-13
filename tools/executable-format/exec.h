#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tools/json/minijson.h"

#define DecodeBeExecutableHeader(PTR)						\
	BeHeader *BH = (PTR) + sizeof(RelativeVirtualOffset);	\
	BeSectionDescriptor *BSDs = (PTR) + sizeof(RelativeVirtualOffset) + (BH->bSectionTableOffset? BH->bSectionTableOffset - *((RelativeVirtualOffset *)(PTR)): sizeof(BeHeader));
#define RDecodeBeExecutableHeader(PTR)				\
	BH =	(PTR) + sizeof(RelativeVirtualOffset);	\
	BSDs =	(PTR) + sizeof(RelativeVirtualOffset) + (BH->bSectionTableOffset? BH->bSectionTableOffset - *((RelativeVirtualOffset *)(PTR)): sizeof(BeHeader));
#define ManifestJsonLen				(16 * 1024)	// 16 kB
#define SectionPathLen				(16)
#pragma region Defaults
#define JsonImportSectionNamePath	"sections.import.name"
#define JsonExportSectionNamePath	"sections.export.name"
#define JsonRelocSectionNamePath	"sections.reloc.name"

#define DefaultSystemSectionName	((SectionNameBe){".SystemS"})
#define DefaultImportSectionName	((SectionNameBe){".ImportIn"})
#define DefaultExportSectionName	((SectionNameBe){".ExportOut"})
#define BeStandardAlign				4
#define BeHeaderMagic				"BE\0\0"
#define BeSectionMagic				"BES\0\0\0\0\0"
#define BeDefaultNSections			12
#define GenericPathLen				512
#define ExecutableNameLen			32
#define IconDimensionsX				20
#define IconDimensionsY				20
#pragma endregion

#define RoundUp(n, alignment)		((((n) + (alignment) - 1) / (alignment)) * (alignment))

#define enumdef(type, name)		typedef type name;	enum

typedef union{
	char Magic[4];
	uint32_t uMagic;
}GenericMagic4;
typedef union{
	char Magic[8];
	uint64_t uMagic;
}GenericMagic8;
typedef uint64_t GenericHashType[2];
// This refers to the Mapped Offset within the Executable Image.
typedef uint64_t RelativeVirtualOffset;
typedef uint32_t GenericLengthType, GenericIndexType;
typedef uint16_t SectionReference, Alignment;
typedef char SectionNameBe[SectionPathLen], 
				GenericPath[GenericPathLen], 
				JsonManifest[ManifestJsonLen];
// A-Channel is a Brightness.
typedef uint8_t Color[4];
typedef Color ExecIcon[IconDimensionsX][IconDimensionsY];

enumdef(uint16_t, BeSectionFlags64){
	//	The Section should remain in Memory in the Final Load3ed Executable.
	SFAllocatable = 0x1, 
	SFPersistent = SFAllocatable, 
	// The Section can be Relocated, It supports being Placed anywhere in Memory.
	SFRelocatable = 0x2, 
	//	The Section Data should always be kept the same as it is on the Disk.
	SFInitData = 0x4, 
	//	The Section Data can be R/W all willy-nilly.
	//	This Flag usually tells the Loader to Allocase generic Memory.
	SFUInitData = 0x8, 
	//	The Section is Executable.
	SFExecutable = 0x10, 
	//	The Section can be Read from.
	SFReadable = 0x20, 
	//	The Section can be Written To.
	SFWritable = 0x40, 
	//	The Sectiopn can be Shared accross Multiple Programs.
	SFSharable = 0x80, 
};

enumdef(uint16_t, BeSectionDefaults64){
	SDExecutableCode =			(SFExecutable | SFInitData | SFReadable | SFAllocatable), 
	SDReadonlyData =			(SFReadable | SFWritable | SFRelocatable | SFInitData | SFAllocatable), 
	SDReadWritableData =		(SFReadable | SFWritable | SFInitData | SFAllocatable), 
	SDRandomStaticReadonlyData =(SFReadable | SFUInitData | SFAllocatable), 
	SDRandomStaticData =		(SFReadable | SFWritable | SFUInitData | SFAllocatable), 
	SDMountableResource =		(SFReadable | SFWritable | SFInitData | SFRelocatable), 
	SDRelocExport =				(SFReadable | SFInitData), 
	SDRelocImport =				(SFReadable | SFInitData), 
	SDDiscardableReadWriteData =(SFReadable | SFInitData | SFInitData), 
	SDDiscardableReadOnlyData =	(SFReadable | SFInitData), 
};

typedef struct BeHeader{
	//	"PE\0\0"
	GenericMagic4			bMagic;
	SectionNameBe				bSystemSection;
	// ExecIcon			Icon;
	RelativeVirtualOffset	bIconRVO;
	// JsonManifest		Manifest;
	RelativeVirtualOffset	bManifestRVO;
	// The Number of Continuing Sections.
	GenericLengthType		bNSections;
	//	The Raw Size of the Header, Including the BeHeader.
	GenericLengthType		bRawSize;
	//	From the 1st Byte of the File.
	RelativeVirtualOffset	bSectionTableOffset;
}__attribute__((packed)) BeHeader;

typedef struct BeSectionDescriptor{
	GenericMagic8		SectionMagic;
	SectionNameBe 		bName;
	BeSectionFlags64	bFlags;
	Alignment			bAlignment;
	uint64_t			bVirtualAddress;
	uint64_t			bVirtualSize;
	uint64_t			bRawPointer;
	uint64_t			bRawSize;
}__attribute__((packed)) BeSectionDescriptor;

enumdef(uint16_t, BeMountableResourceFormatType){
	BMFTSimpleFileSystem, BMFTRandomPersistentData, 

};
typedef struct BeMountableResourceEntry{
	//	The Alias to be Granted to the Resource when Mounted.
	GenericMagic4					bAlias;
	//	The Type of the Resource, It determines the Socket to use to Mount the Section.
	BeMountableResourceFormatType	Format;
	//	The Offset into the Resource Section to load the Array.
	//	The Number of Bytes to Map to the Array.
	uint64_t 						bBaseOffset, 
									bRawSize;
}__attribute__((packed)) BeMountableResourceEntry;

enumdef(uint32_t, BeRelExportOrdinalEntryFlags){
	//	Ignore the Ordinal.
	BREFUnknown = 0x0, 
	//	Use the Ordinal.
	BREFExportable = 0x1
};
// The Index of this corrseponds to the Index of the Symbol Hash.
typedef struct BeRelExposeOrdinalEntry{
	// The True Index into the Export Address Table.
	GenericIndexType				bIndex;
	//	The Flags for the Ordinal.
	BeRelExportOrdinalEntryFlags	bFlags;
}__attribute__((packed)) BeRelExposeOrdinalEntry;
//	The Actual Exported "Address"
typedef struct BeRelExportAddressEntry{
	//	The Parent Section that the Address lies at.
	SectionReference	bParent;
	//	The Offset within the Parent.
	uint64_t			bOffset;
}__attribute__((packed)) BeRelExportAddressEntry;
typedef struct BeRelExportHeader{
	//	The Number of Total Exported Entries within the Whole Header.
	GenericLengthType		bNExported;
	//	The RVO of the Hash(uint64_t) Table of all Symbols that can be Exported, 
	//		Sorted to be Binary-Searchable.
	RelativeVirtualOffset	bImportSymbolHashTableRVO;
	//	The RVO of the Ordinal Array([uint32_t, Flags] *).
	//	The Entries refer to an Intermediate Index in the Export Address Table.
	RelativeVirtualOffset	bIxportExposeTableRVO;
	//	The RVO of the Export Address Table, (BeRelExportAddressEntry *).
	RelativeVirtualOffset	bExportAddressTable;
	//	The Exported Symbols.
	//		This can differ from the Exposed Symbols
	GenericLengthType		bNExportAddresses;
}__attribute__((packed)) BeRelExportHeader;

enumdef(uint32_t, BeRelImportFlags){
	BRIFUnknown = 0, BRIFExternal = 0x1, BRIFInternal = 0x2
};
typedef struct BeRelImportEntry{
	//	The Hash of the Imported Symbol.
	GenericHashType		bHash;
	//	This is the Index within the RelocationDirectoryTable that corresponds to the Import.
	//	We use the bParent + bOffset stored within to save the Dll's Exported Pointer.
	GenericIndexType	bRelocation;
	//	The Flags for the Entry.
	BeRelImportFlags	bFlags;
}__attribute__((packed)) BeRelImportEntry;
typedef struct BeRelImportDllRef{
	//	The Index within the Import Path Table that refers to the Path to the Import Image.
	GenericIndexType		bImportPathIndex;
	// //	RVO to the Symbol Hash Table.
	// RelativeVirtualOffset	bImportSymbolHashTableRVO;
	//	The Number of Entries.
	GenericLengthType		bNHashes;
}__attribute__((packed)) BeRelImportDllRef;
typedef struct BeRelImportHeader{
	//	The Number of Imported DLLs.
	GenericLengthType		bNDllReferences;
	//	The RVO of the Paths to the Imported DLLs.
	//	Points to an Array of NULL-Terminated ASCII Strings.
	RelativeVirtualOffset	bImportPathRVO;
	//	The RVO to the (BeRelImportDllRef *) Import Table.
	RelativeVirtualOffset	bImportTableRVO;
}__attribute__((packed)) BeRelImportHeader;

enumdef(uint16_t, BeRelocationType){
	//	Ignore this Relocation.
	BRETAbsolute =		(0x0 >> 12) & 0x000F, 
	//	16-bit Relocation.
	BRET16 = 			(0x1 >> 12) & 0x000F, 
	//	32-bit Relocation.
	BRET32 =			(0x2 >> 12) & 0x000F, 
	// Add the High 16-bits of the Difference in the Relocation.
	BRET16High =		(0x4 >> 12) & 0x000F, 
	// Add the Low 16-bits of the Difference in the Relocation.
	BRET16Low =			BRET16, 
	//	64-bit Relocation.
	BRET64 =			(0x10 >> 12) & 0x000F
};

#define GET_RELOC_OFFSET(entry) ((entry).bRaw & 0x0FFF)
#define GET_RELOC_TYPE(entry)   (((entry).bRaw >> 12) & 0x000F)
// Entries in a Relocation Directory.
typedef union BeRelocationDirectoryEntry{
	uint16_t		bRaw;
	struct{
		//	The Offset into a 4kb Mapped Region.
		//	A Type of BRETAbsolute will Move on to the Next Relocation.
		uint16_t	bOffset:	12;
		uint16_t	bType:		4;
	}Bits;
}__attribute__((packed)) BeRelocationDirectoryEntry, 
						BeRelocationDirectoryTable[];
typedef struct BeRelocationDirectory{
	SectionReference			bSection;
	GenericLengthType			bOffset;
	GenericLengthType			bDirectorySize;
	BeRelocationDirectoryTable	bRelocationTable;
}__attribute__((packed)) BeRelocationDirectory;
typedef struct BeRelocationHeader{
	RelativeVirtualOffset	bRelocationDirTableRVO;
	GenericLengthType		bNDirectories;
}__attribute__((packed)) BeRelocationHeader;



void GenerateBeHeader(
	const char *path, SectionNameBe SystemSecton, 
	ExecIcon Icon, JsonManifest Manifest, 
	GenericLengthType RawDataSize
);
void *ReadBeHeader(const char *path);
bool DumpBeHeader(const char *path, void *bheader);
uint64_t RvoToFileOffsetBe(RelativeVirtualOffset RVO, BeSectionDescriptor *Section);
RelativeVirtualOffset FileOffsetToRvoBe(uint64_t Offset, BeSectionDescriptor *Section);
BeSectionDescriptor *FindSectionBe(void *bheader, SectionNameBe name);
void *ReadSectionBe(const char *path, void *bheader, SectionNameBe name);
bool WriteSectionBe(const char *path, void *bheader, SectionNameBe name, void *Data, RelativeVirtualOffset RVO, GenericLengthType NBytes);
bool UpdateJsonSchemaBe(const char *path, const char *target, JsonType Type, ...);
void *ReadSectionFromManifest(const char *path, char *manifestpath);
bool AddSectionBe(const char *path, SectionNameBe Name, BeSectionFlags64 Flags, 
	void *RawData, uint64_t NBytes, uint64_t VirtualAddress, uint64_t VirtualSize, Alignment Align
);
bool RemoveSectionBe(const char *path, void *bheader, SectionNameBe name);
bool CreateRelocationSectionBe(char *path, SectionNameBe Name, 
	SectionNameBe *NamePerDirectory, uint16_t **OffsetPerDirectoryEntry, BeRelocationType **TypePerDirectoryEntry, 
	GenericLengthType nDirectories, GenericLengthType *nPerDirectory 
);
bool CreateImportSectionBe(
	const char *path, SectionNameBe Name, char **DLLs, char ***imports, 
	GenericLengthType *nImportsPerDll, GenericLengthType nImports, 
	//	Info for Generating Relocations.
	uint32_t BaseRelocationTableOffset
);
