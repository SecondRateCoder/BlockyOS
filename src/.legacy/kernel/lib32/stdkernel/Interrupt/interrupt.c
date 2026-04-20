#include "interrupt.h"

const char *ERRORS[22] = {
    "Div Error",
    "Debug Break",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow Error",
    "Bound Range Exceeded Error",
    "Invalid Opcode Error",
    "Unavailable Device Error",
    "Double-Fault Exception",
    "Co-Processor Segment Overrun Error",
    "Invalid TSS Error",
    "Segment Not Present Error",
    "Stack Segment Fault Error",
    "General Protection Fault Error",
    "Page Fault Error",
    "RESERVED",
    "FPU Error",
    "Alignment Check Error",
    "Machine Check Error",
    "SIMD Floating-Point Exception Error",
    "Virtualisation Error",
    "Control Protection Exception Error",
};

void ASMCALL isr_inthandleC(InterruptFrame *IFrame){
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

void InitInterrupt(uint32_t interrupt, bool present, void *base, uint16_t segDesc, uint8_t flags){
    idtDESC_t desc;
    getIDTDesc(&desc);
    if(interrupt > 0 && interrupt < 256){
        ((idtENTRY_t *)(desc.table))[interrupt] = (idtENTRY_t){
            .baseLow = (uint32_t)base & 0xFFFF,
            .segDesc = segDesc,
            .reserved = 0,
            .flags = flags | (present? IDTFLAGS_PRESENT: 0),
            .baseLow = ((uint32_t)base >> 16) & 0xFFFF
        };
    }
}

void ToggleInterrupt(uint8_t interrupt, bool present){
    idtDESC_t desc;
    getIDTDesc(&desc);
    if(interrupt > 0 && interrupt < 256){
        if(present){((idtENTRY_t *)(desc.table))[interrupt].flags |= IDTFLAGS_PRESENT;}
        else{((idtENTRY_t *)(desc.table))[interrupt].flags &= ~IDTFLAGS_PRESENT;}
    }
}