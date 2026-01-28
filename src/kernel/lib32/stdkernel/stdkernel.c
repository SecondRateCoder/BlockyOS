#include "IDT/interrupt.h"
#include "GDT/GDT.h"

void __attribute__((cdecl)) isr_handlerC(InterruptFrame *IFrame){return;}

void __attribute__((cdecl)) InitInterrupt(uint32_t interrupt, bool present, void *base, uint16_t segDesc, uint8_t flags){
    if(interrupt > 0 && interrupt < 256){
        IDT[interrupt] = (IDTentry){
            .baseLow = (uint32_t)base & 0xFFFF,
            .segDesc = segDesc,
            .reserved = 0,
            .flags = flags | (present? IDTFLAGS_PRESENT: 0),
            .baseLow = ((uint32_t)base >> 16) & 0xFFFF
        };
    }
}

void ToggleInterrupt(uint8_t interrupt, bool present){
    if(interrupt > 0 && interrupt < 256){
        if(present){IDT[interrupt].flags |= IDTFLAGS_PRESENT;}
        else{IDT[interrupt].flags &= ~IDTFLAGS_PRESENT;}
    }
}
