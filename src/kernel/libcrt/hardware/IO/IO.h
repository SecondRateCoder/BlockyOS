#pragma once

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"

LibAPI extern void __sysvabi __outb(const uint16_t port, const uint8_t value);
LibAPI extern uint8_t __sysvabi __inb(const uint16_t port);

LibAPI extern void __sysvabi __outw(const uint16_t port, const uint16_t value);
LibAPI extern uint16_t __sysvabi __inw(const uint16_t port);

LibAPI extern void __sysvabi __outl(const uint16_t port, const uint32_t value);
LibAPI extern uint32_t __sysvabi __inl(const uint16_t port);

LibAPI extern void __sysvabi __outsb(const uint16_t port, const uint8_t *data, const uint32_t count);
LibAPI extern uint64_t __sysvabi __insb(const uint16_t port, const uint8_t *data, const uint32_t count);

LibAPI extern void __sysvabi __outsw(const uint16_t port, const uint16_t *data, const uint32_t count);
LibAPI extern uint64_t __sysvabi __insw(const uint16_t port, const uint16_t *data, const uint32_t count);

LibAPI extern void __sysvabi __outsl(const uint16_t port, const uint32_t *data, const uint32_t count);
LibAPI extern uint64_t __sysvabi __insl(const uint16_t port, const uint32_t *data, const uint32_t count);