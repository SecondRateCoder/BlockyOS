#include "util.h"
#include "inc/Struct/Struct32.h"

keyptr *stateptrs = NULL;
uint8_t numstate = 4;

bool cmdCreate(opbuffer *buffer){
	uint32_t currState;
	bool force = false;
	char *file = NULL, *param = NULL, *statename = NULL;
	
	for(uint8_t buffcc = 0; buffcc < buffer->len; ++buffcc){
		if(!strcmp(buffer->opbuffer[buffcc].opt, "-state")){
			for(uint32_t statecc = 0; statecc < numstate; ++statecc){
				if(!strcmp(buffer->opbuffer[buffcc].optype.value, stateptrs[statecc].keyptr)){
					currState = statecc;
					statename = buffer->opbuffer[buffcc].optype.value;
					break;
				}
			}
		}else if(!strcmp(buffer->opbuffer[buffcc].opt, "-force")){
			force = true;
		}else if(!strcmp(buffer->opbuffer[buffcc].opt, "-file") && param == NULL){
			file = buffer->opbuffer[buffcc].optype.value;
		}else if(!strcmp(buffer->opbuffer[buffcc].opt, "-param") && file == NULL){
			param = buffer->opbuffer[buffcc].optype.value;
		}

		switch(currState){
			
		}
	}
}

uint32_t FhashCrunch(void *buffer, size_t bufflen, uint8_t collapse){
	sha256_context ctx;
	uint8_t bf[SHA256_SIZE_BYTES];
	sha256_init(&ctx);
	sha256_hash(&ctx, buffer, bufflen);
	sha256_done(&ctx, bf);
	return hashCrunch(bf, collapse);
}

uint32_t hashCrunch(uint8_t buffer[SHA256_SIZE_BYTES], uint8_t collapse){
	uint32_t out = 0;
	for(uint8_t cc = 0; cc < SHA256_SIZE_BYTES; ++cc){
		uint8_t item = buffer[cc] ^ (collapse & buffer[SHA256_SIZE_BYTES - 1]);
		if(item > collapse){out |= (1 << item);}
	}
	return out;
}

bool strcheck(char *in, char c){
	for(size_t cc = 0; cc < strlen(in); ++cc){
		if(in[cc] == c){return true;}
	}
	return false;
}

char strchecks(char *in, char *tokens){
	for(size_t cc = 0; cc < strlen(in); ++cc){
		for(uint8_t cc_ = 0; cc_ < strlen(tokens); ++cc_){
			if(strcheck(in + cc, tokens[cc_])){
				return tokens[cc_];
			}
		}
	}
	return 0;
}
char *strcheckp(char *in, char token){
	for(size_t cc = 0; cc < strlen(in); ++cc){
		char *ptr = 0;
		if(in[cc] == token){
			char *next = strcheckp(in + cc, token);
			char *out = malloc(next - (in + cc - 1));
			memcpy(out, (in + cc - 1), next - (in + cc - 1));
			return out;
		}
	}
	return in + strlen(in) - 1;
}
char *strchecksp(char *in, char *tokens, char *tokout){
	for(size_t cc = 0; cc < strlen(in); ++cc){
		for(uint8_t cc_ = 0; cc_ < strlen(tokens); ++cc_){
			char *ptr = 0;
			if(ptr = strcheckp(in + cc, tokens[cc_])){
				if(tokout){*tokout = tokens[cc_];}
				char *next = strcheckp(in + cc, tokens[cc_]);
				char *out = malloc(next - ptr);
				memcpy(out, ptr, next - ptr);
				return out;
			}
		}
	}
	return NULL;
}

void *memdup(void *src, uint32_t len){
	void *out = malloc(len);
	if(out){
		memcpy(out, src, len);
		return out;
	}
	return NULL;
}

uint32_t dbptr_memsize(char **ptr, uint32_t item, uint32_t stride){
	uint32_t out = 0;
	char **cmp = ptr + item;
	// Iterate till item is equal to 1st item in next index
	while(cmp[0][out * stride] != cmp[1][0]){out++;}
	return out;
}

bool memsearch(void *in, uint32_t inlen, void *cmp, uint32_t cmplen, uint32_t stride){
	for(uint32_t cc = stride; cc < inlen; cc += (cmplen + stride)){
		if(cmplen + cc > inlen){return false;}
		if(!memcmp(in + cc, cmp, cmplen)){return true;}
	}
	return false;
}

bool Virt_active;
#ifdef _WIN32
#include <windows.h>
#include <consoleapi.h>
void enable_ansi(void){
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
	Virt_active = true;
}
#else
	void enable_ansi(){return;}
#endif