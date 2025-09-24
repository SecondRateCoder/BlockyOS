#pragma once
#include "../public/public.h"
#include "../kernel/ram/ram.h"

#ifndef KERNPUBLIC_H
#define KERNPUBLIC_H

#define RAMH_TYPE uint

#define KERNEL_ID ((size_t)0x4446788592u)

extern char *RAM;
extern volatile uint num_headers;
extern char *RAMHeaders;
#endif