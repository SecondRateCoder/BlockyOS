#pragma once

//* https://wiki.osdev.org/Enhanced_Host_Controller_Interface
#include "kernel/services/USB/service.h"

typedef struct{
	uint8_t		CapabilityRegisterLength;
	uint8_t		Reserved;
	uint16_t	InterfaceVersionNumber;
	uint32_t	StructuralParameters;
	uint32_t	CapabilityParameters;
	uint32_t	CompanionPortRouteDescription;
}__packed EHCICapabilities_t;

typedef struct{
	uint32_t	Run									: 1;
	uint32_t	HostControllerReset					: 1;
	uint32_t	ProgrammableFrameListSize			: 2;
	uint32_t	PeriodicScheduleEnable				: 1;
	uint32_t	AsyncScheduleEnable					: 1;
	uint32_t	AsyncAdvanceDoorbellInterruptEnable	: 1;
	//	Resets the controller without affecting connected devices.
	uint32_t	LightHostControllerReset			: 1;
	uint32_t	AsyncScheduleParkModeCount			: 2;
	uint32_t										: 1;
	uint32_t	AsyncScheduleParkModeEnable			: 1;
	uint32_t										: 4;
	//	Number of micro frames to process between interrupts.
	uint32_t	InterruptThreshold					: 8;
	uint32_t										: 8;
}__packed EHCICommandRegister_t;
typedef struct{
	uint32_t	USBTransferInterrupt		: 1;
	uint32_t	USBErrorInterrupt			: 1;
	uint32_t	PortChangeDetect			: 1;
	uint32_t	FrameListRollover			: 1;
	uint32_t	HostSystemError				: 1;
	uint32_t	DoorbellInterrupt			: 1;
	uint32_t								: 6;
	uint32_t	Halted						: 1;
	uint32_t	Reclamation					: 1;
	uint32_t	PeriodicScheduleStatus		: 1;
	uint32_t	AsyncScheduleStatus			: 1;
	uint32_t								: 16;
}__packed EHCIStatusRegister_t;
typedef struct{
	uint32_t	USBTransferInterruptEnable		: 1;
	uint32_t	USBErrorInterruptEnable			: 1;
	uint32_t	PortChangeInterruptEnable		: 1;
	uint32_t	FrameListRolloverInterruptEnable: 1;
	uint32_t	HostSystemErrorInterruptEnable	: 1;
	uint32_t	AsyncAdvanceInterruptEnable		: 1;
	uint32_t									: 26;
}__packed EHCIInterruptEnableRegister_t;
typedef struct{
	uint32_t	Connected						: 1;
	uint32_t	ConnectChange					: 1;
	uint32_t	PortEnabled						: 1;
	uint32_t	PortEnabledChange				: 1;
	uint32_t	Overcurrent						: 1;
	uint32_t	OvercurrentChange				: 1;
	uint32_t	ForcePortResume					: 1;
	uint32_t	Suspend							: 1;
	uint32_t	PortReset						: 1;
	uint32_t									: 1;
	uint32_t	LineStatus						: 2;
	uint32_t	PortPower						: 1;
	//	0 = Local
	//	1 = Companion Host Controller
	uint32_t	CompanionPortControl			: 1;
	//	0 = Off
	//	1 = Amber
	//	2 = Green
	uint32_t	PortIndicatorControl			: 2;
	uint32_t	PortTestControl					: 1;
	uint32_t	WakeOnConnectEnable				: 1;
	uint32_t	WakeOnDisconnectEnable			: 1;
	uint32_t	WakeOnOvercurrentEnable			: 1;
	uint32_t									: 7;
}__packed EHCIPortStatusControlRegister_t;
typedef struct{
	EHCICommandRegister_t			Command;
	EHCIStatusRegister_t			Status;
	EHCIInterruptEnableRegister_t	InterruptEnable;
	uint32_t						FrameIndex;
	uint32_t						_4GbSegmentSelector;
	uint32_t						FrameListBaseAddress;
	uint32_t						ConfiguredFlags;
	EHCIPortStatusControlRegister_t	Ports[];
}EHCIOperationRegisters_t;

enumdef(uint8_t, EHCIHorizontalPointerNextQueueType){
	Isochronous = 0b00, QueueHead = 0b01, 
	SplitTransactionsIsochronous = 0b01, 
	FrameSpanTraversalNode = 0b11
};
typedef struct{
	uint32_t	Terminate		: 1;
	uint32_t	NextQueueType	: 2;
	uint32_t					: 2;
	uint32_t	NextQueueHead	: 27;
}__packed EHCIHorizontalLinkPointer_t;
typedef struct{
	uint32_t	DeviceAddress			: 7;
	uint32_t	Inactivate				: 1;
	uint32_t	EndpointNumber			: 4;
	//	0 = Full Speed 
	//	1 = Low Speed 
	//	2 = High Speed
	uint32_t	EndpointSpeed			: 2;
	uint32_t	DataToggleControl		: 1;
	//	 Set if this is the first Queue Head in an Asynchronous List.
	uint32_t	HeadOfReclamationList	: 1;
	uint32_t	MaximumPacketLength		: 10;
	//	 	Not used for High Speed devices
	uint32_t	ControlEndpoint			: 1;
	uint32_t	NAKReload				: 4;
}__packed EHCIEndpointCharacteristics_t;
typedef struct{
	//	Used for split transactions.
	uint32_t	InterruptScheduleMask		: 8;
	//	Used for split transactions.
	uint32_t	SplitCompletionMask			: 8;
	//	Used for split transactions.
	uint32_t	HubAddress					: 6;
	//	Used for split transactions.
	uint32_t	PortNumber					: 7;
	//	Must be greater than zero.
	uint32_t	HighBandwidthPipeMultiplier	: 3;
}__packed EHCIEndpointCapabilities_t;
typedef struct{
	EHCIHorizontalPointerNextQueueType	Pointer;
	EHCIEndpointCharacteristics_t		EndpointCharacteristics;
	uint32_t							EndpointCapabilities;
	uint32_t							CurrentTransferDescriptorAddress, 
										CurrentTransferDescriptorWorkingArea;
}__packed EHCIQueueHead_t;


void *GetEHCICapabilitiesBase(void *acpibase, uint32_t N){
	uint32_t bus, slot;
	PCIDevice *d = PCIeSearchDevice(acpibase, GetPCIeStruct(SerialBus_USB_EHCI), N, &bus, &slot);
	if(d->HeaderType.Type == PCIGeneralDevice){
		if(((PCIHeader0x0 *)d)->BAR[0].IOBAR.IsIOSpace){return NULL;}
		return MapVirtual(((PCIHeader0x0 *)d)->BAR[0].MSBAR._16ByteAlignedAddress);
	}
	return NULL;
}
void *GetEHCIBase(void *acpibase, uint32_t N){
	void *Temp = GetEHCICapabilitiesBase(acpibase, N);
	return Temp? Temp + ((EHCICapabilities_t *)Temp)->CapabilityRegisterLength: NULL;
}