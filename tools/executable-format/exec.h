#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include "ref/blake2.h"

#define __FINALSTAGE 10
#define __PATHlen 16

typedef union __SYMMAGIC{
	char MAGIC[8];
	size_t MAGICINT;
}__SYMMAGIC;

typedef struct __SYMREF{
	__SYMMAGIC signature;
	size_t byteOFFSET;
	uint8_t ptrSIZE;
}__SYMREF, __RELOCtable[];

typedef struct __EXECFSitem{
	char path[32];
	size_t firstBYTE, lastBYTE;
	uint16_t __attributes;
}__EXECFSitem;

