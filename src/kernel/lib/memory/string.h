#pragma once

#include "memory.h"

#define strncmp(a, b, len) memcmp((a), (b), (len))
#define strcmp(a, b) memcmp((a), (b), (strlen((a)) < strlen((b))? strlen((a)): strlen((b))))

#define strncpy(dest, src, len) memcpy((dest), (src), (len))
#define strcpy(dest, src) memcpy((dest), (src), (strlen((dest)) < strlen((src))? strlen((dest)): strlen((src))))

#define strnclr(str, len) memset((str), (len), 0)
#define strclr(str) memset((str), strlen(str), 0)

#define strcheck(str, c) memcheckb((str), strlen(str), (c))

#define tobit(c) ((c) - '0')

typedef char *string;

#define NULLSTR {0}

bool isdigit(char c);
size_t strlen(char *str);
bool strchecks(char *str, char *snippet);
size_t strcmpc(char *a, char *b);

int8_t tobitcomplex(char c);
int8_t tobyte(char *str);
int16_t toword(char *str);
signed long tolong(char *str);
ssize_t tolonglong(char *str);