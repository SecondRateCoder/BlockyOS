#include "memory.h"

void *memset(void *buffer, int val, uint64_t len){
	xmmRegister r0;
	// Only use SIMD if the buffer is large enough and we can get a register
	if(len >= 16 && AllocateSSERegister(&r0)){
		// Broadcast the 8-bit 'val' to a 128-bit pattern
		uint8_t v = (uint8_t)val;
		uint32_t pattern = v | (v << 8) | (v << 16) | (v << 24), 
				vec_data[4] = {pattern, pattern, pattern, pattern};
		uint64_t chunks = len / 16;
		uint8_t *ptr = (uint8_t *)buffer;
		
		// Mem -> Reg (Load the pattern into XMM)
		StoreUAlignedIntegers(r0, vec_data);
		
		// Write 16 bytes at a time
		for(uint64_t i = 0; i < chunks; i++){LoadUAlignedIntegers(r0, ptr + (i * 16));}
		FreeSSERegister(r0);
		// Handle remainder bytes
		for(uint64_t i = chunks * 16; i < len; i++){ptr[i] = v;}
	// 	Fallback for small buffers
	}else{for(uint64_t i = 0; i < len; i++){((uint8_t *)buffer)[i] = (uint8_t)val;}}
	return buffer;
}

//*	We cannot easily use your SIMD wrapper for memcmp because we don't 
//*	have an assembly wrapper for PMOVMSKB (which is required to extract 
//*	SIMD comparison results into a CPU register to branch on).
//*	However, comparing in 64-bit (8-byte) chunks is heavily optimized.
int memcmp(const void * __restrict__ a, const void * __restrict__ b, uint64_t len){
	const uint8_t *ptr_a = (const uint8_t *)a, 
				*ptr_b = (const uint8_t *)b;
	
	// Fast path: Compare 8 bytes at a time
	uint64_t chunks = len / 8;
	for(uint64_t i = 0; i < chunks; i++){
		const uint64_t val_a = *(const uint64_t *)(ptr_a + (i * 8)), 
					val_b = *(const uint64_t *)(ptr_b + (i * 8));
		
		// If they differ, we must find exactly which byte differed 
		// to return the standard C positive/negative difference.
		if(val_a != val_b){
			for(uint64_t j = i * 8; j < (i * 8) + 8; j++){
				if(ptr_a[j] != ptr_b[j]){return ptr_a[j] - ptr_b[j];}
			}
		}
	}
	// Remainder loop
	for(uint64_t i = chunks * 8; i < len; i++){
		if(ptr_a[i] != ptr_b[i]){return ptr_a[i] - ptr_b[i];}
	}
	return 0; // Equal
}

void *memcpy(void * __restrict dst, const void * __restrict src, uint64_t len){
	xmmRegister r0;
	if(len >= 16 && AllocateSSERegister(&r0)){
		uint64_t chunks = len / 16;
		uint8_t *d = (uint8_t *)dst;
		const uint8_t *s = (const uint8_t *)src;
		
		// Copy 16 bytes at a time
		for(uint64_t i = 0; i < chunks; i++){
			uint64_t offset = i * 16;
			StoreUAlignedIntegers(r0, (void *)(s + offset)); // Mem -> Reg
			LoadUAlignedIntegers(r0, (void *)(d + offset));  // Reg -> Mem
		}
		FreeSSERegister(r0);
		
		// Handle remainder bytes
		for(uint64_t i = chunks * 16; i < len; i++){d[i] = s[i];}
	}else{
		// Fallback for small buffers
		uint8_t *d = (uint8_t *)dst;
		const uint8_t *s = (const uint8_t *)src;
		for(uint64_t i = 0; i < len; i++){d[i] = s[i];}
	}
	return dst;
}

GenericChecksum *ResolveGenericChecksum(void *ptr, uint64_t nbytes){
	static GenericChecksum out = {0};
	uint64_t split = (nbytes % 2) == 0? (nbytes / 2): (nbytes % 2) + 1;
	while(nbytes--){
		if(nbytes > split){out[0] ^= ((uint8_t *)ptr)[nbytes] & (((uint8_t *)ptr)[nbytes] << 2);}
		else{out[1] ^= ((uint8_t *)ptr)[nbytes] & (((uint8_t *)ptr)[nbytes] << 2);}
	}
	return &out;
}

#include "kernel/libcrt/memory/allocator/malloc.h"
errno_t memmove_s(void *__restrict a, uint64_t alen, void *__restrict b, uint64_t blen){
	if(!a || !b){return NullError;}
	if(blen > alen){return OutOfBoundsError;}
	void *temp = AllocatePages(NULL, blen, (ReadWritable | UserMode), 0x00);
	memcpy(temp, b, blen);		memcpy(a, temp, blen);
	FreePages(temp);
	return 0x00;
}