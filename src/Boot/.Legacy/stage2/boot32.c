#include "boot.h"

LINKERSECTION("PROGHEADER") standardHeader boot2 = {
    .PAGES.CODE = &__CODEADDR,
    .PAGES.DATA = &__DATAADDR,
    .ID = {0},
    .program = NULLSTR(256),
    .standardChildren = {
        .stdfile = {
            .usedFiles = {0},
            .FAT = {0},
            .FATCHUNKS = 0,
            .files = {0},
        },
        .threads[0] = {0}
    }
};

idtENTRY_t LINKERSECTION("SymbolIDT") IDT[256] = {0};

gdtENTRY_t LINKERSECTION("SymbolGDT") GDT[16] = {
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
    GDTENTRY(
        0,
		0xFFFFF,
		GDTACCESS_PRESENT | GDTACCESS_SEGCODE | 
		GDTACCESS_RING0   | GDTACCESS_READABLE,
		GDTFLAGS_16B | GDTFLAGS_GRAN4K
	),
	// 32-Bit Data segment
	GDTENTRY(
        0,
		0xFFFFF,
		GDTACCESS_PRESENT | GDTACCESS_SEGDATA | 
		GDTACCESS_RING0   | GDTACCESS_WRITABLE,
		GDTFLAGS_16B | GDTFLAGS_GRAN4K
	),
	// 13 NULL segments
	GDTENTRY(0, 0, 0, 0), GDTENTRY(0, 0, 0, 0),
	GDTENTRY(0, 0, 0, 0), GDTENTRY(0, 0, 0, 0), GDTENTRY(0, 0, 0, 0), GDTENTRY(0, 0, 0, 0),
	GDTENTRY(0, 0, 0, 0), GDTENTRY(0, 0, 0, 0), GDTENTRY(0, 0, 0, 0), GDTENTRY(0, 0, 0, 0),
	GDTENTRY(0, 0, 0, 0)
};
const uint16_t GDTSize = sizeof(GDT) - 1, IDTSize = sizeof(IDT) - 1;
gdtDESC_t FORCE_SYMBOLEXPOSURE GDTdesc = {.table = GDT, .limit = sizeof(GDT) - 1};
idtDESC_t FORCE_SYMBOLEXPOSURE IDTdesc = {.table = IDT, .limit = sizeof(IDT) - 1};

void IRQHandlerFunc(InterruptFrame *IFrame){
    printf("\nInterrupt: %i, Error Code:\t%s", IFrame->interrupt, IFrame->error_code < 32? ERRORS[IFrame->error_code]: "");
    printf(
        "\nInterrupt Frame:"
        "\nds: %i, es: %i,"
        "\nedi: %i, esi: %i, ebp: %i, esp: %i"
        "\nebx: %i, edx: %i, ecx: %i, eax: %i"
        "\neip: %i, cs: %i, eflags: %i, Pre-Call esp: %i, ss: %i",
        *IFrame
    );
    return;
}

size_t timer = 0;
void timerPrint(InterruptFrame *IFrame){
	setColor(ANSI_RED);
	printf("Timer: %uxz,\t%udz", timer, timer);
}


void ASMCALL setup32(BootIn in){
    // Setup standard Program header
    boot2.PAGES.loadedCODEPages = (&__kernel_start - &__CODEEND) / (4*kB);
    boot2.PAGES.loadedCODEPages = (&__DATAADDR - &__DATAEND) / (4*kB);
    memcpy(&boot2.standardChildren.stdfile.drive, (void *)0x7C00, sizeof(driveHeader));
    systemState.Programs[0] = &boot2;
    systemState.loaded = 1;

	InitIDT(IDT, sizeof(IDT), i868GDT_SEGCODE);
    for(uint8_t cc =0; cc < PIC_MAXIRQ; ++cc){RegIRQHandler(cc, 0, IRQHandlerFunc);}
	RegIRQHandler(0, 0, timerPrint);
	printf(
		"Formatted 32-bit string: %a, %c, %h, %l, %i, %z, %s",
		(u8_t)99u, 'H', (short)88u, 66ul, (int)98u, 3334848348ull, "Look, it's a Negro"
	);
    halt32();
}
