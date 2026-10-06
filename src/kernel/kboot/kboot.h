#pragma once

#include "src/Boot/UEFI/standard.h"
#include "src/Boot/UEFI/tools/tools.h"
#include "src/Boot/UEFI/drivers/executable/pe.h"
#include "src/Boot/UEFI/guid.h"

#include "kernel/libcrt/def.h"
#include "kernel/libcrt/hardware/IDT/APIC/APIC.h"
#include "kernel/libcrt/hardware/GDT/GDT.h"
#include "kernel/libcrt/hardware/IDT/IDT.h"
#include "kernel/libcrt/hardware/IDT/ISR.h"
#include "kernel/libcrt/hardware/ACPI/ACPI.h"
#include "kernel/libcrt/hardware/SSE/SSE.h"
#include "kernel/libcrt/hardware/paging/paging.h"
#include "kernel/libcrt/math/math.h"
#include "kernel/libcrt/memory/memory.h"
#include "kernel/libcrt/memory/string.h"

#ifdef __IMPORT
	#undef __IMPORT
#endif
#define __IMPORT
#include "kernel/services/Video/service.h"
#include "kernel/services/Video/controllers/Text.h"

#include "kernel/services/IO/service.h"

extern KernalMainDef(kboot);

#define bochs_breakpoint	__bochs_breakpoint
extern __sysvabi void __bochs_breakpoint(void);