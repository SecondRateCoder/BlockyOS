#pragma once
//*	https://wiki.osdev.org/Universal_Serial_Bus
#include "kernel/libcrt/def.h"
#include "kernel/libcrt/services.h"
#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/hardware/IDT/ISR.h"
#include "kernel/libcrt/hardware/GDT/GDT.h"
#include "kernel/libcrt/memory/string.h"
#include "kernel/libcrt/hardware/PCIe/PCIe.h"
#include "kernel/libcrt/hardware/PCIe/Devices.h"
#include "kernel/libcrt/memory/allocator/malloc.h"

#include "kernel/libcrt/hardware/PIT.h"
#include "kernel/libcrt/hardware/IDT/APIC/LocalAPICTimer.h"

typedef struct{
	//	Direction, type, recipient (e.g., 0x80 for Device-to-Host)
    uint8_t		bmRequestType;
	//	Command (e.g., GET_DESCRIPTOR = 0x06, SET_ADDRESS = 0x05)
    uint8_t		bRequest;
	//	Descriptor type/index or address value
    uint16_t	wValue;
	//	Zero or Language ID / Interface number
    uint16_t	wIndex;
	//	Length of data phase payload (0 if no data phase)
    uint16_t	wLength;
} __packed USBControlRequest_t;

enumdef(uint8_t, USBTransferDescriptorPacketType){
	SetupPacket = 0x2D, DeviceToHostTransferPacket = 0x69, 
	HostToDeviceTransferPacket = 0xE1, StartOfFramePacket = 0xA5, 
	LowSpeedDeviceSignalPacket = 0xC3
};
enumdef(uint8_t, USBSetupPacketRequest){
	//	Reads status flags for a device, interface, or endpoint.
	GET_STATUS = 0x00, 
	//	Disables a specific feature (e.g., clears an endpoint stall/halt).
	CLEAR_FEATURE, 
	//	Enables a specific device or endpoint feature (e.g., remote wakeup).
	SET_FEATURE = 0x03, 
	//	Assigns a unique 7-bit USB address (1–127) to an enumerated device.
	SET_ADDRESS = 0x05, 
	//	Reads descriptors (Device, Configuration, String, HID, Endpoint).
	GET_DESCRIPTOR, 
	//	Updates existing descriptors on supported hardware.
	SET_DECRIPTOR, 
	//	Returns the current active configuration number.
	GET_CONFIGURATION, 
	//	Activates a specific device configuration layout.
	SET_CONFIGURATION, 
	//	Reads the current alternate setting for an interface.
	GET_INTERFACE, 
	//	Selects an alternate setting for an interface.
	SET_INTERFACE, 
	//	Synchronizes isochronous endpoints.
	SYNCH_FRAME
};

enumdef(uint8_t, USBGetDescriptorTypes){
	USBGetDescriptorDevice = 0x01, 
	USBGetDescriptorCurrentSpeedConfiguration = 0x02, 
	USBGetDescriptorDeviceQualifier = 0x06, 
	USBGetDescriptorOtherSpeedConfiguration = 0x07, 
	USBGetDescriptorInterfacePower = 0x08, 
	USBGetDescriptorString = 0x03, 
	USBGetDescriptorInterface = 0x04, 
	USBGetDescriptorEndpoint = 0x05, 
};

typedef struct{
	uint8_t					DescriptorSize;
	USBGetDescriptorTypes	DescriptorType;
}__packed USBGetDescriptorHeader_t;

typedef struct{
    USBGetDescriptorHeader_t	Header;
	//	 USB Specification Release Number in Binary-Coded Decimal (i.e, 2.10 is expressed as 210h). 
	//	Identifies the release of the USB Specification with with the device and its descriptors are compliant
	uint16_t				USBSpecificationNumber;
	//	This field is reset to zero if each interface within a configuration specifies 
	//		its own class information and the various interfaces operate independently.
	//	A value of FFh in this field indicates the device class is vendor-specific.
	uint8_t					ClassCode;
	//	The subclass code of a device is qualified by the class code of that device.
	//	If bDeviceClass is reset to zero, then this field must also be reset to zero.
	//	When bDeviceClass is not set to FFh, 
	//		then all values for this field are reserved for assignment by the USB-IF.
	uint8_t					SubclassCode;
	//	The protocol code of a device is qualified by both the class and subclass codes of that device.
	//	A value of 00h in this field means that the device may specify class-specific protocols on an interface basis, though this is not a requirement.
	//	If this field is set to FFh, then the device uses a vendor-specific protocol.
	uint8_t					ProtocolCode;
	//	Maximum packet size for endpoint zero.
	uint8_t					MaxPacketSize0;
	uint16_t				VendorID, 
							ProductID;
	uint16_t				DeviceReleaseNumber;
	uint8_t					ManufacturerStringDescriptorIndex;
	uint8_t					ProductStringDescriptorIndex;
	uint8_t					SerialNumberStringDescriptorIndex;
	uint8_t					NPossibleConfigurations;
}__packed USBGetDescriptorDevice_t;
typedef struct{
    USBGetDescriptorHeader_t	Header;
	//	This field must be at least 0200h.
	//	USB Specification Release Number in Binary-Coded Decimal (i.e, 2.10 is expressed as 210h). 
	//	Identifies the release of the USB Specification with with the device and its descriptors are compliant
	uint16_t				USBSpecificationNumber;
	//	This field is reset to zero if each interface within a configuration specifies 
	//		its own class information and the various interfaces operate independently.
	//	A value of FFh in this field indicates the device class is vendor-specific.
	uint8_t					ClassCode;
	//	The subclass code of a device is qualified by the class code of that device.
	//	If bDeviceClass is reset to zero, then this field must also be reset to zero.
	//	When bDeviceClass is not set to FFh, 
	//		then all values for this field are reserved for assignment by the USB-IF.
	uint8_t					SubclassCode;
	//	The protocol code of a device is qualified by both the class and subclass codes of that device.
	//	A value of 00h in this field means that the device may specify class-specific protocols on an interface basis, though this is not a requirement.
	//	If this field is set to FFh, then the device uses a vendor-specific protocol.
	uint8_t					ProtocolCode;
	//	Maximum packet size for endpoint zero.
	uint8_t					MaxPacketSize0;
	uint8_t					Reserved;
}__packed USBGetDescriptorDeviceQualifier_t;
typedef struct{
    USBGetDescriptorHeader_t	Header;
    //	Total bytes including Interface & Endpoint descriptors
    uint16_t				TotalLength;
    //	Number of interfaces in this configuration
    uint8_t					NumInterfaces;
    //	Value used as parameter in SET_CONFIGURATION
    uint8_t					ConfigurationValue;
    //	Index of string descriptor describing configuration
    uint8_t					Configuration;
    //	Power & Wakeup options
	//	Bit 6:
	//		0 = Device runs on power supplied by the bus
	//		1 = Device provides a local power source, if bMaxPower is non-zero, the device also may use bus power.
	//	Bit 5:
	//		0 = Remote Wakeup not supported
	//		1 = Remote Wakeup supported
    uint8_t					Attributes;
    //	Maximum power consumption in 2mA units
    uint8_t					MaxPower;
}__packed USBGetDescriptorCurrentSpeedConfiguration_t, 
			USBGetDescriptorOtherSpeedConfiguration_t;
typedef struct{
    USBGetDescriptorHeader_t	Header;
	//	Number of this interface. 
	//	Zero-based value which identifies the index of this interface 
	//		in the array of interfaces supported within a configuration.
	uint8_t						InterfaceNumber;
	//	Value used to select the alternate settings described by this INTERFACE descriptor for the interface with the bInterfaceNumber in the previous field. 
	//	This value is zero if this descriptor describes the default settings for a particular interface.
	uint8_t						AlternateInterfaceSetting;
	//	 Number of endpoints used by this interface, 
	//		not including endpoint zero.
	uint8_t						TotalEndpoints;
	//	A value of zero here is reserved for future standardization.
	//	If this value is FFh, the interface class is vendor-specific.
	//	All other values are reserved for assignment by the USB-IF.
	uint8_t						InterfaceClassCode;
	//	The subclass code in this field is qualified by the value of the bInterfaceClass field.
	//	If bInterfaceClass is reset to zero, then this field must also be reset to zero.
	//	If bInterfaceClass is not set to the value of FFh, then all values of this field are reserved for assignment by the USB-IF.
	uint8_t						InterfaceSubClassCode;
	//	The protocol code in this field is qualified by the values of the bInterfaceClass and bInterfaceSubClass fields.
	//	If an interface supports class-specific requests, 
	//		then this field identifies the protocols that the device uses as defined by the specifications of the device class.
	//	If this field is reset to zero, 
	//		then the device does not use a class-specific protocol on this interface.
	//	If this field is set to FFh, then the devices uses a vendor-specific protocol on this interface.
	uint8_t						InterfaceProtocolCode;
	uint8_t						StringDescriptorIndex;
}__packed USBGetDescriptorInterfacePower_t;

enumdef(uint8_t, USBGetDescriptorEndpointTransferType){
	ControlTransferType = 0x00, IsochronousTransferType = 0x01, 
	BulkTransferType = 0x02, InterruptTransferType = 0x03
};
enumdef(uint8_t, USBGetDescriptorEndpointSynchronisationType){
	NoSynchronisationType = 0x00, AsynchronousSyncType = 0x01, 
	AdaptiveSyncType = 0x02, SynchronousSyncType = 0x03
};
enumdef(uint8_t, USBGetDescriptorEndpointUsageType){
	DataEndpointUsageType = 0x00, FeedbackEndpointUsageType = 0x01, 
	ImplicitFeedbackDataEndpointUsageType = 0x02, ReservedUsageType = 0x03
};
typedef struct{
	USBGetDescriptorHeader_t	Header;
	uint8_t						EndpointNumber		: 4;
	uint8_t											: 3;
	//	0 = OUT endpoint
	//	1 = IN endpoint
	uint8_t						EndpointDirection	: 1;

	uint8_t						TransferType		: 2;
	uint8_t						SynchronisationType	: 2;
	uint8_t						UsageType			: 2;
	uint8_t											: 2;
	uint16_t					MaximumPacketSize	: 10;
	uint16_t					TransactionsPerMF	: 2;
	uint16_t										: 4;
	//	Interval for polling a device during a data transfer, expressed in units of microframes for high-speed devices, and frames for low- and full-speed devices. 
	//	The exact meaning of the value in this field depends on the endpoint type and the operating speed of the device:
    //		Full- and High-speed isochronous endpoints, and high-speed interrupt endpoints:
    //			This field must be in the range from 1 to 16.
    //			This field is used to calculate the period as 2 << (PollingInterval - 1).
    //		Full- and Low-speed interrupt endpoints:
    //			This field must be in the range from 1 to 255.
    //		High-speed bulk and control OUT endpoints:
    //			This field must be in the range from 0 to 255.
    //			This field specifies the maximum NAK rate of the endpoint.
    //			A value of zero indicates that the endpoint never NAKs
    //			Other values indicate at most 1 NAK each bInterval number of microframes.
    //			See PING Transaction Protocol
	uint8_t						PollingInterval;
}__packed USBGetDescriptorEndpoint_t;
typedef struct{
	USBGetDescriptorHeader_t	Header;
	uint16_t					Data[];
}__packed USBGetDescriptorString_t;