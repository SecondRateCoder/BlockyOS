#include "tools.h"

BOOLEAN strcheck(char *s, char c){
	for(size_t cc = 0; cc < __strlen(s); ++cc){
		if(s[cc] == c){return TRUE;}
	}
	return FALSE;
}

INT64 strchecki(char *s, char c){
	for(size_t cc = 0; cc < __strlen(s); ++cc){
		if(c == s[cc]){return cc;}
	}
	return -1;
}

void *memdup(void *mem, size_t s){
	void *out = AllocatePool(s);
	__memcpy(out, mem, s);
	return out;
}

BOOLEAN isascii(char c){return c <= 0x7F;}

UINT64 __getfcode(char *s_){
	char *s = __strdup(s_);
	for(UINT32 cc = 0; cc < __strlen(s_); ++cc){
		if(s[cc] == PATHnoSEP){s[cc] = PATHSEP;}
	}
	UINT64 hash;
	blake2b_state hashstate;
	blake2b_init(&hashstate, sizeof(UINT64));
	blake2b_update(&hashstate, s, __strlen(s));
	blake2b_final(&hashstate, &hash, sizeof(UINT64));
	hash &= 0x3FFFFFFFFFF;
    FreePool(s);
	return hash;
}

char *readbuf(size_t s, CHAR16 *prefix){
    Print(prefix);
	char *out = __calloc(s + 1, sizeof(char));
	if(out){
		size_t i = 0;
		for(; i < s; ++i){
            EFI_INPUT_KEY key;
            if(!EFI_ERROR(ST->ConIn->ReadKeyStroke(ST->ConIn, &key))){
                if(isascii((char)(key.UnicodeChar & 0xFF))){
                    out[i] = (char)(key.UnicodeChar & 0xFF);
                    if(out[i] == '\n' || out[i] == '\r'){break;}
                }
            }
        }
		out[i] = '\0';
	}
	return out;
}

strtok_t *strtok_i(char *in, char *delims, UINT32 enables){
    if(
        (flagcheck(enables, strtok__ForceSameBorderingDelims) || flagcheck(enables, strtok__ForceDifferentBorderingDelims)) &&
        flagcheck(enables, strtok__ForceEndingDelim) || flagcheck(enables, strtok__ForceStartingDelim)
    ){return NULL;}
    if(flagcheck(enables, strtok__ForceSameBorderingDelims) && flagcheck(enables, strtok__ForceDifferentBorderingDelims)){return NULL;}
	strtok_t *out = __calloc(1, sizeof(strtok_t));
	out->dup = __strdup(in);
	out->delims = __strdup(delims);
    out->flags = enables;
	return out;
}

char *strtok_k(strtok_t *tstate){
    if(!tstate || !tstate->dup || !tstate->delims){return NULL;}

    char *s = tstate->tok;
    if (!s){s = tstate->dup;}

    /* Skip leading delimiters */
    s += __strspn(s, tstate->delims);
    if(*s == '\0'){
        tstate->tok = NULL;
        return NULL;
    }
    /* Token begins here */
    char *start = s;
    /* Find end of token */
    s = start + __strspn(start, tstate->delims);
    if(*s != '\0'){
        char sdel = *s;
        char edel = *s;
        /* === FLAG: ForceSameBorderingDelims === */
        if(tstate->flags & strtok__ForceSameBorderingDelims){
            /* Look ahead to find the next delimiter */
            char *next = s + 1;
            next += __strspn(next, tstate->delims);
            if(*next != '\0'){
                char nextdel = *next;
                /* If the next delimiter differs, skip this token */
                if(nextdel != sdel){
                    tstate->tok = next + 1;
                    return strtok_k(tstate);
                }
            }
        }

        /* === FLAG: ForceDifferentBorderingDelims === */
        if(tstate->flags & strtok__ForceDifferentBorderingDelims){
            if(sdel == edel){
                /* Skip this token */
                tstate->tok = s + 1;
                return strtok_k(tstate);
            }
        }

        /* === FLAG: ForceStartingDelim (high word OR) === */
        if (tstate->flags & strtok__ForceStartingDelim){sdel |= (char)(strtok__ForceStartingDelim >> 24);}

        /* === FLAG: ForceEndingDelim (low word OR) === */
        if (tstate->flags & strtok__ForceEndingDelim){edel |= (char)(strtok__ForceEndingDelim & 0xFF);}

        /* Store delimiters */
        tstate->sdelim = sdel;
        tstate->edelim = edel;

        /* Null‑terminate token */
        *s = '\0';
        tstate->tok = s + 1;
    }else{
        /* End of string */
        tstate->sdelim = '\0';
        tstate->edelim = '\0';
        tstate->tok = NULL;
    }

    return start;
}


void strtok_d(strtok_t *tstate){
	FreePool(tstate->dup);
	FreePool(tstate->delims);
	FreePool(tstate->tok);
	FreePool(tstate);
}

EFI_STATUS trng__(void *buffer, UINTN size){
    EFI_STATUS Status;
    EFI_RNG_PROTOCOL *Rng;
    EFI_GUID guid__ = EFI_RNG_PROTOCOL_GUID;
    if(EFI_ERROR(ST->BootServices->LocateProtocol(&guid__, NULL, (void **)&Rng))){return ((UINT64)-1);}

    // Ask firmware for random bytes (any algorithm)
    return Rng->GetRNG(Rng, NULL, size, buffer);
}

char tolower(char upper){return upper - abs('A' - 'a');}
BOOLEAN isdigit(char c){return (c >= '0') && (c <= '9');}
char *str_tolower(char *s){
    for(UINT32 cc = 0; cc < __strlen(s); ++cc){if(!isdigit(s[cc])){s[cc] = tolower(s[cc]);}}
    return s;
}

// Recursive match function
BOOLEAN match_rec(const char *p, const char *s){
    if(!*p){return !*s;}
    if(*p == '*' && *(p+1) == '*' && *(p+2) == '*'){
        // Block wildcard ***
        p += 3;
        if(!*p){
            // No proceeding char, match to end
            return TRUE;
        }
        char proceed = *p;
        // Skip in s until find proceed
        while(*s){
            if(*s == proceed){if(match_rec(p, s)){return TRUE;}}
            s++;
        }
        return FALSE;
    }else if(*p == '*'){
        // Single char wildcard
        p++;
        if(!*s){return FALSE;}
        return match_rec(p, s+1);
    }else{
        if(*p != *s){return FALSE;}
        return match_rec(p+1, s+1);
    }
}

BOOLEAN __pattmatch(const char *pattern, const char *str){
    // Preprocess pattern to handle escapes
    UINT64 len = __strlen(pattern);
    char *proc = AllocatePool(len + 1);
    UINT64 i = 0, j = 0;
    while(pattern[i]){
        if(pattern[i] == '\\' && pattern[i+1]){
            proc[j++] = pattern[i+1];
            i += 2;
        }else{proc[j++] = pattern[i++];}
    }
    proc[j] = 0;

    BOOLEAN result = match_rec(proc, str);
    FreePool(proc);
    return result;
}

void *__realloc(void *memory, UINT64 currSize, UINT64 nSize){
    void *out = AllocatePool(nSize);
    if(out){
        __memcpy(out, memory, (nSize > currSize? nSize: (currSize - nSize)));
        FreePool(memory);
    }
    return out;
}

void  *__calloc(UINT64 nLen, UINT64 nSize){
    void *out = AllocatePool(nLen * nSize);
    __memset(out, 0, nSize * nLen);
    return out;
}

void __memset(void *dst, UINT8 val, UINT64 len){
    while(len){((UINT8 *)dst)[len - 1] = val;    len--;}
}

void __memcpy(void *dst, void *src, UINT64 len){
    while(len){
        ((UINT8 *)dst)[len - 1] = ((UINT8 *)src)[len - 1];
        len--;
    }
}

UINT64 __strlen(char *s){
    UINT64 out = 0;
    while(s[out]){out++;}
    return out;
}

char *__strdup(char *s){
    char *out = AllocatePool(__strlen(s));
    __memcpy(out, s, __strlen(s));
    return out;
}

UINT64 __strspn(const char *s, const char *reject){
    const char *p;
    const char *r;
    for(p = s; *p; p++){
        for(r = reject; *r; r++){
            if(*p == *r){return (UINT64)(p - s);}
        }
    }
    return (UINT64)(p - s);
}

UINT64 __memcmp(void *a, void *b, UINT64 len){
    while(len){if(((UINT64 *)a)[len - 1] != ((UINT64 *)b)[len - 1]){break;} len--;}
    return len;
}

// EFI_STATUS getDriveMediaID(EFI_HANDLE Image, UINT32 *MediaID){
//     EFI_LOADED_IMAGE_PROTOCOL *lImage = NULL;
// 	EFI_GUID LoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
// 	if(!EFI_ERROR(BS->HandleProtocol(Image, &LoadedImageProtocolGuid, (void**)&lImage))){
// 		EFI_BLOCK_IO_PROTOCOL *Blk;
// 		EFI_GUID BlockIoGuid = EFI_BLOCK_IO_PROTOCOL_GUID;
// 		if(!EFI_ERROR(BS->HandleProtocol(lImage->DeviceHandle, &BlockIoGuid, (void **)&Blk))){
//             return Blk->Media->MediaId;
// 		}
// 	}
// #ifdef __DEBUG__
//             Print(L"Found no MediaID");
// #endif
//     return EFI_ERROR_MASK;
// }
EFI_STATUS getDriveMediaID(EFI_HANDLE Image, UINT32 *MediaID){
    EFI_STATUS Status;
    EFI_LOADED_IMAGE_PROTOCOL *lImage = NULL;
    EFI_GUID LoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    Status = BS->HandleProtocol(Image, &LoadedImageProtocolGuid, (void**)&lImage);
    if(EFI_ERROR(Status)){
#ifdef __DEBUG__
        Print(L"\nHandleProtocol(LoadedImage) failed: %r\n", Status);
#endif
        return Status;
    }
    EFI_BLOCK_IO_PROTOCOL *Blk;
    EFI_GUID BlockIoGuid = EFI_BLOCK_IO_PROTOCOL_GUID;
    Status = BS->HandleProtocol(lImage->DeviceHandle, &BlockIoGuid, (void **)&Blk);
    if(EFI_ERROR(Status)){
#ifdef __DEBUG__
        Print(L"\nHandleProtocol(BlockIo) failed: %r\n", Status);
#endif
        return Status;
    }
    *MediaID = Blk->Media->MediaId;
    return EFI_SUCCESS;
}

EFI_STATUS ValidateImageHandle(EFI_HANDLE Image){
    EFI_LOADED_IMAGE_PROTOCOL *Loaded = NULL;
    EFI_GUID Guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;

    return BS->HandleProtocol(Image, &Guid, (void**)&Loaded);
}