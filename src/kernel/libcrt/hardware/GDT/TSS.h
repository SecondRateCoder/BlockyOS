#pragma once

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"

#define DefaultStackSize	nKB(16)

typedef struct{
	uint32_t reserved0;
	
	/* Privilege Level Stack Table (RSP0 - RSP2)
	 * RSP0 is automatically loaded by CPU when an interrupt/exception 
	 * switches execution from Ring 3 (User) to Ring 0 (Kernel).
	 */
	uint64_t RSP[3];
	
	uint64_t reserved1;
	
	/* Interrupt Stack Table (IST1 - IST7)
	 * Pointers to dedicated stacks used when an IDT entry specifies an IST index (1-7).
	 * Ideal for stack faults (#DF Double Fault, #MC Machine Check, etc.).
	 */
	uint64_t IST[8];
	
	uint64_t reserved2;
	uint16_t reserved3;
	
	/* Offset from the TSS base to the I/O Permission Bit Map (IOPB).
	 * If I/O bitmap is not used, set this to sizeof(TSS64) or higher (e.g., 0x68).
	 */
	uint16_t io_map_base;
}__packed TSS_t;