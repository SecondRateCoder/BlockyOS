#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define enumdef(name, type)     typedef type name;  enum

#define flagcheck(v, f) ((v & f) == f)
#define flagset(v, f)   (v |= f)
#define flagunset(v, f) (v &= ~f)

bool strcheck(char *s, char c);
ssize_t strchecki(char *s, char c);
void *memdup(void *mem, size_t s);