#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "enum.h"

/*	* =========================
	*  PE / COFF definitions
	* ========================= 
*/
#define PeHeaderOffsetAddress			0x3C
#define PeDefaultNDataDirectories		16
#define PeHeaderMagicStr				((char[4]){'P', 'E', 0, 0})
#define PeHeaderMagicU32				(((uint32_t)0) << 24) | (((uint32_t)0) << 16) | (((uint32_t)'E') << 8) | ((uint32_t)'P')

// Read the Offset of the PeHeader from Address 0x3C.
typedef struct PeHeader{
	//	"PE\0\0"
	union{
		char	mMagic[4];
		uint32_t mUMagic;
	};
	// The number that identifies the type of target machine.
	PeMachineType	mMachine;
	//	The number of sections.
	//	This indicates the size of the section table.
	uint16_t	mNumberOfSections;
	//	The low 32 bits of the number of seconds since 00:00 January 1, 1970.
	//	When the file was created.
	uint32_t	mTimeDateStamp;
	//	The file offset of the COFF symbol table.
	//		Zero if no COFF symbol table is present.
	uint32_t	mPointerToSymbolTable;
	//	The number of entries in the symbol table.
	//	This data can be used to locate the string(The ASCII-Sorted Strings that The PeExportsTableHeader.mNamePointerRVA RVA's refers to) table.
	//		It immediately follows the symbol table.
	uint32_t	mNumberOfSymbols;
	//	The size of the optional header, which is required for executable files.
	uint16_t	mSizeOfOptionalHeader;
	//	The flags that indicate the attributes of the file.
	uint16_t	mCharacteristics;
}__attribute__((packed)) PeHeader;

//* Source: "https://wiki.osdev.org/PE?__cf_chl_f_tk=.T9lp0m3.pXqaiO7WEjHNnPHRi0IizT0Sdh0f8gCWoc-1783009675-1.0.1.1-2_VObqX4vqRP0wizoNJxGCEW3LE17pF155H4lNteJTw"
// Index	Position			Contents											Section			Notes
// 			PE		PE32+																			PE32+ is 16-bytes Ahead.
// 0		96		112			export table RVA & size								.edata 	
// 1		104 	120			import table RVA & size								.idata 	
// 2		112 	128			resource table RVA & size							.rsrc 	
// 3		120 	136			exception table RVA & size							.pdata 	
// 4		128 	144			certificate table offset (not RVA!) & size 							see Signed PE below
// 5		136 	152			base relocation table RVA & size					.reloc 	
// 6		144 	160			debug data RVA & size								.debug 	
// 7		152 	168			architecture, reserved 												both fields must be zero
// 8		160 	176			global pointer register value RVA & size 							size is always 0
// 9		168 	184			thread local storage (TLS) table RVA & size			.tls
// 10		176 	192			load configuration table RVA & size 		
// 11		184 	200			bound import table RVA & size 		
// 12		192 	208			import address table (IAT) RVA & size 		
// 13		200 	216			delay import descriptor RVA & size 		
// 14		208 	224			Common Language Runtime (CLR) header RVA & size		.cormeta 	
// 15		216 	232			reserved 		both fields must be zero 
typedef struct PeRVAnSize{
	uint32_t RVA, Size;
}__attribute__((packed)) PeRVAnSize, PeDataDirectory[];

typedef struct Pe32OptionalHeader{
	PeOHeaderType			mMagic; // 0x010b - PE32
	uint8_t					mMajorLinkerVersion;
	uint8_t					mMinorLinkerVersion;
	uint32_t				mSizeOfCode;
	uint32_t				mSizeOfInitializedData;
	uint32_t				mSizeOfUninitializedData;
	uint32_t				mAddressOfEntryPoint;
	uint32_t				mBaseOfCode;
	uint32_t				mBaseOfData;
	uint32_t				mImageBase;
	uint32_t				mSectionAlignment;
	uint32_t				mFileAlignment;
	uint16_t				mMajorOperatingSystemVersion;
	uint16_t				mMinorOperatingSystemVersion;
	uint16_t				mMajorImageVersion;
	uint16_t				mMinorImageVersion;
	uint16_t				mMajorSubsystemVersion;
	uint16_t				mMinorSubsystemVersion;
	uint32_t				mWin32VersionValue;
	uint32_t				mSizeOfImage;
	uint32_t				mSizeOfHeaders;
	uint32_t				mCheckSum;
	PeSubsystem				mSubsystem;
	PeDllCharacteristics	mDllCharacteristics;
	uint32_t				mSizeOfStackReserve;
	uint32_t				mSizeOfStackCommit;
	uint32_t				mSizeOfHeapReserve;
	uint32_t				mSizeOfHeapCommit;
	uint32_t				mLoaderFlags;
	uint32_t				mNumberOfRvaAndSizes;
}__attribute__((packed)) Pe32OptionalHeader;
typedef struct Pe32PlusOptionalHeader{
	PeOHeaderType			mMagic; // 0x020b - PE32+ (64 bit)
	uint8_t					mMajorLinkerVersion;
	uint8_t					mMinorLinkerVersion;
	uint32_t				mSizeOfCode;
	uint32_t				mSizeOfInitializedData;
	uint32_t				mSizeOfUninitializedData;
	uint32_t				mAddressOfEntryPoint;
	uint32_t				mBaseOfCode;
	uint64_t				mImageBase;
	uint32_t				mSectionAlignment;
	uint32_t				mFileAlignment;
	uint16_t				mMajorOperatingSystemVersion;
	uint16_t				mMinorOperatingSystemVersion;
	uint16_t				mMajorImageVersion;
	uint16_t				mMinorImageVersion;
	uint16_t				mMajorSubsystemVersion;
	uint16_t				mMinorSubsystemVersion;
	uint32_t				mWin32VersionValue;
	uint32_t				mSizeOfImage;
	uint32_t				mSizeOfHeaders;
	uint32_t				mCheckSum;
	PeSubsystem				mSubsystem;
	PeDllCharacteristics	mDllCharacteristics;
	uint64_t				mSizeOfStackReserve;
	uint64_t				mSizeOfStackCommit;
	uint64_t				mSizeOfHeapReserve;
	uint64_t				mSizeOfHeapCommit;
	uint32_t				mLoaderFlags;
	uint32_t				mNumberOfRvaAndSizes;
}__attribute__((packed)) Pe32PlusOptionalHeader;


//	There is a Chunk of The PE32(+) Format that contains an array of These(Around 16 as since the Spec defines for 16, 
//	although since this may change then we will work with __max(n, DefaultNDataDirectories))
typedef struct PeImageSectionHeader{ // size 40 bytes
	char        				mName[8];
	uint32_t    				mVirtualSize;
	uint32_t    				mVirtualAddress;
	uint32_t    				mSizeOfRawData;
	uint32_t    				mPointerToRawData;
	uint32_t    				mPointerToRelocations;
	uint32_t    				mPointerToLinenumbers;
	uint16_t    				mNumberOfRelocations;
	uint16_t    				mNumberOfLinenumbers;
	PeSectionCharacteristics    mCharacteristics;
}__attribute__((packed)) PeImageSectionHeader;

// At PeDataDirectory[0]
typedef struct PeExportsTableHeader{ // size 40 bytes
	uint32_t	mExportFlags;
    uint32_t	mTimeDateStamp;
    uint16_t	mMajorVersion;
    uint16_t	mMinorVersion;
    uint32_t	mNameRVA;
    uint32_t	mOrdinalBase;
    uint32_t	mAddressTableEntryCount;
    uint32_t	mNamePointerEntryCount;
	//	Points to a Table of Export Addresses.
    uint32_t	mAddressTableRVA;
	//	Each element is an RVA that leads to a NULL-terminated string representing a symbol. 
	//	The table is sorted by ASCII values to make binary searches possible. 
    uint32_t	mNamePointerRVA;
	//  Since the name pointer table is sorted by name, there needs to be an additional source of information to map names to ordinals. 
	//	This Pairs Symbol Indexes, to indices within the Export Address Table.
    uint32_t	mNameOrdinalTableRVA;
}__attribute__((packed)) PeExportsTableHeader;

typedef struct PeDebugDirectoryEntry{
	// Reserved, must be zero. 
	uint32_t	mCharacteristics;
	// The time and date that the debug data was created. 
	uint32_t	mTimeDateStamp;
	uint16_t	mVersionMajor, mVersionMinor;
	PeDebugType	mdebugType;
	uint32_t	mSizeOfData;
	uint32_t	mRawDataOffset;
	uint32_t	mRawDataPtr;
}__attribute__((packed)) PeDebugDirectoryEntry;

typedef struct PeExportAddressEntry{
	//	The address of the exported symbol when loaded into memory, relative to the image base. 
	//		For example, the address of an exported function.
	uint16_t mExportRVA;
	//	The pointer to a null-terminated ASCII string in the export section.
	//	This string must be within the range that is given by the export table data directory entry.
	//	This string gives the DLL name and the name of the export e.g "MYDLL.expfunc" or the DLL name and the ordinal number of the export e.g "MYDLL.#27". 
	uint16_t mForwarderRVA;
}__attribute__((packed)) PeExportAddressEntry;
//	The Name Pointer refers to the Exported Name for an Index, 
//	whilst the Ordinal Pointer RVAs refers to a Biased Indices to the Actual Addresses/Pointers that are exported.
//	Since the Indices are the Same: 
//		Then The Index of NamePtr[Name] corresponds to Index of OrdinalPtr[Ordinal]
typedef struct PeExportDirectoryEntry{
	uint32_t 	mExportFlags;
	uint32_t 	TimeDateStamp;
	uint16_t 	mVersionMajor, mVersionMinor;
	uint32_t 	NameRVA;
	uint32_t 	OrdinalBase;
	uint32_t 	mNTableEntries;
	uint32_t 	mNNamePointers;
	//	The export name table contains the actual string data that was pointed to by the export name pointer table.
	//	The strings in this table are public names that other images can use to import the symbols.
	//	These public export names are not necessarily the same as the private symbol names
	//		that the symbols have in their own image file and source code, although they can be.
	uint32_t	mExportTableRVA;
	//	The export name pointer table is an array of addresses (RVAs) into the export name table.
	//	The pointers are 32 bits each and are relative to the image base.
	//	The pointers are ordered lexically to allow binary searches.
	uint32_t	mNamePointerRVA;
	//	The export ordinal table is an array of 16-bit unbiased indexes into the export address table.
	//	Ordinals are biased by the Ordinal Base field of the export directory table.
	//	In other words, the ordinal base must be subtracted from the ordinals to obtain true indexes into the export address table.
	uint32_t	OrdinalPointerRVA;
}__attribute__((packed)) PeExportDirectoryEntry;

typedef struct PeImportDirectoryEntry{
	//	The RVA of the import lookup table.
	//	This table contains a name or ordinal for each import.
	uint32_t	ImportLookupTableRVA;
	uint32_t	TimeDateStamp;
	//	The index of the first forwarder reference. 
	uint32_t	ForwarderChain;
	//	The address of an ASCII string that contains the name of the DLL.
	//	This address is relative to the image base. 
	uint32_t	NameRVA;
	//	The RVA of the import address table.
	//	The contents of this table are identical to the contents of the import lookup table until the image is bound. 
	uint32_t	ImportAddressTableRVA;
}PeImportDirectoryEntry;

// Terminator entry (all zeros)
#define PE_IMPORT_DIRECTORY_TERMINATOR {0, 0, 0, 0, 0}

// Import Lookup Table Entry (ILT) - PE32
//	Bit(s)		Size		Bit-field				Description
//	31			1			Ordinal/Name Flag		If this bit is set, import by ordinal. Otherwise, import by name.
//													Bit is masked as 0x80000000 for PE32, 0x8000000000000000 for PE32+.
//	15-0		16			Ordinal Number			A 16-bit ordinal number.
//													This field is used only if the Ordinal/Name Flag bit field is 1 (import by ordinal).
//													Bits 30-15 or 62-15 must be 0.
//	30-0		31			Hint/Name Table RVA		A 31-bit RVA of a hint/name table entry.
//													This field is used only if the Ordinal/Name Flag bit field is 0 (import by name).
//													For PE32+ bits 62-31 must be zero.
typedef union PeImportLookupEntry32{
    uint32_t Raw;
    struct{
        // Bit 0-30: RVA to the Hint/Name table entry (if ImportByOrdinal is 0)
        // Or Bit 0-15: The Ordinal number (if ImportByOrdinal is 1)
        uint32_t OrdinalNumberOrNameRVA : 31;
        // Bit 31: 1 if imported by ordinal, 0 if imported by name
        uint32_t ImportByOrdinal        : 1;
    }Bits;
}__attribute__((packed)) PeImportLookupEntry32, PeImportAddressEntry32;
// Import Lookup Table Entry (ILT) - PE32+
//	Bit(s)		Size		Bit-field				Description
//	63			1			Ordinal/Name Flag		If this bit is set, import by ordinal. Otherwise, import by name.
//													Bit is masked as 0x80000000 for PE32, 0x8000000000000000 for PE32+.
//	15-0		16			Ordinal Number			A 16-bit ordinal number.
//													This field is used only if the Ordinal/Name Flag bit field is 1 (import by ordinal).
//													Bits 30-15 or 62-15 must be 0.
//	30-0		31			Hint/Name Table RVA		A 31-bit RVA of a hint/name table entry.
//													This field is used only if the Ordinal/Name Flag bit field is 0 (import by name).
//													For PE32+ bits 62-31 must be zero.
typedef union PeImportLookupEntry64{
    uint64_t Raw;
    struct{
        // Bit 0-62: RVA to Hint/Name table entry / Ordinal number
        uint64_t OrdinalNumberOrNameRVA : 63;
        // Bit 63: 1 if imported by ordinal, 0 if imported by name
        uint64_t ImportByOrdinal        : 1;
    }Bits;
}__attribute__((packed)) PeImportLookupEntry64, PeImportAddressEntry64;

// Import Name Entry (when importing by name)
typedef struct PeImportNameEntry{
    uint16_t Hint;                      // Index into Export Name Pointer table (for faster lookup)
    char Name[];                       // NULL-terminated name string (variable length)
}__attribute__((packed)) PeImportNameEntry, PeImportHintEntry;

// Macros for relocation entry
#define PE_RELOC_OFFSET(entry)          ((entry).Data & 0x0FFF)
#define PE_RELOC_TYPE(entry)            (((entry).Data >> 12) & 0x0F)
// Base Relocation Block Header
// Relocation Entry (within each block)
typedef union PeRelocationEntry{
	uint16_t Raw;
	struct{
		//	The Offset within the Page, that this Relocation is in.
		//	It can point to a 32-bit(Pe32) or 64-bit(Pe32Plus) Integer.
		uint16_t Offset		: 12;
		uint16_t Type		: 4;
	};
}__attribute__((packed)) PeRelocationEntry, PeRelocationTable[];
// Relocations within a 4kB Page.
typedef struct PeBaseRelocationBlock{
	//	This is the base address of the 4KB page.
	//	All individual relocation entries inside this block are small offsets relative to this single address.
    uint32_t PageRVA;
	// Total Size of this relocation block, Being All Relocation Entries + Relocation Base
    uint32_t BlockSize;
	PeRelocationTable relocations;
}__attribute__((packed)) PeBaseRelocationBlock;




typedef struct ExpandedPeExecutable{
	void *Raw;
	struct {
		PeHeader *Header;
		union{
			Pe32OptionalHeader *Pe32;
			Pe32PlusOptionalHeader *Pe32Plus;
		}Optional;
		PeRVAnSize *RVAs;
		PeImageSectionHeader *SectionTable;
		
		struct{
			PeExportDirectoryEntry *exportEntries;
			uint32_t nExports;
			uint32_t *NamePointerRVAs;
			uint16_t *NormalisedOrdinalPointerRVAs;
			PeExportAddressEntry *RawExportAddresses;
		}exports;

		struct{
			PeImportDirectoryEntry *imports;
			uint32_t nImports;
			struct{
				char **Names;
				union{
					PeImportLookupEntry32 **ImportLookups32;
					PeImportLookupEntry64 **ImportLookups64;
				}lookups;
				union{
					PeImportLookupEntry32 **ImportAddresses32;
					PeImportLookupEntry64 **ImportAddresses64;
				}addresses;
			}perImport;
		}imports;

		struct{
			union{
				void *Raw;
				PeBaseRelocationBlock *relocations;
			}data;
			uint32_t nRelocationBlocks, *nRelocationEntriesPerBlock;
		}reloc;
	}Format;
}ExpandedPeExecutable;