#include "tools.h"

bool strcheck(char *s, char c){
	for(size_t cc = 0; cc < strlen(s); ++cc){
		if(c == s[cc]){return true;}
	}
	return false;
}

ssize_t strchecki(char *s, char c){
	for(size_t cc = 0; cc < strlen(s); ++cc){
		if(c == s[cc]){return cc;}
	}
	return -1;
}

void *memdup(void *mem, size_t s){
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
	return hash;
}

char *readbuf(size_t s){
	char *out = calloc(s + 1, sizeof(char));
	if(out){
		size_t i = 0;
		for(; i < s; ++i){out[i] = getc(stdin);		if(out[i] == '\n' || out[i] == '\r'){break;}}
		out[i - 1] = '\0';
	}
	return out;
}

strtok_t *strtok_i(char *in, char *delims){
	strtok_t *out = calloc(1, sizeof(strtok_t));
	out->dup = strdup(in);
	out->delims = strdup(delims);
	return out;
}

char *strtok_k(strtok_t *tstate){
    if (!tstate || !tstate->dup || !tstate->delims){return NULL;}

    char *s = tstate->tok;     // Current scan position
    if(!s){s = tstate->dup;}

    // Skip leading delimiters
    s += strspn(s, tstate->delims);
    if(*s == '\0'){
        tstate->tok = NULL;
        return NULL;
    }

    // Token begins here
    char *start = s;

    // Find end of token
    s = start + strcspn(start, tstate->delims);

    if(*s != '\0'){
        // Record delimiters encountered
        tstate->sdelim = *s;
        tstate->edelim = *s;

        *s = '\0';        // Null‑terminate token
        tstate->tok = s + 1;   // Next scan position
    }else{
        // End of string reached
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