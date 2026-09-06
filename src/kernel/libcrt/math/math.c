#include "math.h"

uint32_t __noinline float_bits(float f){ union{float f; uint32_t u;}c; c.f=f; return c.u; }
float __noinline bits_float(uint32_t u){ union{uint32_t u; float f;}c; c.u=u; return c.f; }

#define LN2 0.6931471805599453f
#define INV_LN2 1.4426950408889634f

/* fast polynomial ln(1+z) for z in [0,1) */
float fast_log1p(float z){
	/* Use a 5-term Taylor / minimax-ish series for decent accuracy on [0,1) */
	// xmmRegister r0, r1;
	// AllocateSSERegister(&r0);	AllocateSSERegister(&r1);
	// StoreUAlignedFloats(r0, (void *)((float[4]){z, z * z, z * z * z, z * z * z * z}));
	// StoreUAlignedFloats(r1)
	float z2 = z * z;
	float z3 = z2 * z;
	float z4 = z3 * z;
	return z - z2 * 0.5f + z3 * (1.0f/3.0f) - z4 * 0.25f + z4 * z * (1.0f/5.0f);
}

/* natural log for positive x */
float fast_logf(float x){
	if(x <= 0.0f){return 0.0f;} /* undefined, return 0 for kernel use */
	uint32_t u = float_bits(x);
	int exp = ((u >> 23) & 0xFF) - 127;
	uint32_t mant = (u & 0x7FFFFF) | 0x800000; /* 1.mantissa */
	float m = mant / (float)(1 << 23); /* in [1,2) */
	float z = m - 1.0f;
	return (float)exp * LN2 + fast_log1p(z);
}

/* exp(r) for small r (|r| < ~0.7) via polynomial */
float fast_expf_small(float r){
	/* 6-term Taylor for decent accuracy */
	float r2 = r*r;
	float r3 = r2*r;
	float r4 = r3*r;
	float r5 = r4*r;
	return 1.0f + r + r2*0.5f + r3*(1.0f/6.0f) + r4*(1.0f/24.0f) + r5*(1.0f/120.0f);
}

/* compute 2^k as float (integer k) */
float pow2_int(int k){
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
float fast_expf(float x){
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
float powf_int_base(float x, int n){
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
	if(x == 0.0f){if(y > 0.0f){return 0.0f;}else{return 0.0f;}}

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
void setprecision(uint8_t new){local_precision = new;}

// Newton-Raphson square root approximation
float sqrtf_newton(float x, uint8_t precision){
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

float sqrtf(float x){return sqrtf_newton(x, local_precision);}

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

double pow_10(int n){
 	int i = 1;
 	double p = 1., m;	
 	if(n < 0){n = -n;	m = .1;
 	}else{m = 10.;}
 	for(; i <= n; i++){p *= m;}
 	return p;
}
int log_10(double r){
	int i = 0;
	double result = 1.;
	if(r == 0.0){return 0;}
	if(r < 0.0){r = -r;}
	if(r < 1.){
		for(; result >= r; i++){result *= .1;}
		i = -i;
	}else{
		for(; result <= r; i++){result *= 10.;}
		--i;
	}
  return i;
}
double integral(double real, double *ip){
	int log;
	double real_integral = 0.;

	/* equal to zero ? */
	if(real == 0.){*ip = 0.;		return 0.;}

	/* negative number ? */
	if(real < 0.){real = -real;}

  /* a fraction ? */
  if(real < 1.){*ip = 0.;			return real;}

	/* the real work :-) */
	for(log = log_10(real); log >= 0; log--){
		double i = 0., p = pow_10(log);
		double s = (real - real_integral) / p;
		while(i + 1. <= s){i++;}
		real_integral += i * p;
	}
	*ip = real_integral;
	return (real - real_integral);
}

float absf(float f){return (f < 0? -f: f);}
unsigned int absi(signed int i){return (i < 0? -i: i);}
unsigned long absl(signed long l){return (l < 0? -l: l);}
size_t absll(ssize_t z){return (z < 0? -z: z);}

//	https://stackoverflow.com/questions/26342823/implementation-of-fmod-function
#include "kernel/libcrt/memory/memory.h"
uint64_t double_as_uint64(double a){uint64_t r;	memcpy(&r, &a, sizeof r);	return r;}
double uint64_as_double(uint64_t a){double r;	memcpy(&r, &a, sizeof r);	return r;}

#define FP64_MANT_BITS      (52)
#define FP64_MANT_MASK      ((1ULL << FP64_MANT_BITS) - 1)
#define FP64_MANT_INT_BIT   (1ULL << FP64_MANT_BITS)
#define FP64_UPSCALE        (1ULL << FP64_MANT_BITS)
#define FP64_EXPO_BITS      (11)
#define FP64_EXPO_BIAS      ((1ULL << FP64_EXPO_BITS) - 1)
#define FP64_EXPO_MASK      (FP64_EXPO_BIAS << FP64_MANT_BITS)
#define FP64_NAN_INDEFINITE (0xfff8000000000000ULL) /* machine dependent; x86 */
#define FP64_QNAN_BIT       (0x0008000000000000ULL) /* machine dependent; x86 */

/* returns exact floating-point remainder of a/b (rounded towards zero) */
double fmod(double a, double b){
    double r;
    if(isnan (a) || isnan (b)) {
        r = a + b;
    } else if (isinf (a) || (b == 0.0)) {
        r = uint64_as_double (FP64_NAN_INDEFINITE);
    } else {
        uint64_t ia = double_as_uint64 (a);
        uint64_t ib = double_as_uint64 (b);
        if ((ia << 1) >= (ib << 1)) { // fabs (a) >= fabs (b)
            int ea = (ia & FP64_EXPO_MASK) >> FP64_MANT_BITS;
            int eb = (ib & FP64_EXPO_MASK) >> FP64_MANT_BITS;
            /* normalize source operands */
            if (ea == 0) {
                ia = double_as_uint64 (a * FP64_UPSCALE);
                ea = ((ia & FP64_EXPO_MASK) >> FP64_MANT_BITS) - FP64_MANT_BITS;
            }
            ia = (ia & FP64_MANT_MASK) | FP64_MANT_INT_BIT;
            if (eb == 0) {
                ib = double_as_uint64 (b * FP64_UPSCALE);
                eb = ((ib & FP64_EXPO_MASK) >> FP64_MANT_BITS) - FP64_MANT_BITS;
            }
            ib = (ib & FP64_MANT_MASK) | FP64_MANT_INT_BIT;

            /* perform binary longhand division */
            while (ea > eb) {
                if (ia >= ib) {
                    ia -= ib;
                }
                ia <<= 1;
                ea--;
            }
            /* ensure remainder is less than divisor */
            if (ia >= ib) {
                ia -= ib;
            }
            /* generate final result for non-zero remainder */
            if (ia != 0) {
                while (ia < FP64_MANT_INT_BIT) { // normalize remainder
                    ia <<= 1;
                    ea--;
                }            
                // combine exponent and significand; denormalize if necessary
                ia = (ea > 0) ? 
                    (ia + ((uint64_t)(ea - 1) << FP64_MANT_BITS)) : 
                    (ia >> (1 - ea));
            }
            r = copysign (uint64_as_double (ia), a);
        }else{r = a;}
    }
    return r;
}

//	https://wiki.osdev.org/Random_Number_Generator#Wichmann-Hill
static uint16_t seed[3]; /* seed with numbers between 1 and 30000 */
float wichmann_hill(void){
	seed[0] = (171 * seed[0]) % 30269;
	seed[1] = (172 * seed[1]) % 30307;
	seed[2] = (170 * seed[2]) % 30323;
	return fmod(seed[0] / 30269.0 + seed[1] / 30307.0 + seed[2] / 30323.0, 1.0);
}
float wh_rng(){return wichmann_hill();}
void wh_seed(uint16_t a, uint16_t b, uint16_t c){
	seed[0] = __clamp(1, 30000, a);
	seed[1] = __clamp(1, 30000, b);
	seed[2] = __clamp(1, 30000, c);
}


//	https://wiki.osdev.org/Random_Number_Generator#Mersenne_Twister
#define STATE_SIZE	312
#define MIDDLE		156
#define INIT_SHIFT	62
#define TWIST_MASK	0xb5026f5aa96619e9
#define INIT_FACT	6364136223846793005
#define SHIFT1		29
#define MASK1		0x5555555555555555
#define SHIFT2		17
#define MASK2		0x71d67fffeda60000
#define SHIFT3		37
#define MASK3		0xfff7eee000000000
#define SHIFT4		43
#define LOWER_MASK	0x7fffffff
#define UPPER_MASK	(~(uint64_t)LOWER_MASK)
static uint64_t state[STATE_SIZE];
static size_t index = STATE_SIZE + 1;

static void mt_seed(uint64_t s){
    index = STATE_SIZE;
    state[0] = s;
    for(size_t i = 1; i < STATE_SIZE; i++){state[i] = (INIT_FACT * (state[i - 1] ^ (state[i - 1] >> INIT_SHIFT))) + i;}
}
static void twist(void){
    for(size_t i = 0; i < STATE_SIZE; i++){
        uint64_t x = (state[i] & UPPER_MASK) | (state[(i + 1) % STATE_SIZE] & LOWER_MASK);
        x = (x >> 1) ^ (x & 1? TWIST_MASK : 0);
        state[i] = state[(i + MIDDLE) % STATE_SIZE] ^ x;
    }
    index = 0;
}
uint64_t mt_rng(void){
    if(index >= STATE_SIZE){twist();}
    uint64_t y = state[index];
    y ^= (y >> SHIFT1) & MASK1;
    y ^= (y << SHIFT2) & MASK2;
    y ^= (y << SHIFT3) & MASK3;
    y ^= y >> SHIFT4;
    index++;
    return y;
}