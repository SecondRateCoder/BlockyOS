#pragma once

//*	Adapted from (https://wiki.osdev.org/PCI#Command_Register)

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"
#include "kernel/libcrt/hardware/ACPI/RSDP/MCFG.h"
#include "kernel/libcrt/hardware/ACPI/XSDP/MCFG.h"

// #define PCIeConfigurationACPISignature  "MCFG"
#define PCIConfigAddress		0xCF8
#define PCIDataAddress			0xCFC
#define PCICapabilitiesOffset	0x34

#define PCIMaxBuses				(256)
#define PCIMaxDevices			(32)
#define PCIMaxSlots				PCIMaxDevices
#define PCIMaxPhysicalFunctions	(8)

enumdef(uint8_t, PCIDeviceType){PCIGeneralDevice = 0x0, PCItoPCIBridge = 0x1, PCItoCardBusBridge = 0x2};
typedef struct{PCIDeviceType	Type	: 7;	uint8_t	MultipleFunctions	: 1;}__packed PCIHeaderType;

typedef struct{
	uint32_t	IsIOSpace				: 1;
	//	0x01: 64-bit BAR, 
	//	0x00: 32-bit BAR.
	uint32_t	Type					: 3;
	uint32_t	Prefetchable			: 1;
	uint32_t	_16ByteAlignedAddress	: 24;
}__packed MemorySpaceBAR;
typedef struct{
	uint32_t	IsIOSpace				: 1;
	uint32_t							: 1;
	uint32_t	_4ByteAlignedAddress	: 30;
}__packed IOSpaceBAR;
typedef union{
	MemorySpaceBAR	MSBAR;
	IOSpaceBAR		IOBAR;
}BARRegister;
typedef struct{
	//	Will return 0, after BIST execution, if the test completed successfully.
	uint8_t			CompletionCode	: 4;
	uint8_t							: 2;
	//	When set to 1 the BIST is invoked. 
	//	This bit is reset when BIST completes. 
	//	If BIST does not complete after 2 seconds the device should be failed by system software.
	uint8_t			StartBIST		: 1;
	//	Will return 1 the device supports BIST.
	uint8_t			BISTCapable		: 1;
}__packed PCIBISTRegister;
typedef struct{
	uint32_t		RegisterOffset	: 8;
	uint32_t		FunctionNumber	: 3;
	uint32_t		DeviceNumber	: 5;
	uint32_t		BusNumber		: 8;
	uint32_t						: 7;
	uint32_t		Enable			: 1;
}__packed PCIConfigRegister;
typedef struct{
	//	If set to 1 the device can respond to I/O Space accesses; otherwise, the device's response is disabled.
	uint16_t		IOSpace				: 1;
	//	If set to 1 the device can respond to Memory Space accesses; otherwise, the device's response is disabled.
	uint16_t		MemorySpace			: 1;
	//	If set to 1 the device can behave as a bus master; otherwise, the device can not generate PCI accesses.
	uint16_t		BusMaster			: 1;
	//	If set to 1 the device can monitor Special Cycle operations; otherwise, the device will ignore them.
	const uint16_t	SpecialCycles		: 1;
	//	If set to 1 the device can generate the Memory Write and Invalidate command; otherwise, the Memory Write command must be used.
	const uint16_t	MemoryWrite			: 1;
	//	If set to 1 the device does not respond to palette register writes and will snoop the data; 
	//		otherwise, the device will trate palette write accesses like all other accesses.
	const uint16_t	VGAPaletteSnoop		: 1;
	//	If set to 1 the device will take its normal action when a parity error is detected; 
	//		otherwise, when an error is detected, the device will set bit 15 of the Status register (Detected Parity Error Status Bit), 
	//		but will not assert the PERR# (Parity Error) pin and will continue operation as normal.
	uint16_t		ParityErrorResponse	: 1;
	const uint16_t						: 1;
	//	If set to 1 the SERR# driver is enabled; otherwise, the driver is disabled.
	uint16_t		SERREnable			: 1;
	//	If set to 1 indicates a device is allowed to generate fast back-to-back transactions; 
	//		otherwise, fast back-to-back transactions are only allowed to the same agent.
	const uint16_t	FastBackToBackEnable: 1;
	//	If set to 1 the assertion of the devices INTx# signal is disabled; otherwise, assertion of the signal is enabled.
	uint16_t		InterruptDisable	: 1;
	uint16_t							: 5;
}__packed PCICommandRegister;
typedef struct{
	uint16_t							: 3;
	//	Represents the state of the device's INTx# signal. 
	//		If set to 1 and bit 10 of the Command register (Interrupt Disable bit) is set to 0 the signal will be asserted; 
	//		otherwise, the signal will be ignored.
	const uint16_t			InterruptStatus			: 1;
	//	If set to 1 the device implements the pointer for a New Capabilities Linked list at offset 0x34; otherwise, the linked list is not available.
	const uint16_t			CapabilitiesList		: 1;
	//	If set to 1 the device is capable of running at 66 MHz; otherwise, the device runs at 33 MHz.
	const uint16_t			_66MHzCapable			: 1;
	//	In revision 2.1 of the specification this bit was used to indicate whether or not a device supported User Definable Features.
	const uint16_t			UDFEnable				: 1;
	//	 If set to 1 the device can accept fast back-to-back transactions that are not from the same agent; otherwise, transactions can only be accepted from the same agent.
	const uint16_t			FastBackToBackEnable	: 1;
	//	This bit is only set when the following conditions are met. 
	//	The bus agent asserted PERR# on a read or observed an assertion of PERR# on a write, 
	//		the agent setting the bit acted as the bus master for the operation in which the error occurred, 
	//		and bit 6 of the Command register (Parity Error Response bit) is set to 1.
	uint16_t Write1ToClear	MasterDataParityError	: 1;
	//	Read only bits that represent the slowest time that a device will assert DEVSEL# for any bus command except Configuration Space read and writes. 
	//	Where a value of 0x0 represents fast timing, a value of 0x1 represents medium timing, and a value of 0x2 represents slow timing.
	const uint16_t			DEVSELTimig				: 3;
	//	This bit will be set to 1 whenever a target device terminates a transaction with Target-Abort.
	uint16_t Write1ToClear	SignalledTimedAbort		: 1;
	//	This bit will be set to 1, by a master device, whenever its transaction is terminated with Target-Abort.
	uint16_t Write1ToClear	RecievedTimedAbort		: 1;
	//	This bit will be set to 1, by a master device, whenever its transaction (except for Special Cycle transactions) is terminated with Master-Abort.
	uint16_t Write1ToClear	RecievedMasterAbort		: 1;
	//	This bit will be set to 1 whenever the device asserts SERR#.
	uint16_t Write1ToClear	SignalledSystemAbort	: 1;
	uint16_t Write1ToClear	DetectedParityError		: 1;
}__packed PCIStatusRegister;

typedef struct{uint8_t		ProgIF, SubClass, ClassCode;}__packed PCIDeviceSpecifier;
typedef union{
	uint128_t				Raw;
	struct{
		uint16_t			VendorID, 
							DeviceID;
		PCICommandRegister	Command;
		PCIStatusRegister	Status;
		uint8_t				RevisionID;
		PCIDeviceSpecifier	DeviceCode;
		//	 Specifies the system cache line size in 32-bit units.
		uint8_t				CacheLineSize, 
							LatencyTimer;
		PCIHeaderType		HeaderType;
		//	Represents that status and allows control of a devices BIST (built-in self test).
		PCIBISTRegister		BIST;
	};
}__packed PCIDevice;

typedef struct{
	PCIDevice				This;
	BARRegister				BAR[6];
	//	Points to the Card Information Structure and is used by devices that share silicon between CardBus and PCI.
	uint32_t				CardbusCISPointer;
	uint16_t				SubsystemVendorID;
	uint16_t				SubsystemID;
	uint32_t				ExpansionROMBaseAddress;
	//	Points (i.e. an offset into this function's configuration space) to a linked list of new capabilities implemented by the device. 
	//	Used if bit 4 of the status register (Capabilities List bit) is set to 1. 
	//	The bottom two bits are reserved and should be masked before the Pointer is used to access the Configuration Space.
	uint8_t					CapabilitiesListOffset;
	uint8_t					Reserved0[3];
	uint32_t				Reserved1;
	//	Specifies which input of the system interrupt controllers the device's interrupt pin is connected to and is implemented by any device that makes use of an interrupt pin. 
	//	For the x86 architecture this register corresponds to the PIC IRQ numbers 0-15 (and not I/O APIC IRQ numbers) and a value of 0xFF defines no connection.
	uint8_t					InterruptLine;
	//	 Specifies which interrupt pin the device uses. Where a value of 0x1 is INTA#, 0x2 is INTB#, 0x3 is INTC#, 0x4 is INTD#, and 0x0 means the device does not use an interrupt pin.
	uint8_t					InterruptPIN;
	//	A read-only register that specifies the burst period length, in 1/4 microsecond units, that the device needs (assuming a 33 MHz clock rate).
	uint8_t					MinGrant;
	//	A read-only register that specifies how often the device needs access to the PCI bus (in 1/4 microsecond units).
	uint8_t					MaxLatency;
}PCIHeader0x0;

typedef struct{
	PCIDevice				This;
	BARRegister				BAR[2];
	uint8_t					PrimaryBusNumber, 
							SecondaryBusNumber, 
							SubordinateBusNumber;
	uint8_t					SecondaryLatencyTimer;
	uint8_t					IOBaseLow, 
							IOLimitLow;
	uint16_t				SecondaryStatus;
	uint16_t				MemoryBase, 
							MemoryLimit;
	uint16_t				PrefetchableMemoryBaseLow, 
							PrefetchableMemoryLimitLow;
	uint32_t				PrefetchableMemoryBaseHigh, 
							PrefetchableMemoryLimitHigh;
	uint16_t				IOBaseHigh, 
							IOLimitHigh;
	uint8_t					CapabilitiesListOffset;
	uint8_t					Reserved[3];
	uint32_t				ExpansionROMBaseAddress;
	uint8_t					InterruptLine, 
							InterruptPIN;
	uint16_t				BridgeControl;
}PCIHeader0x1;

typedef struct{
	PCIDevice				This;
	uint32_t				CardBusSocketBaseAddress;
	uint8_t					CapabilitiesListOffset;
	uint8_t					Reserved0;
	uint16_t				SecondaryStatus;
	uint8_t					PCIBusNumber;
	uint8_t					CardbusBusNumber;
	uint8_t					SubordinateBusNumber;
	uint8_t					CardbusLatencyTimer;
	uint32_t				MemoryBaseAddress0, 
							MemoryLimit0;
	uint32_t				MemoryBaseAddress1, 
							MemoryLimit1;
	uint32_t				IOBaseAddress0, 
							IOLimit0;
	uint32_t				IOBaseAddress1, 
							IOLimit1;
	uint8_t					InterruptLine, 
							InterruptPIN;
	uint16_t				BridgeControl;
	uint16_t				SubsystemDeviceID;
	uint16_t				SubsystemVendorID;
	uint32_t				_16BitPCLegacyModeBaseAddress;
}PCIHeader0x2;