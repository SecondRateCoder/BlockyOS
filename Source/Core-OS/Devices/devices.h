#include "./Source/Public/Publics.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC
#define PCI_BAR0OFFSET   0x10
#define PCI_COMMANDOFFSET 0x04
#define PCI_STATUSOFFSET 0x06

#define device_t Device
device_t *Devices;
size_t devicenum;

typedef struct Device{
    uint8_t bus, slot, func;
    uint16_t vendor_id, device_id;
    size_t base_address, size;
} Device;

/*

*/