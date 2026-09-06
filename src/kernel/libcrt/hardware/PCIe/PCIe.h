#pragma once

//*	Adapted from (https://wiki.osdev.org/PCI#Command_Register)

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"
#include "PCI.h"

#define PCIeMakeAddress(base, bus, slot, func, offset)	\
	((base) + (((uint32_t)(bus) << 20) | ((uint32_t)(slot) << 15) | ((uint32_t)(func) << 12)) + (uint32_t)(offset))
#define PCIeGetNextCapability(Capability)	((void *)(Capability) + ((Capability)->Header.Next))
#define GetPCIeDeviceHeader(base, n, bus, slot)	PCIeMakeAddress(GetPCIeConfigurationBase(base, n), bus, slot, 0x0, 0x0)

#define PCIGetBar(HDR, N)	((uint64_t)((HDR)->BAR[N].IOBAR.IsIOSpace? 0x00ULL: ((HDR)->BAR[N].MSBAR.Type == 0x02? /*64-bit*/		\
	(((uint64_t)((HDR)->BAR[N >= 5? N - 1: N + 1].MSBAR._16ByteAlignedAddress) << 32) + (HDR)->BAR[N].MSBAR._16ByteAlignedAddress):	\
	(HDR)->BAR[N].MSBAR._16ByteAlignedAddress)))
#define PCIeGetBar			PCIGetBar

enumdef(uint16_t, PCIeCapabilityID){
	PCIeGenericCapabilites = 0x10, 
	PCIeSecondaryExtendedCapabilities = 0x0019, 
	PCIeSRIOVExtendedCapabilities = 0x0010, 
	PCIeAdvancedErrorReporting = 0x0001
};
enumdef(uint16_t, PCIeCapabilitiesDeviceType){
	PCIeEndpoint = 0x0, LegacyPCIeEndpoint = 0x1, 
	PCIeRootComplexRootPort = 0x4, PCIeSwitchUpstreamPort = 0x5, 
	PCIeSwitchDownstreamPort = 0x6, PCIePCIxBridge = 0x7, 
	PCIxPCIeBridge = 0x8, RCIntegratedEndpoint = 0x9, 
	RootComplexEventCollector = 0xA, PCIeMSIxCapability = 0x11
};

typedef struct{
	//	Version 1 doesn't have Root Capabilities nor the "2" fields.
	uint16_t	Version					: 4;
	uint16_t	DeviceType				: 4;
	uint16_t	SlotImplementedDevice	: 1;
	uint16_t	InterruptMessageNumber	: 5;
	uint16_t							: 2;
}__packed PCIeCapabilitiesRegister;
typedef struct{
	uint32_t	MaxPayloadSize				: 3;
	//	The Function uses unused function numbers.
	//	Then multiple requests can be sent at the same time concurrently.
	uint32_t	NPhantomFunctions			: 2;
	//	Indicates tag ID capacity (0 = 32 tags / 5-bit, 1 = 256 tags / 8-bit).
	uint32_t	ExtendedTagFieldEnabled		: 1;
	//	Defines the maximum delay the device can tolerate when exiting the L0s low-power link state back to L0.
	uint32_t	EndpointL0AcceptableLatency	: 3;
	//	Defines the maximum delay the device can tolerate when exiting the L1s low-power link state back to L1.
	uint32_t	EndpointL1AcceptableLatency	: 3;
	uint32_t								: 3;
	//	Set to 1 if the device supports modern PCIe error reporting rules and Advanced Error Reporting
	uint32_t	AdvancedErrorReporting		: 1;
	uint32_t								: 2;
	//	Contains the maximum power allocation set by the upstream slot.
	uint32_t	CapturedSlotPowerLimitValue	: 8;
	//	Contains the maximum power allocation set by the upstream slot.
	uint32_t	CapturedSlotPowerLimitScale	: 2;
	//	Set to 1 if software can independently reset this specific PCIe function without disturbing the rest of the physical chip or link.
	uint32_t	FunctionLevelReset			: 1;
}__packed PCIeDeviceCapabilitiesRegister;
typedef struct{
	uint16_t CorrectableErrorReportingEnable	: 1;
	uint16_t NonFatalErrorReportingEnable		: 1;
	uint16_t FatalErrorReportingEnable			: 1;
	uint16_t UnsupportedRequestReportingEnable	: 1;
	uint16_t EnableRelaxedOrdering				: 1;
	uint16_t MaxPayloadSize						: 3;
	uint16_t ExtendedTagFieldEnable				: 1;
	uint16_t PhantomFunctionsEnable				: 1;
	uint16_t AuxPowerPMEnable					: 1;
	uint16_t EnableNoSnoop						: 1;
	uint16_t MaxReadRequestSize					: 3;
	//	Also called Initiate Function Level Reset in newer specs
	uint16_t BridgeConfigurationRetryEnable		: 1;
}__packed PCIeDeviceControlRegister;
typedef struct{
	uint16_t CorrectableErrorDetected	: 1;
	uint16_t NonFatalErrorDetected		: 1;
	uint16_t FatalErrorDetected			: 1;
	uint16_t UnsupportedRequestDetected	: 1;
	uint16_t AuxPowerDetected			: 1;
	uint16_t TransactionsPending		: 1;
	uint16_t Reserved					: 10;
}__packed PCIeDeviceStatusRegister;
typedef struct{
	PCIeDeviceCapabilitiesRegister	Capabilities;
	PCIeDeviceControlRegister		Control;
	PCIeDeviceStatusRegister		Status;
}__packed PCIeDeviceRegisterSet;


typedef struct{
	uint32_t MaxLinkSpeed						: 4;
	uint32_t MaxLinkWidth						: 6;
	uint32_t ASPMSupport						: 2;
	uint32_t L0sExitLatency						: 3;
	uint32_t L1ExitLatency						: 3;
	uint32_t ClockPowerManagement				: 1;
	uint32_t SurpriseDownErrorReportingCapable	: 1;
	uint32_t DataLinkLayerLinkActiveReporting	: 1;
	uint32_t LinkBandwidthNotificationCapable	: 1;
	uint32_t ASPMOptionalityCompliance			: 1;
	uint32_t 									: 1;
	uint32_t PortNumber							: 8;
}__packed PCIeLinkCapabilitiesRegister;
typedef struct{
	uint16_t ActiveStatePowerManagementControl	: 2;
	uint16_t Reserved1							: 1;
	uint16_t ReadCompletionBoundary				: 1;
	uint16_t LinkDisable						: 1;
	uint16_t RetrainLink						: 1;
	uint16_t CommonClockConfiguration			: 1;
	uint16_t ExtendedSynch						: 1;
	uint16_t EnableClockPowerManagement			: 1;
	uint16_t HardwareAutonomousWidthDisable		: 1;
	uint16_t LinkBandwidthManagementIntEnable	: 1;
	uint16_t LinkAutonomousBandwidthIntEnable	: 1;
	uint16_t 									: 4;
}__packed PCIeLinkControlRegister;
typedef struct{
	uint16_t CurrentLinkSpeed				: 4;
	uint16_t NegotiatedLinkWidth			: 6;
	uint16_t								: 1;
	uint16_t LinkTraining					: 1;
	uint16_t SlotClockConfiguration			: 1;
	uint16_t DataLinkLayerLinkActive		: 1;
	uint16_t LinkBandwidthManagementStatus	: 1;
	uint16_t LinkAutonomousBandwidthStatus	: 1;
}__packed PCIeLinkStatusRegister;
typedef struct{
	PCIeLinkCapabilitiesRegister	Capabilities;
	PCIeLinkControlRegister			Control;
	PCIeLinkStatusRegister			Status;
}__packed PCIeLinkRegisterSet;


typedef struct{
	uint16_t CRSSoftwareVisibility	: 1;
	uint16_t 						: 15;
}__packed PCIeRootCapabilitiesRegister;
typedef struct{
	uint16_t SystemErrorOnCorrectableEnable	: 1;
	uint16_t SystemErrorOnNonFatalEnable	: 1;
	uint16_t SystemErrorOnFatalEnable		: 1;
	uint16_t PMEInterruptEnable				: 1;
	uint16_t CRSSoftwareVisibilityEnable	: 1;
	uint16_t 								: 11;
}__packed PCIeRootControlRegister;
typedef struct{
	uint32_t PMERequesterID	: 16;
	uint32_t PMEStatus		: 1;
	uint32_t PMEPending		: 1;
	uint32_t 				: 14;
}__packed PCIeRootStatusRegister;
typedef struct{
	PCIeRootCapabilitiesRegister	Capabilities;
	PCIeRootControlRegister			Control;
	PCIeRootStatusRegister			Status;
}PCIeRootRegisterSet;

typedef struct{
	uint32_t AttentionButtonPresent				: 1;
	uint32_t PowerControllerPresent				: 1;
	uint32_t MRLSensorPresent					: 1;
	uint32_t AttentionIndicatorPresent			: 1;
	uint32_t PowerIndicatorPresent				: 1;
	uint32_t HotPlugSurprise					: 1;
	uint32_t HotPlugCapable						: 1;
	uint32_t SlotPowerLimitValue				: 8;
	uint32_t SlotPowerLimitScale				: 2;
	uint32_t ElectromechanicalInterlockPresent	: 1;
	uint32_t NoCommandCompletedSupport			: 1;
	uint32_t PhysicalSlotNumber					: 13;
}__packed PCIeSlotCapabilitiesRegister;
typedef struct{
	uint16_t AttentionButtonPressedEnable		: 1;
	uint16_t PowerFaultDetectedEnable			: 1;
	uint16_t MRLSensorChangedEnable				: 1;
	uint16_t PresenceDetectChangedEnable		: 1;
	uint16_t CommandCompletedInterruptEnable	: 1;
	uint16_t HotPlugInterruptEnable				: 1;
	uint16_t AttentionIndicatorControl			: 2;
	uint16_t PowerIndicatorControl				: 2;
	uint16_t PowerControllerControl				: 1;
	uint16_t ElectromechanicalInterlockControl	: 1;
	uint16_t DataLinkLayerStateChangedEnable	: 1;
	uint16_t									: 3;
}__packed PCIeSlotControlRegister;
typedef struct{
	uint16_t AttentionButtonPressed				: 1;
	uint16_t PowerFaultDetected					: 1;
	uint16_t MRLSensorChanged					: 1;
	uint16_t PresenceDetectChanged				: 1;
	uint16_t CommandCompleted					: 1;
	uint16_t MRLSensorState						: 1;
	uint16_t PresenceDetectState				: 1;
	uint16_t ElectromechanicalInterlockStatus	: 1;
	uint16_t DataLinkLayerStateChanged			: 1;
	uint16_t									: 7;
}__packed PCIeSlotStatusRegister;
typedef struct{
	PCIeSlotCapabilitiesRegister	Capabilities;
	PCIeSlotControlRegister			Control;
	PCIeSlotStatusRegister			Status;
}__packed PCIeSlotRegisterSet;

typedef struct{
	uint32_t	ExtCAPID	: 8;
	uint32_t	Version		: 4;
	uint32_t	Next		: 20;
}__packed PCIeCapabilitiesHeader, PCIeExtendedCapabilitiesHeader;
typedef struct{
	//	0x10
	PCIeCapabilitiesHeader			Header;
	PCIeCapabilitiesRegister		PCIeCapabilities;
	PCIeDeviceRegisterSet			Device;
	PCIeLinkRegisterSet				Link;
	PCIeSlotRegisterSet				Slot;
	PCIeRootRegisterSet				Root;
}__packed PCIeCapablitiesVersion1;
typedef struct{
	//	0x10
	PCIeCapablitiesVersion1			_V1;
	PCIeDeviceRegisterSet			DeviceRegister;
	PCIeDeviceRegisterSet			Device2;
	PCIeLinkRegisterSet				Link2;
	PCIeSlotRegisterSet				Slot2;
	PCIeRootRegisterSet				Root2;
}__packed PCIeCapablitiesVersion2;


typedef struct{
	//	Peform link equalization. Applies during link retraining at 8 GT/s+.
	uint32_t	LinkEqualisation				: 1;
	uint32_t	LinkEqualisationInterruptEnable	: 1;
	uint32_t									: 7;
	uint32_t	LowerSKPOSGenerationVectorEnable: 1;
	uint32_t									: 22;
}PCIeExtendedCapabilitiesLinkControl3;
typedef struct{
	PCIeExtendedCapabilitiesHeader			Header;
	PCIeExtendedCapabilitiesLinkControl3	LinkControl3;
	uint32_t								LaneErrorStatus;
	uint16_t								LEQ[32];
}__packed PCIeSecondaryExtendedCapabilities_t;

typedef struct{
	uint32_t VFMigrationCapable				: 1;
	uint32_t ARICapableHierarchyPreserved	: 1;
	uint32_t 								: 19;
	uint32_t VFMigrationInterruptMessageNum	: 11;
}__packed PCIeSRIOVCapabilitiesRegister;
typedef struct{
	uint16_t VFEnable					: 1;
	uint16_t VFMigrationEnable			: 1;
	uint16_t VFMigrationInterruptEnable	: 1;
	uint16_t VFMSE						: 1; // Virtual Function Memory Space Enable
	uint16_t ARICapableHierarchy		: 1;
	uint16_t Reserved					: 11;
}__packed PCIeSRIOVControlRegister;
typedef struct{
	uint16_t VFMigrationStatus	: 1;
	uint16_t					: 15;
}__packed PCIeSRIOVStatusRegister;
typedef struct{
	PCIeSRIOVCapabilitiesRegister	Capabilities;
	PCIeSRIOVControlRegister		Control;
	PCIeSRIOVStatusRegister			Status;
}__packed PCIeSRIOVRegisterSet;
//	(Single Root I/O Virtualization
typedef struct{
	PCIeExtendedCapabilitiesHeader	Header;
	PCIeSRIOVRegisterSet			SRIOV;
	uint16_t						InitialVirtualFunctions, 
									TotalVirtualFunctions, 
									NumVirtualFunctions;
	uint16_t						FcnDepLink;
	uint16_t						FirstVirtualFunctionOffset, 
									VirtualFunctionStride;
	uint32_t						SupportedPageSize, 
									SystemPageSize;
	uint32_t						VirtualFunction_BAR[6];
	uint32_t						VirtualFunctionMigrationStateArrayOffset;
}__packed PCIeSRIOVExtendedCapabilities_t;

typedef struct{
	PCIeExtendedCapabilitiesHeader	Header;
	uint32_t						UncorrectableErrorStatus, 
									UncorrectableErrorMask, 
									UncorrectableErrorSeverity, 
									CorrectableErrorStatus, 
									CorrectableErrorMask, 
									AdvancedErrorCapabilitiesAndControl;
	//	Logs the TLP header that caused the error
	uint32_t 						HeaderLog[4];
}__packed PCIeAERExtendedCapability_t;

typedef struct{
	//	N - 1
	uint16_t	TableSize				: 11;
	uint16_t							: 3;
	uint16_t	InterruptDisable		: 1;
	uint16_t	MSIxEnable				: 1;
}__packed PCIeMSIxCapabilityMessageControlRegister;
typedef struct{
	//	N - 1
	uint32_t	MSIxBARIndex	: 3;
	uint32_t	MSIxByteOffset	: 29;
}__packed PCIeMSIxCapabilityTableControlRegister;
typedef struct{
	//	N - 1
	uint32_t	PBABARIndex	: 3;
	uint32_t	PBAByteOffset	: 29;
}__packed PCIeMSIxCapabilityPendingBitArrayRegister;
typedef struct{
	PCIeCapabilitiesHeader						Header;
	PCIeMSIxCapabilityMessageControlRegister	MessageControl;
	PCIeMSIxCapabilityTableControlRegister		TableControl;
	PCIeMSIxCapabilityPendingBitArrayRegister	PendingBitArray;
}__packed PCIeMSIxCapability_t;

typedef struct{
	uint32_t						: 2;
	uint32_t	LogicalAddressMode	: 1;
	uint32_t	RedirectionHint		: 1;
	uint32_t						: 8;
	uint32_t	DestinationID		: 8;
	//	0x0FEE, or the High 12 Bits of the LocalAPIC Base Address.
	uint32_t	Signature			: 12;
}__packed MSIxTableEntryMessageAddressRegister;
typedef struct{
	uint64_t	Vector		: 8;
	uint64_t	DeliveryMode: 3;
	uint64_t				: 3;
	uint64_t	Level		: 1;
	uint64_t	TriggerMode	: 1;
	uint64_t				: 48;
}__packed MSIxTableEntryMessageDataRegister;
typedef struct{
	uint32_t	Masked		: 1;
	uint32_t				: 31;
}__packed MSIxTableEntryVectorControlRegister;
typedef struct{
	MSIxTableEntryMessageAddressRegister	MessageAddressLow;
	uint32_t								MessageAddressHigh;
	MSIxTableEntryMessageDataRegister		MessageData;
	MSIxTableEntryVectorControlRegister		VectorControl;
}__packed MSIxTableEntry_t;

typedef struct{
	union{
		PCIeExtendedCapabilitiesHeader	ExtHeader;
		PCIeCapabilitiesHeader			Header;
	};
	uint8_t							Data[];
}__packed UndefinedPCIeCapability;

LibAPI uint32_t PCIeBARSize(PCIDevice *dev, uint8_t BAR);
LibAPI void *PCIeResolveBar(PCIDevice *Device, uint8_t BAR);
LibAPI void *GetPCIeConfigurationBase(void *acpibase, uint32_t n);
LibAPI PCIDevice *PCIeSearchDevice(void *acpibase, PCIDeviceSpecifier Code, uint32_t N, uint32_t *bus, uint32_t *slot);

LibAPI bool ReadPCIeU32(void *acpibase, uint32_t *dataout, uint32_t n, uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset);
LibAPI bool WritePCIeU32(void *acpibase, uint32_t datain, int32_t n, uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset);

LibAPI bool ReadPCIeVOIDPTR(void *acpibase, void *dataout, uint32_t datasize, uint32_t n, uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset);
LibAPI bool WritePCIeVOIDPTR(void *acpibase, void *datain, uint32_t datasize, uint32_t n, uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset);

LibAPI UndefinedPCIeCapability *ReadPCIeCapabilities(void *acpibase, uint32_t n, uint16_t bus, uint16_t slot, uint16_t func);
LibAPI UndefinedPCIeCapability *SearchPCIeCapabilitiesN(void *acpibase, uint32_t n, 
	uint16_t bus, uint16_t slot, uint32_t *func, PCIeCapabilitiesDeviceType type, uint32_t encounter);

LibAPI bool PCIeEnableMsiX(void *acpibase, uint32_t n, uint16_t bus, uint16_t slot, uint32_t instance, uint32_t entry);
LibAPI bool PCIeDisableMsiX(void *acpibase, uint32_t n, uint16_t bus, uint16_t slot, uint32_t instance, uint32_t entry);
LibAPI MSIxTableEntry_t *PCIeGetMSIxEntry(void *acpibase, uint32_t n, uint16_t bus, uint16_t slot, uint32_t instance, uint32_t entry);