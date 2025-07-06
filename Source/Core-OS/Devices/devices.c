#include "./Source/Core-OS/Devices/devices.h"
#include "./Source/Core-OS/Ports/Ports.h"

uint32_t pci_config_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (1U << 31)
        | ((uint32_t)bus << 16)
        | ((uint32_t)slot << 11)
        | ((uint32_t)func << 8)
        | (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void scan_pcie_devices() {
    int devicenum_ = 0; // Reset device count
    for (uint8_t bus = 0; bus < 256; ++bus) {
        for (uint8_t slot = 0; slot < 32; ++slot) {
            for (uint8_t func = 0; func < 8; ++func) {
                uint32_t data = pci_config_read32(bus, slot, func, 0x00);
                uint16_t vendor_id = data & 0xFFFF;
                if (vendor_id == 0xFFFF) continue; // No device
                uint16_t device_id = (data >> 16) & 0xFFFF;
                // Now you have bus, slot, func, vendor_id, device_id
                Devices[devicenum_] = (device_t){
                    .bus = bus,
                    .slot = slot,
                    .func = func,
                    .vendor_id = vendor_id,
                    .device_id = device_id
                };
                devicenum_++;
            }
        }
    }
}

// uint32_t bar0 = pci_config_read32(dev.bus, dev.slot, dev.func, 0x10);
// uint32_t base_address = bar0 & ~0xF; // Mask off the lower 4 bits for MMIO
/*
**Notes:**
- For I/O BARs, mask with `~0x3` instead.
- For 64-bit BARs, you need to read two BARs (BARn and BARn+1) and combine them.
- Always check the BAR type (memory or I/O) by examining the lowest bits.

**Summary:**  
- Read BAR register with `pci_config_read32(bus, slot, func, 0x10 + n*4)`
- Mask off the lower bits to get the actual base address.
*/