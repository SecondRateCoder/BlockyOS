#pragma once
#include "../src/kernel/public/public/public.h"

#define RAMH_TYPE uint32_t
#include "../src/kernel/ram/ram.h"
#include "../src/kernel/public/public/math/int/_bool.h"
#include "../src/kernel/public/public/math/int/_int.h"
#include "../src/kernel/public/public/memory/memory.h"
#include "../src/kernel/IO/IO.h"

#ifndef KERNPUBLIC_H
#define KERNPUBLIC_H

#define KERNEL_ID (0x4446788592ull)



typedef struct os_obj{
	const size_t ID;
	const void *DATA;
}os_obj;
#define HANDLE os_obj

extern char *RAM;
extern volatile u8_t num_headers;
extern char *RAMHeaders;
#endif