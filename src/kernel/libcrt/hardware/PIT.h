#pragma once

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"
#include "kernel/libcrt/hardware/IDT/ISR.h"
#include "kernel/libcrt/hardware/GDT/GDT.h"
#include "kernel/libcrt/hardware/IDT/APIC/LocalAPIC.h"
#include "kernel/libcrt/hardware/IDT/APIC/IOAPIC.h"
#include "kernel/libcrt/hardware/IO/IO.h"

extern volatile uint128_t pit_ticks;

// Call this inside your IRQ0 handler routine
LibAPI ISRCallbackDefinition(_PITTick);
LibAPI void InitializePIT(void *acpibase, uint8_t IOAPIC, uint8_t LocalAPIC, uint32_t Priviledge);
LibAPI void SleepMS(uint32_t milliseconds);
LibAPI inline void SleepS(uint32_t seconds);