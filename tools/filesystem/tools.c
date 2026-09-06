#include "tools.h"

FILE *fs_logf;

bool strcheck(char *s, char c){
	for(uint64_t cc = 0; cc < strlen(s); ++cc){
		if(s[cc] == c){return true;}
	}
	return false;
}

ssize_t strchecki(char *s, char c){
	for(uint64_t cc = 0; cc < strlen(s); ++cc){
		if(c == s[cc]){return cc;}
	}
	return -1;
}

void *memdup(void *mem, uint64_t s){
	void *out = malloc(s);
	memcpy(out, mem, s);
	return out;
}

uint64_t *__getfcode(char *s_){
	static uint64_t hash[2];
	char *s = strdup(s_);
	for(uint32_t cc = 0; cc < strlen(s_); ++cc){
		if(s[cc] == PATHnoSEP){s[cc] = PATHSEP;}
	}
	blake2b_state hashstate;
	blake2b_init(&hashstate, sizeof(uint64_t) * 2);
	blake2b_update(&hashstate, s, strlen(s));
	blake2b_final(&hashstate, &hash, sizeof(uint64_t) * 2);
	hash[1] &= (UINT64_MAX & ~(UINT16_MAX << 48));
	free(s);
	return hash;
}

char *readbuf(uint64_t s, const char *prefix){
    dualprintf(fs_logf, stdout, "%s", prefix);
    fflush(stdout);
    if(s > 0){
        char *out = calloc(s + 1, sizeof(char));
        if(!out){return NULL;}
        uint64_t i = 0;
        int c;
        while(i < s && (c = getc(stdin)) != EOF && c != '\n' && c != '\r'){
            out[i++] = (char)c;
        }
        if(c == EOF && i == 0){
            free(out);
            return NULL;    // EOF → signal caller to exit
        }
        out[i] = '\0';
        return out;
    }

    // Dynamic read
    uint64_t cap = 32;
    uint64_t len = 0;
    char *out = malloc(cap);
    if(!out){return NULL;}
    int c;
    while((c = getc(stdin)) != EOF && c != '\n' && c != '\r'){
        if(len + 1 >= cap){
            cap *= 2;
            char *tmp = realloc(out, cap);
            if(!tmp){
                free(out);
                return NULL;
            }
            out = tmp;
        }
        out[len++] = (char)c;
    }
    if(c == EOF && len == 0){
        free(out);
        return NULL;    // EOF → caller exits
    }
    out[len] = '\0';
    return out;
}

strtok_t *strtok_i(char *in, char *delims, uint32_t enables){
	if(
		(flagcheck(enables, strtok__ForceSameBorderingDelims) || flagcheck(enables, strtok__ForceDifferentBorderingDelims)) &&
		(flagcheck(enables, strtok__ForceEndingDelim) || flagcheck(enables, strtok__ForceStartingDelim))
	){return NULL;}
	if(flagcheck(enables, strtok__ForceSameBorderingDelims) && flagcheck(enables, strtok__ForceDifferentBorderingDelims)){return NULL;}
	strtok_t *out = calloc(1, sizeof(strtok_t));
	out->dupr = strdup(in);
	out->dup = out->dupr;
	out->delims = strdup(delims);
	out->sdelim = *in;
	out->edelim = in[strlen(in) - 1];
	out->flags = enables;
	return out;
}

char *__strspn(char *src, char *delims){
	while(*src && src[1]){
		if(strcheck(delims, *src)){return src;}
		src++;
	}
	return src;
}

char *__nstrspn(char *src, char *delims){
	while(*src && src[1]){
		if(!strcheck(delims, *src)){return src;}
		src++;
	}
	return src;
}

char *strtok_k(strtok_t *tstate){
	if(!tstate || !tstate->dup || !tstate->delims){return NULL;}

	char *s = tstate->tok;
	if(!tstate->edelim || !tstate->sdelim){return NULL;}
	if(!s){s = tstate->dup;}

	/* Skip leading delimiters */
	s = __nstrspn(s, tstate->delims);
	if(*s == '\0'){
		tstate->tok = NULL;
		return NULL;
	}
	char *start = s;

	/* Find end of token */
	s = (strcheck(tstate->delims, *start)? start: __strspn(start, tstate->delims));

	if(*s != '\0'){
		char sdel = *s; // Set to *s if is the start of the string
		char edel = 0;

		/* === FLAG: ForceSameBorderingDelims === */
		if(flagcheck(tstate->flags, strtok__ForceSameBorderingDelims)){
			/* Look ahead to find the next delimiter */
			char *next = __strspn(s, tstate->delims);

			if(*next != '\0'){
				edel = *next;

				/* If the next delimiter differs, skip this token */
				if(edel != sdel){
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
		if(tstate->flags & strtok__ForceStartingDelim)
			{sdel |= (char)(strtok__ForceStartingDelim >> 24);}

		/* === FLAG: ForceEndingDelim (low word OR) === */
		if(tstate->flags & strtok__ForceEndingDelim)
			{edel |= (char)(strtok__ForceEndingDelim & 0xFF);}

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

char *strtok_ff(strtok_t *tstate){
	if(!tstate || !tstate->dup){return NULL;}

	/* If no token has ever been parsed, the whole string is unparsed */
	if(!tstate->tok){return NULL;}

	/* If tok points to end of string, nothing left */
	if(*(tstate->tok) == '\0'){return NULL;}
	char *out = tstate->tok;
	return out;
}

void strtok_d(strtok_t *tstate){
	free(tstate->dupr);
	free(tstate->delims);
	free(tstate);
}

#ifdef __LINUX__ || __UNIX__

#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

int trng__(void *key, uint64_t keylen){
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0) return -1;
	ssize_t r = read(fd, key, keylen);
	close(fd);
	return (r == (ssize_t)keylen) ? 0 : -1;
}

#elifdef __WIN32__ || __WIN64__

#include <windows.h>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

int trng__(void *key, uint64_t keylen){
	NTSTATUS st = BCryptGenRandom(
		NULL,
		key,
		(ULONG)keylen,
		BCRYPT_USE_SYSTEM_PREFERRED_RNG
	);
	return st == 0 ? 0 : -1;
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
	uint64_t len = strlen(pattern);
	char *proc = malloc(len + 1);
	uint64_t i = 0, j = 0;
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

GPTeNSTR *makeGPTeNSTR(char *str){
	GPTeNSTR *out = calloc(sizeof(GPTeNSTR), 1);
	for(uint64_t cc = 0; cc < __min(strlen(str), GPTeNAMELEN); ++cc){
		(*out)[cc] = (uint16_t)(str[cc]);
	}
	return out;
}

char *str_replace(const char *str, const char *old, const char *newstr){
	if(!str || !old || !newstr){return NULL;}

	uint64_t len_str = strlen(str);
	uint64_t len_old = strlen(old);
	uint64_t len_new = strlen(newstr);

	if(len_old == 0){return NULL;} // avoid infinite loop

	// Count occurrences of old substring
	uint64_t count = 0;
	const char *p = str;
	while((p = strstr(p, old)) != NULL){
		count++;
		p += len_old;
	}

	// If no occurrences, return a duplicate of the original
	if(count == 0){return strdup(str);}

	// Allocate new buffer
	uint64_t new_len = len_str + count * (len_new - len_old);
	char *result = malloc(new_len + 1);
	if(!result){return NULL;}

	char *out = result;
	const char *in = str;

	// Perform replacements
	while((p = strstr(in, old)) != NULL){
		uint64_t prefix_len = p - in;
		memcpy(out, in, prefix_len);
		out += prefix_len;
		memcpy(out, newstr, len_new);
		out += len_new;
		in = p + len_old;
	}

	// Copy the remainder
	strcpy(out, in);
	return result;
}