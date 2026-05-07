#include "tools.h"

bool strcheck(char *s, char c){
	for(size_t cc = 0; cc < strlen(s); ++cc){
		if(s[cc] == c){return true;}
	}
	return false;
}

ssize_t strchecki(char *s, char c){
	for(size_t cc = 0; cc < strlen(s); ++cc){
		if(c == s[cc]){return cc;}
	}
	return -1;
}

void *__memdup(void *mem, size_t s){
	void *out = malloc(s);
	memcpy(out, mem, s);
	return out;
}

size_t __getfcode(char *s_){
	char *s = strdup(s_);
	for(uint32_t cc = 0; cc < strlen(s_); ++cc){
		if(s[cc] == PATHnoSEP){s[cc] = PATHSEP;}
	}
	size_t hash;
	blake2b_state hashstate;
	blake2b_init(&hashstate, 8);
	blake2b_update(&hashstate, s, strlen(s));
	blake2b_final(&hashstate, &hash, 8);
	hash &= 0x3FFFFFFFFFF;
    free(s);
	return hash;
}

char *readbuf(size_t s, char *prefix){
    printf(prefix);
	char *out = calloc(s + 1, sizeof(char));
	if(out){
		size_t i = 0;
		for(; i < s; ++i){out[i] = getc(stdin);		if(out[i] == '\n' || out[i] == '\r'){break;}}
		out[i] = '\0';
	}
	return out;
}

strtok_t *strtok_i(char *in, char *delims, uint32_t enables){
    if(
        (flagcheck(enables, strtok__ForceSameBorderingDelims) || flagcheck(enables, strtok__ForceDifferentBorderingDelims)) &&
        flagcheck(enables, strtok__ForceEndingDelim) || flagcheck(enables, strtok__ForceStartingDelim)
    ){return NULL;}
    if(flagcheck(enables, strtok__ForceSameBorderingDelims) && flagcheck(enables, strtok__ForceDifferentBorderingDelims)){return NULL;}
	strtok_t *out = calloc(1, sizeof(strtok_t));
	out->dup = strdup(in);
	out->delims = strdup(delims);
    out->flags = enables;
	return out;
}

char *strtok_k(strtok_t *tstate){
    if (!tstate || !tstate->dup || !tstate->delims){return NULL;}

    char *s = tstate->tok;
    if (!s){s = tstate->dup;}

    /* Skip leading delimiters */
    s += strspn(s, tstate->delims);
    if(*s == '\0'){
        tstate->tok = NULL;
        return NULL;
    }

    /* Token begins here */
    char *start = s;

    /* Find end of token */
    s = start + strcspn(start, tstate->delims);

    if(*s != '\0'){
        char sdel = *s;
        char edel = *s;

        /* === FLAG: ForceSameBorderingDelims === */
        if(tstate->flags & strtok__ForceSameBorderingDelims){
            /* Look ahead to find the next delimiter */
            char *next = s + 1;
            next += strcspn(next, tstate->delims);

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
        if (tstate->flags & strtok__ForceStartingDelim)
            sdel |= (char)(strtok__ForceStartingDelim >> 24);

        /* === FLAG: ForceEndingDelim (low word OR) === */
        if (tstate->flags & strtok__ForceEndingDelim)
            edel |= (char)(strtok__ForceEndingDelim & 0xFF);

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
	free(tstate->dup);
	free(tstate->delims);
	free(tstate->tok);
	free(tstate);
}

#ifdef __LINUX__ || __UNIX__

#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

int trng__(void *key, size_t keylen){
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) return -1;
    ssize_t r = read(fd, key, keylen);
    close(fd);
    return (r == (ssize_t)keylen) ? 0 : -1;
}

#elifdef __WIN32__ && __WIN64__

#include <windows.h>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

int trng__(void *key, size_t keylen){
    NTSTATUS gST = BCryptGenRandom(
        NULL,
        key,
        (ULONG)keylen,
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );
    return gST == 0 ? 0 : -1;
}

#endif

char *str_tolower(char *s){
    for(uint32_t cc = 0; cc < strlen(s); ++cc){if(!isdigit(s[cc])){s[cc] = tolower(s[cc]);}}
    return s;
}

// Recursive match function
bool match_rec(const char *p, const char *s){
    if(!*p){return !*s;}
    if(*p == '*' && *(p+1) == '*' && *(p+2) == '*'){
        // Block wildcard ***
        p += 3;
        if(!*p){
            // No proceeding char, match to end
            return true;
        }
        char proceed = *p;
        // Skip in s until find proceed
        while(*s){
            if(*s == proceed){if(match_rec(p, s)){return true;}}
            s++;
        }
        return false;
    }else if(*p == '*'){
        // Single char wildcard
        p++;
        if(!*s){return false;}
        return match_rec(p, s+1);
    }else{
        if(*p != *s){return false;}
        return match_rec(p+1, s+1);
    }
}

bool __pattmatch(const char *pattern, const char *str){
    // Preprocess pattern to handle escapes
    size_t len = strlen(pattern);
    char *proc = malloc(len + 1);
    size_t i = 0, j = 0;
    while(pattern[i]){
        if(pattern[i] == '\\' && pattern[i+1]){
            proc[j++] = pattern[i+1];
            i += 2;
        }else{proc[j++] = pattern[i++];}
    }
    proc[j] = 0;

    bool result = match_rec(proc, str);
    free(proc);
    return result;
}