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