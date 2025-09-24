
#ifndef STDINT_H
#define STDINT_H
typedef char uint8_t;

typedef unsigned int uint32_t;
typedef uint32_t uint;

typedef unsigned long long size_t;
typedef signed long long ssize_t;

typedef long float lfloat_t;
typedef long double ldouble_t;

typedef size_t uint128_t[2];
typedef ssize_t int128_t[2];

// uint32_t uintconv64_32(uint64_t value){return (uint32_t)(value & 0xFFFFFFFFULL);}
// uint32_t uintconv32_64(uint32_t high, uint32_t low){return ((uint64_t)high << 32) | low;}
// uint16_t uintconv32_16(uint32_t value){return (uint16_t)(value & 0xFFFFU);}
// uint32_t uintconv16_32(uint16_t high, uint16_t low){return ((uint32_t)high << 16) | low;}
// uint8_t uintconv16_8(uint16_t value){return (uint8_t)(value & 0xFF);}
// uint32_t uintconv8_16(uint8_t high, uint8_t low){return ((uint16_t)high << 8) | low;}

size_t decode64(const uint8_t *array);
int decode32(const uint8_t *array);
uint32_t decode_32u(const uint8_t *array);
size_t clamp_sizet(size_t lower, size_t upper, size_t value);
void encode64(uint8_t *array, size_t value);
void encode32(uint8_t *array, int value);
void encode32u(uint8_t *array, const uint32_t value);
#endif