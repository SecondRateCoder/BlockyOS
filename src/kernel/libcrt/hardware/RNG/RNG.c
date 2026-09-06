#include "RNG.h"

LibAPI __noinline uint64_t GetTrueRandomRange64(uint64_t min, uint64_t max){return GetTrueRandom64() % ((__max(max, min) - min) + 1);}
LibAPI __noinline uint64_t GetTrueRandomRange64U(uint64_t min, uint64_t max){
	uint64_t range = (__max(max, min) - min) + 1;
	// Calculate remainder ceiling to discard non-uniform biased tail
	uint64_t limit = UINT64_MAX - (UINT64_MAX % range);
	uint64_t rand;
	do{rand = GetTrueRandom64();}while(rand >= limit);
	return min + (rand % range);
}