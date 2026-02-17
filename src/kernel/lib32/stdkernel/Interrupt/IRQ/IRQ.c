#include "IRQ.h"
#include "./kernel/lib32/stdio/stdio.h"
#include "./kernel/lib32/stdkernel/IO/IO.h"
#include "./kernel/lib32/stdkernel/stdkernel.h"

void RegIRQHandler(uint32_t irq, uint8_t ring, IRQHandler handler){
    idtDESC_t desc;
    getIDTDesc(&desc);
    if(irq > 0 && irq <= PIC_MAXIRQ){
        InitInterrupt((idtENTRY_t *)(desc.table), irq + PICB_REMAPOFFSET, true, handler, i868GDT_SEGCODE, 
        (ring == 0? IDTFLAGS_RING0: (ring == 1? IDTFLAGS_RING1: (ring == 2? IDTFLAGS_RING2: IDTFLAGS_RING3))) 
            | IDTFLAGS32B_INTRGATE | (ring > 0 && ring <= 3? IDTFLAGS_PRESENT: 0) | IDTFLAGS32B_INTRGATE);
    }
}

void IRQDefaultAction(InterruptFrame *in){
    // PRINTINTFRAME(in);
    PICSendSpecificEOI(in->interrupt - 0x20);
    return;
}

void IRQInit(){
    int32disable();
    PICInit(PICM_REMAPOFFSET, PICS_REMAPOFFSET);
    for(uint8_t cc =0; cc < PIC_MAXIRQ; ++cc){
        RegIRQHandler(PICB_REMAPOFFSET + cc, 0, IRQDefaultAction);
        PICMask(cc, true);
    }
    int32enable();
}

/// @brief Initialise the Master and Slave PIC
/// @param offsetPICM The offset from 0x20 of the Master PIC's expected Interrupt Line.
/// @param offsetPICS The offset from 0x20 of the Slave PIC's expected Interrupt Line.
void PICInit(uint8_t offsetPICM, uint8_t offsetPICS){
    // Init Control Word 1
    outb(PICM_PORTCMD, PIC_ICW1_ICW4 | PIC_ICW1_INIT);
    iowait();
    outb(PICS_PORTCMD, PIC_ICW1_ICW4 | PIC_ICW1_INIT);
    iowait();

    // Init Control Word 2; PIC Offsets
    outb(PICM_PORTDATA, offsetPICM);
    iowait();
    outb(PICS_PORTDATA, offsetPICS);
    iowait();

    // Init Control Word 3, Index of Slaves and Cascade IDs
    outb(PICM_PORTDATA, 0x04);
    iowait();
    outb(PICS_PORTDATA, 0x02);
    iowait();

    // Init Control Word 4
    outb(PICM_PORTDATA, PIC_ICW4_8086 | PIC_ICW4_SFNM);
    iowait();
    outb(PICS_PORTDATA, PIC_ICW4_8086 | PIC_ICW4_SFNM);
    iowait();
}

void PICMask(uint32_t irq, bool toggle){
    if(irq < 8){
        uint8_t mask = inb(PICM_PORTDATA) | (toggle << irq);
        outb(PICM_PORTDATA, mask);
    }else{
        uint8_t mask = inb(PICS_PORTDATA) | (toggle << (irq - 8));
        outb(PICS_PORTDATA, mask);
    }
}

void PICDisable(){
    outb(PICM_PORTDATA, 0xFF);
    iowait();
    outb(PICS_PORTDATA, 0xFF);
    iowait();
}

void ASMCALL PICSendSEOI(uint32_t irq){
    if(irq > 8){
        outb(PICS_PORTCMD, PIC_SPECEOI(irq - 1));
        outb(PICM_PORTCMD, PIC_SPECEOI(0x02));
    }else{outb(PICM_PORTCMD, PIC_SPECEOI(irq));}
}

PICOut PICReadIRQRequestReg(){
    outb(PICM_PORTCMD, PIC_CMD_READIRR);
    outb(PICS_PORTCMD, PIC_CMD_READIRR);
    // return ((uint16_t)inb(PICM_PORTCMD) << 8) | inb(PICS_PORTCMD);
    return (PICOut){.PICMOut = inb(PICM_PORTCMD), .PICSOut = inb(PICS_PORTCMD)};
}

PICOut PICReadIRQServiceReg(){
    outb(PICM_PORTCMD, PIC_CMD_READISR);
    outb(PICS_PORTCMD, PIC_CMD_READISR);
    // return ((uint16_t)inb(PICM_PORTCMD) << 8) | inb(PICS_PORTCMD);
    return (PICOut){.PICMOut = inb(PICM_PORTCMD), .PICSOut = inb(PICS_PORTCMD)};
}