#pragma once
#include "../public/public.h"
#include "../kernel/ram/ram.h"
#include "../public/public/_bool.h"
#include "../public/public/_int.h"
#include "../kernel/IO/IO.h"

#ifndef KERNPUBLIC_H
#define KERNPUBLIC_H

#define RAMH_TYPE uint
#define KERNEL_ID (0x4446788592ull)



typedef struct os_obj{
	const size_t ID;
	const void *DATA;
}os_obj;
#define HANDLE os_obj

extern char *RAM;
extern volatile uint num_headers;
extern char *RAMHeaders;
#endif