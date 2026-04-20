#include "stdkernel.h"

void LoadSTDKernelState(gdtDESC_t *inGDT, idtDESC_t *inIDT, BootIn in, uint8_t PIC1InterruptLine, uint8_t PIC2InterruptLine){
    InitIDT((idtENTRY_t *)(inIDT->table), inIDT->limit, i868GDT_SEGCODE);
    PICInit(PIC1InterruptLine - 0x20, PIC2InterruptLine - 0x20);
    LoadGDT(inGDT, i868GDT_SEGCODE, i868GDT_SEGDATA);
    return;
}