#include "Interrupt/interrupt.h"
#include "GDT/GDT.h"
#include "IO/IO.h"

#define i868GDT_SEGCODE 0x08
#define i868GDT_SEGDATA 0x10