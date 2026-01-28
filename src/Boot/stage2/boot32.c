#include "boot.h"
#include "./kernel/lib32/stdkernel/GDT/GDT.h"

static gdtENTRY_t GDT[] = {
    // NULL entry
    GDTENTRY(0, 0, 0, 0),
	// 32-bit Code Segment
	GDTENTRY(
		0,
		0xFFFFF,
		GDTACCESS_PRESENT | GDTACCESS_SEGCODE | 
		GDTACCESS_RING0   | GDTACCESS_READABLE,
		GDTFLAGS_32B | GDTFLAGS_GRAN4K
	),
	// 32-Bit Data segment
	GDTENTRY(
		0,
		0xFFFFF,
		GDTACCESS_PRESENT | GDTACCESS_SEGDATA | 
		GDTACCESS_RING0   | GDTACCESS_WRITABLE,
		GDTFLAGS_32B | GDTFLAGS_GRAN4K
	)
};

gdtDESC_t GDTdesc = {
	.address = GDT,
	.limit = sizeof(GDT) - 1
};

uint16_t gbootDrive = 0;

void main32(uint16_t bootDrive){
	gbootDrive = bootDrive;
	LoadGDT(&GDTdesc, i868GDT_SEGCODE, i868GDT_DATASEG);
	printf32(
		"Formatted 32-bit string: %a, %c, %h, %l, %i, %z, %s",
		(u8_t)99u, 'H', (short)88u, 66ul, (int)98u, 3334848348ull, "Look, it's a Negro"
	);
}
