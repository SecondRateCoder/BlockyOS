#pragma once

#include "kernel/lib/math/int/int.h"

#define CRC32_DEFAULT_POLY 0x04C11DB7u
#define CRC32_DEFAULT_INIT 0xFFFFFFFFu
#define CRC32_DEFAULT_XOR  0xFFFFFFFFu

#define CRC32_STEP(crc, poly) (((crc) & 0x80000000u) ? (((crc) << 1) ^ (poly)) : ((crc) << 1))
#define CRC32_BYTE(crc, byte, poly) ( \
    CRC32_STEP((crc) ^ ((uint32_t)(byte) << 24), poly), \
    CRC32_STEP(CRC32_STEP(CRC32_STEP(CRC32_STEP( \
    CRC32_STEP(CRC32_STEP(CRC32_STEP(CRC32_STEP( \
        (crc) ^ ((uint32_t)(byte) << 24), poly), poly), poly), poly), \
        poly), poly), poly), poly) \
)
#define CRC32_1(data, poly, init) CRC32_BYTE(init, data[0], poly)
#define CRC32_2(data, poly, init) CRC32_BYTE(CRC32_1(data, poly, init), data[1], poly)
#define CRC32_3(data, poly, init) CRC32_BYTE(CRC32_2(data, poly, init), data[2], poly)
#define CRC32_4(data, poly, init) CRC32_BYTE(CRC32_3(data, poly, init), data[3], poly)
#define CRC32_5(data, poly, init) CRC32_BYTE(CRC32_4(data, poly, init), data[4], poly)
#define CRC32_6(data, poly, init) CRC32_BYTE(CRC32_5(data, poly, init), data[5], poly)
#define CRC32_7(data, poly, init) CRC32_BYTE(CRC32_6(data, poly, init), data[6], poly)
#define CRC32_8(data, poly, init) CRC32_BYTE(CRC32_7(data, poly, init), data[7], poly)

#define CRC32def(data, len) crc32(data, len, CRC32_DEFAULT_POLY, CRC32_DEFAULT_INIT, CRC32_DEFAULT_XOR)
uint32_t crc32(const uint8_t *data, size_t len, uint32_t polynomial, uint32_t init, uint32_t xor);