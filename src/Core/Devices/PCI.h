#include "./src/Core/General.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC
#define PCI_BAR0OFFSET   0x10
#define PCI_COMMANDOFFSET 0x04
#define PCI_STATUSOFFSET 0x06
#define PCI_CONFIGSIZE 256

// #define PCIe_CONFIGSIZE 4*1024
#define INVALID_DEVICE 0xFFFF
#define LENSOLVE8(h){(h.Length - sizeof(h))/8;}
#define LENSOLVE4(h){(h.Length - sizeof(h))/4;}

#pragma region COMMANDBITS
//Read-Write
#define COMMANDBITS_INTERRUPTDISABLEW(C, V) set((size_t)C, 10, V)
#define COMMANDBITS_INTERRUPTDISABLE(C) is_set((size_t)C, 10)
//Read-Only
#define COMMANDBITS_FASTBACK_TO_BACKENABLE(C) is_set((size_t)C, 9)
//Read-Write
#define COMMANDBITS_SERR_ENABLEW(C, V) set((size_t)C, 8, V)
#define COMMANDBITS_SERR_ENABLE(C) is_set((size_t)C, 8)
//Read-Only
#define COMMANDBITS_PARITYERROR_RESPONSE(C) is_set((size_t)C, 6)
//Read-Write
#define COMMANDBITS_VGAPALLETTE_SNOOPW(C, V) set((size_t)C, 5, V)
#define COMMANDBITS_VGAPALLETTE_SNOOP(C) is_set((size_t)C, 5)
//Read-Only
#define COMMANDBITS_MEMORYWRITE(C) is_set((size_t)C, 4)
//Read-Only
#define COMMANDBITS_INVALIDATE_ENABLE(C) is_set((size_t)C, 4)
//Read-Only
#define COMMANDBITS_SPECIALCYCLE(C) is_set((size_t)C, 3)
//Read-Write
#define COMMANDBITS_BUSMASTERW(C, V) set((size_t)C, 2, V)
#define COMMANDBITS_BUSMASTER(C) is_set((size_t)C, 2)
//Read-Write
#define COMMANDBITS_MEMORYSPACEW(C, V) set((size_t)C, 1, V)
#define COMMANDBITS_MEMORYSPACE(C) is_set((size_t)C, 1)
//Read-Write
#define COMMANDBITS_IO_SPACEW(C, V) set((size_t)C, 0, V)
#define COMMANDBITS_IO_SPACE(C) is_set((size_t)C, 0)
#pragma endregion

#pragma region STATUSBITS
/*
RW1C: Read-Write, 1 = Clear
Analogy: Imagine a row of warning lights on a dashboard. 
    When a warning light comes on (hardware sets the bit), 
        you can acknowledge and turn off only that specific light by pressing a button corresponding to it 
        (software writing a 1 to that bit's position), 
        without affecting other warning lights that might also be on.
*/

//Read-Write, 1 = Clear. Set if data corruption detected (From the Buffer.).
#define STATUSBITS_DETECTEDPARITY_ERRORC(S) set((size_t)S, 15, 1)
#define STATUSBITS_DETECTEDPARITY_ERROR(S) is_set((size_t)S, 15)
//Read-Write, 1 = Clear. System error or fatal hardware failure, relating to the PCI device, Connection or Driver.
#define STATUSBITS_SIGNALLEDSYSTEM_ERRORC(S) set((size_t)S, 14, 1)
#define STATUSBITS_SIGNALLEDSYSTEM_ERROR(S) is_set((size_t)S, 14)
//Read-Write, 1 = Clear. The Master Device(PC) termination signal recieved.
#define STATUSBITS_RECIEVED_MASTERABORTC(S) set((size_t)S, 13, 1)
#define STATUSBITS_RECIEVED_MASTERABORT(S) is_set((size_t)S, 13)
//Read-Write, 1 = Clear. "TARGET_ABORT" recieved.
#define STATUSBITS_RECIEVED_TARGETABORTC(S) set((size_t)S, 12, 1)
#define STATUSBITS_RECIEVED_TARGETABORT(S) is_set((size_t)S, 12)
//Read-Write, 1 = Clear. The PCI device terminated data transfer.
#define STATUSBITS_SIGNALLED_TARGETABORTC(S) set((size_t)S, 11, 1)
#define STATUSBITS_SIGNALLED_TARGETABORT(S) is_set((size_t)S, 11)
//Read-Only, Longest time it takes for PCI device to recognise any bus command(except Config space Read-Write), 0x0:FAST, 0x2:SLOW.
#define STATUSBITS_DESVEL_TIMING(S) ((uint8_t)((is_set((size_t)(S), 9) << 1) | (is_set((size_t)(S), 10) << 0)))
//Read-Write, 1 = Clear. Set if Data corruption recieved, device driver acted as the bus master for the Error-ed operation & "COMMANDBITS_PARITYERROR_RESPONSE" is set.
#define STATUSBITS_MASTERDATA_PARITYERRORC(S) set((size_t)S, 8, 1)
#define STATUSBITS_MASTERDATA_PARITYERROR(S) is_set((size_t)S, 8)
//Read-Only, can support data transfers from multiple sources.
#define STATUSBITS_FASTBACK_TO_BACKCAPABLE(S) is_set((size_t)S, 7)
//Read-Only, Runs at 66MHz. Otherwise, 33MHz.
#define STATUSBITS_66MHzCAPABLE(S) is_set((size_t)S, 5)
//Read-Only, Has Capabilities?
#define STATUSBITS_CAPABILITIES(S) is_set((size_t)S, 4)
//Read-Only, if set and "COMMANDBITS_INTERRUPTDISABLE" is !set, then the Interrupt goes through.
#define STATUSBITS_INTERRUPTSTATUS(S) is_set((size_t)S, 3)
#pragma endregion



// #define rsdt_t RSDT
// #define rsdtdesc_t RSDPDescriptor
// #define xsdt_t XSDT
#define pcih_t PCI_Header

#define device_t Device
#define pci_meta_t PCI_Header
device_t *Devices;
size_t devicenum;

typedef struct Device{
    uint8_t bus, slot, *func, numof_funcs;
    uint16_t vendor_id, device_id;
    size_t size;
    pci_meta_t *meta;
    /*
    uint32_t bar[6];           // Base Address Registers
    uint32_t cardbus_cis_ptr;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom_base_addr;
    uint8_t capabilities_ptr;
    uint8_t reserved[7];       // Reserved bytes
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint8_t min_gnt;
    uint8_t max_lat;
    */
} Device;

#pragma pack(push, 1)
//!MIGHT NOT SUPPORT Header Type 0x1
typedef struct PCI_Header{
    //
    uint16_t vendor_id, device_id, command, status;
    uint8_t revision_id, prog_if, subclass, base_class, cache_line_size, latency_timer, header_type, bist;
    // 
    #pragma region Type 0x0
    uint16_t subsystem_id, subsystem_vendor_id;
    // interrupt_line: Specifies what part of the Interrupt Line is used by the device(that uses an interrupt PIN), 0xFF:No Connection,
    // interrupt_pin: Which Interrupt PIN the Device uses, 0x0: NONE, 0x1: INTA#, 0x1: INTA#, 0x2: INTB#, 0x3: INTC#, 0x4: INTD#,
    // max_latency: Read-Only, how often the Device needs to read the PCI bus(in 1/4 microsecond units, 0.25secs),
    // min_grant: Read-Only, How long the Device takes to complete it's burst period: a series of data transfers that occur consecutively after an initial address phase(on 33MHz),
    // capabilities_pointer: The Offset of the function's Config space that points to a linked list of new capabilities set by the device(Only if "STATUSBITS_CAPABILITIES"is set.),
    uint8_t capabilities_pointer, max_latency, min_grant, interrupt_pin, interrupt_line;
    // cardbus_cis_pointer:Points to the Card-Information-Structure & used used by devices that use the same silicon between the CardBus & PCI,
    uint32_t BAR0, BAR1, BAR2, BAR3, BAR4, BAR5, cardbus_cis_pointer, expansion_ROM_base_address;
    #pragma endregion

    #pragma region Type 0x1
    //! DOES NOT USE: BAR2, BAR3, BAR4, BAR5, subsystem_id, max_latency, min_grant, cardbus_cis_pointer.
    uint8_t secondary_latency_timer, subordinate_bus_number, secondary_bus_number, primary_bus_number, IO_limit, IO_base;
    uint16_t secondary_status, memory_limit;
    #pragma endregion
}PCI_Header;
#pragma pop(1)

typedef enum REGOFFSET{
    REG1OFFSET 0x0,
    REG2OFFSET 0x4,
    REG3OFFSET 0x8,
    REG4OFFSET 0xC,
    REG5OFFSET 0x10,
    REG6OFFSET 0x14,
    REG7OFFSET 0x18,
    REG8OFFSET 0x1C,
    REG9OFFSET 0x20,
    REG10OFFSET 0x24,
    REG11OFFSET 0x2C,
    REG12OFFSET 0x30,
    REG13OFFSET 0x34,
    REG14OFFSET 0x38,
    REG15OFFSET 0x3C,
}REGOFFSET;

// typedef struct ACPISDTHeader {
//     char Signature[4];
//     uint32_t Length;
//     uint8_t Revision;
//     uint8_t Checksum;
//     char OEMID[6];
//     char OEMTableID[8];
//     uint32_t OEMRevision;
//     uint32_t CreatorID;
//     uint32_t CreatorRevision;
// }ACPISDTHeader;

// typedef struct RSDT {
//     struct ACPISDTHeader h;
//     uint32_t *PointerToOtherSDT;
// }RSDT;
// typedef struct XSDT {
//     struct ACPISDTHeader h;
//     uint64_t *PointerToOtherSDT;
// }XSDT;

// #pragma region PCI Metadata
// typedef struct RSDPDescriptor{
    //     char     Signature[8];    // "RSD PTR "
    //     uint8_t  Checksum;
    //     char     OEMID[6];
    //     uint8_t  Revision;
    //     uint32_t RsdtAddress;     // Physical address of the RSDT
    //     // ACPI 2.0+ fields follow here (Length, XsdtAddress, ExtendedChecksum, Reserved)
    // } RSDPDescriptor;
    
    
    
#pragma endregion



void link_devices(int Device_config[2]);
uint8_t calculate_checksum(const void* ptr, size_t length);
uint16_t pci_checkvendor(uint8_t bus, uint8_t slot);
uint16_t pciconfig_readword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_deviceconfigure(device_t *device);
void pci_baseaddressconfigure(device_t *device);
void scan_pci_devices();
uint32_t pciconfig_read32(device_t device, uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

/*
typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t revision_id;
    uint8_t prog_if;
    uint8_t subclass;
    uint8_t base_class;
    uint8_t cache_line_size;
    uint8_t latency_timer;
    uint8_t header_type; // Important for determining the rest of the struct
    uint8_t bist;
} __attribute__((packed)) pci_common_header_t;

// Header Type 0x00 specific fields
typedef struct {
    pci_common_header_t common;
    uint32_t bar[6];           // Base Address Registers
    uint32_t cardbus_cis_ptr;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom_base_addr;
    uint8_t capabilities_ptr;
    uint8_t reserved[7];       // Reserved bytes
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint8_t min_gnt;
    uint8_t max_lat;
} __attribute__((packed)) pci_device_header_0_t;
 */


 //Base_Address + (Bus_Number << 20) + (Device_Number << 15) + (Function_Number << 12) + Offset