#pragma once

#include "kernel/libcrt/math/math.h"
#include "kernel/libcrt/def.h"

LibAPI GenericChecksum *ResolveGenericChecksum(void *ptr, uint64_t nbytes);

LibAPI void *memcpy(void *__restrict dst, const void *__restrict src, uint64_t len);
LibAPI errno_t memmove_s(void *__restrict a, uint64_t alen, void *__restrict b, uint64_t blen);
LibAPI int memcmp(const void * __restrict a, const void * __restrict b, uint64_t len);
LibAPI void *memset(void *buffer, int val, uint64_t len);