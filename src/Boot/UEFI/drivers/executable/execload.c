#include "execfile.h"

static execsectionref srchSecWithPrefix(exech *header, const char sympath[EXECSECRTIONREFPATHLEN], const char *prefix){
	char path[EXECSECRTIONREFPATHLEN] = {0};
	size_t plen = AsciiStrLen(prefix);
	if(plen >= EXECSECRTIONREFPATHLEN){plen = EXECSECRTIONREFPATHLEN - 1;}
	__memcpy(path, prefix, plen);
	__memcpy(path + plen, sympath, EXECSECRTIONREFPATHLEN - plen);
	for(UINTN i = 0; i < header->nSections; i++){
		if(__memcmp(path, header->sections[i].path, EXECSECRTIONREFPATHLEN) == 0){return header->sections[i];}
	}
	return (execsectionref){0};
}

static symdecl srchSymbol(loadedsymbols *secs, UINTN nSecs, SYMMAGIC ref){
	for(UINTN i = 0; i < nSecs; i++){
		if(!secs[i].export.symexttable){continue;}
		for(UINTN j = 0; j < secs[i].export.len; j++){
			symdecl *d = &secs[i].export.symexttable[j];
			if (__memcmp(d->symbol.magic.MAGICINT, ref.MAGICINT, sizeof(ref.MAGICINT)) == 0){return *d;}
		}
	}
	return (symdecl){0};
}

ptrdiff_t computeReloc(const loadedsymbols *targetSec, const loadedsymbols *declSec, const symdecl *decl){
	// symbol absolute address = base of its section + offset
	UINT8 *symbolAbs = (UINT8 *)declSec->data + decl->symbol.byteLoc;
	// relocation site absolute address = base of target section + offset
	UINT8 *relocAbs = (UINT8 *)targetSec->data + decl->symbol.byteLoc;
	return (ptrdiff_t)(symbolAbs - relocAbs);
}

static void *loadSection(socket_t *socket, execsectionref sectionref, bool fullLoad, UINTN *nLoaded){
	if(fullLoad){*nLoaded = sectionref.nBytes;
	}else{
		if(flagcheck(sectionref.attributes, __noliveload)){
			*nLoaded = sectionref.parameter ? sectionref.parameter : sectionref.nBytes;
		}else{*nLoaded = DEFAULTSECLOADSIZE;}
	}
	if(*nLoaded == 0){return NULL;}

	socket_ret ret = socketfunc(socket->raw.read)(socket, sectionref.bOffset, *nLoaded, 0);
	if(!socketreterr(ret, *nLoaded)){return ret.data;}
	return NULL;
}

// void *__resolve(socket_t *socket, void *newbase){
// 	// 1. Read header
// 	socket_ret ret = socket->raw.read(socket, 0, sizeof(exech), 0);
// 	if(socketreterr(ret, sizeof(exech))){
// 		Print(L"[exec] Failed to read header\n");
// 		return NULL;
// 	}
// 	exech *header = ret.data;

// 	// 2. Read section table
// 	UINTN secTableSize = header->nSections * sizeof(execsectionref);
// 	ret = socket->raw.read(socket, sizeof(exech), secTableSize, 0);
// 	if(socketreterr(ret, secTableSize)){
// 		Print(L"[exec] Failed to read section table\n");
// 		return NULL;
// 	}
// 	header->sections = ret.data;

// 	// 3. Allocate loader structures
// 	loadedsymbols *secs = __calloc(header->nSections, sizeof(loadedsymbols));
// 	if(!secs){
// 		Print(L"[exec] OOM allocating loader structures\n");
// 		return NULL;
// 	}
// 	UINTN nLoaded = 0;

// 	// 4. Load each section + its sym/reloc tables
// 	for(UINTN i = 0; i < header->nSections; i++){
// 		UINTN len = 0;
// 		void *data = loadSection(
// 			socket,
// 			header->sections[i],
// 			header->sections[i].nBytes < DEFAULTSECLOADMAX,
// 			&len
// 		);

// 		if (!data){continue;}
// 		secs[nLoaded].data    = data;
// 		secs[nLoaded].len     = len;
// 		secs[nLoaded].section = header->sections[i];

// 		// Load symbol table
// 		execsectionref symSec = srchSecWithPrefix(header, secs[nLoaded].section.path, "sym.");
// 		if(symSec.nBytes > 0){secs[nLoaded].export.symexttable = loadSection(socket, symSec, true, &secs[nLoaded].export.len);}
		
// 		// Load relocation table
// 		execsectionref relSec = srchSecWithPrefix(header, secs[nLoaded].section.path, "rel.");
// 		if(relSec.nBytes > 0){secs[nLoaded].import.symimporttable = loadSection(socket, relSec, true, &secs[nLoaded].import.len);}
// 		nLoaded++;
// 	}

// 	// 5. Resolve relocations
// 	for(UINTN i = 0; i < nLoaded; i++){
// 		symimporttable *imports = &secs[i].import;
// 		if(!imports->symimporttable){continue;}
// 		for(UINTN j = 0; j < imports->len; j++){
// 			symref *imp = &imports->symimporttable[j];

// 			// Find symbol declaration
// 			symdecl decl = srchSymbol(secs, nLoaded, imp->magic);
// 			if(!*decl.symbol.magic.MAGICINT){
// 				Print(L"[exec] Unresolved symbol in section %a\n", secs[i].section.path);
// 				__builtin_trap();
// 			}

// 			// Find section that owns the symbol
// 			loadedsymbols *declSec = NULL;
// 			for(UINTN k = 0; k < nLoaded; k++){
// 				if(__memcmp(secs[k].section.path, decl.parent, EXECSECRTIONREFPATHLEN) == 0){
// 					declSec = &secs[k];
// 					break;
// 				}
// 			}
// 			if(!declSec){
// 				Print(L"[exec] Symbol declared in unknown section\n");
// 				__builtin_trap();
// 			}

// 			// Compute relocation:
// 			//   symbolAbs = base_of_decl_section + symbol_offset
// 			//   relocAbs  = base_of_target_section + relocation_offset
// 			//   value     = symbolAbs - relocAbs
// 			UINT8 *symbolAbs = (UINT8 *)declSec->data + decl.symbol.byteLoc;
// 			UINT8 *relocAbs  = (UINT8 *)secs[i].data + imp->byteLoc;
// 			ptrdiff_t rel = (ptrdiff_t)(symbolAbs - relocAbs);

// 			// Bounds check
// 			if(imp->byteLoc + imp->ptrSize > secs[i].len){
// 				Print(L"[exec] Relocation out of bounds in section %a\n", secs[i].section.path);
// 				__builtin_trap();
// 			}

// 			// Write relocation
// 			UINT8 *dst = (UINT8 *)secs[i].data + imp->byteLoc;
// 			switch(imp->ptrSize){
// 				case sizeof(UINT8): {
// 					UINT8 v = (UINT8)rel;
// 					CopyMem(dst, &v, sizeof(UINT8));
// 					break;
// 				} case sizeof(UINT16): {
// 					UINT16 v = (UINT16)rel;
// 					CopyMem(dst, &v, sizeof(UINT16));
// 					break;
// 				} case sizeof(UINT32): {
// 					UINT32 v = (UINT32)rel;
// 					CopyMem(dst, &v, sizeof(UINT32));
// 					break;
// 				} case sizeof(UINT64): {
// 					UINT64 v = (UINT64)rel;
// 					CopyMem(dst, &v, sizeof(UINT64));
// 					break;
// 				} default: {
// 					Print(L"[exec] Unsupported relocation size: %u\n", imp->ptrSize);
// 					__builtin_trap();
// 				}
// 			}
// 		}
// 	}
// 	// 5b. Find and store the Main symbol (store into header->loadin)
// 	//  - Prefer any exported symbol with symattributes & __required
// 	//  - Then try to match ASCII "main" in the SYMMAGIC bytes
// 	//  - Then prefer first exported symbol in a .text (or text) section
// 	//  - Fallback: synthesize a loadin pointing to offset 0 of first loaded section
// 	//
// 	symref chosen = {0};
// 	bool found_main = false;
// 	// 1) Look for __required attribute
// 	for(UINTN si = 0; si < nLoaded && !found_main; ++si){
// 		if(!secs[si].export.symexttable){continue;}
// 		for(UINTN ei = 0; ei < secs[si].export.len; ++ei){
// 			symdecl *d = &secs[si].export.symexttable[ei];
// 			if(d->symbol.symattributes & __required){
// 				chosen = d->symbol;
// 				found_main = true;
// 				Print(L"[exec] Main symbol found by __required in section %a\n", secs[si].section.path);
// 				break;
// 			}
// 		}
// 	}
// 	// 2) Try to match ASCII "main" in magic bytes (case-sensitive)
// 	if(!found_main){
// 		const char want[] = "main";
// 		for (UINTN si = 0; si < nLoaded && !found_main; ++si){
// 			if(!secs[si].export.symexttable){continue;}
// 			for (UINTN ei = 0; ei < secs[si].export.len; ++ei){
// 				symdecl *d = &secs[si].export.symexttable[ei];
// 				// Compare first bytes of magic to "main"
// 				if (__memcmp(d->symbol.magic.MAGIC, want, sizeof(want) - 1) == 0) {
// 					chosen = d->symbol;
// 					found_main = true;
// 					Print(L"[exec] Main symbol matched by name \"main\" in section %a\n", secs[si].section.path);
// 					break;
// 				}
// 			}
// 		}
// 	}
// 	// 3) Prefer first exported symbol in .text section
// 	if(!found_main){
// 		for(UINTN si = 0; si < nLoaded && !found_main; ++si){
// 			// Accept ".text" or "text" (producer may omit dot)
// 			if(__memcmp(secs[si].section.path, ".text", 5) != 0 && __memcmp(secs[si].section.path, "text", 4) != 0){continue;}
// 			if(!secs[si].export.symexttable || secs[si].export.len == 0){continue;}
// 			symdecl *d = &secs[si].export.symexttable[0];
// 			chosen = d->symbol;
// 			found_main = true;
// #ifdef __DEBUG__
// 			Print(L"[exec] Main symbol chosen as first export in .text section\n");
// #endif
// 			break;
// 		}
// 	}
// 	// 4) Any first exported symbol anywhere
// 	if(!found_main){
// 		for(UINTN si = 0; si < nLoaded && !found_main; ++si){
// 			if(!secs[si].export.symexttable || secs[si].export.len == 0){continue;}
// 			symdecl *d = &secs[si].export.symexttable[0];
// 			chosen = d->symbol;
// 			found_main = true;
// #ifdef __DEBUG__
// 			Print(L"[exec] Main symbol chosen as first available export in section %a\n", secs[si].section.path);
// #endif
// 			break;
// 		}
// 	}
// 	// 5) Fallback: synthesize a loadin pointing to offset 0 of first loaded section
// 	if(!found_main){
// 		if(nLoaded > 0){
// 			chosen.byteLoc = 0;
// 			chosen.ptrSize = sizeof(UINT64);
// 			chosen.symattributes = __required;
// 			// zero magic indicates synthetic
// 			__memset(chosen.magic.MAGIC, 0, sizeof(chosen.magic.MAGIC));
// #ifdef __DEBUG__
// 			Print(L"[exec] No exported symbols found; synthesizing loadin at offset 0 of first section\n");
// #endif
// 		}else{
// 			// Nothing loaded at all; leave header->loadin zeroed and return NULL
// #ifdef __DEBUG__
// 			Print(L"[exec] No sections loaded; cannot determine main symbol\n");
// #endif
// 			__builtin_trap();
// 		}
// 	}
// 	// Store chosen symbol into header->loadin
// 	// header points to the in-memory header read from the socket; update it so callers can read it.
// 	header->loadin = chosen;
// 	// print chosen loadin
// #ifdef __DEBUG__
// 	Print(L"[exec] Stored loadin: byteLoc=0x%llx ptrSize=%u attrs=0x%x\n",
// 		(unsigned long long)chosen.byteLoc, (UINTN)chosen.ptrSize, (UINTN)chosen.symattributes);
// #endif
// 	// Return entry point or first section
// 	if (newbase)
// 		return newbase;
// 	return (nLoaded > 0) ? secs[0].data : NULL;
// }
void *__resolve(socket_t *socket, void *newbase)
{
    // 1. Read header
    socket_ret ret = socketfunc(socket->raw.read)(socket, 0, sizeof(exech), 0);
    if (socketreterr(ret, sizeof(exech))) {
        Print(L"[exec] Failed to read header\n");
        return NULL;
    }
    exech *header = ret.data;

    // 2. Read section table
    UINTN secTableSize = header->nSections * sizeof(execsectionref);
    ret = socketfunc(socket->raw.read)(socket, sizeof(exech), secTableSize, 0);
    if (socketreterr(ret, secTableSize)) {
        Print(L"[exec] Failed to read section table\n");
        return NULL;
    }
    header->sections = ret.data;

    // 3. Allocate loader structures
    loadedsymbols *secs = __calloc(header->nSections, sizeof(loadedsymbols));
    if (!secs) {
        Print(L"[exec] OOM allocating loader structures\n");
        return NULL;
    }
    UINTN nLoaded = 0;

    // 4. Load each section + its sym/reloc tables
    for (UINTN i = 0; i < header->nSections; i++) {
        UINTN len = 0;
        void *data = loadSection(
            socket,
            header->sections[i],
            header->sections[i].nBytes < DEFAULTSECLOADMAX,
            &len
        );

        if (!data) {
            continue;
        }
        secs[nLoaded].data    = data;
        secs[nLoaded].len     = len;
        secs[nLoaded].section = header->sections[i];

        // Load symbol table
        execsectionref symSec = srchSecWithPrefix(header, secs[nLoaded].section.path, SYMPREFIX);
        if (symSec.nBytes > 0) {
            secs[nLoaded].export.symexttable =
                loadSection(socket, symSec, true, &secs[nLoaded].export.len);
        }

        // Load relocation table
        execsectionref relSec = srchSecWithPrefix(header, secs[nLoaded].section.path, RELOCPREFIX);
        if (relSec.nBytes > 0) {
            secs[nLoaded].import.symimporttable =
                loadSection(socket, relSec, true, &secs[nLoaded].import.len);
        }
        nLoaded++;
    }

    // 5. Resolve relocations
    for (UINTN i = 0; i < nLoaded; i++) {
        symimporttable *imports = &secs[i].import;
        if (!imports->symimporttable) { continue; }

        for (UINTN j = 0; j < imports->len; j++) {
            symref *imp = &imports->symimporttable[j];

            // Find symbol declaration
            symdecl decl = srchSymbol(secs, nLoaded, imp->magic);

            // Test for an empty/invalid decl by checking magic bytes are zero
            {
                uint8_t zero_magic[sizeof(decl.symbol.magic.MAGIC)] = {0};
                if (__memcmp(decl.symbol.magic.MAGIC, zero_magic, sizeof(zero_magic)) == 0) {
                    Print(L"[exec] Unresolved symbol in section %a\n", secs[i].section.path);
                    __builtin_trap();
                }
            }

            // Find section that owns the symbol
            loadedsymbols *declSec = NULL;
            for (UINTN k = 0; k < nLoaded; k++) {
                if (__memcmp(secs[k].section.path, decl.parent, EXECSECRTIONREFPATHLEN) == 0) {
                    declSec = &secs[k];
                    break;
                }
            }
            if (!declSec) {
                Print(L"[exec] Symbol declared in unknown section\n");
                __builtin_trap();
            }

            // Compute relocation:
            //   symbolAbs = base_of_decl_section + symbol_offset
            //   relocAbs  = base_of_target_section + relocation_offset
            //   value     = symbolAbs - relocAbs
            UINT8 *symbolAbs = (UINT8 *)declSec->data + decl.symbol.byteLoc;
            UINT8 *relocAbs  = (UINT8 *)secs[i].data + imp->byteLoc;
            ptrdiff_t rel = (ptrdiff_t)(symbolAbs - relocAbs);

            // Bounds check
            if (imp->byteLoc + imp->ptrSize > secs[i].len) {
                Print(L"[exec] Relocation out of bounds in section %a\n", secs[i].section.path);
                __builtin_trap();
            }

            // Write relocation (unaligned-safe via CopyMem)
            UINT8 *dst = (UINT8 *)secs[i].data + imp->byteLoc;
            switch (imp->ptrSize) {
                case sizeof(UINT8): {
                    UINT8 v = (UINT8)rel;
                    CopyMem(dst, &v, sizeof(UINT8));
                    break;
                }
                case sizeof(UINT16): {
                    UINT16 v = (UINT16)rel;
                    CopyMem(dst, &v, sizeof(UINT16));
                    break;
                }
                case sizeof(UINT32): {
                    UINT32 v = (UINT32)rel;
                    CopyMem(dst, &v, sizeof(UINT32));
                    break;
                }
                case sizeof(UINT64): {
                    UINT64 v = (UINT64)rel;
                    CopyMem(dst, &v, sizeof(UINT64));
                    break;
                }
                default: {
                    Print(L"[exec] Unsupported relocation size: %u\n", imp->ptrSize);
                    __builtin_trap();
                }
            }
        }
    }

    // Find and store the Main symbol (in header->loadin)
    //  - Prefer any exported symbol with symattributes & __required
    //  - Then try to match ASCII "main" in the SYMMAGIC bytes
    //  - Then prefer first exported symbol in a .text (or text) section
    //  - Fallback: synthesize a loadin pointing to offset 0 of first loaded section
    symref chosen = {0};
    bool found_main = false;

    // Look for __required attribute
    for(UINTN si = 0; si < nLoaded && !found_main; ++si){
        if(!secs[si].export.symexttable){continue;}
        for(UINTN ei = 0; ei < secs[si].export.len; ++ei){
            symdecl *d = &secs[si].export.symexttable[ei];
            if(d->symbol.symattributes & __required){
                chosen = d->symbol;
                found_main = true;
                Print(L"[exec] Main symbol found by __required in section %a\n", secs[si].section.path);
                break;
            }
        }
    }

    // Try to match ASCII "main" in magic bytes (case-sensitive)
    if(!found_main){
        const char want[] = "main";
        for(UINTN si = 0; si < nLoaded && !found_main; ++si){
            if(!secs[si].export.symexttable){continue;}
            for(UINTN ei = 0; ei < secs[si].export.len; ++ei){
                symdecl *d = &secs[si].export.symexttable[ei];
                if(__memcmp(d->symbol.magic.MAGIC, want, sizeof(want) - 1) == 0){
                    chosen = d->symbol;
                    found_main = true;
                    Print(L"[exec] Main symbol matched by name \"main\" in section %a\n", secs[si].section.path);
                    break;
                }
            }
        }
    }

    // Prefer first exported symbol in .text section
    if(!found_main){
        for(UINTN si = 0; si < nLoaded && !found_main; ++si){
            // Accept ".text" or "text" (producer may omit dot)
            if (__memcmp(secs[si].section.path, ".text", 5) != 0 && __memcmp(secs[si].section.path, "text", 4) != 0){continue;}
            if(!secs[si].export.symexttable || secs[si].export.len == 0){continue;}
            symdecl *d = &secs[si].export.symexttable[0];
            chosen = d->symbol;
            found_main = true;
#ifdef __DEBUG__
            Print(L"[exec] Main symbol chosen as first export in .text section\n");
#endif
            break;
        }
    }

    // Any first exported symbol anywhere
    if(!found_main){
        for(UINTN si = 0; si < nLoaded && !found_main; ++si){
            if(!secs[si].export.symexttable || secs[si].export.len == 0){continue;}
            symdecl *d = &secs[si].export.symexttable[0];
            chosen = d->symbol;
            found_main = true;
#ifdef __DEBUG__
            Print(L"[exec] Main symbol chosen as first available export in section %a\n", secs[si].section.path);
#endif
            break;
        }
    }

    // Fallback: synthesize a loadin pointing to offset 0 of first loaded section
    if(!found_main){
        if(nLoaded > 0){
            chosen.byteLoc = 0;
            chosen.ptrSize = sizeof(UINT64);
            chosen.symattributes = __required;
            __memset(chosen.magic.MAGIC, 0, sizeof(chosen.magic.MAGIC));
#ifdef __DEBUG__
            Print(L"[exec] No exported symbols found; synthesizing loadin at offset 0 of first section\n");
#endif
        }else{
#ifdef __DEBUG__
            Print(L"[exec] No sections loaded; cannot determine main symbol\n");
#endif
            __builtin_trap();
        }
    }

    // Store chosen symbol into header->loadin
    header->loadin = chosen;
#ifdef __DEBUG__
    Print(L"[exec] Stored loadin: byteLoc=0x%llx ptrSize=%u attrs=0x%x\n",
          (unsigned long long)chosen.byteLoc, (UINTN)chosen.ptrSize, (UINTN)chosen.symattributes);
#endif

    // Clean up loader structures (keep section data alive; free the array)
    __free(secs);

    // Return entry point or first section
    if(newbase){return newbase;}
    return (nLoaded > 0) ? secs[0].data : NULL;
}