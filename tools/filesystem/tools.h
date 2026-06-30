#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <inttypes.h>
#include "ref/blake2.h"

typedef size_t _GUID[2];

#define dualprintf(streamA, streamB, ...)	fprintf(streamA, __VA_ARGS__);	fprintf(streamB, __VA_ARGS__)
extern volatile FILE *logf;
#define LOGFOFFSET		"tools\\filesystem\\fsshell.log"

#if defined(_WIN32) || defined(_WIN64)
    #include <direct.h>
    #define getcwd _getcwd
#else
    #include <unistd.h>
    #define GetCurrentDir getcwd
#endif

#define GPTeNAMESIZE (72)
#define GPTeNAMELEN (GPTeNAMESIZE / sizeof(uint16_t))
typedef uint16_t GPTeNSTR[GPTeNAMELEN];
GPTeNSTR *makeGPTeNSTR(char *str);

#define SAFEOP(A, B, CompAOp, CompA, CompBOp, CompB, Comp, OP, Alt)    (((A CompAOp CompA) Comp (B CompBOp CompB))? (A OP B): Alt)

#define __safediv(A, B)     SAFEOP((A), (B), ||, true, !=, 0, &&, /, 1)

#define PATHSEP '/'
#define PATHnoSEP '\\'
#define CMDiMAX 256

#define enumdef(name, type)     typedef type name;  enum
#define bufdef(name, ptr, scale)		typedef struct name{ptr *name;		scale len;}name;

#define flagcheck(v, f) ((v & f) == f)
#define flagset(v, f)   (v |= f)
#define flagunset(v, f) (v &= ~f)

enumdef(strtokflags, uint32_t){
	strtok__ForceSameBorderingDelims = 0x1,
	strtok__ForceDifferentBorderingDelims,
	strtok__ForceStartingDelim = 0x80000000,
	strtok__ForceEndingDelim = 0x00002000
};

typedef struct strtok_t{
	/// @brief A duplicate of the Token.
	char *dup;
	/// @brief The recovered Token.
	char *tok;
	/// @brief The configured delims;
	char *delims;
	// The encountered delims
	char sdelim, edelim;
	uint32_t flags;
}strtok_t;

bool strcheck(char *s, char c);
ssize_t strchecki(char *s, char c);
void *memdup(void *mem, size_t s);
size_t __getfcode(char *s_);
char *readbuf(size_t s, const char *prefix);

strtok_t *strtok_i(char *in, char *delims, uint32_t enables);
char *strtok_ff(strtok_t *tstate);
char *strtok_k(strtok_t *tstate);
void strtok_d(strtok_t *tstate);

int trng__(void *buffer, size_t len);

char *str_tolower(char *s);

bool __pattmatch(const char *pattern, const char *str);
char *str_replace(const char *str, const char *old, const char *newstr);