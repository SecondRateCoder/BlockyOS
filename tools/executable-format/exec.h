#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ManifestJsonLen		(16 * 1024)
#define SectionPathLen			(32)
#pragma region Defaults
#define DllPathLen				512
#define ExecutableNameLen		32
#define IconDimensionsX			64
#define IconDimensionsY			64
// The Prefix for the DLL Buffer.
#define DLLPREFIX				".dll."
// The Prefix for the Import Buffer for the Section w/o this Prefix.
#define RELSYMPREFIX			".rela."
// The Prefix for the Export Buffer for the Section w/o this Prefix.
#define RELDEFPREFIX			".rel."
#pragma endregion

typedef uint32_t BExecFlags;	enum{
	// This Section should never be R/W.
	BExecFlags__Executable = 0x1, 
	// This Section should NEVER be Executed.
	BExecFlags__Data = 0x2, 
	// This Section refers to a Buffer of Relocation References/Imports.
	BExecFlags__RelocReference = 0x4, 
	// This Section refers to a Buffer of Relocation Definitions/Exports.
	BExecFlags__RelocDefinition = 0x8, 
	// This Section does not get Allocated in Memory. Implied for Non-Executable/Non-Data Sections e.g Reloc-Ref and Reloc-Def.
	BExecFlags__NoAlloc = 0x10, 
	// This Section gets Mounted as RS<N>:\\ for the Program.
	BExecFlags__Resource = 0x20
};

// A-Channel is a Brightness.
typedef uint8_t Color[4];
typedef Color ExecIcon[IconDimensionsX][IconDimensionsY];

typedef struct BExecFileItem{
	// The Name of the Section.
	char name[SectionPathLen];
	// The Flags for the File Item e.g BExecFlags__NoAlloc or BExecFlags__Resource.
	BExecFlags flags;
	// Aligned to 8 Bytes.
	size_t nBytes;
}__attribute__((aligned(8))) BExecFileItem;
typedef struct BExecHeader{
	// The Display Image.
	ExecIcon Image;
	// The Disaplay Name of the Exectuable.
	char ExecutableName[ExecutableNameLen];
	// The JsonManifest. This contains nonReallocation-centric Info.
	//	Aside from:
	//		"Main": <SYMBOL INDEX>
	//		"Enable": {<VARIOUS SECTION PATHS>}
	//		"Disable": {<VARIOUS SECTION PATHS>}
	//		etc...
	char JsonManifest[ManifestJsonLen];
	// The Number of Continuing Sections.
	uint64_t nSections;
	// The Continuing File Item.
	BExecFileItem FileItems[];
}BExecHeader;

typedef struct symbolReference{
	// The Index of the Imported Hash.
	uint64_t symIndex;
	// The Blake-2 Hash for the Symbol Name, should be used as Validation.
	uint64_t symbolHash;
	// The Flags for the Symbol Reference.
	BExecFlags flags;
	struct{
		// The Number of Bytes for the Pointer.
		uint32_t nBytesForPtr;
		// The Name of the Parent Section of the Imported Symbol.
		// The necessary Pre-processing should occur for DLL Sections etc.
		char parentSection[SectionPathLen];
	}parent;
}__attribute__((aligned(8))) symbolReference;
typedef struct symbolDefinition{
	// The Blake-2 Hash for the Symbol Name.
	uint64_t symHash;
	// The Flags for the Symbol Reference.
	BExecFlags flags;
	struct{
		// The Number of Bytes for the Pointer.
		uint32_t nBytesForPtr;
	}target;
}__attribute__((aligned(8))) symbolDefinition;

typedef struct DllItem{
	// The Path to the DLL.
	char DllPath[DllPathLen];
	// The Number of Imported Symbols.
	uint64_t nImports;
	// The Symbols to Import.
	symbolReference SymbolRefs[];
}__attribute__((aligned(8))) DllItem;