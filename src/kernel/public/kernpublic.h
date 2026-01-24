#pragma once
#include "../src/kernel/public/public/public.h"

#define RAMH_TYPE uint32_t
#include "./kernel/public/public/math/int/_bool.h"
#include "./kernel/public/public/math/int/_int.h"
#include "./kernel/public/public/memory/memory.h"
#include "./kernel/lib32/stdfile/stdfile.h"
#include "./kernel/lib32/stdfile/f-rat.h"
// #include "../src/kernel/ram/ram.h"
// #include "../src/kernel/IO/IO.h"

#define KERNEL_ID (0x4446788592ull)

#define __cdecl __attribute__((cdecl))

typedef struct os_obj{
	const size_t ID;
	const void *DATA;
}os_obj;
#define HANDLE os_obj

extern char *RAM;
extern volatile RAMH_TYPE num_headers;
extern char *RAMHeaders;