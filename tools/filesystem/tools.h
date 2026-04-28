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

#define PATHSEP '/'
#define PATHnoSEP '\\'
#define CMDiMAX 256

#define enumdef(name, type)     typedef type name;  enum

#define flagcheck(v, f) ((v & f) == f)
#define flagset(v, f)   (v |= f)
#define flagunset(v, f) (v &= ~f)

typedef struct strtok_t{
    /// @brief A duplicate of the Token.
    char *dup;
    /// @brief The recovered Token.
    char *tok;
    /// @brief The configured delims;
    char *delims;
    // The encountered delims
    char sdelim, edelim;
}strtok_t;

bool strcheck(char *s, char c);
ssize_t strchecki(char *s, char c);
void *memdup(void *mem, size_t s);
size_t __getfcode(char *s_);
char *readbuf(size_t s);

strtok_t *strtok_i(char *in, char *delims);
char *strtok_k(strtok_t *tstate);
void strtok_d(strtok_t *tstate);

typedef struct cmddesc{
	char *cmd;
	char *alias;
	char *desc;
	struct flags{
		char **flags_;
		uint8_t numflags;
	}flags;
}cmddesc;

// cmddesc commands[] = {
// 	{"mkfile", ";m", "Create a File or Directory", {{"d", "f", "r"}, 3}},
// 	{"create", ";c", "List all Help Info all help or help on a Specific Function", {{"d", "f", "r"}, 3}}
// };