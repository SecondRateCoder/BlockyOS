#pragma once

#include <stdint.h>

#define flagcheck(n, f)			(((n) & (f)) == (f))
#define flagset(n, f)			((n) |= (f))
#define flaguset(n, f)			((n) &= ~(f))
#define enumdef(type, name)     typedef type name;  enum

enumdef(uint16_t, PeOHeaderType){Pe32Plus = 0x20b, Pe32 = 0x10b};

enumdef(uint16_t, PeMachineType){
	PeMachineType_UNKNOWN = 0x0, 
	PeMachineType_ALPHA = 0x184, PeMachineType_ALPHA64 = 0x284, 
	PeMachineType_AM33 = 0x1D3, 
	PeMachineType_AMD64 = 0x8664, 
	PeMachineType_ARM = 0x1C0, PeMachineType_ARM64 = 0xAA64, PeMachineType_ARM64EC = 0xA641, PeMachineType_ARM64X = 0xA64E, PeMachineType_ARMNT = 0x1C4, 
	PeMachineType_AXP64 = 0x284, 
	PeMachineType_EBC = 0xEBC, 
	PeMachineType_I386 = 0x14C, PeMachineType_IA64 = 0x200, 
	PeMachineType_LOONGARCH32 = 0x6232, PeMachineType_LOONGARCH64 = 0x6264, 
	PeMachineType_M32R = 0x9041, 
	PeMachineType_MIPS16 = 0x226, PeMachineType_MIPSFPU = 0x366, PeMachineType_MIPSFPU16 = 0x466, 
	PeMachineType_POWERPC = 0x1F0, PeMachineType_POWERPCFP = 0x1F1, 
	PeMachineType_R3000BE = 0x160, PeMachineType_R3000 = 0x162, PeMachineType_R4000 = 0x166, PeMachineType_R10000 = 0x168, 
	PeMachineType_RISCV32 = 0x5032, PeMachineType_RISCV64 = 0x5064, PeMachineType_RISCV128 = 0x5128, 
	PeMachineType_SH3 = 0x1A2, PeMachineType_SH3DSP = 0x1A3, PeMachineType_SH4 = 0x1A6, PeMachineType_SH5 = 0x1A8, 
	PeMachineType_THUMB = 0x1C2, 
	PeMachineType_WCEMIPSV2 = 0x169
};

enumdef(uint16_t, PeCharacteristics){
	PeCharacteristics_NRELOCS = 0X0001, PeCharacteristics_EXECUTABLE = 0X0002, 
	PeCharacteristics_NLINENUMS = 0X0004, PeCharacteristics_NLOCALSYM = 0X0008, 
	PeCharacteristics_WSTRIM = 0X0010, PeCharacteristics_LADDRESS = 0X0020, 
	PeCharacteristics_RBITS_LO = 0X0080, PeCharacteristics_32BIT = 0X0100, 
	PeCharacteristics_NDEBUG = 0X0200, 
	PeCharacteristics_LLOAD_ON_REMMEDIA = 0X0400, PeCharacteristics_LLOAD_ON_NETMEDIA = 0X0800, 
	PeCharacteristics_SYSEXE = 0X1000, PeCharacteristics_DLL = 0X2000, 
	PeCharacteristics_SYSONLY = 0X4000, PeCharacteristics_RBITS_HI = 0X8000, 
};

enumdef(uint16_t, PeSubsystem){
	PeSubsystem_UNKNOWN = 0X0, PeSubsystem_NATIVE = 0X1, 
	PeSubsystem_WINGUI = 0X2, PeSubsystem_WINCUI = 0X3, 
	PeSubsystem_0S2CUI = 0X5, PeSubsystem_POSIXCUI = 0X7, 
	PeSubsystem_WINNATIVE = 0X8, PeSubsystem_WINCE_GUI = 0X9, 
	PeSubsystem_EFI_APPLICATION = 0X10, PeSubsystem_EFIBOOT_SERVICEDRIVER = 0X11, 
	PeSubsystem_RUNTIME_DRIVER = 0X12, PeSubsystem_EFIROM = 0X13, PeSubsystem_XBOX = 0X14, 
	PeSubsystem_WINBOOT_APPLICATION = 0X16
};

enumdef(uint16_t, PeDllCharacteristics){
	PeDllCharacteristics_HIGHENTROPY_VA = 0X0020, PeDllCharacteristics_DYNBASE = 0X0040, 
	PeDllCharacteristics_FINTEGRITY = 0X0080, PeDllCharacteristics_NX = 	0X0100, 
	PeDllCharacteristics_NISOLATION = 0X0200, PeDllCharacteristics_NSEH = 0X0400, 
	PeDllCharacteristics_NBIND = 0X0800, PeDllCharacteristics_CONTAINER = 0X1000, 
	PeDllCharacteristics_WDMDRIVER = 0X2000, PeDllCharacteristics_GUARDCF = 0X4000, 
	PeDllCharacteristics_TerminalServerAware = 0x8000
};

enumdef(uint32_t, PeSectionCharacteristics){
	PeSectionCharacteristics_CODE = 0X00000020, PeSectionCharacteristics_INITDATA = 0X00000040, 
	PeSectionCharacteristics_UINITDATA = 0X00000080,PeSectionCharacteristics_LNK_OTHERS = 0X00000100, 
	PeSectionCharacteristics_GPREL = 0X00008000, PeSectionCharacteristics_PURGABLEMEM = 0X00020000, 
	PeSectionCharacteristics_16BITMEM = 0X00020000, PeSectionCharacteristics_MEMLOCKED = 0X00040000, 
	PeSectionCharacteristics_PRELOADMEM = 0X00080000, PeSectionCharacteristics_NRELOC_OVFL = 0X01000000, 
	PeSectionCharacteristics_DISCARDABLE = 0X02000000, PeSectionCharacteristics_NCACHABLE = 0X04000000, 
	PeSectionCharacteristics_NPAGABLE = 0X08000000, PeSectionCharacteristics_MSHARED = 0X10000000, 
	PeSectionCharacteristics_MEXECUTABLE = 0X20000000, PeSectionCharacteristics_MREADABLE = 0X40000000, 
	PeSectionCharacteristics_MWRITABLE = 0X80000000
};

enumdef(PeSectionCharacteristics, DefaultSectionCharacteristics){
	dataSectionCharacteristics = (PeSectionCharacteristics_INITDATA | PeSectionCharacteristics_MWRITABLE | PeSectionCharacteristics_MREADABLE), 
	bssSectionCharacteristics = (PeSectionCharacteristics_UINITDATA | PeSectionCharacteristics_MWRITABLE | PeSectionCharacteristics_MREADABLE), 
	// debugDollarFSectionCharacteristics = (PeSectionCharacteristics_INITDATA | PeSectionCharacteristics_MREADABLE | PeSectionCharacteristics_DISCARDABLE), 
	drectiveSectionCharacteristics = (PeSectionCharacteristics_LNK_OTHERS), 
	//	Table Name						Description
	//	Export directory table			A table with just one row (unlike the debug directory).
	//										This table indicates the locations and sizes of the other export tables.
	//	Export address table			An array of RVAs of exported symbols.
	//										These are the actual addresses of the exported functions and data within the executable code and data sections.
	//										Other image files can import a symbol by using an index to this table (an ordinal) or, optionally, 
	//										by using the public name that corresponds to the ordinal if a public name is defined.
	//	Name pointer table				An array of pointers to the public export names, sorted in ascending order.
	//	Ordinal table					An array of the ordinals that correspond to members of the name pointer table.
	//										The correspondence is by position; therefore, the name pointer table and the ordinal table must have the same number of members.
	//										Each ordinal is an index into the export address table.
	//	Export name table				A series of null-terminated ASCII strings.
	//										Members of the name pointer table point into this area.
	//										These names are the public names through which the symbols are imported and exported;
	//										they are not necessarily the same as the private names that are used within the image file. 
	edataSectionCharacteristics = (PeSectionCharacteristics_INITDATA | PeSectionCharacteristics_MREADABLE), 
	idataSectionCharacteristics = (PeSectionCharacteristics_INITDATA | PeSectionCharacteristics_MREADABLE | PeSectionCharacteristics_DISCARDABLE), 
	idlsymSectionCharacteristics = (PeSectionCharacteristics_LNK_OTHERS), 
	pdataSectionCharacteristics = (PeSectionCharacteristics_INITDATA | PeSectionCharacteristics_MREADABLE), 
	rdataSectionCharacteristics = (PeSectionCharacteristics_INITDATA | PeSectionCharacteristics_MREADABLE), 
	relocSectionCharacteristics = (PeSectionCharacteristics_INITDATA | PeSectionCharacteristics_MREADABLE | PeSectionCharacteristics_DISCARDABLE), 
	rsrcSectionCharacteristics = (PeSectionCharacteristics_INITDATA | PeSectionCharacteristics_MREADABLE), 
	sbssSectionCharacteristics = (PeSectionCharacteristics_UINITDATA | PeSectionCharacteristics_MREADABLE | PeSectionCharacteristics_MWRITABLE), 
	sdataSectionCharacteristics = (PeSectionCharacteristics_UINITDATA | PeSectionCharacteristics_MREADABLE | PeSectionCharacteristics_MWRITABLE), 
	srdataSectionCharacteristics = (PeSectionCharacteristics_UINITDATA | PeSectionCharacteristics_MREADABLE),
	textSectionCharacteristics = (PeSectionCharacteristics_CODE | PeSectionCharacteristics_MEXECUTABLE | PeSectionCharacteristics_MREADABLE), 
	xdataSectionCharacteristics = (PeSectionCharacteristics_INITDATA | PeSectionCharacteristics_MREADABLE)
};

enumdef(uint32_t, PeDebugType){
	PeDebugType_UNOKNOWN = 0, PeDebugType_COFF, PeDebugType_CODEVIEW, 
	PeDebugType_FPO, PeDebugType_MISC, PeDebugType_EXCEPTION, PeDebugType_FIXUP, 
	PeDebugType_OMAP_TO_SRC, PeDebugType_OMAP_FROM_SRC, PeDebugType_BORLAND, PeDebugType_CLSID, 
	PeDebugType_REPRO, PeDebugType_EX_DLLCHARACTERISTICS
};


// Bit flags for OrdinalOrNameRVA
#define PE_IMPORT_BY_ORDINAL_PE32       0x80000000  // If set, import by ordinal (PE32)
#define PE_IMPORT_BY_ORDINAL_PE32PLUS   0x8000000000000000LL  // If set, import by ordinal (PE32+)
#define PE_IMPORT_ORDINAL_NUMBER_MASK   0x0000FFFF
enumdef(uint16_t, PeRelocationTypes){
	//	The base relocation is skipped.
    PE_REL_ABSOLUTE        = 0, 
	//	The Relocation at the Offset is 16-bits.
	//	Add the high 16 bits of the difference from the Actual Address and the Expected Address to the 16-bit field at offset.
    PE_REL_HIGH            = 1, 
	//	The Relocation at the Offset is 16-bits.
	//	Add the low 16 bits of the difference from the Actual Address and the Expected Address to the 16-bit field at offset.
    PE_REL_LOW             = 2, 
	//	The Base Relocation is a 32-bit Pointer.
    PE_REL_HIGHLOW         = 3, 
	//	The Relocation at the Offset is 32-bits Pointer.
	//	This Entry refers to a High 16-bits.
	//	The Next entry refers to the Low 16-bits. 
    PE_REL_HIGHADJ         = 4, 
	//	When the machine type is MIPS, the base relocation applies to a MIPS jump instruction.
    PE_REL_MIPS_JMPADDR    = 5, 
	//	The base relocation applies the 32-bit address of a symbol across a consecutive MOVW/MOVT instruction pair.
    PE_REL_ARM_MOV32       = 5, 
	//	The base relocation applies to a MIPS16 jump instruction.
    PE_REL_MIPS_JMPADDR16  = 7, 
    PE_REL_IA64_IMM64      = 9, 
	//	The base relocation applies the difference to the 64-bit field at offset.
    PE_REL_DIR64           = 10, 
    // PE_REL_HIGH3BITS       = 11, 
    // PE_REL_RESERVED2       = 12, 
    // PE_REL_RESERVED3       = 13, 
    // PE_REL_RESERVED4       = 14, 
    // PE_REL_RESERVED5       = 15
};