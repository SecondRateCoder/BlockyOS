#include "kernel/libcrt/math/math.h"

LibAPI extern uint64_t __sysvabi GetTrueRandom64(void);
//	Faster, Biased TRNG
#define FTRNG(MIN, MAX)		GetTrueRandomRange64(MIN, MAX)
//	Slower, Unbiased TRNG
#define STRNG(MIN, MAX)		GetTrueRandomRange64U(MIN, MAX)
LibAPI uint64_t __noinline GetTrueRandomRange64(uint64_t min, uint64_t max);
LibAPI uint64_t __noinline GetTrueRandomRange64U(uint64_t min, uint64_t max);