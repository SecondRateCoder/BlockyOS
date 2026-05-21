#include "execfile.h"

static execsectionref srchSecRelocTable(socket_t *socket, exech *header, const char sympath[EXECSECRTIONREFPATHLEN]){
    const char prefix[] = "rel.";
    char path[EXECSECRTIONREFPATHLEN] = {0};
    __memcpy(path, prefix, MIN(sizeof(prefix) - 1, sizeof(path)));
    __memcpy(path + (sizeof(prefix) - 1), sympath, sizeof(path) - (sizeof(prefix) - 1));

    for (UINTN cc = 0; cc < header->nSections; ++cc) {
        if (__memcmp(path, header->sections[cc].path, EXECSECRTIONREFPATHLEN) == 0) {
            return header->sections[cc];
        }
    }
    return (execsectionref){0};
}

static execsectionref srchSecSymTable(socket_t *socket, exech *header, const char sympath[EXECSECRTIONREFPATHLEN]){
    const char prefix[] = "sym.";
    char path[EXECSECRTIONREFPATHLEN] = {0};
    __memcpy(path, prefix, MIN(sizeof(prefix) - 1, sizeof(path)));
    __memcpy(path + (sizeof(prefix) - 1), sympath, sizeof(path) - (sizeof(prefix) - 1));
    for (UINTN cc = 0; cc < header->nSections; ++cc) {
        if (__memcmp(path, header->sections[cc].path, EXECSECRTIONREFPATHLEN) == 0) {
            return header->sections[cc];
        }
    }
    return (execsectionref){0};
}
// Get the declaration for a symbol by magic
static symdecl srchSymbol(loadedsymbols *sections, UINTN nSections, SYMMAGIC ref){
    for(UINTN cc = 0; cc < nSections; ++cc){
        if(!sections[cc].export.symexttable){continue;}
        for (UINTN cc_ = 0; cc_ < sections[cc].export.len; ++cc_) {
            if (__memcmp(sections[cc].export.symexttable[cc_].symbol.magic.MAGICINT,
				ref.MAGICINT,
				sizeof(ref.MAGICINT)) == 0
			){return sections[cc].export.symexttable[cc_];}
        }
    }
    return (symdecl){0};
}

static void *loadSection(socket_t *socket, execsectionref sectionref, bool fullLoad, UINTN *nLoaded){
    if(fullLoad){*nLoaded = sectionref.nBytes;
    }else{
        if(flagcheck(sectionref.attributes, __noliveload)){
            *nLoaded = sectionref.parameter ? sectionref.parameter : sectionref.nBytes;
        }else{*nLoaded = DEFAULTSECLOADSIZE;}
    }
    if(*nLoaded == 0){return NULL;}

    socket_ret ret = socket->raw.read(socket, sectionref.bOffset, *nLoaded, 0);
    if(!socketreterr(ret, *nLoaded)){return ret.data;}
    return NULL;
}
void *__resolve(socket_t *socket, void *newbase){
    // Read header
    socket_ret ret = socket->raw.read(socket, 0, sizeof(exech), 0);
    exech *header = NULL;
    if(!socketreterr(ret, sizeof(exech))){
        header = ret.data;
    }else{return NULL;}
    // Read section table
    ret = socket->raw.read(socket, sizeof(exech), header->nSections * sizeof(execsectionref), 0);
    if(!socketreterr(ret, header->nSections * sizeof(execsectionref))){
        header->sections = ret.data;
    }else{return NULL;}

    // Allocate loader structures
    loadedsymbols *sections = __calloc(header->nSections, sizeof(loadedsymbols));
    if(!sections){return NULL;}

    UINTN nLoaded = 0;

    // Load sections + their sym/reloc tables
    for(UINT16 cc = 0; cc < header->nSections; ++cc){
        UINTN len = 0;
        void *data = loadSection(socket,
                                 header->sections[cc],
                                 header->sections[cc].nBytes < DEFAULTSECLOADMAX,
                                 &len);
        if (!data) {continue;}
        sections[nLoaded].data    = data;
        sections[nLoaded].len     = len;
        sections[nLoaded].section = header->sections[cc];

        // Use the section we just stored (nLoaded), not cc
        execsectionref symSec = srchSecSymTable(socket, header, sections[nLoaded].section.path);
        execsectionref relSec = srchSecRelocTable(socket, header, sections[nLoaded].section.path);

        sections[nLoaded].export.symexttable =
            loadSection(socket, symSec, true, &sections[nLoaded].export.len);
        sections[nLoaded].import.symimporttable =
            loadSection(socket, relSec, true, &sections[nLoaded].import.len);

        nLoaded++;
    }
	for(UINTN cc = 0; cc < nLoaded; ++cc){
        if(!sections[cc].import.symimporttable){continue;}
        for(UINTN cc_ = 0; cc_ < sections[cc].import.len; ++cc_){
            symref  *imp  = &sections[cc].import.symimporttable[cc_];
            symdecl  decl = srchSymbol(sections, nLoaded, imp->magic);
            if(!*decl.symbol.magic.MAGICINT){__builtin_trap();}// Unresolved symbol
            // Compute pointer/offset: symbol location relative to section base
            // Assume decl.symbol.byteLoc is an absolute address
            // and sections[cc].data is the base of the section.
            ptrdiff_t rel = offsetcalc((void *)decl.symbol.byteLoc, sections[cc].data);

            UINTN writeOffset = imp->byteLoc;
            if(writeOffset + imp->ptrSize > sections[cc].len){__builtin_trap();}// Out-of-bounds relocation
            switch (imp->ptrSize) {
            case sizeof(UINT8): {
                UINT8 v = (UINT8)rel;
                CopyMem((UINT8 *)sections[cc].data + writeOffset, &v, sizeof(v));
                break;
            } case sizeof(UINT16): {
                UINT16 v = (UINT16)rel;
                CopyMem((UINT8 *)sections[cc].data + writeOffset, &v, sizeof(v));
                break;
            } case sizeof(UINT32): {
                UINT32 v = (UINT32)rel;
                CopyMem((UINT8 *)sections[cc].data + writeOffset, &v, sizeof(v));
                break;
            } case sizeof(UINT64): {
                UINT64 v = (UINT64)rel;
                CopyMem((UINT8 *)sections[cc].data + writeOffset, &v, sizeof(v));
                break;
            }
            default: {__builtin_trap();} // Unsupported relocation size
            }
        }
    }
    // Probably want to return the entry point or base of some section.
    // For now, just return newbase or first section’s data if newbase is NULL.
    if(newbase){return newbase;}
    return (nLoaded > 0) ? sections[0].data : NULL;
}