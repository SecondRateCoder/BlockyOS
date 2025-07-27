typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;
typedef unsigned int uint;
typedef unsigned long uint64_t;
typedef unsigned long uintptr_t;
typedef unsigned long long size_t;


typedef signed char int8_t;
typedef signed short int int16_t;
typedef signed int int32_t;
typedef signed long int64_t;
typedef signed long intptr_t;
typedef signed long long ssize_t;

#define INT8_MIN   (-1-0x7f)
#define INT16_MIN  (-1-0x7fff)
#define INT32_MIN  (-1-0x7fffffff)
#define INT64_MIN  (-1-0x7fffffffffffffff)

#define INT8_MAX   (0x7f)
#define INT16_MAX  (0x7fff)
#define INT32_MAX  (0x7fffffff)
#define INT64_MAX  (0x7fffffffffffffff)

#define UINT8_MAX  (0xff)
#define UINT16_MAX (0xffff)
#define UINT32_MAX (0xffffffffu)
#define UINTPTR_MAX UINT64_MAX
#define UINT64_MAX (0xffffffffffffffffu)


typedef long double ldouble_t;

#define FLT_MAX 3.4028235* pow(10, 38)
#define DBL_MAX 1.7976931348623157* pow(10, 308)
#define LDBL_MAX 1.189731495357231765* pow(10, 4932)