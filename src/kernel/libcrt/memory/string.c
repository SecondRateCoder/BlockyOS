#include "string.h"
#include "allocator/malloc.h"

size_t strlen(const char *str){
	size_t len = 0;
	while(*str++){len++;}
	return len;
}

bool strchecks(const char *str, const char *snippet){
	const size_t str_len = strlen(str), 
			snip_len = strlen(snippet);
	if(snip_len > str_len || snip_len == 0){return false;}
	
	size_t temp = str_len - snip_len;
	for(size_t cc = 0; cc <= temp; ++cc) {
		if(memcmp(str + cc, snippet, snip_len) == 0){return true;}
	}
	return false;
}

int strcmpc(const char *a, const char *b){
	while(*a && (*a == *b)){
		a++;
		b++;
	}
	return *(const unsigned char*)a - *(const unsigned char*)b;
}
bool isdigit(char c) {return (c >= '0') && (c <= '9');}
int8_t tobitcomplex(char c){
	if(isdigit(c)){return c - '0';}
	return -1;
}

int8_t tobyte(const char *str){
	int8_t out = 0;
	while(isdigit(*str)){
		out = (out * 10) + (*str - '0');
		str++;
	}
	return out;
}

int16_t toword(const char *str){
	bool negative = false;
	int16_t out = 0;
	if(*str == '-'){
		negative = true;
		str++;
	}
	while(isdigit(*str)){
		out = (out * 10) + (*str - '0');
		str++;
	}
	return negative ? -out : out;
}

int32_t tolong(const char *str){
	bool negative = false;
	long out = 0;
	if(*str == '-'){
		negative = true;
		str++;
	}
	while(isdigit(*str)){
		out = (out * 10) + (*str - '0');
		str++;
	}
	return negative ? -out : out;
}

ssize_t tolonglong(const char *str){
	bool negative = false;
	ssize_t out = 0;
	if(*str == '-'){
		negative = true;
		str++;
	}
	while(isdigit(*str)){
		out = (out * 10) + (*str - '0');
		str++;
	}
	return negative ? -out : out;
}

static bool match_rec(const char *p, const char *s){
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

bool pattmatch(const char *pattern, const char *str){
    // Preprocess pattern to handle escapes
    uint64_t len = strlen(pattern);
    char *proc = mcalloc(1, len + 1);
    uint64_t i = 0, j = 0;
    while(pattern[i]){
        if(pattern[i] == '\\' && pattern[i+1]){
            proc[j++] = pattern[i+1];
            i += 2;
        }else{proc[j++] = pattern[i++];}
    }
    proc[j] = 0;

    bool result = match_rec(proc, str);
    mfree(proc);
    return result;
}