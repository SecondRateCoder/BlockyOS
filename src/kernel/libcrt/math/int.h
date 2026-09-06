#pragma once

#define MATH_INT_H

#include "bool.h"
#include "kernel/libcrt/def.h"

#define bitselect(i, shift, mask)	((((uint64_t)(i)) >> (shift)) & (mask))

typedef unsigned char		uint8_t;
#define UINT8_MAX			(0xFFU)
#define UINT8_MIN			(0x0U)
typedef signed char			int8_t;
#define INT8_MAX			(0x7FU)
#define INT8_MIN			(0xFF)
#define byte				int8_t

typedef unsigned short		uint16_t;
#define UINT16_MAX			(0xFFFFU)
#define UINT16_MIN			(0x0U)
typedef signed short		int16_t;
#define INT16_MAX			(0x7FFFU)
#define INT16_MIN			(0xFFFFU)
#define uword				uint16_t
#define word				int16_t

typedef uint16_t			wchar_t;

typedef unsigned int		uint32_t;
#define UINT32_MAX			(0xFFFFFFFFU)
#define UINT32_MIN			(0x0U)
typedef signed int			int32_t;
#define INT32_MAX			(0x7FFFFFFFU)
#define INT32_MIN			(0xFFFFFFFFU)
#define udword				uint32_t
#define dword				int32_t

enumdef(uint32_t, errno_t){OutOfBoundsError = 0x01, NullError = 0x01};

#ifdef DEF_H
typedef unsigned long long  __align(8)	uint64_t, uintptr_t;
typedef signed long long    __align(8)	int64_t, intptr_t;
#else
typedef unsigned long long	uint64_t, uintptr_t;
typedef signed long long	int64_t, intptr_t;
#endif
#define UINT64_MAX			(0xFFFFFFFFFFFFFFFFU)
#define UINT64_MIN			(0x0U)
#define INT64_MAX			(0x7FFFFFFFFFFFFFFFU)
#define INT64_MIN			(0xFFFFFFFFFFFFFFFFU)
#define uqword				uint64_t
#define qword				int64_t

typedef uint64_t			uint128_t[2];
#define UINT128_MAX			((uint128_t){UINT64_MAX, UINT64_MAX})
#define UINT128_MIN			((uint128_t){UINT64_MIN, UINT64_MIN})
typedef int64_t				int128_t[2];
#define INT128_MAX			((int128_t){INT64_MAX, INT64_MIN})
#define INT128_MIN			((int128_t){INT64_MIN, INT64_MIN})

#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64__)
typedef uint64_t			size_t;
#define SIZE_MAX			UINT64_MAX
#define SIZE_MIN			UINT64_MIN
typedef int64_t				ssize_t;
#define SSIZE_MAX			INT64_MAX
#define SSIZE_MIN			INT64_MIN
typedef uint64_t            bool64_t;
#elif defined(__i386__) || defined(_M_IX86) || defined(_X86_)
typedef uint32_t			size_t;
#define SIZE_MAX			UINT32_MAX
#define SIZE_MIN			UINT32_MIN
typedef int32_t				ssize_t;
#define SSIZE_MAX			INT32_MAX
#define SSIZE_MIN			INT32_MIN
typedef uint32_t            bool64_t;
#endif
