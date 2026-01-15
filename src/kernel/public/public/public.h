#pragma once

#include "../public/math/int/_int.h"
#include "../public/math/int/_bool.h"
#include "../public/memory/memory.h"
#include "../public/memory/string.h"

#ifndef PUBLIC_H
#define PUBLIC_H
#define NULL (void *)0
#define min(A, B) A < B? A: B
#define max(A, B) A > B? A: B
#endif

size_t decode64(const uint8_t *array);
int decode32(const uint8_t *array);
uint32_t decode_32u(const uint8_t *array);
size_t clamp_sizet(size_t lower, size_t upper, size_t value);
void encode64(uint8_t *array, size_t value);
void encode32(uint8_t *array, int value);
void encode32u(uint8_t *array, const uint32_t value);
size_t bit_max(uint8_t num_bits);
