#include "./Source/Public/Publics.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

#define device_t Device
device_t *Devices;
size_t devicenum;

typedef struct Device{
    uint8_t bus, slot, func;
    uint16_t vendor_id, device_id;
} Device;
