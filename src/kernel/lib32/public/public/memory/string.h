#pragma once

#include "memory.h"

#define strncmp(a, b, len) memcmp(a, b, len)
#define strcmp(a, b) memcmp(a, b, (strlen(a) < strlen(b)? strlen(a): strlen(b)))
#define strncpy(dest, src, len) memcpy(dest, src, len)
#define strnclr(str, len) memset(str, len, '')

typedef char *string;

size_t strlen(char *str);
bool strcheck(char *str, char c);
