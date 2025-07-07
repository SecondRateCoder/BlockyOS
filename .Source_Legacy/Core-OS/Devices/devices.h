#include "./Source/Public/Publics.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC
#define PCI_BAR0OFFSET   0x10
#define PCI_COMMANDOFFSET 0x04
#define PCI_STATUSOFFSET 0x06
#define PCI_CONFIGSIZE 256
#define PCIe_CONFIGSIZE 4*1024

#define device_t Device
#define pci_meta_t PCI_Header
device_t *Devices;
size_t devicenum;

typedef struct Device{
    uint8_t bus, slot, func;
    uint16_t vendor_id, device_id;
    size_t base_address, size;
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


#pragma region PCI Metadata
#pragma pack(push, 1)
typedef struct PCI_Header{
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
}PCI_Header;
#pragma pop(1)
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
