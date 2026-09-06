#pragma once

#include "math.h"
#include "kernel/libcrt/hardware/SSE/SSE.h"

#define MATH_FLOAT_H

//	16-bit IEEE 754 Half-Precision
#if defined(__FLT16_MAX__)
typedef _Float16 float16_t;
#else
typedef uint16_t float16_t; // Fallback storage container if hardware lacks float16
#endif
typedef float float32_t;
typedef double float64_t;
typedef long double ldouble_t;
//	128-bit IEEE 754 Quad-Precision
#if defined(__FLOAT128__) || defined(__SIZEOF_FLOAT128__)
typedef __float128 float128_t;
#else
typedef struct { uint64_t low; uint64_t high; } float128_t; // Storage fallback
#endif

#define INFINITY	(__builtin_inff ())
#define NaNf		(__builtin_nanf (""))
#define SNaNf		(__builtin_nansf (""))
#define SNaNd		(__builtin_nans (""))
#define SNaNld		(__builtin_nansl (""))

#define INFINITY_F    (__builtin_inff())
#define INFINITY_D    (__builtin_inf())
#define INFINITY_LD   (__builtin_infl())
#define INFINITY_D32  (__builtin_infd32())
#define INFINITY_D64  (__builtin_infd64())
#define INFINITY_D128 (__builtin_infd128())

#ifndef INFINITY
#define INFINITY      INFINITY_F
#endif

//	Quiet NaNs
#define NaNf    (__builtin_nanf(""))
#define NaNd    (__builtin_nan(""))
#define NaNld   (__builtin_nanl(""))
#define NaNd32  (__builtin_nand32(""))
#define NaNd64  (__builtin_nand64(""))
#define NaNd128 (__builtin_nand128(""))

//	Signaling NaNs
#define SNaNf    (__builtin_nansf(""))
#define SNaNd    (__builtin_nans(""))
#define SNaNld   (__builtin_nansl(""))
#define SNaNd32  (__builtin_nansd32(""))
#define SNaNd64  (__builtin_nansd64(""))
#define SNaNd128 (__builtin_nansd128(""))

//	Value Limits (Min/Max Positive Normalized Values)
#if defined(__FLT16_MAX__)
#define FLOAT16_MAX  __FLT16_MAX__
#define FLOAT16_MIN  __FLT16_MIN__
#endif

#define FLOAT32_MAX  __FLT_MAX__
#define FLOAT32_MIN  __FLT_MIN__

#define FLOAT64_MAX  __DBL_MAX__
#define FLOAT64_MIN  __DBL_MIN__

#define LDOUBLE_MAX  __LDBL_MAX__
#define LDOUBLE_MIN  __LDBL_MIN__

#if defined(__FLOAT128__) || defined(__SIZEOF_FLOAT128__)
#define FLOAT128_MAX __FLT128_MAX__
#define FLOAT128_MIN __FLT128_MIN__
#endif

// Decimal Limits
#if defined(__DEC32_MAX__)
#define DEC32_MAX   __DEC32_MAX__
#define DEC32_MIN   __DEC32_MIN__
#define DEC64_MAX   __DEC64_MAX__
#define DEC64_MIN   __DEC64_MIN__
#define DEC128_MAX  __DEC128_MAX__
#define DEC128_MIN  __DEC128_MIN__
#endif


#define isnanf(f)   (f == NaNf)
#define isinff(f)   (f == INFINITY_F)
#define isnand(d)   (d == NaNd)
#define isinfd(d)   (d == INFINITY_D)

#define isnan       isnand
#define isinf       isinfd

#define copysign(a, f)  ((f) > 0? (a): -(a))