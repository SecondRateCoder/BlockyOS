#include "./src/Core/General.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC
#define PCI_BAR0OFFSET   0x10
#define PCI_COMMANDOFFSET 0x04
#define PCI_STATUSOFFSET 0x06
#define PCIe_CACHELINESIZE 0x0C
#define PCI_CONFIGSIZE 256
#pragma region PCIe Type 0x0
    #define PCIe_BARBASE 0x10
    #define PCIe_BARLIMIT 0x24 
#pragma endregion
#pragma region PCIe Type 0x1
    #define PCIe_PRIMARY_BUSNUMBER 0x18
    //64-bit addresses
    #define PCIe_PREFETCHMEMORY_BASEORLIMIT 0x24
#pragma endregion
#define BARSIZE 6

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
#define STATUSBITS_DESVEL_TIMING(S) (uint8_t[2]){(is_set((size_t)(S), 9) << 1), (is_set((size_t)(S), 10) << 0)}
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

// #pragma region MSIBITS
// #define PER_VECTORMASKING(M) is_set((size_t)M, 8)
// #define BIT64(M) is_set((size_t)M, 7)
// /*
// Defines the number of low Message Data bits("message_addr_low") may be modified.
// MME |   #Interrupts
// 000 |	1
// 001 |	2
// 010 |	4
// 011 |	8
// 100 |	16
// 101 |	32
// */
// #define MULTIMESSAGE_ENABLE(M) (uint8_t[3]){is_set((size_t)M, 6), is_set((size_t)M, 5), is_set((size_t)M, 4)}
// #define MULTIMESSAGE_CAPABLE(M) (uint8_t[3]){is_set((size_t)M, 3), is_set((size_t)M, 2), is_set((size_t)M, 1)}
// #define ENABLE(M) is_set((size_t)M, 0)
// #pragma endregion



// #define rsdt_t RSDT
// #define rsdtdesc_t RSDPDescriptor
// #define xsdt_t XSDT
#define pcih_t PCI_Header

#define device_t Device
#define pci_meta_t PCI_Header
device_t *Devices;
size_t devicenum;

typedef struct Device{
    uint8_t bus, slot, *funcs, numof_funcs;
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
    //header_type:[7]: Multple functions?, [6 -> 0]: Literal Header Type,
    //bist(Built-In Self Test): [7]: BIST Capable, [6]: Start BIST(Invoke BIST, reset when completed, if not completed by 2 secs, device considered failed.), [4, 5]: Reserved, [0->3]: Completion code(Return 0 if BIST completed successfully)
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
    uint32_t BAR[6], cardbus_cis_pointer, expansion_ROM_base_address;
    #pragma endregion

    #pragma region Type 0x1
    //! DOES NOT USE: BAR2, BAR3, BAR4, BAR5, subsystem_id, max_latency, min_grant, cardbus_cis_pointer.
    uint8_t secondary_latency_timer, subordinate_bus_number, secondary_bus_number, primary_bus_number, IO_limit, IO_base;
    uint16_t secondary_status, memory_limit, memory_base, prefetch_memory_limit, prefetch_memory_base, IO_limit_upper_16bits, IO_base_upper_16bits, bridge_control;
    uint32_t prefetch_base_upper_32_bits, prefetch_limit_upper_32_bits;
    #pragma endregion

	#pragma region MSI_X Interrupts
    uint16_t message_control, messabe_data;
    uint8_t next_pointer, capability_id;
    uint32_t message_addr_low, message_addr_high, mask, pending;

	uint8_t table_offset[3], pendingbit_offset[3], BIR, pendingbit_BIR;
	#pragma endregion
}PCI_Header;
#pragma pop(1)

typedef enum REGOFFSET{
    REG0OFFSET= 0x0,
    REG1OFFSET= 0x4,
    REG2OFFSET= 0x8,
    REG3OFFSET= 0xC,
    REG4OFFSET= 0x10,
    REG5OFFSET= 0x14,
    REG6OFFSET= 0x18,
    REG7OFFSET= 0x1C,
    REG8OFFSET= 0x20,
    REG9OFFSET= 0x24,
    REG10OFFSET= 0x28,
    REG11OFFSET= 0x2C,
    REG12OFFSET= 0x30,
    REG13OFFSET= 0x34,
    REG14OFFSET= 0x38,
    REG15OFFSET= 0x3C,
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

void scan_pci_devices();
uint32_t pciconfig_read32(device_t device, uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void MMIO_configure(device_t *dev);
void enableaddr_read(const device_t dev);
void disableaddr_read(const device_t dev);
bool device_alloca(const device_t dev, uint8_t baroffset);
uint32_t pci_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint32_t device_read32(const device_t dev, uint8_t offset);
void pci_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);
void device_write32(const device_t dev, uint8_t offset, uint32_t value);
uint16_t pci_vendorcheck(uint16_t bus, uint16_t slot);
uint16_t device_vendorcheck(const device_t dev);
void device_metaconfig(device_t *dev);


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




/*
Class Codes
The Class Code, Subclass, and Prog IF registers are used to identify the device's type, the device's function, and the device's register-level programming interface, respectively.

The following table details most of the known device types and functions:

Class Code	Subclass	Prog IF
0x0 - Unclassified	0x0 - Non-VGA-Compatible Unclassified Device	--
0x1 - VGA-Compatible Unclassified Device	--
0x1 - Mass Storage Controller	0x0 - SCSI Bus Controller	--
0x1 - IDE Controller	0x0 - ISA Compatibility mode-only controller
0x5 - PCI native mode-only controller
0xA - ISA Compatibility mode controller, supports both channels switched to PCI native mode
0xF - PCI native mode controller, supports both channels switched to ISA compatibility mode
0x80 - ISA Compatibility mode-only controller, supports bus mastering
0x85 - PCI native mode-only controller, supports bus mastering
0x8A - ISA Compatibility mode controller, supports both channels switched to PCI native mode, supports bus mastering
0x8F - PCI native mode controller, supports both channels switched to ISA compatibility mode, supports bus mastering
0x2 - Floppy Disk Controller	--
0x3 - IPI Bus Controller	--
0x4 - RAID Controller	--
0x5 - ATA Controller	0x20 - Single DMA
0x30 - Chained DMA
0x6 - Serial ATA Controller	0x0 - Vendor Specific Interface
0x1 - AHCI 1.0
0x2 - Serial Storage Bus
0x7 - Serial Attached SCSI Controller	0x0 - SAS
0x1 - Serial Storage Bus
0x8 - Non-Volatile Memory Controller	0x1 - NVMHCI
0x2 - NVM Express
0x80 - Other	--
0x2 - Network Controller	0x0 - Ethernet Controller	--
0x1 - Token Ring Controller	--
0x2 - FDDI Controller	--
0x3 - ATM Controller	--
0x4 - ISDN Controller	--
0x5 - WorldFip Controller	--
0x6 - PICMG 2.14 Multi Computing Controller	--
0x7 - Infiniband Controller	--
0x8 - Fabric Controller	--
0x80 - Other	--
0x3 - Display Controller	0x0 - VGA Compatible Controller	0x0 - VGA Controller
0x1 - 8514-Compatible Controller
0x1 - XGA Controller	--
0x2 - 3D Controller (Not VGA-Compatible)	--
0x80 - Other	--
0x4 - Multimedia Controller	0x0 - Multimedia Video Controller	--
0x1 - Multimedia Audio Controller	--
0x2 - Computer Telephony Device	--
0x3 - Audio Device	--
0x80 - Other	--
0x5 - Memory Controller	0x0 - RAM Controller	--
0x1 - Flash Controller	--
0x80 - Other	--
0x6 - Bridge	0x0 - Host Bridge	--
0x1 - ISA Bridge	--
0x2 - EISA Bridge	--
0x3 - MCA Bridge	--
0x4 - PCI-to-PCI Bridge	0x0 - Normal Decode
0x1 - Subtractive Decode
0x5 - PCMCIA Bridge	--
0x6 - NuBus Bridge	--
0x7 - CardBus Bridge	--
0x8 - RACEway Bridge	0x0 - Transparent Mode
0x1 - Endpoint Mode
0x9 - PCI-to-PCI Bridge	0x40 - Semi-Transparent, Primary bus towards host CPU
0x80 - Semi-Transparent, Secondary bus towards host CPU
0x0A - InfiniBand-to-PCI Host Bridge	--
0x80 - Other	--
0x7 - Simple Communication Controller	0x0 - Serial Controller	0x0 - 8250-Compatible (Generic XT)
0x1 - 16450-Compatible
0x2 - 16550-Compatible
0x3 - 16650-Compatible
0x4 - 16750-Compatible
0x5 - 16850-Compatible
0x6 - 16950-Compatible
0x1 - Parallel Controller	0x0 - Standard Parallel Port
0x1 - Bi-Directional Parallel Port
0x2 - ECP 1.X Compliant Parallel Port
0x3 - IEEE 1284 Controller
0xFE - IEEE 1284 Target Device
0x2 - Multiport Serial Controller	--
0x3 - Modem	0x0 - Generic Modem
0x1 - Hayes 16450-Compatible Interface
0x2 - Hayes 16550-Compatible Interface
0x3 - Hayes 16650-Compatible Interface
0x4 - Hayes 16750-Compatible Interface
0x4 - IEEE 488.1/2 (GPIB) Controller	--
0x5 - Smart Card Controller	--
0x80 - Other	--
0x8 - Base System Peripheral	0x0 - PIC	0x0 - Generic 8259-Compatible
0x1 - ISA-Compatible
0x2 - EISA-Compatible
0x10 - I/O APIC Interrupt Controller
0x20 - I/O(x) APIC Interrupt Controller
0x01 - DMA Controller	0x00 - Generic 8237-Compatible
0x01 - ISA-Compatible
0x02 - EISA-Compatible
0x02 - Timer	0x00 - Generic 8254-Compatible
0x01 - ISA-Compatible
0x02 - EISA-Compatible
0x03 - HPET
0x3 - RTC Controller	0x0 - Generic RTC
0x1 - ISA-Compatible
0x4 - PCI Hot-Plug Controller	--
0x5 - SD Host controller	--
0x6 - IOMMU	--
0x80 - Other	--
0x9 - Input Device Controller	0x0 - Keyboard Controller	--
0x1 - Digitizer Pen	--
0x2 - Mouse Controller	--
0x3 - Scanner Controller	--
0x4 - Gameport Controller	0x0 - Generic
0x10 - Extended
0x80 - Other	--
0xA - Docking Station	0x0 - Generic	--
0x80 - Other	--
0xB - Processor	0x0 - 386	--
0x1 - 486	--
0x2 - Pentium	--
0x3 - Pentium Pro	--
0x10 - Alpha	--
0x20 - PowerPC	--
0x30 - MIPS	--
0x40 - Co-Processor	--
0x80 - Other	--
0xC - Serial Bus Controller	0x0 - FireWire (IEEE 1394) Controller	0x0 - Generic
0x10 - OHCI
0x1 - ACCESS Bus Controller	--
0x2 - SSA	--
0x3 - USB Controller	0x0 - UHCI Controller
0x10 - OHCI Controller
0x20 - EHCI (USB2) Controller
0x30 - XHCI (USB3) Controller
0x80 - Unspecified
0xFE - USB Device (Not a host controller)
0x4 - Fibre Channel	--
0x5 - SMBus Controller	--
0x6 - InfiniBand Controller	--
0x7 - IPMI Interface	0x0 - SMIC
0x1 - Keyboard Controller Style
0x2 - Block Transfer
0x8 - SERCOS Interface (IEC 61491)	--
0x9 - CANbus Controller	--
0x80 - Other	--
0xD - Wireless Controller	0x0 - iRDA Compatible Controller	--
0x1 - Consumer IR Controller	--
0x10 - RF Controller	--
0x11 - Bluetooth Controller	--
0x12 - Broadband Controller	--
0x20 - Ethernet Controller (802.1a)	--
0x21 - Ethernet Controller (802.1b)	--
0x80 - Other	--
0xE - Intelligent Controller	0x0 - I20	--
0xF - Satellite Communication Controller	0x1 - Satellite TV Controller	--
0x2 - Satellite Audio Controller	--
0x3 - Satellite Voice Controller	--
0x4 - Satellite Data Controller	--
0x10 - Encryption Controller	0x0 - Network and Computing Encrpytion/Decryption	--
0x10 - Entertainment Encryption/Decryption	--
0x80 - Other	--
0x11 - Signal Processing Controller	0x0 - DPIO Modules	--
0x1 - Performance Counters	--
0x10 - Communication Synchronizer	--
0x20 - Signal Processing Management	--
0x80 - Other	--
0x12 - Processing Accelerator	--	--
0x13 - Non-Essential Instrumentation	--	--
0x14 - 0x3F (Reserved)	--	--
0x40 - Co-Processor	--	--
0x41 - 0xFE (Reserved)	--	--
0xFF - Unassigned Class (Vendor specific)	--	--
*/