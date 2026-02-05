#pragma once

#include "./kernel/lib32/stdmath/math.h"
#include "./kernel/lib32/stdmemory/memory.h"
#include "./kernel/lib32/stdmemory/string.h"
#include "./kernel/import/SHA256/sha256.h"

#define RAMH_TYPE uint32_t
#define aliasLen 10
#define PATHMAX 256

#define PACKEDSTRUCT __attribute__((packed))
#define ASMCALL __attribute__((cdecl))
#define LINKERSECTION(SECTION) __attribute__((section(SECTION)))
#define INTERRUPTCALL __attribute__((interrupt))
#define STACKLESSCALL __attribute__((naked))

#define FLAGSET(INT, FLAG) (INT |= FLAG)
#define FLAGUNSET(INT, FLAG) (INT |= ~FLAG)
#define FLAGTOGGLE(INT, FLAG) (INT ^ FLAG)
#define FLAGCHECK(INT, FLAG) ((INT & FLAG) == FLAG)

typedef uint8_t SHA256HASH[SHA256_SIZE_BYTES];

#define LOW64(_64) ((_64) & 0xFFFFFFFF)
#define LOW32(_32) ((_32) & 0xFFFF)
#define LOW16(_16) ((_16) & 0x00FF)

#define NULL (void *)0
#define min(A, B) (A) < (B)? (A): (B)
#define max(A, B) (A) > (B)? (A): (B)

typedef struct EnviromentVar{
    char alias[aliasLen];
    char value[PATHMAX];
}EnviromentVar;

size_t decode64(const uint8_t *array);
int decode32(const uint8_t *array);
uint32_t decode_32u(const uint8_t *array);
size_t clamp_sizet(size_t lower, size_t upper, size_t value);
void encode64(uint8_t *array, size_t value);
void encode32(uint8_t *array, int value);
void encode32u(uint8_t *array, const uint32_t value);
size_t bit_max(uint8_t num_bits);
