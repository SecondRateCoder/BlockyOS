#pragma once

#include "int/int.h"
#include "int/bool.h"

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

static inline uint32_t float_bits(float f);
static inline float bits_float(uint32_t u);

static float fast_log1p(float z);
static float fast_logf(float x);
static float fast_expf_small(float r);
static float pow2_int(int k);
static float fast_expf(float x);
static float powf_int_base(float x, int n);

ssize_t powll(signed long n, uint8_t pow);
float powf(float x, float y);

static float sqrtf_newton(float x, uint8_t precision);
float sqrtf(float x);
uint32_t isqrt(uint32_t n);


#define abs absf
float absf(float f);
unsigned int absi(signed int i);
unsigned long absl(signed long l);
size_t absll(ssize_t z);

#define max(a, b) (a > b? a: b)
#define min(a, b) (a < b? a: b)

void setprecision(uint8_t new);