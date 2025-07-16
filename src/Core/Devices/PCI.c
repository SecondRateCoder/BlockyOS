#include "./src/Core/Devices/PCI.h"

void scan_pci_devices(){
    int devicenum_ = 0; // Reset device count
    for (uint8_t bus = 0; bus < 256; ++bus) {
        for (uint8_t slot = 0; slot < 32; ++slot) {
            uint8_t func =0;
            for (; func < 8; ++func) {
                uint32_t data = pci_config_read32(bus, slot, func, 0x00);
                uint16_t vendor_id = data & 0xFFFF;
                if (vendor_id == 0xFFFF) continue; // No device
                uint16_t device_id = (data >> 16) & 0xFFFF;
                // Now you have bus, slot, func, vendor_id, device_id
                Devices[devicenum_] = (device_t){
                    .bus = bus,
                    .slot = slot,
                    .func[func] = func,
                    .vendor_id = vendor_id,
                    .device_id = device_id
                };
                Devices[devicenum_].meta = device_readmeta(Devices[devicenum_]);
                Devices[devicenum_].numof_funcs = func;
            }
            devicenum_++;
        }
    }
}

uint32_t pci_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset){
    uint32_t addr = (uint32_t)(
        (1U << 31)|
        ((uint32_t)bus << 16)|
        ((uint32_t)slot << 11)|
        ((uint32_t)func << 8)|
        (offset & 0xFC)|
        ((uint32_t)0x80000000));
    outl(addr+PCI_CONFIG_ADDRESS, addr);
    return inl(addr+PCI_CONFIG_DATA);
}
uint32_t device_read32(const device_t dev, uint8_t offset){
    uint32_t addr = (uint32_t)(
        (1U << 31)|
        ((uint32_t)dev.bus << 16)|
        ((uint32_t)dev.slot << 11)|
        ((uint32_t)dev.func << 8)|
        (offset & 0xFC)|
        ((uint32_t)0x80000000));
    outl(addr+PCI_CONFIG_ADDRESS, addr);
    return inl(addr+PCI_CONFIG_DATA);
}

void pci_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value){
    uint32_t addr = (uint32_t)(
        (1U << 31)|
        ((uint32_t)bus << 16)|
        ((uint32_t)slot << 11)|
        ((uint32_t)func << 8)|
        (offset & 0xFC)|
        ((uint32_t)0x80000000));
    outl(addr, value);
    return;
}
void device_write32(const device_t dev, uint8_t offset, uint32_t value){
    uint32_t addr = (uint32_t)(
        (1U << 31)|
        ((uint32_t)dev.bus << 16)|
        ((uint32_t)dev.slot << 11)|
        ((uint32_t)dev.func << 8)|
        (offset & 0xFC)|
        ((uint32_t)0x80000000));
    outl(addr, value);
    return;
}

uint16_t pci_vendorcheck(uint16_t bus, uint16_t slot){
    uint16_t vendor = (uint16_t)pci_read32(bus, slot, 0, 0);
    if(vendor != INVLAID_DEVICE){pci_read32(bus, slot, 0, 2);}
    return vendor;
}
uint16_t device_vendorcheck(const device_t dev){
    uint16_t vendor = (uint16_t)pci_read32(dev.bus, dev.slot, 0, 0);
    if(vendor != INVLAID_DEVICE){pci_read32(dev.bus, dev.slot, 0, 2);}
    return vendor;
}

//Return a 128 byte buffer.
pcih_t device_readmeta(const device_t dev){
    //Needs to read 4 32bit values.
    uint32_t h[4];
    h[0] = device_read32(dev, REG1OFFSET);
    h[1] = device_read32(dev, REG2OFFSET);
    h[2] = device_read32(dev, REG3OFFSET);
    h[3] = device_read32(dev, REG4OFFSET);
    return (pcih_t){
        .vendor_id =(uint16_t)(h[0] & 0xFFFF),//uint16_t ->
        .device_id =(uint16_t)((h[0] >> 16) & 0xFFFF),
        .command =(uint16_t)(h[1] & 0xFFFF),
        .status =(uint16_t)((h[1] >> 16) & 0xFFFF),//END.
        .revision_id =(uint8_t)(h[2] & 0xFF),//uint8_t ->
        .prog_if =(uint8_t)((h[2] >> 8) & 0xFF),
        .subclass =(uint8_t)((h[2] >> 16) & 0xFF),
        .base_class =(uint8_t)((h[2] >> 24) & 0xFF),
        .cache_line_size =(uint8_t)(h[3] & 0xFF),
        .latency_timer =(uint8_t)((h[3] >> 8) & 0xFF),
        .header_type =(uint8_t)((h[3] >> 16) & 0xFF),
        .bist =(uint8_t)((h[3] >> 24) & 0xFF)//END.
    };
}

// void pci_baseaddressconfigure(device_t *device){
//     /*
//     PhysicalAddress = MCFG_BASE_ADDRESS +
//                   (BusNumber << 20) +       // Shift by 20 bits for bus
//                   (DeviceNumber << 15) +    // Shift by 15 bits for device
//                   (FunctionNumber << 12) +  // Shift by 12 bits for function
//                   RegisterOffset            // Offset of the desired register
//     */
//     size_t bar0 = pci_config_read32(device->bus, device->slot, device->func, PCI_BAR0OFFSET);
// 	if(bar0 & 0x1){device->base_address = bar0 & ~0x3;
// 	}else{device->base_address = bar0 & ~0xF;}
// }

// void pci_deviceconfigure(device_t *device){
//     pci_baseaddressconfigure(device);
// 	size_t command = (size_t)pci_config_read32(device->bus, device->slot, device->func, PCI_COMMANDOFFSET);
// 	command |= (1 << 2) | (1 << 1); // Set Bus Master and Memory Space Enable
// 	outl(device->base_address+PCI_CONFIG_ADDRESS, (1U << 31) | (device->bus << 16) | (device->slot << 11) | (device->func << 8) | 0x04);
// 	outl(device->base_address+PCI_CONFIG_DATA, command);
// }

// #pragma region PCI Config access
// //Read a 16 byte buffer from a pci device's MMIO
// uint16_t pciconfig_readword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset){
//     uint32_t address;
//     uint32_t lbus  = (uint32_t)bus;
//     uint32_t lslot = (uint32_t)slot;
//     uint32_t lfunc = (uint32_t)func;
//     uint16_t tmp = 0;
  
//     // Create configuration address as per Figure 1
//     address = (uint32_t)((lbus << 16) | (lslot << 11) |
//               (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

//     // Write out the address
//     outl(0xCF8, address);
//     // Read in the data
//     // (offset & 2) * 8) = 0 will choose the first word of the 32-bit register
//     tmp = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
//     return tmp;
// }

// uint16_t pci_checkvendor(uint8_t bus, uint8_t slot){
//     uint16_t vendor, device;
//     /* Try and read the first configuration register. Since there are no
//     * vendors that == 0xFFFF, it must be a non-existent device. */
//     if ((vendor = pciconfig_readword(bus, slot, 0, 0)) != 0xFFFF) {
//        device = pciconfig_readword(bus, slot, 0, 2);
//     } return (vendor);
// }

// // Function to calculate checksum
// uint8_t calculate_checksum(const void* ptr, size_t length){
//     uint8_t sum = 0;
//     const uint8_t* bytes = (const uint8_t*)ptr;
//     for (size_t i = 0; i < length; ++i) {
//         sum += bytes[i];
//     }
//     return sum;
// }

// // Function to find RSDP for BIOS systems
// RSDPDescriptor* find_rsdp_bios(){
//     const char* rsdp_signature = "RSD PTR ";
//     RSDPDescriptor* rsdp = NULL;
    
//     // 1. Search in EBDA
//     // In real mode/early protected mode, you'd access 0x40E directly.
//     // This is highly platform-specific and requires careful memory mapping if in protected mode.
//     // For simplicity, let's assume direct physical access (DANGEROUS for real OS without mapping)
//     uint16_t ebda_segment_ptr = *(volatile uint16_t*)0x40E;
//     uint32_t ebda_start_address = (uint32_t)ebda_segment_ptr << 4; // Segment * 16
    
//     for (uint32_t addr = ebda_start_address; addr < ebda_start_address + 1024; addr += 16) {
//         RSDPDescriptor* current_rsdp = (RSDPDescriptor*)addr; // Treat memory as RSDP structure
//         if (memcmp(current_rsdp->Signature, rsdp_signature, 8) == 0) {
//             // Check ACPI 1.0 checksum
//             if (calculate_checksum(current_rsdp, 20) == 0) { // ACPI 1.0 RSDP is 20 bytes
//                 return current_rsdp;
//             }
//             // If ACPI 2.0+, also check extended checksum
//             if (current_rsdp->Revision >= 2) {
//                 // For ACPI 2.0, the length field is valid
//                 typedef struct {
//                     RSDPDescriptor v1_part;
//                     uint32_t Length;
//                     uint64_t XsdtAddress;
//                     uint8_t  ExtendedChecksum;
//                     uint8_t  Reserved[3];
//                 } RSDPDescriptor_20;
//                 uint32_t total_length = ((RSDPDescriptor_20 *)current_rsdp)->Length;
//                 if (calculate_checksum(current_rsdp, total_length) == 0) {return current_rsdp;}
//             }
//         }
//     }
    
//     // 2. Search in BIOS ROM area (0xE0000 to 0xFFFFF)
//     for (uint32_t addr = 0xE0000; addr < 0x100000; addr += 16) { // 0x100000 is 1MB
//         RSDPDescriptor* current_rsdp = (RSDPDescriptor*)addr;
//         if (memcmp(current_rsdp->Signature, rsdp_signature, 8) == 0) {
//             // Check ACPI 1.0 checksum
//             if (calculate_checksum(current_rsdp, 20) == 0) {return current_rsdp;}
//             // If ACPI 2.0+, also check extended checksum
//             if (current_rsdp->Revision >= 2) {
//                 // Need a separate struct for ACPI 2.0 RSDP to access 'Length'
//                 typedef struct RSDPDescriptor_20{
//                     RSDPDescriptor v1_part;
//                     uint32_t Length;
//                     uint64_t XsdtAddress;
//                     uint8_t  ExtendedChecksum;
//                     uint8_t  Reserved[3];
//                 }RSDPDescriptor_20;
//                 uint32_t total_length = ((RSDPDescriptor_20*)current_rsdp)->Length;
//                 if (calculate_checksum(current_rsdp, total_length) == 0) {return current_rsdp;}
//             }
//         }
//     }
    
//     return NULL; // RSDP not found
// }

// void *findFACP(void *RootSDT){
//     rsdt_t *rsdt = (RSDT *)RootSDT;
//     int entries = (rsdt->h.Length - sizeof(rsdt->h)) / 4;
//     for (int i = 0; i < entries; i++){
//         ACPISDTHeader *h = (ACPISDTHeader *) rsdt->PointerToOtherSDT[i];
//         if (!strcompare_l(h->Signature, "FACP", 4)){return (void *) h;}
//     }
//         // No FACP found
//         return NULL;
// }
// #pragma endregion