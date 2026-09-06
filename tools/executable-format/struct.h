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

// Read the Offset of the PeHeader from Address 0x3C.
typedef struct PeHeader{
	//	"PE\0\0"
	union{
		char	mMagic[4];
		uint32_t mUMagic;
	};
	// The number that identifies the type of target machine.
	PeMachineType		mMachine;
	//	The number of sections.
	//	This indicates the size of the section table.
	uint16_t			mNumberOfSections;
	//	The low 32 bits of the number of seconds since 00:00 January 1, 1970.
	//	When the file was created.
	uint32_t			mTimeDateStamp;
	//	The file offset of the COFF symbol table.
	//		Zero if no COFF symbol table is present.
	uint32_t			mPointerToSymbolTable;
	//	The number of entries in the symbol table.
	//	This data can be used to locate the string(The ASCII-Sorted Strings that The PeExportsTableHeader.mNamePointerRVA RVA's refers to) table.
	//		It immediately follows the symbol table.
	uint32_t			mNumberOfSymbols;
	//	The size of the optional header, which is required for executable files.
	uint16_t			mSizeOfOptionalHeader;
	//	The flags that indicate the attributes of the file.
	PeCharacteristics	mCharacteristics;
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

typedef struct{
	uint32_t	mVirtualAddress;
	uint32_t	mVirtualEnd;
	uint32_t	mHandler;
	uint32_t	mHandlerData;
	uint32_t	mVirtualPrologAddress;
}__attribute__((packed)) Pe32MIPSExceptionDataEntry;
typedef struct{
	uint32_t	mAddressRVA;
	uint32_t	mEndRVA;
	uint32_t	mUnwindRVA;
}__attribute__((packed)) Pe32PlusExceptionDataEntry, PeItaniumExceptionDataEntry;
typedef struct{
	uint32_t	mVirtualAddress;
	uint32_t	mPrologLength	:	8;
	uint32_t	mFunctionLength	:	22;
	uint32_t	mIs32Bit		:	1;
	uint32_t	mHasHandler		:	1;
}__attribute__((packed)) PeARMExceptionDataEntry, PePowerPCExceptionDataEntry, PeSH3WinCEExceptionDataEntry, PeSH4WinCEExceptionDataEntry;

typedef struct PeResourceRootDirectory{
	PeResourceType					mCharacteristics;
	uint32_t						mDateTimeStamp;
	uint16_t						mVersionMajor, mVersionMinor;
	//	The number of directory entries immediately following the table that use strings to identify Type, 
	//		Name, 
	//		Language entries
	//	(depending on the level of the table). 
	uint16_t						mNNameEntries;
	//	The number of directory entries immediately following the Name entries that use numeric IDs for Type,
	//		Name, 
	//		Language entries. 
	uint16_t						mNIDEntres;
}__attribute__((packed)) PeResourceRootDirectory;
typedef struct PeResourceDirectoryEntry{
	union{
		//	The actual Integer ID.
		struct {
			//	Offset to PeResourceString if mNameIsString is 1
            uint32_t mNameOffset   : 31;
			//	1 = Name is a string
			//	0 = Name is an Integer ID
            uint32_t mNameIsString : 1;
        };
		uint32_t	mID;
	};
	union{
		struct{
			//	This points to either a PeResourceDirectoryEntry, or to a PeResourceDataEntry.
			uint32_t mOffset		:	31;
			//	1 = Points to another Directory
			//	0 = Points to a PeResourceDataEntry
			uint32_t mIsDirectory	:	1;
		};
	};
}__attribute__((packed)) PeResourceDirectoryEntry;

typedef struct PeResourceString{
	uint16_t mLength;
	//	Binary		Comments
	//	0xxxxxxx	Only byte of a 1-byte character encoding
	//	10xxxxxx	Continuation byte: one of 1-3 bytes following the first
	//	110xxxxx	First byte of a 2-byte character encoding
	//	1110xxxx	First byte of a 3-byte character encoding
	//	11110xxx	First byte of a 4-byte character encoding
	char string[];
}PeResourceString;
typedef struct PeResourceDataEntry{
	uint32_t	mDataRVA;
	uint32_t	mDataSize;
	//	The code page that is used to decode code point values within the resource data.
	//	Typically, the code page would be the Unicode code page. 
	uint32_t	mCodePage;
}PeResourceDataEntry;

typedef struct PeExportAddressEntry{
	//	The address of the exported symbol when loaded into memory, relative to the image base. 
	//		For example, the address of an exported function.
	uint16_t mExportRVA;
	//	The pointer to a null-terminated ASCII string in the export section.
	//	This string must be within the range that is given by the export table data directory entry.
	//	This string gives the DLL name and the name of the export e.g "MYDLL.expfunc" 
	//		or the DLL name and the ordinal number of the export e.g "MYDLL.#27". 
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
	uint32_t 	mOrdinalBase;
	uint32_t 	mNTableEntries;
	uint32_t 	mNNamePointers;
	//	The export Address Table
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
}__attribute__((packed)) PeImportLookupEntry32;
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
}__attribute__((packed)) PeImportLookupEntry64;

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
}__attribute__((packed)) PeBaseRelocationBlock;

typedef union PeUnwindCode{
    struct {
		/* Offset in the prolog where the operation occurs */
        uint8_t CodeOffset;     
		/* UnwindOpCode enum value */
        uint8_t UnwindOp : 4;   
		/* Operation-specific info (e.g. register number) */
        uint8_t OpInfo   : 4;   
    };
	/* Used as raw 16-bit offset by ALLOC_LARGE, SAVE_NONVOL, etc. */
    uint16_t FrameOffset;       
}PeUnwindCode;
/*
 * Note: Variable fields follow the UnwindCode array based on alignment and flags:
 * * 1. If CountOfCodes is odd: 
 * 		1 unused padding slot (UnwindCode) follows for 32-bit alignment.
 * 2. If (Flags & UNW_FLAG_EHANDLER) or (Flags & UNW_FLAG_UHANDLER):
 *		uint32_t ExceptionHandler;	// RVA of language-specific handler
 *		uint8_t  ExceptionData[];	// Language-specific payload
 * 3. Else if (Flags & UNW_FLAG_CHAININFO):
 * 		RuntimeFunction ChainedFunction;	// Primary function bounds/unwind info
 */
typedef struct PeUnwindInfo{
	/* Unwind info version (currently 1 or 2) */
    uint8_t Version       : 3;
	/* Bitmask of UnwindFlags */
    uint8_t Flags         : 5;
	/* Length of the function prolog in bytes */
    uint8_t SizeOfProlog;
	/* Number of slots in the UnwindCode array */
    uint8_t CountOfCodes;
	/* FP register index (0 if no frame pointer used) */
    uint8_t FrameRegister : 4;
	/* Scaled frame pointer offset: FP = RSP - (FrameOffset * 16) */
    uint8_t FrameOffset   : 4;
	/* Variable-length array of UnwindCode entries [CountOfCodes] */
    PeUnwindCode UnwindCode[];

}PeUnwindInfo;


typedef struct ExpandedPeExecutable{
	void *Raw;
	char *Path;
	struct {
		PeHeader *Header;
		union{
			PeOHeaderType HeaderType;
			Pe32OptionalHeader *Pe32;
			Pe32PlusOptionalHeader *Pe32Plus;
		}Opt;
		PeRVAnSize *RVAs;
		PeImageSectionHeader *SectionTable;
		
		struct{
			void *Raw;
			PeExportDirectoryEntry *exportEntries;
			uint32_t nExports;
			uint32_t *NamePointerRVAs;
			uint16_t *NormalisedOrdinals;
			PeExportAddressEntry *RawExportAddresses;
		}exp;

		struct{
			void *Raw;
			PeImportDirectoryEntry *imports;
			uint32_t nImports;
			uint32_t *nEntries;
			struct{
				char **Names;
				union{
					PeImportLookupEntry32 **ImportLookups32;
					PeImportLookupEntry64 **ImportLookups64;
				}lookups;
			}perImport;
		}imp;

		struct{
			union{
				void *Raw;
				PeBaseRelocationBlock *relocations;
			}data;
			uint32_t nRelocationBlocks, *nRelocationEntriesPerBlock;
		}reloc;
		union{
			void *Raw;
			Pe32MIPSExceptionDataEntry	*ExceptionTableMIPS;
			Pe32PlusExceptionDataEntry	*ExceptionTable64;
			PeItaniumExceptionDataEntry	*ExceptionTableItanium;
			PeARMExceptionDataEntry		*ExceptionTableARM;
			PePowerPCExceptionDataEntry	*ExceptionTablePowerPC;
			PeSH3WinCEExceptionDataEntry*ExceptionTableSH3WinCE;
			PeSH4WinCEExceptionDataEntry*ExceptionTableSH4WinCE;
		}exception;
		struct{
			void *Raw;
			PeResourceRootDirectory	*RootDirectory;
			struct{
				union{
					void *Raw;
					PeResourceDirectoryEntry	*ResourceEntries;
				};
			}REntries;
			// void *Data;
		}rsrc;
	}Fmt;
}ExpandedPeExecutable;