#include "efi.h"
#include "efilib.h"

#include "src/Boot/UEFI/drivers/socket/socket.h"
#include "src/Boot/UEFI/tools/tools.h"

#define DEFAULTSECLOADSIZE (4 * (1024 * 1024))  // 4Mb
#define DEFAULTSECLOADMAX (32 * (1024 * 1024))  // 32Mb
#define MAGICDEF(name, nItems)      typedef union name{char MAGIC[nItems];      size_t MAGICINT[nItems / 8];}name;

#define EXECSECRTIONREFPATHLEN 16
#define offsetcalc(sym, base) (sym > base? ((sym) - (base)): ((base) - (sym)))

MAGICDEF(SYMMAGIC, 8);
typedef struct symref{
    SYMMAGIC magic;
    UINTN byteLoc;
    UINT16 symattributes;
    UINT8 ptrSize;
}symref;
bufdef(symimport, symref, UINTN);

typedef struct symdecl{
    symref symbol;
    char parent[EXECSECRTIONREFPATHLEN];
}symdecl;
bufdef(symexport, symdecl, UINTN);

enumdef(__execsecref_attributes_t, UINT32){
    __noliveload = 0x1,
    __noload = 0x2,
    __required = 0x4,
    __noreloc = 0x8,
    __reloctable = 0x10,
};

typedef struct execsectionref{
    char path[EXECSECRTIONREFPATHLEN];
    __execsecref_attributes_t attributes;
    UINT32 parameter;
    UINTN bOffset, nBytes, confBASE;
}execsectionref;

typedef struct loadedsymbols{
    symexport export; symimport import;
    execsectionref section;
    void *data;
    UINTN len;
}loadedsymbols;

MAGICDEF(exechvermagic, 16);
MAGICDEF(exechmagic, 16);
typedef struct exech{
    exechmagic magic;
    exechvermagic versionmagic;
    UINTN attributes, imageBase;
    UINT16 nSections;
    // UnImportant in acc file, this is for the loader
    execsectionref *sections;
}exech;