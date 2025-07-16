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
    pci_meta_t header;
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

typedef enum REGOFFSET{
    REG1OFFSET 0x0,
    REG2OFFSET 0x4,
    REG3OFFSET 0x8,
    REG4OFFSET 0xC
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
// #pragma pack(push, 1)
// typedef struct RSDPDescriptor{
//     char     Signature[8];    // "RSD PTR "
//     uint8_t  Checksum;
//     char     OEMID[6];
//     uint8_t  Revision;
//     uint32_t RsdtAddress;     // Physical address of the RSDT
//     // ACPI 2.0+ fields follow here (Length, XsdtAddress, ExtendedChecksum, Reserved)
// } RSDPDescriptor;



typedef struct PCI_Header{
    uint16_t vendor_id, device_id, command, status;
    uint8_t revision_id, prog_if, subclass, base_class, cache_line_size, latency_timer, header_type, bist;
}PCI_Header;
#pragma pop(1)
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