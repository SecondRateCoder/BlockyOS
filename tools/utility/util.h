#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "inc/SHA256/sha256.h"

#define PACKEDSTRUCT __attribute__((packed))

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_LBLUE "\x1b[94m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define NANSIRED(TEXT)		"\n" ANSI_COLOR_RED TEXT ANSI_COLOR_RESET
#define NANSIGREEN(TEXT)	"\n" ANSI_COLOR_GREEN TEXT ANSI_COLOR_RESET
#define NANSIYELLOW(TEXT)	"\n" ANSI_COLOR_YELLOW TEXT ANSI_COLOR_RESET
#define NANSIBLUE(TEXT)		"\n" ANSI_COLOR_BLUE TEXT ANSI_COLOR_RESET
#define NANSIMAGENTA(TEXT)	"\n" ANSI_COLOR_MAGENTA TEXT ANSI_COLOR_RESET
#define NANSICYAN(TEXT)		"\n" NANSI_COLOR_CYAN TEXT ANSI_COLOR_RESET
#define NANSILBLUE(TEXT)	"\n" ANSI_COLOR_LBLUE TEXT ANSI_COLOR_RESET

#define ANSIRED(TEXT)		ANSI_COLOR_RED TEXT ANSI_COLOR_RESET
#define ANSIGREEN(TEXT)		ANSI_COLOR_GREEN TEXT ANSI_COLOR_RESET
#define ANSIYELLOW(TEXT)	ANSI_COLOR_YELLOW TEXT ANSI_COLOR_RESET
#define ANSIBLUE(TEXT)		ANSI_COLOR_BLUE TEXT ANSI_COLOR_RESET
#define ANSIMAGENTA(TEXT)	ANSI_COLOR_MAGENTA TEXT ANSI_COLOR_RESET
#define ANSICYAN(TEXT)		ANSI_COLOR_CYAN TEXT ANSI_COLOR_RESET
#define ANSILBLUE(TEXT)		ANSI_COLOR_LBLUE TEXT ANSI_COLOR_RESET

#define NULLSTR(s) (char[s]){0}
#define defenum(type, name) typedef type name; enum
#define optypedef(type, exclude, input) (optype_t){.value = NULL, .optexclude = exclude, .optype = (type), .hasinput = ((input)? true: false)}
#define cmddef(cmd_, alias_, optlen_, opt_, desc_, proc_) {\
	.cmd = cmd_, \
	.alias = alias_, \
	.optionsbf = (opbuffer){optlen_, opt_}, \
	.cmddesc = desc_, \
	.func = proc_\
}

#define BUFFERPRINT_MAXX 10
#define FACTORCHECK(n, f) ((n % f) == 0)
#define bufdef(ptr, name, scale)typedef struct name{scale len;		ptr *name;}name;
#define PROXYFLAGSET(n, f) ((n) | (f))
#define FLAGSET(n, f) ((n) = PROXYFLAGSET(n, f))
#define PROXYFLAGUNSET(n, f) ((n) & ~(f))
#define FLAGUNSET(n, f) (n = PROXYFLAGUNSET(n, f))
#define FLAGCHECK(n, f) (((n) & (f)) == (f))

defenum(uint32_t, optype){
	optypeMULTIPLE = 1,
	optypeSWITCH,
	optypeFPATH,
	optypeSTR,
	optypeDIGIT,
	// Give super-number to prevent collisions
	optypeREQUIRED = 0xA0000000,
};

typedef struct optype_t{
	void *value;
	char optexclude[8];
	optype optype;
	bool hasinput;
}optype_t;

typedef struct option_t{
	char opt[8];
	optype_t optype;
	bool enabled;
	char *opdesc;
}option_t;

bufdef(void, optypemulbufferitem, uint32_t);
bufdef(optypemulbufferitem, optypemulbuffer, uint8_t);
bufdef(option_t, opbuffer, uint32_t);
defenum(uint16_t, STATE){
	S_ds_vfs = 1,
	S_ds_true,
	S_gpt,
	S_partition
};
bufdef(char, keyptr, STATE)

typedef bool (*cmdfunc)(opbuffer *buffer);

typedef struct command_t{
	char cmd[4];
	char *alias;
	union{
		opbuffer optionsbf;
		struct{
			uint32_t len;
			option_t *options;
		};
	};
	char *cmddesc;
	cmdfunc func;
}command_t;

char *strcheckp(char *in, char token);
char *strchecksp(char *in, char *tokens, char *tokout);

bool strcheck(char *in, char c);
char strchecks(char *in, char *tokens);
bool cmdstandardFunc(opbuffer *buffer);
uint32_t dbptr_memsize(char **ptr, uint32_t item, uint32_t stride);
void *memdup(void *src, uint32_t len);
bool memsearch(void *in, uint32_t inlen, void *cmp, uint32_t cmplen, uint32_t stride);
char *optype2str(optype_t t);
void *paramparse(optype optype, void *in);
bool process(command_t *cmd);
command_t *consume(char **arg, uint32_t argl, uint32_t *argcc);