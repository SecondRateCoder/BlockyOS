#pragma once

typedef unsigned char uint8_t;
typedef signed char int8_t;
#define u8_t uint8_t
#define i8_t int8_t
#define byte int8_t
#define ubyte uint8_t

typedef unsigned short int uint16_t;
typedef signed short int int16_t;
#define u16_t uint16_t
#define i16_t int16_t

typedef unsigned int uint32_t;
typedef unsigned long uinl32_t;
typedef signed int int32_t;
typedef signed long inl32_t;
#define u32_t uint32_t
#define i32_t int32_t

typedef unsigned long long uint64_t;
typedef signed long long int64_t;
#define u64_t uint64_t
#define size_t uint64_t
#define i64_t int64_t
#define ssize_t int64_t

typedef uint64_t uint128_t[2];
typedef int64_t int128_t[2];
