#include <src/Public/stdint.h>
#include <src/Public/stdbool.h>

size_t decode_size_t(const uint8_t *array, const int Offset);
int decode_int(const uint8_t *array, size_t Offset);
int *decode_int_array(const uint8_t *array, size_t Offset, const size_t Length);
uint8_t *slice_bytes(uint8_t *array, size_t start, size_t Length);
size_t clamp_size_t(size_t lower, size_t upper, size_t value);
uint8_t encode_size_t(uint8_t *array, size_t value, size_t offset);
uint8_t encode_int(uint8_t *array, int value, size_t offset);
uint8_t encode_uint32(uint8_t array, const uint32_t value, size_t offset)
bool compare_array(uint8_t *array1, size_t size, uint8_t *array2, size_t size2);
bool is_set(uint8_t Item, uint8_t X);
uint8_t *hash(const uint8_t *val, size_t length);