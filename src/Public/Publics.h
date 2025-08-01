#include <src/Public/stdint.h>
#include <src/Public/stdbool.h>

#define MAXSTR_LENGTH 1024*1024*1024
#define MAXNAME_LENGTH 30
#define NULL (void *)0
#define path_t PATH

path_t *Paths;
size_t numofpaths;
typedef struct PATH{
    char *path;
    size_t strlength;
    char Name[MAXNAME_LENGTH];
}PATH;

#define rad_deg_t rad_deg
typedef struct rad_deg{ldouble_t radians, degrees;}rad_deg;

size_t decode_size_t(const uint8_t *array, const int Offset);
int decode_int(const uint8_t *array, size_t Offset);
int *decode_int_array(const uint8_t *array, size_t Offset, const size_t Length);
uint8_t *slice_bytes(uint8_t *array, size_t start, size_t Length);
size_t clamp_size_t(size_t lower, size_t upper, size_t value);
void encode_size_t(uint8_t *array, size_t value, size_t offset);
void encode_int(uint8_t *array, int value, size_t offset);
void encode_uint32(uint8_t *array, const uint32_t value, size_t offset);
bool compare_array(uint8_t *array1, uint8_t *array2, size_t size2);
bool is_set(uint8_t Item, uint8_t X);
void set(size_t Item, uint8_t X, uint8_t val);
uint8_t *hash(const uint8_t *val, size_t length);
bool strcompare(char *str1, char *str2);
bool strcompare_s(char *str1, char *str2, float benchmark);
size_t strlen(char *string);
bool strcompare_l(char *str1, char *str2, size_t length);
bool is_num(char c);
char *strslice(char *c, size_t newlen);
int strslice_till(char *src, char *dest, char cond, size_t maxlen);

uint32_t uintconv64_32(uint64_t value);
uint32_t uintconv32_64(uint32_t high, uint32_t low);
uint16_t uintconv32_16(uint32_t value);
uint32_t uintconv16_32(uint16_t high, uint16_t low);
uint8_t uintconv16_8(uint16_t value);
uint32_t uintconv8_16(uint8_t high, uint8_t low);

#define IDSize 8
void *alloca(size_t Size, uint8_t ProcessID[IDSize]);