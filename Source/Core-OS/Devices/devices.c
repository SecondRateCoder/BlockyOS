#include "./Source/Core-OS/Devices/devices.h"
#include "./Source/Core-OS/Ports/Ports.h"

uint16_t pciconfig_readword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint16_t tmp = 0;
  
    // Create configuration address as per Figure 1
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
  
    // Write out the address
    outl(0xCF8, address);
    // Read in the data
    // (offset & 2) * 8) = 0 will choose the first word of the 32-bit register
    tmp = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
    return tmp;
}
uint32_t pciconfig_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
	uint32_t address = (1U << 31)
		| ((uint32_t)bus << 16)
		| ((uint32_t)slot << 11)
		| ((uint32_t)func << 8)
		| (offset & 0xFC);
	outl(PCI_CONFIG_ADDRESS, address);
	return inl(PCI_CONFIG_DATA);
}

uint16_t pci_checkvendor(uint8_t bus, uint8_t slot) {
    uint16_t vendor, device;
    /* Try and read the first configuration register. Since there are no
     * vendors that == 0xFFFF, it must be a non-existent device. */
    if ((vendor = pciconfig_readword(bus, slot, 0, 0)) != 0xFFFF) {
       device = pciconfig_readword(bus, slot, 0, 2);
    } return (vendor);
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

void pcie_baseaddressconfigure(device_t *device){
	size_t bar0 = pci_config_read32(device->bus, device->slot, device->func, 0x10);
	if(bar0 & 0x1){device->base_address = bar0 & ~0x3;
	}else{device->base_address = bar0 & ~0xF;}
}


void pcie_deviceconfigurate(device_t *device){
	pcie_baseaddressconfigure(device);
	size_t command = (size_t)pci_config_read32(device->bus, device->slot, device->func, 0x04);
	command |= (1 << 2) | (1 << 1); // Set Bus Master and Memory Space Enable
	outl(PCI_CONFIG_ADDRESS, (1U << 31) | (device->bus << 16) | (device->slot << 11) | (device->func << 8) | 0x04);
	outl(PCI_CONFIG_DATA, command);
}