#include "execfile.h"

execsectionref srchSecRelocTable(socket_t * socket, exech * header, char sympath[EXECSECRTIONREFPATHLEN]){
	static const char __relocSnippet[3] = "rel.";
	char path[EXECSECRTIONREFPATHLEN] = {0};
	CopyMem(path, __relocSnippet, ARRSIZE(__relocSnippet));
	CopyMem(path + ARRSIZE(__relocSnippet), sympath, ARRSIZE(path) - ARRSIZE(__relocSnippet));
	for(UINTN cc = 0; cc < header->nSections; ++cc){
		if(!CompareMem(path, header->sections[cc].path, EXECSECRTIONREFPATHLEN)){
			return header->sections[cc];
		}
	}
	return (execsectionref){0};
}

// Get the Declaration for a Symbol
symdecl srchSymbol(loadedsymbols *sections, UINTN nSections, SYMMAGIC ref){
	for(UINTN cc = 0; cc < nSections; ++cc){
		if(!sections[cc].export.symexport){continue;}
		for(UINTN cc_ = 0; cc_ < sections[cc].export.len; ++cc_){
			if(!CompareMem(sections[cc].export.symexport[cc_].symbol.magic.MAGICINT, ref.MAGICINT, sizeof(ref.MAGICINT))){
				return sections[cc].export.symexport[cc_];
			}
		}
	}
	return (symdecl){0};
}

execsectionref srchSecSymTable(socket_t * socket, exech * header, char sympath[EXECSECRTIONREFPATHLEN]){
	static const char __relocSnippet[3] = "sym.";
	char path[EXECSECRTIONREFPATHLEN] = {0};
	CopyMem(path, __relocSnippet, ARRSIZE(__relocSnippet));
	CopyMem(path + ARRSIZE(__relocSnippet), sympath, ARRSIZE(path) - ARRSIZE(__relocSnippet));
	for(UINTN cc = 0; cc < header->nSections; ++cc){
		if(!CompareMem(path, header->sections[cc].path, EXECSECRTIONREFPATHLEN)){
			return header->sections[cc];
		}
	}
	return (execsectionref){0};
}

void *loadSection(socket_t * socket, execsectionref sectionref, bool fullLoad, UINTN *nLoaded){
	if(fullLoad){*nLoaded = sectionref.nBytes;}else{
		if(flagcheck(sectionref.attributes, __noliveload)){(*nLoaded) = sectionref.parameter? sectionref.parameter: sectionref.nBytes;}    // Load full Size or Parameter Bytes
		else{*nLoaded = DEFAULTSECLOADSIZE;}
	}
	if(*nLoaded){
		socket_ret ret = socket->raw.read(socket, sectionref.bOffset, (*nLoaded), 0);
		if(!socketreterr(ret, *nLoaded)){
			return ret.data;
		}else{return NULL;}
	}else{return NULL;}
}

void *__resolve(socket_t * socket, void * newbase){
	socket_ret ret = socket->raw.read(socket, 0, sizeof(exech), 0);
	exech *header = NULL;
	if(!socketreterr(ret, sizeof(exech))){
		header = ret.data;
	}else{return NULL;}
	ret = socket->raw.read(socket, sizeof(exech), header->nSections * sizeof(execsectionref), 0);
	if(!socketreterr(ret, header->nSections * sizeof(execsectionref))){
		header->sections = ret.data;
	}else{return NULL;}
	loadedsymbols *sections = AllocatePool(sizeof(loadedsymbols) * header->nSections);      UINTN nLoaded = 0;
	for(UINT16 cc = 0; cc < header->nSections; ++cc){
		if(sections[nLoaded].data = loadSection(socket, header->sections[cc], header->sections[cc].nBytes < DEFAULTSECLOADMAX, &(sections[nLoaded].len))){
			sections[nLoaded].section = header->sections[cc];
			sections[nLoaded].export.symexport = loadSection(socket, srchSecSymTable(socket, header, sections[cc].section.path), true, &sections[nLoaded].export.len);
			sections[nLoaded].import.symimport = loadSection(socket, srchSecRelocTable(socket, header, sections[cc].section.path), true, &sections[nLoaded].import.len);
			nLoaded++;
		}
	}
	// For all imports, get the declaration and resolve it.
	for(UINTN cc = 0; cc < nLoaded; ++cc){
		if(!sections[cc].import.symimport){continue;}
		for(UINTN cc_ = 0; cc_ < sections[cc].import.len; ++cc_){
			symdecl decl = srchSymbol(sections, nLoaded, sections[cc].import.symimport[cc_].magic);
			if(*decl.symbol.magic.MAGICINT){
				switch(sections[cc].import.symimport[cc_].ptrSize){
					case sizeof(UINT8): {
						UINT8 ptr = offsetcalc(decl.symbol.byteLoc, (size_t)sections[cc].data);
						((UINT8 *)sections[cc].data)[sections[cc].import.symimport[cc_].byteLoc] = ptr;
					} case sizeof(UINT16): {
						UINT16 ptr = offsetcalc(decl.symbol.byteLoc, (size_t)sections[cc].data);
						((UINT16 *)sections[cc].data)[sections[cc].import.symimport[cc_].byteLoc] = ptr;
					} case sizeof(UINT32): {
						UINT32 ptr = offsetcalc(decl.symbol.byteLoc, (size_t)sections[cc].data);
						((UINT32 *)sections[cc].data)[sections[cc].import.symimport[cc_].byteLoc] = ptr;
					}
				}
			}else{__builtin_trap();}
		}
	}
}