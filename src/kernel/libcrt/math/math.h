#pragma once

#include "int.h"
#include "bool.h"
#include "float.h"

#define MATH_H

typedef union fui{
	float f;
	uint32_t u;
	int32_t i;
}fui;

typedef union ldzi{
	long double f;
	size_t u;
	ssize_t i;
}ldzi;


#define abs absf
#define __min(a, b)			((a) > (b)? (b): (a))
#define __max(a, b)			((b) > (a)? (b): (a))
#define __clamp(a, b, n)	((n) > (a)? ((n) < (b)? (n): (b)): (a))
#define __set(a, f)			((a) &= (f))
#define __uset(a, f)		((a) &= ~(f))
#define __check(a, f)		((a) & (f) == (f))
#define __roundupt(a, i)	((((a) + ((i) - 1)) / (i)) * (i))
#define __rounddownt(a, i)	((((a) - (i)) + 1 / (i)) * (i))
#define __roundup(a, i)		((((volatile __typeof(a))(a) + ((volatile __typeof(a))(i) - 1)) / (volatile __typeof(a))(i)) * (volatile __typeof(a))(i))
#define __rounddown(a, i)	((((volatile __typeof(a))(a) - (volatile __typeof(a))((i) + 1)) / (volatile __typeof(a))(i)) * (volatile __typeof(a))(i))

LibAPI uint32_t __noinline float_bits(float f);
LibAPI float __noinline bits_float(uint32_t u);

LibAPI float fast_log1p(float z);
LibAPI float fast_logf(float x);
LibAPI float fast_expf_small(float r);
LibAPI float pow2_int(int k);
LibAPI float fast_expf(float x);
LibAPI float powf_int_base(float x, int n);

LibAPI ssize_t powll(signed long n, uint8_t pow);
LibAPI float powf(float x, float y);

LibAPI float sqrtf_newton(float x, uint8_t precision);
LibAPI float sqrtf(float x);
LibAPI uint32_t sqrti(uint32_t n);

LibAPI float absf(float f);
LibAPI unsigned int absi(signed int i);
LibAPI unsigned long absl(signed long l);
LibAPI size_t absll(ssize_t z);

LibAPI void setprecision(uint8_t new);

LibAPI double pow_10(int n);
LibAPI int log_10(double r);
LibAPI double integral(double real, double *ip);

LibAPI double fmod(double a, double b);

LibAPI float wh_rng();
LibAPI uint64_t mt_rng(void);
#define randomf		wh_rng
#define random64	mt_rng