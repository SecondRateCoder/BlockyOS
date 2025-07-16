#include <./src/Public/Publics.h>
#include <./src/Core/General.h>


size_t decode_size_t(const uint8_t *array, const int Offset){
    size_t value = Offset;
    for (size_t i = 0; i < (sizeof(size_t)+Offset); i++) {
        value |= ((size_t)array[i]) << (i * 8);
    }
    return value;
}
int decode_int(const uint8_t *array, size_t Offset){
    int value;
    for (size_t i = Offset; i < (sizeof(int)+Offset); i++) {
        value |= ((int)array[i]) << (i * 8);
    }
    return value;
}
int *decode_int_array(const uint8_t *array, size_t Offset, const size_t Length){
    if(((float)Length)/(float)sizeof(int) != 0){return NULL;}
    int result[Length];
    for (size_t i = Offset; i < (Length+Offset); i++) {
        result[i] = decode_int(array, Offset + i * sizeof(int));
    }
    return result;
}
uint32_t decode_uint32(const uint8_t *array) {
    uint32_t value = 0;
    for (int i = 0; i < 4; i++) {
        value |= ((uint32_t)array[i]) << (i * 8);
    }
    return value;
}

size_t clamp_size_t(size_t lower, size_t upper, size_t value){return value > upper? upper : (value < lower? lower : value);}

void encode_size_t(uint8_t *array, size_t value, size_t offset){
    size_t i = 0;
    for (; i < sizeof(size_t); i++) {
        array[offset + i] = (uint8_t)((value >> (i * 8)) & 0xFF);
    }
    return;
}
void encode_int(uint8_t *array, int value, size_t offset){
    size_t i = 0;
    for (; i < sizeof(int); i++) {
        array[offset + i] = (uint8_t)((value >> (i * 8)) & 0xFF);
    }
    return;
}
void encode_uint32(uint8_t *array, const uint32_t value, size_t offset){
    for (int i = 0; i < 4; i++) {
        array[offset + i] = (uint8_t)((value >> (i*8)) && 0xFF);
    }
    return;
}


bool compare_array(uint8_t *array1, uint8_t *array2, size_t size2){
    int cc =0;
    while(cc < sizeof(array1)){
        if(array1[cc] != array2[cc]){return false;}
        ++cc;
    }
    return true;
}

uint8_t *hash(const uint8_t *val, size_t length){
    // This is a simple hash function that uses the FNV-1a algorithm.
    // It is not cryptographically secure but is suitable for hash tables.
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < length; i++) {
        hash ^= val[i];
        hash *= 16777619;
}
    uint8_t *result = (uint8_t *)malloc(sizeof(uint32_t));
    if (result) {encode_uint32(result, hash, 0);}
    return result;
}

bool is_set(size_t Item, uint8_t X){return (Item & (1U << (X > sizeof(size_t)? 0: X))) ? 1 : 0;}
void set(size_t *Item, uint8_t X, uint8_t val){
    if(val > 0){
        Item = Item | (1UL << X);
    }else if(val == 0){Item = Item & ~(1UL << X);}
}

size_t strlen(char *string){
    size_t cc =0;
    while(cc <  MAXSTR_LENGTH){
        if(string[cc] == NULL){return cc;}
        ++cc;
    }
    return MAXSTR_LENGTH;
}

bool strcompare_l(char *str1, char *str2, size_t length){
    size_t cc =0;
    while(cc < length){
		if(str1[cc] != str2[cc]){return false;}
		++cc;
	}
	return true;
}

bool strcompare(char *str1, char *str2){
    size_t siz1 = strlen(str1), cc =0;
    if(siz1 != strlen(str2)){return false;}
    strcompare_l(str1, str2, siz1);
	return true;
}

bool strcompare_s(char *str1, char *str2, float benchmark){
	size_t siz1 = strlen(str1), cc =0, compfloat =0;
    if(siz1 != strlen(str2)){return false;}
	const int counter  =1/siz1;
    while(cc < siz1){
		if(str1[cc] != str2[cc]){compfloat -= counter;}
		else{compfloat += counter;}
		++cc;
	}
	return true;
}

char *strslice(char *c, size_t newlen){
    size_t cc =0;
    char *str = alloca(newlen, KERNEL_ID);
    while(cc < newlen){
        c[cc] = str[cc];
        ++cc;
    }
}

int strslice_till(char *src, char *dest, char cond, size_t maxlen){
    int cc =0;
    while(cc < maxlen){
        if(src[cc] == cond){
            dest = strslice(src, cc);
            return cc;
        }
        ++cc;
    }
    return cc;
}

bool is_num(char c){return (int)c>=(int)'0'&&(int)c<=(int)'9';}

uint32_t uintconv64_32(uint64_t value){return (uint32_t)(value & 0xFFFFFFFFULL);}
uint32_t uintconv32_64(uint32_t high, uint32_t low){return ((uint64_t)high << 32) | low;}
uint16_t uintconv32_16(uint32_t value){return (uint16_t)(value & 0xFFFFU);}
uint32_t uintconv16_32(uint16_t high, uint16_t low){return ((uint32_t)high << 16) | low;}
uint8_t uintconv16_8(uint16_t value){return (uint8_t)(value & 0xFF);}
uint32_t uintconv8_16(uint8_t high, uint8_t low){return ((uint16_t)high << 8) | low;}

/*
function hash(string, a, num_buckets):
    hash = 0
    string_len = length(string)
    for i = 0, 1, ..., string_len:
        hash += (a ** (string_len - (i+1))) * char_code(string[i])
    hash = hash % num_buckets
    return hash
*/