#include "Interrupt.h"
#include "GDT.h"

void __attribute__((cdecl)) isr_handlerC(){return;}

void SetInterrupt(int interrupt, bool present, void *base, uint16_t segDesc, uint8_t flags){
    if(interrupt > 0 && interrupt < 256){
        IDT[interrupt] = (IDTentry){
            .baseLow = (uint32_t)base & 0xFFFF,
            .segDesc = segDesc,
            .reserved = 0,
            .flags = flags | (present? IDTFLAG_PRESENT: 0),
            .baseLow = ((uint32_t)base >> 16) & 0xFFFF
        }
    }
}

void ToggleInterrupt(uint8_t interrupt, bool present){
    if(interrupt > 0 && interrupt < 256){
        if(present){IDT[interrupt].flag |= IDTFLAG_PRESENT;}
        else{IDT[interrupt].flag &= ~IDTFLAG_PRESENT;}
    }
}
