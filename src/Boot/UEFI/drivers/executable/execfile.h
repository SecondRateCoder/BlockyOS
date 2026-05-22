#pragma once

#include "efi.h"
#include "efilib.h"

#include "src/Boot/UEFI/drivers/socket/socket.h"
#include "src/Boot/UEFI/tools/tools.h"

void *__resolve(socket_t *socket, void *newbase);

#define DEFAULTSECLOADSIZE (4 * (1024 * 1024))   // 4Mb
#define DEFAULTSECLOADMAX  (32 * (1024 * 1024))  // 32Mb
#define MAGICDEF(name, nItems) typedef union name{char MAGIC[nItems];    UINTN MAGICINT[nItems / sizeof(UINTN)];}name;
#define EXECSECRTIONREFPATHLEN 16
#define offsetcalc(sym, base) ((ptrdiff_t)((char *)(sym) - (char *)(base)))
#define SYMPREFIX ".sym."
#define RELOCPREFIX ".rel."
#define FIND_RELOC(sec, hdr) srchSecWithPrefix(hdr, sec, RELOCPREFIX)
#define FIND_SYMTAB(sec, hdr) srchSecWithPrefix(hdr, sec, SYMPREFIX)

MAGICDEF(SYMMAGIC, 8);
typedef struct symref{
	SYMMAGIC magic;
	UINTN    byteLoc;       // Offset within section where relocation applies
	UINT16   symattributes;
	UINT8    ptrSize;       // Size of relocation slot (1,2,4,8)
}symref;
bufdef(symimporttable, symref, UINTN);

typedef struct symdecl{
	symref symbol;                          // Declared symbol info
	char   parent[EXECSECRTIONREFPATHLEN];  // Owning section/path
}symdecl;
bufdef(symexttable, symdecl, UINTN);

// Unsupported yet.
typedef struct symsharein{
	char export[256];
	symimporttable imports;
}symexternal;
typedef struct symshareout{
	char export[256];
	symexttable exports;
}syminternal;


enumdef(__execsecref_attributes_t, UINT32){
	__noliveload  = 0x1,
	__noload      = 0x2,
	__required    = 0x4,
	__noreloc     = 0x8,
	__reloctable  = 0x10,
};

typedef struct execsectionref{
	char                     path[EXECSECRTIONREFPATHLEN];
	__execsecref_attributes_t attributes;
	UINT32                   parameter;
	UINTN                    bOffset;   // File offset
	UINTN                    nBytes;    // Section size
	UINTN                    confBASE;  // Preferred base (if any)
}execsectionref;

typedef struct loadedsymbols{
	symexttable      export;
	symimporttable      import;
	execsectionref section;
	void          *data;   // Loaded section data
	UINTN          len;    // Bytes loaded
}loadedsymbols;

MAGICDEF(exechvermagic, 16);
MAGICDEF(exechmagic, 16);

typedef struct exech{
	exechmagic     magic;
	exechvermagic  versionmagic;
	UINTN          attributes;
	UINTN          imageBase;
	UINT16         nSections;
	symref	loadin;
	// For loader use only
	execsectionref *sections;
}exech;