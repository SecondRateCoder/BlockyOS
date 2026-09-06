#pragma once


#include "efi.h"
#include "efilib.h"
#include "Boot/UEFI/tools/tools.h"
#include "Boot/UEFI/drivers/socket/socket.h"

#define BeDumpVolume		(32)
#define BeDumpVolumeLine	(8)

#define DecodeBeExecutableHeader(PTR)						\
	BeHeader *BH = (PTR) + sizeof(RelativeVirtualOffset);	\
	BeSectionDescriptor *BSDs = (PTR) + sizeof(RelativeVirtualOffset) + (BH->bSectionTableOffset? BH->bSectionTableOffset - *((RelativeVirtualOffset *)(PTR)): sizeof(BeHeader));
#define RDecodeBeExecutableHeader(PTR)				\
	BH =	(PTR) + sizeof(RelativeVirtualOffset);	\
	BSDs =	(PTR) + sizeof(RelativeVirtualOffset) + (BH->bSectionTableOffset? BH->bSectionTableOffset - *((RelativeVirtualOffset *)(PTR)): sizeof(BeHeader));
#define ManifestJsonLen				(4 * 1024)	// 16 kB
#define SectionPathLen				(16)
#pragma region Defaults
#define BeResourceRootName			"##"
#define BeResourceIconPath			BeResourceRootName "\\" "Metadata\\Icon"
#define BeResourceIconFile			"Icon.20x20bmp"
#define BeResourceManifestPath		BeResourceRootName "\\" "Metadata\\Manifest"
#define BeResourceManifestFile		"Manifest.json"

#define DefExceptionSectionName		((SectionNameBe){".ExcS"})
#define DefRelocationSectionName	((SectionNameBe){".RelS"})
#define DefCodeSectionName			((SectionNameBe){".code"})
#define DefDataSectionName			((SectionNameBe){".data"})
#define DefUDefDataSectionName		((SectionNameBe){".bss"})
#define DefReadonlyDataSectionName	((SectionNameBe){".rodata"})
#define DefSystemSectionName		((SectionNameBe){".SystemS"})
#define DefResourceSectionName		DefSystemSectionName
#define DefImportSectionName		((SectionNameBe){".ImportIn"})
#define DefExportSectionName		((SectionNameBe){".ExportOut"})

#define BeResourceSectionSize		(32 * 1024)
#define BeStandardAlign				4
#define BeHeaderMagic				"BE\0\0"
#define BeSectionMagic				"BES\0\0\0\0\0"
#define BeDefaultNSections			32
#define GenericPathLen				512
#define ExecutableNameLen			32
#define IconDimensionsX				20
#define IconDimensionsY				20
#pragma endregion

#define RoundUp(n, alignment)		((((n) + (alignment) - 1) / (alignment)) * (alignment))

#define enumdef(type, name)		typedef type name;	enum

extern char *logfile;
typedef union{
	char Magic[4];
	UINT32 uMagic;
}GenericMagic4;
typedef union{
	char Magic[8];
	UINT64 uMagic;
}GenericMagic8;
typedef UINT64 GenericHashType[2];
// This refers to the Mapped Offset within the Executable Image.
typedef UINT64 RelativeVirtualOffset;
typedef UINT32 GenericLengthType, GenericIndexType;
typedef UINT16 SectionReference, Alignment;
typedef char SectionNameBe[SectionPathLen], 
				GenericPath[GenericPathLen], 
				JsonManifest[ManifestJsonLen];
// A-Channel is a Brightness.
typedef UINT8 Color[4];
typedef Color ExecIcon[IconDimensionsX][IconDimensionsY];

enumdef(UINT16, BeSectionFlags){
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

enumdef(UINT16, BeSectionDefaults){
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
	SectionNameBe			bSystemSection;
	// The Number of Continuing Sections.
	GenericLengthType		bNSections;
	//	The Raw Size of the Header, Including the BeHeader.
	GenericLengthType		bRawSize;
	//	From the 1st Byte of the File.
	RelativeVirtualOffset	bSectionTableOffset;
	RelativeVirtualOffset	bEntryPoint;
}__attribute__((packed)) BeHeader;

typedef struct BeSectionDescriptor{
	GenericMagic8	bSectionMagic;
	SectionNameBe 	bName;
	BeSectionFlags	bFlags;
	Alignment		bAlignment;
	UINT64			bVirtualAddress;
	UINT64			bVirtualSize;
	UINT64			bRawPointer;
	UINT64			bRawSize;
}__attribute__((packed)) BeSectionDescriptor;

typedef struct BeExportHeader{
	//	The Number of Total Exported Entries within the Whole Header.
	GenericLengthType		bNExported;
	RelativeVirtualOffset	bExportTableRVO;
	RelativeVirtualOffset	bExportNameRVO;
	GenericLengthType		bNExportAddresses;
}__attribute__((packed)) BeExportHeader;
enumdef(UINT32, BeExportEntryFlags){
	//	Ignore the Ordinal.
	BREFUnknown = 0x0, 
	//	Use the Ordinal.
	BREFExportable = 0x1
};
// The Index of this corrseponds to the Index of the Symbol Hash.
typedef struct BeExportEntry{
	GenericHashType			bSymbolHash;
	RelativeVirtualOffset	bVirtualAddress;
	BeExportEntryFlags		bFlags;
	GenericIndexType		bNameIndex;
}BeExportEntry;

typedef struct BeImportEntry{
	//	The Hash of the Imported Symbol.
	GenericHashType		bHash;
	//	This is the Index within the RelocationDirectoryTable that corresponds to the Import.
	//	We use the bParent + bOffset stored within to save the Dll's Exported Pointer.
	GenericIndexType	bRelocation;
}__attribute__((packed)) BeImportEntry, 
						BeImportTable[];
typedef struct BeImportDll{
	//	The Index within the Import Path Table that refers to the Path to the Import Image.
	GenericIndexType	bImportPathIndex;
	// //	RVO to the Symbol Hash Table.
	// RelativeVirtualOffset	bImportSymbolHashTableRVO;
	//	The Number of Entries.
	GenericLengthType	bNImports;
	BeImportTable		Table;
}__attribute__((packed)) BeImportDll;
typedef struct BeImportHeader{
	//	The Number of Imported DLLs.
	GenericLengthType		bNDllReferences;
	//	The RVO of the Paths to the Imported DLLs.
	//	Points to an Array of NULL-Terminated ASCII Strings.
	RelativeVirtualOffset	bImportPathRVO;
	//	The RVO to the (BeRelImportDllRef *) Import Table.
	RelativeVirtualOffset	bImportTableRVO;
}__attribute__((packed)) BeImportHeader;

enumdef(UINT16, BeRelocationType){
	//	Ignore this Relocation.
	BRETAbsolute =	0x1, 
	//	16-bit Relocation.
	BRET16 = 		0x2, 
	//	32-bit Relocation.
	BRET32 =		0x3, 
	//	64-bit Relocation.
	BRET64 =		0x10
};

#define GET_RELOC_OFFSET(entry) ((entry).bRaw & 0x0FFF)
#define GET_RELOC_TYPE(entry)   (((entry).bRaw >> 12) & 0x000F)
// Entries in a Relocation Directory.
typedef union BeRelocationEntry{
	UINT32 Raw;
	struct{
		BeRelocationType	bType;
		//	An Offset within the Directory
		UINT16			bOffset;
	};
}__attribute__((packed)) BeRelocationEntry;
typedef struct BeRelocationDirectory{
	RelativeVirtualOffset	bAddress;
	GenericLengthType		bDirectorySize;
}__attribute__((packed)) BeRelocationDirectory;
typedef struct BeRelocationHeader{
	RelativeVirtualOffset	bRelocationDirTableRVO;
	RelativeVirtualOffset	bUnusedRVO;
	GenericLengthType		bNDirectories;
}__attribute__((packed)) BeRelocationHeader;

enumdef(UINT16, BeResourceType){
	BRTFile = 1, 
	BRTDirectory = 2, 
	BRTNameString = 3
};
enumdef(UINT32, BeResourceDirectoryType){
	BRDTNoType
};
enumdef(UINT32, BeResourceFileType){
	BRFTIcon, BRFTManifest, BRFTMetadata, 
	BRFTRawData, BRFTBitmap, BRFTFont, BRFTString
};
typedef struct BeResourceConfigurator{
	char *Name;
	BeResourceType Type;
	union{
		struct{
			BeResourceDirectoryType	Type;
			struct BeResourceConfigurator
									*NextDirectory;
			struct BeResourceConfigurator
									*Files;
			GenericLengthType		nFiles;
		}Directory;
		struct{
			BeResourceFileType		Type;
			GenericLengthType		Length;
			void *Data;
		}File;
	};
}BeResourceConfigurator;
typedef struct TraversalFrame{
    BeResourceConfigurator *Config;
    size_t SavedPathLength;
} TraversalFrame;
typedef struct BeResourceHeader{
	RelativeVirtualOffset	bRootDirectoryRVO;
	//	Points to the First Unused Byte, followed by Unused Contiguous Data, up till the end of the Section.
	RelativeVirtualOffset	bUnusedBytesRVO;
}BeResourceHeader;
typedef struct BeResourceDirectory{
	BeResourceType			bDirectoryMagic;
	BeResourceDirectoryType	bDirectoryType;
	RelativeVirtualOffset	bNextDirectoriesRVO[4];
	RelativeVirtualOffset	bNextFileRVO;
	RelativeVirtualOffset	bDirectoryNameRVO;
}__attribute__((aligned(4))) BeResourceDirectory;
typedef struct BeResourceFile{
	BeResourceType			bFileMagic;
	BeResourceFileType		bFileType;
	RelativeVirtualOffset	bNextFileRVO;
	RelativeVirtualOffset	bFileNameRVO;
	RelativeVirtualOffset	bDataOffsetRVO;
	GenericLengthType		bDataSize;
}__attribute__((aligned(4))) BeResourceFile;
typedef struct BeResourceString{
	BeResourceType	bStringMagic;
	UINT16		bStringLength;
	char			String[];
}__attribute__((aligned(4))) BeResourceString;

typedef struct BeExceptionHandler{
	RelativeVirtualOffset	bVirtualAddressRVO;
	struct{
		bool bUnknownFunctionSize;
		union{
			RelativeVirtualOffset	bVirtualEndRVO;
			GenericLengthType 		bNInstructions;
		};
	}End;
	RelativeVirtualOffset	bHandlerRVO;
}BeExceptionHandler;

void *ReadBeHeader(const socket_t *socket);
UINT64 RvoToFileOffsetBe(RelativeVirtualOffset RVO, BeSectionDescriptor *Section);
RelativeVirtualOffset FileOffsetToRvoBe(UINT64 Offset, BeSectionDescriptor *Section);
void *GetAtRVOFromSectionDataBe(RelativeVirtualOffset RVO, SectionNameBe name, void *data, void *bheader);
void *ReadAtRVOFromSectionBe(RelativeVirtualOffset RVO, uint32_t Size, 
	SectionNameBe name, const socket_t *socket, void *bheader
);
BeSectionDescriptor *FindSectionBe(void *bheader, SectionNameBe name);
void *ReadSectionBe(const socket_t *socket, void *bheader, SectionNameBe name);
UINT64 FindRelocationBe(const socket_t *socket, UINT64 VirtualAddress);
bool ReadResourceBe(const socket_t *socket, const char *target, 
    void *out, GenericLengthType NBytes, GenericLengthType *Remaining
);