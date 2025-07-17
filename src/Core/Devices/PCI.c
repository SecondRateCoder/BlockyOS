#include "./src/Core/Devices/PCI.h"

void MMIO_configure(device_t *dev){
    uint32_t *BAR = dev->meta->BAR;
    
}
void enableaddr_read(const device dev){
    uint8_t new_command =0;
    COMMANDBITS_MEMORYSPACEW(new_command, 0);
    COMMANDBITS_IO_SPACEW(new_command, 0);
    COMMANDBITS_BUSMASTERW(new_command, 0);
    device_write32(dev, 0x4, uintconv8_16(dev->meta->status, new_command));
}
void disableaddr_read(const device dev){
    uint8_t new_command =0;
    COMMANDBITS_MEMORYSPACEW(new_command, 1);
    COMMANDBITS_IO_SPACEW(new_command, 1);
    COMMANDBITS_BUSMASTERW(new_command, 1);
    device_write32(dev, 0x4, uintconv8_16(dev->meta->status, new_command));
}
bool device_alloca(const device_t dev, uint8_t baroffset){
    enableaddr_read(dev);
    uint32_t obar = device_read32(dev, baroffset), nbar =0;
    device_write32(dev, baroffset, 0xFFFFFFFF);
    nbar = device_read32(dev, baroffset);
    device_write32(dev, baroffset, obar);
    size_t size;
    uint64_t base_address;
    switch(nbar & 0x1){
        case 0:
            uint8_t mem_type = (bar_value >> 1) & 0x3;
            if(mem_type == 0x2){
                uint32_t high_bar_value = pci_config_read32(bus, dev, func, bar_offset + 4); // Read next BAR

                // Calculate size for 64-bit BAR
                // Invert the bits that were readable (excluding type/control bits) and add 1
                size = ~(((uint64_t)bar_value & 0xFFFFFFF0) | ((uint64_t)high_bar_value << 32)) + 1;
                // The base address is the original_bar_value with the lowest 4 bits masked out
                // base_address = ((uint64_t)original_bar_value & 0xFFFFFFF0) | ((uint64_t)pci_config_read32(bus, dev, func, bar_offset + 4) << 32);
            }else{
                size = (~(bar_value & 0xFFFFFFF0)) + 1; // Mask out lower 4 bits (type/control)
                // The base address is the original_bar_value with the lowest 4 bits masked out
                // base_address = original_bar_value & 0xFFFFFFF0;
            }
            break;
        
        default:
            return false;
    }
    device_write32(dev, baroffset, (size_t)alloca(size, KERNEL_ID));
    disableaddr_read(dev);
}
void scan_pci_devices(){
    int devicenum_ = 0; // Reset device count
    for (uint8_t bus = 0; bus < 256; ++bus) {
        for (uint8_t slot = 0; slot < 32; ++slot) {
            uint8_t func =0;
            for (; func < 8; ++func) {
                uint32_t data = pci_config_read32(bus, slot, func, 0x00);
                uint16_t vendor_id = data & BIT16_MASK;
                if (vendor_id == BIT16_MASK) continue; // No device
                uint16_t device_id = (data >> 16) & BIT16_MASK;
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
void device_metaconfig(device_t *dev){
    //Needs to read 4 32bit values.
    uint32_t h[15];
    h[0] = device_read32(dev, REG1OFFSET);
    h[1] = device_read32(dev, REG2OFFSET);
    h[2] = device_read32(dev, REG3OFFSET);
    h[3] = device_read32(dev, REG4OFFSET);
    h[4] = device_read32(dev, REG5OFFSET);
    h[5] = device_read32(dev, REG6OFFSET);
    h[6] = device_read32(dev, REG7OFFSET);
    h[7] = device_read32(dev, REG8OFFSET);
    h[8] = device_read32(dev, REG9OFFSET);
    h[9] = device_read32(dev, REG10OFFSET);
    h[10] = device_read32(dev, REG11OFFSET);
    h[11] = device_read32(dev, REG12OFFSET);
    h[12] = device_read32(dev, REG13OFFSET);
    h[14] = device_read32(dev, REG15OFFSET);
    dev->meta = (pcih_t *){
        .vendor_id =(uint16_t)(h[0] & BIT16_MASK),
        .device_id =(uint16_t)((h[0] >> 16) & BIT16_MASK),
        .command =(uint16_t)(h[1] & BIT16_MASK),
        .status =(uint16_t)((h[1] >> 16) & BIT16_MASK),
        .revision_id =(uint8_t)(h[2] & BIT8_MASK),
        .prog_if =(uint8_t)((h[2] >> 8) & BIT8_MASK),
        .subclass =(uint8_t)((h[2] >> 16) & BIT8_MASK),
        .base_class =(uint8_t)((h[2] >> 24) & BIT8_MASK),
        .cache_line_size =(uint8_t)(h[3] & BIT8_MASK),
        .latency_timer =(uint8_t)((h[3] >> 8) & BIT8_MASK),
        .header_type =(uint8_t)((h[3] >> 16) & BIT8_MASK),
        .bist =(uint8_t)((h[3] >> 24) & BIT8_MASK),
        if(dev->meta.header_type & 0xF == 0x0){
            #pragma region Type 0x0
            .BAR[0] = h[4],
            .BAR[1] = h[5],
            .BAR[2] = h[6],
            .BAR[3] = h[7],
            .BAR[4] = h[8],
            .BAR[5] = h[9],
            //Disable Memory & IO write.
            uint8_t new_command =0,
            COMMANDBITS_MEMORYSPACEW(new_command, 0),
            COMMANDBITS_IO_SPACEW(new_command, 0),
            device_write32(dev, 0x4, uintconv8_16(dev->meta.status, new_command)),
            for(int cc =0; cc < BARSIZE, ++cc){.BAR[cc] = ((BAR[cc] & 0xFFFFFFF0) + (BAR[cc+1 >= BARSIZE? BARSIZE-1: cc+1] & 0xFFFFFFFF)) << 32,},
            .subsystem_id =(uint16_t)(h[10] & BIT16_MASK),
            .subsystem_vendor_id =(uint16_t)((h[10] >> 16) & BIT16_MASK),
            .expansion_ROM_base_address = h[11],
            .capabilities_pointer =(uint8_t)((h[12] >> 24) & BIT8_MASK),
            .max_latency =(uint8_t)((h[14]) & BIT8_MASK),
            .min_grant =(uint8_t)((h[14] >> 8) & BIT8_MASK),
            .interrupt_pin =(uint8_t)((h[14] >> 16) & BIT8_MASK),
            .interrupt_line =(uint8_t)((h[14] >> 24) & BIT8_MASK),
            #pragma endregion
        }else if(dev->meta.header_type & 0xF == 0x1){
            #pragma region Type 0x1
            .BAR[0] =h[4],
            .BAR[1] =h[5],
            .secondary_latency_timer =(uint8_t)(h[6] & BIT8_MASK),
            .subordinate_bus_number =(uint8_t)((h[6] >> 8) & BIT8_MASK),
            .secondary_bus_number =(uint8_t)((h[6] >> 16) & BIT8_MASK),
            .primary_bus_number =(uint8_t)((h[6] >> 24) & BIT8_MASK),
            .secondary_status =(uint16_t)(h[7] & BIT16_MASK),
            .IO_limit =(uint8_t)((h[7] >> 16) & BIT8_MASK),
            .IO_base =(uint8_t)((h[7] >> 24) & BIT8_MASK),
            .memory_limit =(uint16_t)(h[8] & BIT16_MASK),
            .memory_base =(uint16_t)((h[8] >> 16) & BIT16_MASK),
            .prefetch_memory_limit =(uint16_t)((h[9]) & BIT16_MASK),
            .prefetch_memory_base =(uint16_t)((h[9] >> 16) & BIT16_MASK),
            .prefetch_base_upper_32_bits =h[10],
            .prefetch_limit_upper_32_bits =h[11],
            .IO_limit_upper_16bits = (uint16_t)(h[12] & BIT16_MASK),
            .IO_base_upper_16bits =(uint16_t)((h[12] >> 16) & BIT16_MASK),
            .capabilities_pointer =(uint8_t)((h[13] >> 24) & BIT8_MASK),
            .expansion_ROM_base_address =h[14],
            .bridge_control =(uint16_t)(h[15] & BIT16_MASK),
            .interrupt_pin =(uint8_t)((h[15] >> 16) & BIT8_MASK),
            .interrupt_line =(uint8_t)((h[15] >> 24) & BIT8_MASK)
            #pragma endregion
        },
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
//     tmp = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & BIT16_MASK);
//     return tmp;
// }

// uint16_t pci_checkvendor(uint8_t bus, uint8_t slot){
//     uint16_t vendor, device;
//     /* Try and read the first configuration register. Since there are no
//     * vendors that == BIT16_MASK, it must be a non-existent device. */
//     if ((vendor = pciconfig_readword(bus, slot, 0, 0)) != BIT16_MASK) {
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
    
//     // 2. Search in BIOS ROM area (0xE0000 to BIT16_MASKF)
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