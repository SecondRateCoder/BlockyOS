#pragma once

#include "./kernel/lib32/public/public/math/int/_int.h"
#include "./kernel/lib32/public/public/math/int/_bool.h"
#include "./kernel/lib32/public/public/memory/memory.h"
#include "./kernel/lib32/public/public/memory/string.h"

#define ASMCALL __attribute__((cdecl))
#define LINKERSECTION(SECTION) __attribute__((section(SECTION)))

#define LOW64(_64) ((_64) & 0xFFFFFFFF)
#define LOW32(_32) ((_32) & 0xFFFF)
#define LOW16(_16) ((_16) & 0x00FF)

#define NULL (void *)0
#define min(A, B) (A) < (B)? (A): (B)
#define max(A, B) (A) > (B)? (A): (B)

size_t decode64(const uint8_t *array);
int decode32(const uint8_t *array);
uint32_t decode_32u(const uint8_t *array);
size_t clamp_sizet(size_t lower, size_t upper, size_t value);
void encode64(uint8_t *array, size_t value);
void encode32(uint8_t *array, int value);
void encode32u(uint8_t *array, const uint32_t value);
size_t bit_max(uint8_t num_bits);
