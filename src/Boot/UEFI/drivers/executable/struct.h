#pragma once

#include "src/Boot/UEFI/guid.h"
#include "src/Boot/UEFI/tools/tools.h"
#include "src/Boot/UEFI/drivers/socket/socket.h"

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
		UINT32 mUMagic;
	};
	// The number that identifies the type of target machine.
	PeMachineType		mMachine;
	//	The number of sections.
	//	This indicates the size of the section table.
	UINT16			mNumberOfSections;
	//	The low 32 bits of the number of seconds since 00:00 January 1, 1970.
	//	When the file was created.
	UINT32			mTimeDateStamp;
	//	The file offset of the COFF symbol table.
	//		Zero if no COFF symbol table is present.
	UINT32			mPointerToSymbolTable;
	//	The number of entries in the symbol table.
	//	This data can be used to locate the string(The ASCII-Sorted Strings that The PeExportsTableHeader.mNamePointerRVA RVA's refers to) table.
	//		It immediately follows the symbol table.
	UINT32			mNumberOfSymbols;
	//	The size of the optional header, which is required for executable files.
	UINT16			mSizeOfOptionalHeader;
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
	UINT32 RVA, Size;
}__attribute__((packed)) PeRVAnSize, PeDataDirectory[];

typedef struct Pe32OptionalHeader{
	PeOptionalHeaderType			mMagic; // 0x010b - PE32
	UINT8					mMajorLinkerVersion;
	UINT8					mMinorLinkerVersion;
	UINT32				mSizeOfCode;
	UINT32				mSizeOfInitializedData;
	UINT32				mSizeOfUninitializedData;
	UINT32				mAddressOfEntryPoint;
	UINT32				mBaseOfCode;
	UINT32				mBaseOfData;
	UINT32				mImageBase;
	UINT32				mSectionAlignment;
	UINT32				mFileAlignment;
	UINT16				mMajorOperatingSystemVersion;
	UINT16				mMinorOperatingSystemVersion;
	UINT16				mMajorImageVersion;
	UINT16				mMinorImageVersion;
	UINT16				mMajorSubsystemVersion;
	UINT16				mMinorSubsystemVersion;
	UINT32				mWin32VersionValue;
	UINT32				mSizeOfImage;
	UINT32				mSizeOfHeaders;
	UINT32				mCheckSum;
	PeSubsystem				mSubsystem;
	PeDllCharacteristics	mDllCharacteristics;
	UINT32				mSizeOfStackReserve;
	UINT32				mSizeOfStackCommit;
	UINT32				mSizeOfHeapReserve;
	UINT32				mSizeOfHeapCommit;
	UINT32				mLoaderFlags;
	UINT32				mNumberOfRvaAndSizes;
}__attribute__((packed)) Pe32OptionalHeader;
typedef struct Pe32PlusOptionalHeader{
	PeOptionalHeaderType			mMagic; // 0x020b - PE32+ (64 bit)
	UINT8					mMajorLinkerVersion;
	UINT8					mMinorLinkerVersion;
	UINT32				mSizeOfCode;
	UINT32				mSizeOfInitializedData;
	UINT32				mSizeOfUninitializedData;
	UINT32				mAddressOfEntryPoint;
	UINT32				mBaseOfCode;
	UINT64				mImageBase;
	UINT32				mSectionAlignment;
	UINT32				mFileAlignment;
	UINT16				mMajorOperatingSystemVersion;
	UINT16				mMinorOperatingSystemVersion;
	UINT16				mMajorImageVersion;
	UINT16				mMinorImageVersion;
	UINT16				mMajorSubsystemVersion;
	UINT16				mMinorSubsystemVersion;
	UINT32				mWin32VersionValue;
	UINT32				mSizeOfImage;
	UINT32				mSizeOfHeaders;
	UINT32				mCheckSum;
	PeSubsystem				mSubsystem;
	PeDllCharacteristics	mDllCharacteristics;
	UINT64				mSizeOfStackReserve;
	UINT64				mSizeOfStackCommit;
	UINT64				mSizeOfHeapReserve;
	UINT64				mSizeOfHeapCommit;
	UINT32				mLoaderFlags;
	UINT32				mNumberOfRvaAndSizes;
}__attribute__((packed)) Pe32PlusOptionalHeader;


//	There is a Chunk of The PE32(+) Format that contains an array of These(Around 16 as since the Spec defines for 16, 
//	although since this may change then we will work with __max(n, DefaultNDataDirectories))
typedef struct PeImageSectionHeader{ // size 40 bytes
	char        				mName[8];
	UINT32    				mVirtualSize;
	UINT32    				mVirtualAddress;
	UINT32    				mSizeOfRawData;
	UINT32    				mPointerToRawData;
	UINT32    				mPointerToRelocations;
	UINT32    				mPointerToLinenumbers;
	UINT16    				mNumberOfRelocations;
	UINT16    				mNumberOfLinenumbers;
	PeSectionCharacteristics    mCharacteristics;
}__attribute__((packed)) PeImageSectionHeader;

// At PeDataDirectory[0]

typedef struct{
	UINT32	mVirtualAddress;
	UINT32	mVirtualEnd;
	UINT32	mHandler;
	UINT32	mHandlerData;
	UINT32	mVirtualPrologAddress;
}__attribute__((packed)) Pe32MIPSExceptionDataEntry;
typedef struct{
	UINT32	mAddressRVA;
	UINT32	mEndRVA;
	UINT32	mUnwindRVA;
}__attribute__((packed)) Pe32PlusExceptionDataEntry, PeItaniumExceptionDataEntry;
typedef struct{
	UINT32	mVirtualAddress;
	UINT32	mPrologLength	:	8;
	UINT32	mFunctionLength	:	22;
	UINT32	mIs32Bit		:	1;
	UINT32	mHasHandler		:	1;
}__attribute__((packed)) PeARMExceptionDataEntry, PePowerPCExceptionDataEntry, PeSH3WinCEExceptionDataEntry, PeSH4WinCEExceptionDataEntry;

typedef struct PeResourceRootDirectory{
	PeResourceType					mCharacteristics;
	UINT32						mDateTimeStamp;
	UINT16						mVersionMajor, mVersionMinor;
	//	The number of directory entries immediately following the table that use strings to identify Type, 
	//		Name, 
	//		Language entries
	//	(depending on the level of the table). 
	UINT16						mNNameEntries;
	//	The number of directory entries immediately following the Name entries that use numeric IDs for Type,
	//		Name, 
	//		Language entries. 
	UINT16						mNIDEntres;
}__attribute__((packed)) PeResourceRootDirectory;
typedef struct PeResourceDirectoryEntry{
	union{
		//	The actual Integer ID.
		struct {
			//	Offset to PeResourceString if mNameIsString is 1
            UINT32 mNameOffset   : 31;
			//	1 = Name is a string
			//	0 = Name is an Integer ID
            UINT32 mNameIsString : 1;
        };
		UINT32	mID;
	};
	union{
		struct{
			//	This points to either a PeResourceDirectoryEntry, or to a PeResourceDataEntry.
			UINT32 mOffset		:	31;
			//	1 = Points to another Directory
			//	0 = Points to a PeResourceDataEntry
			UINT32 mIsDirectory	:	1;
		};
	};
}__attribute__((packed)) PeResourceDirectoryEntry;

typedef struct PeResourceString{
	UINT16 mLength;
	//	Binary		Comments
	//	0xxxxxxx	Only byte of a 1-byte character encoding
	//	10xxxxxx	Continuation byte: one of 1-3 bytes following the first
	//	110xxxxx	First byte of a 2-byte character encoding
	//	1110xxxx	First byte of a 3-byte character encoding
	//	11110xxx	First byte of a 4-byte character encoding
	char string[];
}PeResourceString;
typedef struct PeResourceDataEntry{
	UINT32	mDataRVA;
	UINT32	mDataSize;
	//	The code page that is used to decode code point values within the resource data.
	//	Typically, the code page would be the Unicode code page. 
	UINT32	mCodePage;
}PeResourceDataEntry;

typedef struct PeExportAddressEntry{
	UINT32 mExportRVA;
}__attribute__((packed)) PeExportAddressEntry;
//	The Name Pointer refers to the Exported Name for an Index, 
//	whilst the Ordinal Pointer RVAs refers to a Biased Indices to the Actual Addresses/Pointers that are exported.
//	Since the Indices are the Same: 
//		Then The Index of NamePtr[Name] corresponds to Index of OrdinalPtr[Ordinal]
typedef struct PeExportDirectoryEntry{
	UINT32 	mExportFlags;
	UINT32 	TimeDateStamp;
	UINT16 	mVersionMajor, mVersionMinor;
	UINT32 	NameRVA;
	UINT32 	mOrdinalBase;
	UINT32 	mNTableEntries;
	UINT32 	mNNamePointers;
	//	The export Address Table
	UINT32	mExportTableRVA;
	//	The export name pointer table is an array of addresses (RVAs) into the export name table.
	//	The pointers are 32 bits each and are relative to the image base.
	//	The pointers are ordered lexically to allow binary searches.
	UINT32	mNamePointerRVA;
	//	The export ordinal table is an array of 16-bit unbiased indexes into the export address table.
	//	Ordinals are biased by the Ordinal Base field of the export directory table.
	//	In other words, the ordinal base must be subtracted from the ordinals to obtain true indexes into the export address table.
	UINT32	OrdinalPointerRVA;
}__attribute__((packed)) PeExportDirectoryEntry;

typedef struct PeImportDirectoryEntry{
	//	The RVA of the import lookup table.
	//	This table contains a name or ordinal for each import.
	UINT32	ImportLookupTableRVA;
	UINT32	TimeDateStamp;
	//	The index of the first forwarder reference. 
	UINT32	ForwarderChain;
	//	The address of an ASCII string that contains the name of the DLL.
	//	This address is relative to the image base. 
	UINT32	NameRVA;
	//	The RVA of the import address table.
	//	The contents of this table are identical to the contents of the import lookup table until the image is bound. 
	UINT32	ImportAddressTableRVA;
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
    UINT32 Raw;
    struct{
        // Bit 0-30: RVA to the Hint/Name table entry (if ImportByOrdinal is 0)
        // Or Bit 0-15: The Ordinal number (if ImportByOrdinal is 1)
        UINT32 OrdinalNumberOrNameRVA : 31;
        // Bit 31: 1 if imported by ordinal, 0 if imported by name
        UINT32 ImportByOrdinal        : 1;
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
    UINT64 Raw;
    struct{
        // Bit 0-62: RVA to Hint/Name table entry / Ordinal number
        UINT64 OrdinalNumberOrNameRVA : 63;
        // Bit 63: 1 if imported by ordinal, 0 if imported by name
        UINT64 ImportByOrdinal        : 1;
    }Bits;
}__attribute__((packed)) PeImportLookupEntry64;

// Import Name Entry (when importing by name)
typedef struct PeImportNameEntry{
    UINT16 Hint;                      // Index into Export Name Pointer table (for faster lookup)
    char Name[];                       // NULL-terminated name string (variable length)
}__attribute__((packed)) PeImportNameEntry, PeImportHintEntry;

// Macros for relocation entry
#define PE_RELOC_OFFSET(entry)          ((entry).Data & 0x0FFF)
#define PE_RELOC_TYPE(entry)            (((entry).Data >> 12) & 0x0F)
// Base Relocation Block Header
// Relocation Entry (within each block)
typedef union PeRelocationEntry{
	UINT16 Raw;
	struct{
		//	The Offset within the Page, that this Relocation is in.
		//	It can point to a 32-bit(Pe32) or 64-bit(Pe32Plus) Integer.
		UINT16 Offset		: 12;
		UINT16 Type		: 4;
	};
}__attribute__((packed)) PeRelocationEntry, PeRelocationTable[];
// Relocations within a 4kB Page.
typedef struct PeBaseRelocationBlock{
	//	This is the base address of the 4KB page.
	//	All individual relocation entries inside this block are small offsets relative to this single address.
    UINT32 PageRVA;
	// Total Size of this relocation block, Being All Relocation Entries + Relocation Base
    UINT32 BlockSize;
}__attribute__((packed)) PeBaseRelocationBlock;

typedef union PeUnwindCode{
    struct {
		/* Offset in the prolog where the operation occurs */
        UINT8 CodeOffset;     
		/* UnwindOpCode enum value */
        UINT8 UnwindOp : 4;   
		/* Operation-specific info (e.g. register number) */
        UINT8 OpInfo   : 4;   
    };
	/* Used as raw 16-bit offset by ALLOC_LARGE, SAVE_NONVOL, etc. */
    UINT16 FrameOffset;       
}PeUnwindCode;
/*
 * Note: Variable fields follow the UnwindCode array based on alignment and flags:
 * * 1. If CountOfCodes is odd: 
 * 		1 unused padding slot (UnwindCode) follows for 32-bit alignment.
 * 2. If (Flags & UNW_FLAG_EHANDLER) or (Flags & UNW_FLAG_UHANDLER):
 *		UINT32 ExceptionHandler;	// RVA of language-specific handler
 *		UINT8  ExceptionData[];	// Language-specific payload
 * 3. Else if (Flags & UNW_FLAG_CHAININFO):
 * 		RuntimeFunction ChainedFunction;	// Primary function bounds/unwind info
 */
typedef struct PeUnwindInfo{
	/* Unwind info version (currently 1 or 2) */
    UINT8 Version       : 3;
	/* Bitmask of UnwindFlags */
    UINT8 Flags         : 5;
	/* Length of the function prolog in bytes */
    UINT8 SizeOfProlog;
	/* Number of slots in the UnwindCode array */
    UINT8 CountOfCodes;
	/* FP register index (0 if no frame pointer used) */
    UINT8 FrameRegister : 4;
	/* Scaled frame pointer offset: FP = RSP - (FrameOffset * 16) */
    UINT8 FrameOffset   : 4;
	/* Variable-length array of UnwindCode entries [CountOfCodes] */
    PeUnwindCode UnwindCode[];

}PeUnwindInfo;


typedef struct ExpandedPeExecutable{
	void *Raw;
	socket_t *sck;
	struct {
		PeHeader *Header;
		union{
			PeOptionalHeaderType HeaderType;
			Pe32OptionalHeader *Pe32;
			Pe32PlusOptionalHeader *Pe32Plus;
		}Opt;
		PeRVAnSize *RVAs;
		PeImageSectionHeader *SectionTable;
		
		struct{
			void *Raw;
			PeExportDirectoryEntry *Export;
			UINT32 *NamePointerRVAs;
			UINT16 *Ordinals;
			PeExportAddressEntry *RawExportAddresses;
		}exp;

		struct{
			void *Raw;
			PeImportDirectoryEntry *imports;
			UINT32 nImports;
			UINT32 *nEntries;
			struct{
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
			UINT32 nRelocationBlocks, *nRelocationEntriesPerBlock;
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

typedef struct LoadedPeSection{
	char Name[8];
	void *Base;
	UINT64 Limit;
}LoadedPeSection;
typedef struct LoadedPeExecutable{
	void						*Base, 
								*EntryPoint;
	char						*Name;
	ExpandedPeExecutable		*This;
	LoadedPeSection				*Sections;
	UINT16						NSections, NDependencies;
	struct LoadedPeExecutable	**Dependencies;
}LoadedPeExecutable;