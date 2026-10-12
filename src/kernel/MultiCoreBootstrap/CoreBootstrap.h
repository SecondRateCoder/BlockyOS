#include "kernel/libcrt/hardware/GDT/GDT.h"
#include "kernel/libcrt/hardware/IDT/IDT.h"

typedef volatile struct{
	//	Point to a valid Hang State/Boot Code.
	uint64_t	RIP;
	//	8
	uint16_t	CS, DS, SS, ES, FS, GS;
	//	20
	uint64_t	RFLAGS;
	//	28
	uint64_t	CR0, CR3, CR4;
	//	52
	GDTR64		GDTR;
	//	62
	LDTR64		LDTR;
	//	72
	IDTR64		IDTR;
	//	82
	bool		LA57;
}__packed BootstrapState_t;

uint64_t LoadBase;
// extern uint8_t __image_base__, __code_base__, __code_limit__, __data_base__, __data_limit__;