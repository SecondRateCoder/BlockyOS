#pragma once

#include "kernel/lib32/generic/standard.h"
#include "InterruptRoutines.h"
#include "kernel/lib32/stdio/stdio.h"


/*
	The IDT table is in the format that there are auto-generated stube which lead to the generic Handler,
	But it also allows for re-assigning Interrupts. e.g for the soon to be Dynamic Interrupt buffer that will allow
		a Parent Kernel Thread to load and unload infrequently-used or patching Interrupt Binaries to be used for a specific Program rather than the conventional BlockyOS interrupts.

*/

extern const char *ERRORS[22];

typedef void ASMCALL (*interruptEntry)();
extern interruptEntry interruptTable[256];
extern PACKEDSTRUCT uint8_t interruptTableEnd;
extern uint32_t interruptTableLength;

typedef struct idtENTRY_t{
	uint16_t baseLow;
	uint16_t segDesc;
	uint8_t reserved;
	uint8_t flags;
	uint16_t baseHigh;
}PACKEDSTRUCT idtENTRY_t;

typedef struct idtDESC_t{
	uint16_t limit;
	idtENTRY_t *table;
}PACKEDSTRUCT idtDESC_t;

#define PRINTINTFRAME(FRAME) printf(        			\
	"Interrupt Frame:"                      			\
	"   SS:             %ul"                			\
	"   CPU-esp:        %ul"                			\
	"   EFlags:         %ul"                			\
	"   Code Segment:   %ul"                			\
	"   EIP:            %ul"                			\
	"   Error-Code:     %ul"                			\
	"   Interrupt:      %ul"                			\
	"   EAX:            %ul"                			\
	"   ECX:            %ul"                			\
	"   EDX:            %ul"                			\
	"   EBX:            %ul"                			\
	"   ESP:            %ul"                			\
	"   EBP:            %ul"                			\
	"   ESI:            %ul"                			\
	"   EDI:            %ul"                			\
	"   ES:             %ul"                			\
	"   DS:             %ul",               			\
	(sizeof(FRAME) == sizeof(void *)? (*FRAME): FRAME) 	\
);

typedef enum IDTFLAGS{
	IDTFLAGS_TSKGATE = 0x5,
	IDTFLAGS16B_INTRGATE = 0x6,
	IDTFLAGS16B_TRPGATE = 0x7,
	IDTFLAGS32B_INTRGATE = 0xE,
	IDTFLAGS32B_TRPGATE = 0xF,

	IDTFLAGS_RING0 =     (0 << 5),
	IDTFLAGS_RING1 =     (1 << 5),
	IDTFLAGS_RING2 =     (2 << 5),
	IDTFLAGS_RING3 =     (3 << 5),
	
	IDTFLAGS_PRESENT = 0x80
}IDTFLAGS;

typedef struct InterruptFrame{
	uint32_t ds, es;
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
	uint32_t interrupt, error_code;
	uint32_t eip, cs, eflags, cpu_esp, ss;
	uint32_t stack[];
}PACKEDSTRUCT InterruptFrame;

void InitIDT(idtENTRY_t *IDT, uint16_t limit, uint16_t CodeSegment);
void ASMCALL int32enable(void);
void ASMCALL int32disable(void);

extern void ASMCALL LoadIDT(idtDESC_t *ptr);
void ASMCALL isr_inthandleC(InterruptFrame *IFrame);
extern void ASMCALL getIDTDesc32(idtDESC_t *out);
#define getIDTDesc getIDTDesc32

void ToggleInterrupt(uint8_t interrupt, bool present);
void InitInterrupt(
	uint32_t interrupt, 
	bool present,
	void *base,
	uint16_t segDesc,
	uint8_t flags
);
