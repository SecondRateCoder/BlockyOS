#include "./kernel/lib32/generic/standard.h"
#include "./kernel/lib32/stdkernel/Interrupt/IRQ/IRQ.h"

#define PCICONFIGADDR 0xCF8
#define PCICONFIGDATA 0xCFC

#define PCIDeviceID(bus, slot) (pciReadWord((bus), (slot), 		0, 0) & 0xFFFF)
#define PCIVendorID(bus, slot) (pciReadWord((bus), (slot), 		0, 2) & 0xFFFF)

#define PCIStatusCode(bus, slot) (pciReadWord((bus), (slot), 	0, 4) & 0xFFFF)
#define PCICommandCode(bus, slot) (pciReadWord((bus), (slot), 	0, 6) & 0xFFFF)

#define PCIClassCode(bus, slot) ((pciReadWord((bus), (slot), 	0, 8) >> 8) & 0xFF)
#define PCISubClassCode(bus, slot) (pciReadWord((bus), (slot), 	0, 8) & 0xFF)
#define PCIProgIF(bus, slot) ((pciReadWord((bus), (slot), 		0, 10) >> 8) & 0xFF)
#define PCIRevisionID(bus, slot) (pciReadWord((bus), (slot), 	0, 10) & 0xFF)

#define PCIBIST(bus, slot) ((pciReadWord((bus), (slot), 		0, 12) >> 8) & 0xFF)
#define PCIHeaderType(bus, slot) (pciReadWord((bus), (slot), 	0, 12) & 0xFF)
#define PCILatencyTimer(bus, slot) ((pciReadWord((bus), (slot), 0, 14) >> 8) & 0xFF)
#define PCICacheLineSize(bus, slot) (pciReadWord((bus), (slot), 0, 14) & 0xFF)

#define PCIBAR_MemoryAddress(BAR) ((BAR) >> 4)
#define PCIBAR_MemoryType(BAR) (((BAR) >> 1) & 0x3)
#define PCIBAR_IOAddress(BAR) ((BAR) >> 2)

#define PCIHeader0x1_BISTCompletionCode(BIST) (BIST & 7)
typedef enum PCIFlags{
	PCIFlags_HeaderType0x0 = 0x0,
	PCIFlags_HeaderType0x1 = 0x1,
	PCIFlags_HeaderType0x2 = 0x2,
	PCIFlags_PCIHeader0x1_BISTEnable = 0x40,
	PCIFlags_PCIHeader0x1_BISTCapable = 0x80,
	PCIFlags_PCIBAR_MemoryPrefetch = 0x8,
	PCIFlags_PCIHeader0x1_HasMultipleFunctions = 0x80
}PCIFlags;

// All comments copied from (https://osdev.wiki/wiki/PCI).
typedef enum PCICommandRegister{
	// If set to 1 the device can respond to I/O Space accesses; otherwise, the device's response is disabled.
	PCICommandRegister_IOSpace = 0x1,
	// If set to 1 the device can respond to Memory Space accesses; otherwise, the device's response is disabled.
	PCICommandRegister_MemorySpace = 0x2,
	// If set to 1 the device can behave as a bus master; otherwise, the device can not generate PCI accesses.
	PCICommandRegister_BusMaster = 0x4,
	// If set to 1 the device can monitor Special Cycle operations; otherwise, the device will ignore them.
	PCICommandRegister_SpecialCycles = 0x8,
	// If set to 1 the device can generate the Memory Write and Invalidate command; 
	// otherwise, the Memory Write command must be used.
	PCICommandRegister_MemoryWrite_InvalidateEnabled = 0x10,
	// If set to 1 the device does not respond to palette register writes and will snoop the data; 
	// otherwise, the device will treat palette write accesses like all other accesses.
	PCICommandRegister_VGAPaletteSnoop = 0x20,
	//  If set to 1 the device will take its normal action when a parity error is detected; 
	// otherwise, when an error is detected, the device will set bit 15 of the Status register (Detected Parity Error Status Bit), 
	// but will not assert the PERR# (Parity Error) pin and will continue operation as normal.
	PCICommandRegister_ParityErrorResponse = 0x40,
	// If set to 1 the SERR# driver is enabled; otherwise, the driver is disabled.
	PCICommandRegister_SERREnable = 0x80,
	// If set to 1 indicates a device is allowed to generate fast back-to-back transactions; 
	// otherwise, fast back-to-back transactions are only allowed to the same agent.
	PCICommandRegister_FastB2BEnable = 0x100,
	// If set to 1 the assertion of the devices INTx# signal is disabled; otherwise, assertion of the signal is enabled.
	PCICommandRegister_InterruptDisable = 0x200
}PCICommandRegister;

typedef enum PCIStatusRegister{
	//  Represents the state of the device's INTx# signal. If set to 1 and bit 10 of the Command register (Interrupt Disable bit) is set to 0 the signal will be asserted; 
	// otherwise, the signal will be ignored.
	PCIStatusRegister_InterruptStatus = 0x4,
	// If set to 1 the device implements the pointer for a New Capabilities Linked list at offset 0x34; 
	// otherwise, the linked list is not available.
	PCIStatusRegister_CapabilitiesList = 0x8,
	// If set to 1 the device is capable of running at 66 MHz; otherwise, the device runs at 33 MHz.
	PCIStatusRegister_66MHzCapable = 0x10,
	// If set to 1 the device can accept fast back-to-back transactions that are not from the same agent; 
	// otherwise, transactions can only be accepted from the same agent.
	PCIStatusRegister_FastB2BCapable = 0x20,
	// This bit is only set when the following conditions are met. 
	// The bus agent asserted PERR# on a read or observed an assertion of PERR# on a write, the agent setting the bit acted as the bus master for the operation in which the error occurred, 
	// and bit 6 of the Command register (Parity Error Response bit) is set to 1.
	PCIStatusRegister_MasterDataParityError = 0x40,
	// Read only bits that represent the slowest time that a device will assert DEVSEL# for any bus command except Configuration Space read and writes. 
	// Where a value of 0x0 represents fast timing, a value of 0x1 represents medium timing, and a value of 0x2 represents slow timing.
	PCIStatusRegister_DEVSELTiming = 0x180,
	// This bit will be set to 1 whenever a target device terminates a transaction with Target-Abort.
	PCIStatusRegister_SignalledTargetAbort = 0x200,
	// This bit will be set to 1, by a master device, whenever its transaction is terminated with Target-Abort.
	PCIStatusRegister_RecievedTargetAbort = 0x400,
	// This bit will be set to 1, by a master device, whenever its transaction (except for Special Cycle transactions) is terminated with Master-Abort.
	PCIStatusRegister_RecievedMasterAbort = 0x800,
	// This bit will be set to 1 whenever the device asserts SERR#.
	PCIStatusRegister_SignalledSystemError = 0x1000,
	// This bit will be set to 1 whenever the device detects a parity error, even if parity error handling is disabled.
	PCIStatusRegister_DetectedParityError = 0x2000,
}PCIStatusRegister;

typedef void (*PCIFunc)(void *in, size_t code);

// All comments copied from (https://osdev.wiki/wiki/PCI).
typedef struct PCICommon{
	// Identifies the particular device. 
	// Where valid IDs are allocated by the vendor.
	uint16_t DeviceID;
	// Identifies the manufacturer of the device.
	// Where valid IDs are allocated by PCI-SIG to ensure uniqueness and 0xFFFF is an invalid value that will be returned on read accesses to Configuration Space registers of non-existent devices.
	uint16_t VendorID;
	//  A register used to record status information for PCI bus related events.
	uint16_t StatusCode;
	// Provides control over a device's ability to generate and respond to PCI cycles.
	// Where the only functionality guaranteed to be supported by all devices is, 
	// when a 0 is written to this register, the device is disconnected from the PCI bus for all accesses except Configuration Space access.
	uint16_t CommandCode;
	// A read-only register that specifies the type of function the device performs.
	uint8_t ClassCode; 
	// A read-only register that specifies the specific function the device performs.
	uint8_t SubClassCode;
	// (Programming Interface Byte): A read-only register that specifies a register-level programming interface the device has, if it has any at all.
	uint8_t ProgIF;
	// Specifies a revision identifier for a particular device. Where valid IDs are allocated by the vendor.
	uint8_t RevisionID;
	// Represents that status and allows control of a devices BIST (built-in self test).
	uint8_t BIST;
	// Identifies the layout of the rest of the header beginning at byte 0x10 of the header. 
	// If bit 7 of this register is set, the device has multiple functions; 
	// otherwise, it is a single function device. Types:
	// 	0x0: a general device
	// 	0x1: a PCI-to-PCI bridge
	// 	0x2: a PCI-to-CardBus bridge.
	uint8_t HeaderType;
	// Specifies the latency timer in units of PCI bus clocks.
	uint8_t LatencyTimer;
	// Specifies the system cache line size in 32-bit units. 
	// A device can limit the number of cacheline sizes it can support, 
	// if a unsupported value is written to this field, the device will behave as if a value of 0 was written.
	uint8_t CacheLineSize;
}PACKEDSTRUCT PCICommon;

#define PCIStandard PCIHeader0x0
typedef struct PCIHeader0x0{
	PCICommon common;
	uint32_t BAR0, BAR1, BAR2, 
			 BAR3, BAR4, BAR5;
	// Points to the Card Information Structure and is used by devices that share silicon between CardBus and PCI.
	uint32_t CardbusCISptr;
	uint16_t SubSystemID, SubSystemVendorID;
	uint32_t ExpandableROMBaseAdress;
	// Points (i.e. an offset into this function's configuration space) to a linked list of new capabilities implemented by the device. 
	// Used if bit 4 of the status register (Capabilities List bit) is set to 1. 
	// The bottom two bits are reserved and should be masked before the Pointer is used to access the Configuration Space.
	uint8_t CapabilitiesPtr;
	uint8_t MaxLatency;
	uint8_t MinGrant;
	// Specifies which interrupt pin the device uses. 
	// Where a value of: 
	//	0x1 is INTA#, 
	//	0x2 is INTB#, 
	//	0x3 is INTC#, 
	//	0x4 is INTD#, 
	//	and 0x0 means the device does not use an interrupt pin.
	uint8_t InterruptPIN;
	// Specifies which input of the system interrupt controllers the device's interrupt pin is connected to and is implemented by any device that makes use of an interrupt pin. 
	// For the x86 architecture this register corresponds to the PIC IRQ numbers 0-15 (and not I/O APIC IRQ numbers) and a value of 0xFF defines no connection.
	uint8_t InterruptLine;
}PACKEDSTRUCT PCIHeader0x0;

#define PCIBridge PCIHeader0x1
#define PCIHeader0x1_BIST
typedef struct PCIHeader0x1{
	PCICommon common;
	uint32_t BAR0, BAR1;
	uint8_t SecondaryLatencyTimer, SubordinateBusNumber, 
			SecondaryBusNumber, PrimaryBusNumber;
	uint16_t SecondaryStatus;
	uint32_t IOLimit, IOBase;
	uint16_t MemoryLimit, MemoryBase;
	uint64_t PrefetchMemoryLimit, PrefetchMemoryBase;
	uint8_t CapabilitiesPtr;
	uint32_t ExpansionROMBaseAdress;
	uint16_t BridgeControl;
	uint8_t InterruptPIN; 
	uint8_t InterruptLine;
}PACKEDSTRUCT PCIHeader0x1;

typedef struct PCIHeader0x2{
	PCICommon common;
	union{
		uint32_t CardBus_SocketBaseAddress;
		uint32_t CardBus_ExCABaseAddress;
	};
	uint16_t SecondaryStatus;
	uint8_t CapabilitiesOffset;
	uint8_t CardBusLatencyTimer;
	uint8_t SubordinateBusNumber;
	uint8_t CardBusBusNumber;
	uint8_t PCIBusNumber;
	uint32_t MemoryBase0,
			 MemoryLimit0;
	uint32_t MemoryBase1,
			 MemoryLimit1;
	uint32_t IOBase0,
			 IOLimit0;
	uint32_t IOBase1,
			 IOLimit1;
	uint16_t BridgeControl;
	uint8_t InterruptPIN;
	uint8_t InterruptLine;
	uint16_t SubSytemVendorID;
	uint16_t SubSystemDeviceID;
	uint32_t bit16PC_CardLegacyModeBase;
}PACKEDSTRUCT PCIHeader0x2;

uint16_t pciReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
inline bool pciCheckDevice(uint8_t bus, uint8_t slot);
inline bool pciCheckFunc(uint8_t bus, uint8_t slot, uint8_t func);
bool getPCICommon(uint8_t bus, uint8_t slot, PCICommon *out);
void PCIGetFullHeader(void *out, uint8_t bus, uint8_t slot, uint8_t func);