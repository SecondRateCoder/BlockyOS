#include "CRC32.h"

uint32_t crc32(const uint8_t *data, size_t len, uint32_t polynomial, uint32_t init, uint32_t xor){
    uint32_t crc = init;

    while(len--){
        crc ^= (uint32_t)(*data++) << 24;  // align byte to MSB

        for (int i = 0; i < 8; i++) {
            if (crc & 0x80000000u) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc ^ xor;
}