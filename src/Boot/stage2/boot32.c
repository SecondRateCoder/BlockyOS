#include "boot.h"

static drive_header LINKERSECTION("DRIVEHEADER") drive;

static IDTentry LINKERSECTION("IDT") IDT[256];

static stdfileENVIROMENT LINKERSECTION("FILES") fileENVIROMENT;

static gdtENTRY_t LINKERSECTION("GDT") GDT[16] = {
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
	),
	// 13 NULL segments
	GDTENTRY(0, 0, 0, 0), GDTENTRY(0, 0, 0, 0), GDTENTRY(0, 0, 0, 0), GDTENTRY(0, 0, 0, 0),
	GDTENTRY(0, 0, 0, 0), GDTENTRY(0, 0, 0, 0), GDTENTRY(0, 0, 0, 0), GDTENTRY(0, 0, 0, 0),
	GDTENTRY(0, 0, 0, 0), GDTENTRY(0, 0, 0, 0), GDTENTRY(0, 0, 0, 0), GDTENTRY(0, 0, 0, 0),
	GDTENTRY(0, 0, 0, 0)
};

gdtDESC_t GDTdesc = {
	.address = GDT,
	.limit = sizeof(GDT) - 1
};

size_t timer = 0;
void timerPrint(InterruptFrame *IFrame){
	setColor(ANSI_RED);
	printf("Timer: %uxz", timer);
	printf("Timer: %udz", timer);
}

void setup32(){
	LoadGDT(&GDTdesc, i868GDT_SEGCODE, i868GDT_SEGDATA);
	InitIDT(IDT, i868GDT_SEGCODE);
	RegIRQHandler(IDT, 0, 0, timerPrint);
	printf32(
		"Formatted 32-bit string: %a, %c, %h, %l, %i, %z, %s",
		(u8_t)99u, 'H', (short)88u, 66ul, (int)98u, 3334848348ull, "Look, it's a Negro"
	);
}
