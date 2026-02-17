#pragma once

#include "./kernel/lib32/generic/standard.h"

typedef struct gdtENTRY_t{
	uint16_t limitLow;
	uint16_t baseLow;
	uint8_t  baseMiddle;
	uint8_t  access;
	uint8_t  FlagLimit;
	uint8_t  baseHigh;
}PACKEDSTRUCT gdtENTRY_t;

typedef struct gdtDESC_t{
	// Byte size of GDT table - 1
	uint16_t limit;

	// Address of GDT table
	uint32_t table;
}PACKEDSTRUCT gdtDESC_t;

typedef enum GDTACCESS{
	GDTACCESS_SEGCODE 				= 0x18,
	GDTACCESS_SEGDATA 				= 0x10,
	GDTACCESS_SEGTASK 				= 0x00,

	GDTACCESS_RING0 				= 0x00,
	GDTACCESS_RING1 				= 0x20,
	GDTACCESS_RING2 				= 0x40,
	GDTACCESS_RING3 				= 0x60,

	GDTACCESS_READABLE 				= 0x02,
	GDTACCESS_WRITABLE 				= 0x02,
	GDTACCESS_CODECONFORMING 		= 0x04,
	GDTACCESS_DIRECTION_UP 			= 0x00,
	GDTACCESS_DIRECTION_DWN 		= 0x04,

	GDTACCESS_PRESENT 				= 0x80
}GDTACCESS;

typedef enum GDTFLAGS{
	GDTFLAGS_64B 					= 0x20,
	GDTFLAGS_32B					= 0x40,
	GDTFLAGS_16B					= 0x00,

	GDTFLAGS_GRAN1B					= 0x00,
	GDTFLAGS_GRAN4K					= 0x80,
}GDTFLAGS;

#define GDT_LIMITLOW(limit) LOW32((limit))
#define GDT_BASELOW(base) LOW32((base))
#define GDT_BASEMIDDLE(base) LOW16(((base) >> 16))
#define GDT_FLAGSLIMIT(limit, flags) ((((limit) >> 16) & 0xF) | ((flags) & 0xF0))
#define GDT_BASEHIGH(base) LOW16(((base) >> 24))

#define GDTENTRY(base, limit, access, flags){		\
	GDT_LIMITLOW((limit)),							\
	GDT_BASELOW((base)),							\
	GDT_BASEMIDDLE((base)),							\
	(access),										\
	GDT_FLAGSLIMIT((limit), (flags)),				\
	GDT_BASEHIGH((base))							\
}													\

extern void ASMCALL LoadGDT(gdtDESC_t *, uint16_t Code, uint16_t Data);
void ASMCALL getIDTDesc32(gdtDESC_t *out);