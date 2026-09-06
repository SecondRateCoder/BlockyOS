#include "math.h"

static inline uint32_t float_bits(float f){ union{float f; uint32_t u;}c; c.f=f; return c.u; }
static inline float bits_float(uint32_t u){ union{uint32_t u; float f;}c; c.u=u; return c.f; }

#define LN2 0.6931471805599453f
#define INV_LN2 1.4426950408889634f

/* fast polynomial ln(1+z) for z in [0,1) */
static float fast_log1p(float z){
    /* Use a 5-term Taylor / minimax-ish series for decent accuracy on [0,1) */
    float z2 = z*z;
    float z3 = z2*z;
    float z4 = z3*z;
    return z - z2*0.5f + z3*(1.0f/3.0f) - z4*0.25f + z4*z*(1.0f/5.0f);
}

/* natural log for positive x */
static float fast_logf(float x){
    if(x <= 0.0f){return 0.0f;} /* undefined, return 0 for kernel use */
    uint32_t u = float_bits(x);
    int exp = ((u >> 23) & 0xFF) - 127;
    uint32_t mant = (u & 0x7FFFFF) | 0x800000; /* 1.mantissa */
    float m = mant / (float)(1 << 23); /* in [1,2) */
    float z = m - 1.0f;
    return (float)exp * LN2 + fast_log1p(z);
}

/* exp(r) for small r (|r| < ~0.7) via polynomial */
static float fast_expf_small(float r){
    /* 6-term Taylor for decent accuracy */
    float r2 = r*r;
    float r3 = r2*r;
    float r4 = r3*r;
    float r5 = r4*r;
    return 1.0f + r + r2*0.5f + r3*(1.0f/6.0f) + r4*(1.0f/24.0f) + r5*(1.0f/120.0f);
}

/* compute 2^k as float (integer k) */
static float pow2_int(int k){
    int e = k + 127;
    if(e <= 0){return 0.0f;} /* underflow */
    if(e >= 255) { /* overflow */
        /* return large float */
        uint32_t uf = (254u << 23) | 0x7FFFFFu;
        return bits_float(uf);
    }
    uint32_t u = (uint32_t)(e & 0xFF) << 23;
    return bits_float(u);
}

/* generic expf using decomposition: exp(x) = 2^{k} * exp(r), k = floor(x / ln2), r = x - k*ln2 */
static float fast_expf(float x){
    if(x == 0.0f){return 1.0f;}
    /* limit input a bit to avoid huge outputs in kernel */
    if(x >  88.0f){x =  88.0f;}
    if(x < -88.0f){x = -88.0f;}
    float kd = (float)((int) (x * INV_LN2));
    int k = (int)kd;
    float r = x - (float)k * LN2;
    float er = fast_expf_small(r);
    return pow2_int(k) * er;
}

/* integer-power by squaring for integer exponent (handles negative exponent) */
static float powf_int_base(float x, int n){
    if(n == 0){return 1.0f;}
    bool neg = false;
    if(n < 0){neg = true;   n = -n;}
    float res = 1.0f;
    float base = x;
    while(n){
        if(n & 1){res *= base;}
        base *= base;
        n >>= 1;
    }
    return neg ? 1.0f / res : res;
}

ssize_t powll(signed long n, uint8_t pow){
    const uint8_t pow_ = pow;
    size_t out = n;
    while(pow){
        out *= out;
        pow--;
    }
    return out;
}

/* public powf implementation */
float powf(float x, float y){
    /* handle simple cases */
    if(y == 0.0f){return 1.0f;}
    if(x == 1.0f){return 1.0f;}
    if(x == 0.0f){
        if(y > 0.0f) return 0.0f;
        return 0.0f; /* negative/zero exponent undefined; return 0 for kernel */
    }

    /* if y is (near) integer, do integer exponentiation (handles negative and negative base) */
    float yi = (float)(int)y;
    if((y - yi) > -1e-6f && (y - yi) < 1e-6f){
        int yi_i = (int)yi;
        return powf_int_base(x, yi_i);
    }

    /* non-integer exponent: only defined for x>0 in real numbers */
    if(x <= 0.0f) return 0.0f; /* return 0 for kernel/freestanding environment */

    float lx = fast_logf(x);
    return fast_expf(y * lx);
}

uint8_t local_precision;
void setprecision(uint8_t new){local_precision - new;}

// Newton-Raphson square root approximation
static float sqrtf_newton(float x, uint8_t precision){
    if(x < 0.0f){return 0.0f;} // undefined
    if(x == 0.0f){return 0.0f;}
    
    // Initial guess: use fast inverse sqrt trick for better starting point
    uint32_t i = *(uint32_t*)&x;
    i = 0x5f3759df - (i >> 1); // magic constant
    float y = *(float*)&i;
    
    // Newton-Raphson iterations: y = (y + x/y) / 2
    do{
        y = (y + x / y) * 0.5f;
    }while(precision--);
    
    return y;
}

float sqrtf(float x){
    return sqrtf_newton(x, local_precision);
}

// Fixed-point integer square root (for when you need exact integer result)
uint32_t sqrti(uint32_t n){
    if(n == 0){return 0;}
    
    uint32_t x = n;
    uint32_t x1 = (x + 1) / 2;
    
    while(x1 < x){
        x = x1;
        x1 = (x + n / x) / 2;
    }
    return x;
}

float absf(float f){return (f < 0? -f: f);}
unsigned int absi(signed int i){return (i < 0? -i: i);}
unsigned long absl(signed long l){return (l < 0? -l: l);}
size_t absll(ssize_t z){return (z < 0? -z: z);}