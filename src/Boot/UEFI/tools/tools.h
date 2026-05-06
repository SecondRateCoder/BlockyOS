#pragma once

// #include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "efi.h"
#include "efilib.h"

#include "src/Boot/UEFI/drivers/crypto/blake2/ref/blake2.h"

#define __min(a, b) ((a) > (b)? (b): (a))
#define __max(a, b) ((a) < (b)? (b): (a))
#define abs(n) ((n) < 0? -(n): (n))

#define PATHSEP '/'
#define PATHnoSEP '\\'
#define CMDiMAX 256

#define enumdef(name, type)     typedef type name;  enum

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
	UINT32 flags;
}strtok_t;

BOOLEAN isascii(char c);
BOOLEAN isdigit(char c);

BOOLEAN strcheck(char *s, char c);
INT64 strchecki(char *s, char c);
void *memdup(void *mem, UINT64 s);
UINT64 __getfcode(char *s_);
char *readbuf(size_t s, CHAR16 *prefix);
EFI_STATUS getDriveMediaID(EFI_HANDLE Image, UINT32 *MediaID);
EFI_STATUS ValidateImageHandle(EFI_HANDLE Image);

strtok_t *strtok_i(char *in, char *delims, UINT32 enables);
char *strtok_k(strtok_t *tstate);
void strtok_d(strtok_t *tstate);

EFI_STATUS trng__(void *buffer, UINTN size);

char tolower(char upper);
char *str_tolower(char *s);

BOOLEAN __pattmatch(const char *pattern, const char *str);

UINT64 __strlen(char *s);
char *__strdup(char *s);
UINT64 __strspn(const char *s, const char *reject);

void __memset(void *dst, UINT8 val, UINT64 len);
void __memcpy(void *dst, void *src, UINT64 len);
UINT64 __memcmp(void *a, void *b, UINT64 len);

void  *__calloc(UINT64 nLen, UINT64 nSize);
void *__realloc(void *memory, UINT64 currSize, UINT64 nSize);