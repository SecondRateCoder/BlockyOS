#pragma once

#include "memory.h"

#define strncmp(a, b, len) memcmp((a), (b), (len))
#define strcmp(a, b) strlen(a) != strlen(b)? false: memcmp((a), (b), (strlen(a) < strlen((b))? strlen(a): strlen(b)))

#define strncpy(dest, src, len) memcpy((dest), (src), (len))
#define strcpy(dest, src) memcpy((dest), (src), (strlen((dest)) < strlen((src))? strlen((dest)): strlen((src))))

#define strnclr(str, len) memset((str), 0, (len))
#define strclr(str) memset((str), 0, strlen(str))

#define strcheck(str, c) memcheckb((str), strlen(str), (c))

#define tobit(c) ((c) - '0')

typedef char *string;
typedef uint16_t char16_t;
typedef uint32_t char32_t;

#define NULLSTR {0}

LibAPI bool isdigit(char c);
LibAPI size_t strlen(const char *str);
LibAPI bool strchecks(const char *str, const char *snippet);
LibAPI int strcmpc(const char *a, const char *b);

LibAPI int8_t tobitcomplex(char c);
LibAPI int8_t tobyte(const char *str);
LibAPI int16_t toword(const char *str);
LibAPI int32_t tolong(const char *str);
LibAPI ssize_t tolonglong(const char *str);

LibAPI bool pattmatch(const char *pattern, const char *str);