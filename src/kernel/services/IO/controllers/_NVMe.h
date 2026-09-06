#pragma once

#include "kernel/libcrt/hardware/IO/IO.h"
#include "kernel/services/IO/service.h"
// #include "kernel/libcrt/hardware/PCIe/Devices.h"

#define NVMeAdminCommandStruct(NAME, DWORD)		__packed NVMeIOAdmin##NAME##Command##DWORD##_t
#define NVMeAdminCompletionStruct(NAME, DWORD)	__packed NVMeIOAdmin##NAME##Completion##DWORD##_t
#define NVMeCommandStruct(NAME, DWORD)			__packed NVMeIO##NAME##Command##DWORD##_t
#define NVMeCompletionStruct(NAME, DWORD)		__packed NVMeIO##NAME##Completion##DWORD##_t

enumdef(uint8_t, NVMeAdminSubmissionQueueOperations){
	DeleteIOSubmissionQueue = 0x00, CreateIOSubmissionQueue = 0x01, 
	GetLogPage = 0x02, DeleteIOCompletionQueue = 0x04, 
	CreateIOCompletionQueue = 0x05, Identify = 0x06, Abort = 0x08, 
	SetFeatures = 0x09, GetFeatures = 0x0A, AsynchronousEventRequest = 0x0C, 
	NamespaceManagement = 0x0D, FirmwareCommit = 0x10, FirmwareImageDownload = 0x11, 
	NASQQDeviceSelfTest = 0x14, NamespaceAttachment = 0x15, KeepAlive = 0x18, 
	DirectiveSend = 0x19, DirectiveRecieve = 0x1A, VirtualisationManagement = 0x1C, 
	NVMeMI_Send = 0x1D, NVMeMI_Recieve = 0x1E, CapacityManagement = 0x20, 
	DiscoveryInformationManagement = 0x21, FabricZoningRecieve = 0x22, 
	Lockdown = 0x24, FabricZoningLookup = 0x25, ClearExportedNVMResourceConfiguration = 0x28, 
	FabricZoningSend = 0x29, ManageExportedNVMSubsystemRecieve = 0x2A, 
	ManageExportedNVMSubsystemSend = 0x2D, ManageExportedNamespace = 0x31, 
	ManageExportedPort = 0x35, NASQQCrossControllerReset = 0x38, 
	SendDiscoveryLogPage = 0x39, TrackSend = 0x3D, TrackRecieve = 0x3E, 
	MigrationSend = 0x41, MigrationRecieve = 0x42, NASQQControllerDataQueue = 0x45, 
	DoorbellBufferConfig = 0x7C, FabricsCommand = 0x7F, FormatNVM = 0x80, 
	SecuritySend = 0x81, SecurityRecieve = 0x82, Sanitise = 0x84, 
	LoadProgram = 0x85, GetLBAStatus = 0x86, 
	ProgramActivationManagement = 0x88, MemoryRangeSetManagement = 0x89, 
	SanitiseNamespace = 0x8C
};

typedef struct{
	uint16_t		SubmissionQueueID;
	//	This field specifies the command identifier of the command to be aborted,
	//		that was specified in the CDW0.CID field within the command itself.
	uint16_t		CommandID;
}NVMeAdminCommandStruct(Abort, 10);
typedef struct{
	uint32_t		ImmediateAbortNotPerformed	: 1;
	uint32_t									: 31;
}NVMeAdminCompletionStruct(Abort, 0);
#define NVMeAdminAbortCommandListExceeded		0x03

enumdef(uint8_t, NVMeAsynchronousEventRequestType){
	AERTErrorStatus = 0b000, AERTHealthStatus = 0b001, AERTSMART = 0b001, 
	AERTNotice = 0b010, AERTImmediate = 0b011, AERTOneShot = 0b100
};
enumdef(uint8_t, NVMeAsynchronousEventInformationStatus){
	WriteToInvalidDoorbellRegister = 0x00, InvalidDoorbellWriteValue, 
	DiagnosticFailure, PersistentInternalError, TransientInternalError, 
	FirmwareImageLoadError, 
};
enumdef(uint8_t, NVMeAsynchronousEventInformationSMARTStatus){NAEISNVMSubsystemReliability = 0x00, NAEISTemperatureThreshold, NAEISSpareBelowThreshold};
enumdef(uint8_t, NVMeAsynchronousEventInformationNoticeStatus){
	AttachedNamespaceAttributeChanged = 0x00, FirmwareActivating, TelemetryLogChanged, 
	AssymetricNamespaceAccessChange, PredictableLatencyEventAggregateLogChange, 
	LBAStatusInformationAlert, EnduranceGroupEventAggregateLogPageChange, 
	ReachabilityGroupChange, ReachabilityAssociationChange, 
	AllocatedNamespaceAttributeChanged, RateLimitingConfigurationChange, 
	ZoneDescriptorChanged = 0xEF, DiscoveryLogPageChange, HostDiscoveryLogPageChange, 
	AVEDiscoveryLogPageChange, NAEINSPullModelDDCRequest, CrossControllerResetCompleted, 
	NAEINSLostHostCommunication
};
enumdef(uint8_t, NVMeAsynchronousEventInformationIOCommandSpecificStatus){
	ReservationLogPageAvailable = 0x00, SanitizeOperationCompleted, 
	SanitizeOperationCompletedWithUnexpectedDeallocation, 
	SanitizeOperationEnteredMediaVerificationState
};
enumdef(uint8_t, NVMeAsynchronousEventInformationImmediateStatus){NVMSubsystemNormalShutdown = 0x00, TemperatureThresholdHysteresisRecovery};
enumdef(uint8_t, NVMeAsynchronousEventInformationOneShotStatus){
	ControllerDataQueueTailPointer = 0x00, ControllerDataQueueFullError, 
	PowerThresholdExceeded, VoltageThresholdEvent
};
typedef struct{
	uint32_t	AsynchronousEventType	: 3;
	uint32_t							: 5;
	uint32_t	AsynchronousEventID		: 8;
	uint32_t	LogPageID				: 8;
	uint32_t							: 8;
	uint32_t	EventSpecificParameter;
}NVMeAdminCompletionStruct(AsynchronousEventRequest, 0_1);

enumdef(uint8_t, NVMeControllerDataQueueSelect){CreateControllerQueue, DeleteControllerQueue};
enumdef(uint8_t, NVMeControllerDataQueueManagementOp){UserDataMigrationQueue = 0x00};
typedef struct{
	uint32_t	ManagementOp			: 8;
	uint32_t							: 8;
	uint16_t	SelectSpecific			: 16;

	//	If set to ‘1’, then the Controller Data Queue is physically contiguous and PRP Entry 1 (PRP1) is the address of a contiguous physical buffer. 
	//	If cleared to ‘0’, then the Controller Data Queue is not physically contiguous and PRP Entry 1 (PRP1) is a PRP List pointer.
	uint32_t	PhysicallyContiguous	: 1;
	uint32_t							: 15;
	uint32_t	QueueTypeSpecific		: 16;
	uint32_t	QueueDWORDSize;
}NVMeAdminCommandStruct(CreateControllerDataQueue, 10);
typedef struct{
	uint32_t	ManagementOp			: 8;
	uint32_t							: 8;
	uint16_t	SelectSpecific			: 16;
	uint32_t							: 16;
	uint32_t	QueueID					: 16;
}NVMeAdminCommandStruct(DeleteControllerDataQueue, 10);
//	The submission Queue, PhysicalRegion0 points to a Data Region

enumdef(uint8_t, NVMeDeviceSelfTestCode){
	ShortDeviceSelfTest = 0x1, ExtDviceSelfTest, 
	HostInitiatedRefresh, AbortDeviceSelfTest = 0xF
};
typedef struct{
	uint32_t	SelfTestCode			: 4;
	uint32_t							: 28;
	uint32_t	RSV[4];
	uint32_t	SelfTestParameter;
}NVMeAdminCommandStruct(DeviceSelfTest, 10);

enumdef(uint8_t, NVMeDirectiveType){IdentifyDirectiveType = 0x00, StreamDirectiveType = 0x01, DataPlacement = 0x02};
typedef uint128_t NVMeAdminCommandStruct(DirectiveRecieve, PTR);
typedef struct{
	uint32_t	NDwords;
	uint8_t		DirectiveOperation;
	uint8_t		DirectiveType;
	//	01h		Specifies the identifier of the stream associated with the data.
	//	02h		Specifies the Placement Identifier used to determine where to write the user data
	//			within non-volatile storage of the Endurance Group associated with the namespace.
	uint16_t	TypeDependentField;
}NVMeAdminCommandStruct(DirectiveRecieve, 10);

typedef uint128_t NVMeAdminCommandStruct(DirectiveSend, PTR);
typedef struct{
	uint32_t	NDwords;
	uint8_t		DirectiveOperation;
	uint8_t		DirectiveType;
	//	01h		Specifies the identifier of the stream associated with the data.
	//	02h		Specifies the Placement Identifier used to determine where to write the user data
	//			within non-volatile storage of the Endurance Group associated with the namespace.
	uint16_t	TypeDependentField;
}NVMeAdminCommandStruct(DirectiveSend, 10);

typedef uint128_t NVMeAdminCommandStruct(GetFeatures, PTR);
enumdef(uint8_t, NVMeGetFeaturesAttributeSelect){Current = 0x00, Default = 0x01, Saved = 0x02, SupportedCapabilities = 0x03};
enumdef(uint8_t, NVMeGetFeaturesFeatureID){
	Arbitration = 0x01, PowerManagement, NGFFITemperatureThreshold = 0x04, 
	VolatileWriteCache = 0x06, NumberOfQueues, InterruptCoalescing, 
	InterruptVectorConfiguration, AsynchronousEventConfiguration = 0x0B, 
	AutonomousPowerStateTransition, HostMemoryBuffer, Timestamp, KeepAliveTimer, 
	HostControlledThermalManagement, NonOperationalPowerStateConfig, 
	ReadRecoveryLevelConfig, PredictableLatencyModeConfig, 
	PredictableLatencyModeWindow, HostBehaviourSupport, SanitizeConfig, 
	EnduranceGroupEventConfiguration, IOCommandSetProfile, SpinupControl, 
	PowerLossSignalingConfig, FlexibleDataPlacement, FlexibleDataPlacementEvents, 
	NamespaceAdminLabel, NGFFIControllerDataQueue, ConfigurableDevicePersonality, 
	PowerLimit, PowerThreshold, NGFFIPowerMeasurement, VoltageThreshold, 
	NGFFIVoltageMeasurement, EmbeddedManagementControllerAddress = 0x78, 
	HostManagementAgentAddress, EnhancedControllerMetadata = 0x7D, 
	ControllerMetadata, NamespaceMetadata, SoftwareProgressMarker, 
	HostIdentifier, ReservationNotificationMask, ReservationPersistence, 
	NamespaceWriteProtectionConfig, BootPartitionWriteProtectionConfig, 
};
typedef struct{
	uint32_t	FeatureID		: 8;
	uint32_t	AttributeSelect	: 4;
	uint32_t					: 20;
	uint32_t	RSV[4];
	uint32_t	UUIDIndex		: 7;
	uint32_t					: 25;
}NVMeAdminCommandStruct(GetFeatures, 10);
typedef struct{
	uint32_t	Saveable		: 1;
	uint32_t	NamespaceScope	: 1;
	uint32_t	Changeable		: 1;
	uint32_t					: 29;
}NVMeAdminCompletionStruct(GetFeatures, 2);

enumdef(uint8_t, NVMeGetLogPageLogPageID){
	SupportedLogPages = 0x00, ErrorInformation, 
	SMART, HealthInformation = SMART, 
	FirmwareSlotInformation, 
	ChangedAttachedNamespaceList, 
	CommandsSupportedAndEffects, NGLPLPIDeviceSelfTest, 
	TelemetryHostInitiated, TelemetryControllerInitiated, 
	EnduranceGroupInformation, PredictableLatencyPerNVMSet, 
	AsymmetricNamespaceAccess, PersistentEventLog, 
	SetSpecification, EnduranceGroupEventAggregate, 
	MediaUnitStatus, SupportedCapacityConfigurationList, 
	FeatureIdentifiersSupportedAndEffects, 
	NVMeMICommandsSupportedAndEffects, 
	CommandFeatureLockdown, BootPartition, 
	RotationalMediaInformation, DispersedNamespaceParticipatingNVMSubsystems, 
	ManagementAddressList, ReachabilityGroups, ReachabilityAssociations, 
	ChangedAllocatedNamespaceList, DevicePersonalities, NGLPLPICrossControllerReset, 
	NGLPLPILostHostCommunication, FDPConfigurations, ReclaimUnitHandleUsage, 
	FDPStatistics, FDPEvents, ManufacturerDefaultConfiguration, NGLPLPIPowerMeasurement, 
	NGLPLPIVoltageMeasurement = 0x27, Discovery = 0x70, HostDiscovery, AVEDiscovery, 
	NGLPLPIPullModelDDCRequest, SanitizeNamespaceStatusList, ReservationNotification, SanitizeStatus
};
typedef uint128_t NVMeAdminCommandStruct(GetLogPage, PTR);
typedef struct{
	uint32_t	LogPageID		: 8;
	uint32_t	LogSpecificParam: 7;
	uint32_t	RetainAsyncEvent: 1;
	uint32_t	LowDwordCount	: 16;
	uint32_t	HighDwordCount	: 16;
	uint32_t	LogSpecificID	: 16;
	uint32_t	LowLogPageOffset, 
				HighLogPageOffset;
	uint32_t	UUIDIndex		: 7;
	uint32_t					: 16;
	//	If this bit is set to ‘1’, then the Log Page Offset Lower field and the Log Page Offset 
	//		Upper field specify the index into the list of data structures in the log page to be returned. 
	//	If this bit is cleared to ‘0’, then the Log Page Offset Lower field and 
	//		the Log Page Offset Upper field specify the byte offset into the log page to be returned.
	uint32_t	OffsetType		: 1;
	uint32_t	CommandSetID	: 8;
}NVMeAdminCommandStruct(GetLogPage, 10);

#define NVMeLIDTableLength	256
#define NVMeLogStruct(NAME) __packed NVMeLog##NAME##Information

typedef struct{
	uint32_t	GetLogPageSupport	: 1;
	uint32_t	IndexOffsetSupport	: 1;
	uint32_t						: 14;
	uint32_t	LIDSpecificParameter: 16;
}NVMeLogStruct(Support), NVMeLogSupportTable[NVMeLIDTableLength];

typedef struct{
    uint64_t	ErrorCount;
    uint16_t	SubmissionQueueID;
    uint16_t	CommandID;
    uint16_t	StatusField; // Phase, Status Code, Status Code Type
    uint16_t	ParameterErrorLocation; // Byte/bit offset
    uint64_t	LBA;
    uint32_t	NamespaceID;
    uint8_t		VendorSpecificInformationAvailable;
    uint8_t		TRTYPE; // Transport Type
    uint8_t		Reserved0[2];
    uint64_t	CommandSpecificInformation;
    uint16_t	TransportTypeSpecificErrorInformation;
    uint8_t		Reserved1[22];
}NVMeLogStruct(Error); // 64 bytes per entry

typedef struct{
    uint8_t		CriticalWarning;
    uint16_t	TemperatureKelvin;
    uint8_t		AvailableSpare;
    uint8_t		AvailableSpareThreshold;
    uint8_t		PercentageUsed;
    uint8_t		EnduranceGroupSummary;
    uint8_t		Reserved0[25];
	// 128-bit value (in 1000s of 512-byte units)
    uint128_t	DataUnitsRead;
	// 128-bit value
    uint128_t	DataUnitsWritten;
	// 128-bit value
    uint128_t	HostReadCommands;
	// 128-bit value
    uint128_t	HostWriteCommands;
	// 128-bit value (minutes)
    uint128_t	ControllerBusyTime;
	// 128-bit value
    uint128_t	PowerCycles;
	// 128-bit value
    uint128_t	PowerOnHours;
	// 128-bit value
    uint128_t	UnsafeShutdowns;
    uint64_t	MediaAndDataIntegrityErrors[2];
    uint64_t	NumberOfErrorInformationLogEntries[2];
    uint32_t	WarningCompositeTemperatureTime;
    uint32_t	CriticalCompositeTemperatureTime;
    uint16_t	TemperatureSensor[8];
    uint32_t	ThermalManagementTemperatureTransitionCount[2];
    uint32_t	TotalTimeForThermalManagementTemperature[2];
    uint8_t		Reserved1[280];
}NVMeLogStruct(SMARTInformation); // 512 bytes

typedef struct{
    struct{
        uint8_t	ActiveSlot	: 3;
        uint8_t				: 1;
        uint8_t	PendingSlot	: 3;
        uint8_t				: 1;
    }ActiveFirmwareInfo;
    uint8_t		Reserved0[7];
    uint64_t	FirmwareRevisionSlot[7]; // Up to 7 slots, 8 ASCII chars each
    uint8_t		Reserved1[448];
}NVMeLogStruct(FirmwareSlot); // 512 bytes

typedef uint32_t NVMeLogStruct(ChangedNamespaceList); // 4096 bytes

// typedef struct __attribute__((packed)) {
//     uint32_t CommandSupportedAndEffect; // CSUP, LBCC, NCC, NIC, CCC, CSE
// }NVMeCommandSupportedAndEffectEntry;
typedef struct{
	uint32_t	CommandsSupported			: 1;
	uint32_t	LogicalBlockContentChange	: 1;
	uint32_t	NamespaceCapabilityChange	: 1;
	uint32_t	NamespaceInventoryChange	: 1;
	uint32_t	ControllerCapabilityChange	: 1;
	uint32_t								: 9;
	//	Command Submission and Execution Relaxations
	uint32_t	CSER						: 2;
	//	Command Submission and Execution
	uint32_t	CSE							: 4;
	uint32_t	UUIDSelectionSupport		: 1;
	uint32_t	NamespaceScope				: 1;
	uint32_t	ControllerScope				: 1;
	uint32_t	NVMSetScope					: 1;
	uint32_t	EnduranceGroupScope			: 1;
	uint32_t	DomainScope					: 1;
	uint32_t	NVMSubsystemScope			: 1;
	uint32_t								: 6;
	// uint32_t	CommandScope				: 12;
}NVMeLogStruct(CommandsSupportandEffectsEntry);

typedef struct{
    NVMeLogCommandsSupportandEffectsEntryInformation	AdminCommandSupport[256];
    NVMeLogCommandsSupportandEffectsEntryInformation	IOCommandSupport[256];
    uint8_t Reserved[2048];
}NVMeLogStruct(CommandsSupportandEffectsTable); // 4096 bytes

typedef struct{
    uint8_t			SelfTestStatus;
    uint8_t			SelfTestCompletionPercentage;
    uint8_t			Reserved0[2];
    uint64_t		CurrentOperationLBA;
    uint32_t		CurrentOperationNSID;
    uint8_t			FailingSENSEKey;
    uint8_t			FailingAdditionalSenseCode;
    uint8_t			FailingAdditionalSenseCodeQualifier;
    uint8_t			ValidFields;
    // Followed by 20 Self-Test Result Data Structure entries (28 bytes each)
    struct{
        uint8_t		SelfTestResult;
        uint8_t		SegmentNumber;
        uint8_t		ValidDiagnosticInfo;
        uint8_t		Reserved;
        uint64_t	PowerOnHours;
        uint32_t	NamespaceID;
        uint64_t	FailingLBA;
        uint8_t		StatusCodeType;
        uint8_t		StatusCode;
        uint8_t		VendorSpecific[2];
    }__packed ResultData[20];
    uint8_t Reserved1[484];
}NVMeLogStruct(DeviceSelfTest); // 512 bytes

typedef struct{
    uint8_t		LogIdentifier;
    uint8_t		Reserved0[4];
    uint8_t		IEEE_OUI[3];
    uint16_t	TelemetryControllerDataBlock32BitUnitsBlock1;
    uint16_t	TelemetryControllerDataBlock32BitUnitsBlock2;
    uint16_t	TelemetryControllerDataBlock32BitUnitsBlock3;
    uint8_t		Reserved1[368];
    uint8_t		TelemetryControllerDataAvailable;
    uint8_t		TelemetryControllerInitiatedDataGenerationNumber;
    uint8_t		TelemetryControllerReasonIdentifier[128];
}NVMeLogStruct(Telemetry); // 512-byte header followed by vendor blocks

typedef struct{
    uint32_t	EnduranceEstimate[2]; // 128-bit
    uint32_t	DataUnitsRead[2];
    uint32_t	DataUnitsWritten[2];
    uint32_t	MediaUnitsWritten[2];
    uint8_t		HostReadCommands[16];
    uint8_t		HostWriteCommands[16];
    uint8_t		MediaAndDataIntegrityErrors[16];
    uint8_t		NumberOfErrorInformationLogEntries[16];
    uint8_t		Reserved0[160];
}NVMeLogStruct(EnduranceGroup); // 512 bytes

typedef struct{
    uint32_t	ANAState; // 0x01: Optimized, 0x02: Non-Optimized, 0x03: Inaccessible
    uint32_t	NumberOfNamespaces;
    uint64_t	ANAGroupChangeCount;
    uint8_t		Reserved[16];
    // Followed by uint32_t NSIDs[]
}NVMeLogStruct(ANAGroupDescriptor);
typedef struct{
    uint64_t	ChangeCount;
    uint16_t	NumberOfANAGroupDescriptors;
    uint8_t		Reserved[6];
    // Array of NVMeANAGroupDescriptor entries follows
}NVMeLogStruct(ANALog);

typedef struct{
    uint16_t	SanitizeProgress; // 0 to 65535 (percentage scale)
    uint16_t	SanitizeStatus;   // Status flags, global sanitize status
    uint32_t	GlobalDataEraseProgress;
    uint32_t	OverwriteSanitizeCompletedPasses;
    uint32_t	TimeForOverwrite50Percent;
    uint32_t	TimeForBlockErase50Percent;
    uint32_t	TimeForCryptoErase50Percent;
    uint8_t		Reserved[484];
}NVMeLogStruct(SanitizeStatusLog); // 512 bytes

typedef struct{
	uint32_t	QueueID				: 16;
	uint32_t	QueueSize			: 16;
	uint32_t	PhysicallyContiguous: 1;
	uint32_t	InterruptsEnabled	: 1;
	uint32_t						: 14;
	uint32_t	InterruptVector		: 16;
}NVMeAdminCommandStruct(CreateIOCompletionQueue, 10);

enumdef(uint8_t, NVMeCreateIOSubmissionQueuePriority){QueuePriorityUrgent = 0x00, QueuePriorityHigh, QueuePriorityMedium, QueuePriorityLow};
typedef struct{
	//	This field specifies the identifier to assign to the Submission Queue to be created.
	uint32_t	QueueID				: 16;
	uint32_t	QueueSize			: 16;
	uint32_t	PhysicallyContiguous: 1;
	uint32_t	QueuePriority		: 2;
	uint32_t						: 13;
	//	This field specifies the identifier of the I/O Completion Queue to 
	//		utilize for any command completions entries associated with this Submission Queue.
	uint32_t	CompletionQueueID	: 16;
	uint32_t	NVMSetID			: 16;
	uint32_t						: 16;
}NVMeAdminCommandStruct(CreateIOSubmissionQueue, 10);

typedef struct{
	uint32_t	QueueID				: 16;
	uint32_t						: 16;
}NVMeAdminCommandStruct(DeleteIOCompletionQueue, 10);
typedef struct{
	uint32_t	QueueID				: 16;
	uint32_t						: 16;
}NVMeAdminCommandStruct(DeleteIOSubmissionQueue, 10);

// 4-byte LBA Format Descriptor (16 entries in Identify Namespace, offset 128)
typedef struct {
    uint16_t MetadataSize;         // DW0 [15:0]: Metadata size in bytes
    uint8_t  LBADataSize;          // DW0 [23:16]: Data size (2^DS bytes)
    uint8_t  RelativePerformance; // DW0 [31:24]: 0 = Best, 3 = Degraded
} __packed NVMeLBAFormat_t;

// Selected fields from 4096-byte Identify Namespace Data Structure
typedef struct {
	//	Offset 0: Total size in LBAs (NSZE)
    uint64_t		NamespaceSize;
	//	Offset 8: Allocatable LBAs (NCAP)
    uint64_t		NamespaceCapacity;
    uint8_t			Reserved0[10];
	//	Offset 26: Formatted LBA Size [3:0] = active format index
    uint8_t			ActiveLBAFormat				: 4;
	uint8_t			FormattedLBASize			: 4;
    uint8_t			Reserved1[101];
	//	Offset 128: Supported LBA formats
    NVMeLBAFormat_t	 LBAF[16];
    uint8_t			Reserved2[3904];
} __packed NVMeIdentifyNamespaceData_t;

enumdef(uint8_t, NVMeIdentifyCNS){
    IdentifyNamespace                     = 0x00,
    IdentifyController                    = 0x01,
    ActiveNamespaceIDList                 = 0x02,
    NamespaceIdentificationDescriptorList = 0x03,
    AllocatedNamespaceIDList              = 0x10,
    IdentifyAllocatedNamespace            = 0x11
};
typedef struct{
    uint32_t ControllerOrNamespaceStructure : 8;	// DW10 [7:0]: CNS Field
    uint32_t Reserved0                      : 8;	// DW10 [15:8]
    uint16_t ControllerID;							// DW10 [31:16]: CNTID

    uint32_t Reserved1                      : 24;	// DW11 [23:0]
    uint32_t CommandSetIdentifier           : 8;	// DW11 [31:24]: CSI (0x00 = NVM)

    uint32_t Reserved2[2];							// DW12 - DW13

    uint32_t UUIDIndex                      : 7;	// DW14 [6:0]
    uint32_t Reserved3                      : 25;	// DW14 [31:7]

    uint32_t Reserved4;								// DW15
}NVMeAdminCommandStruct(IdentifyNamespace, 10);

typedef struct{
    // Controller Capabilities and Features (Bytes 0-255)
	//	0-1:   PCI Vendor ID
    uint16_t		vid;
	//	2-3:   PCI Subsystem Vendor ID
    uint16_t		ssvid;
	//	4-23:  Serial Number (ASCII)
    char			sn[20];
	//	24-63: Model Number (ASCII)
    char			mn[40];
	//	64-71: Firmware Revision (ASCII)
    char			fr[8];
	//	72:    Recommended Arbitration Burst
    uint8_t			rab;
	//	73-75: IEEE OUI Identifier
    uint8_t			ieee[3];
	//	76:    Controller Multi-Path I/O Capabilities
    uint8_t			cmic;
	//	77:    Maximum Data Transfer Size
    uint8_t			mdts;
	//	78-79: Controller ID
    uint16_t		cntlid;
	//	80-83: Version
    uint32_t		ver;
	//	84-87: RTD3 Resume Latency
    uint32_t		rtd3r;
	//	88-91: RTD3 Entry Latency
    uint32_t		rtd3e;
	//	92-95: Optional Asynchronous Events Supported
    uint32_t		oaes;
	//	96-99: Controller Attributes
    uint32_t		ctratt;
	//	100-255: Reserved
    uint8_t			rsvd100[156];

    //	Admin Command Set Attributes (Bytes 256-511)
	//	256-257: Optional Admin Command Support
    uint16_t		oacs;
	//	258:     Abort Command Limit
    uint8_t			acl;
	//	259:     Asynchronous Event Request Limit
    uint8_t			aerl;
	//	260:     Firmware Updates
    uint8_t			frmw;
	//	261:     Log Page Attributes
    uint8_t			lpa;
	//	262:     Error Log Page Entries
    uint8_t			elpe;
	//	263:     Number of Power States Supported
    uint8_t			npss;
	//	264:     Admin Vendor Specific Command Config
    uint8_t			avscc;
	//	265:     Autonomous Power State Transition
    uint8_t			apsta;
	//	266-267: Warning Composite Temperature
    uint16_t		wctemp;
	//	268-269: Critical Composite Temperature
    uint16_t		cctemp;
	//	270-511: Reserved (Truncated for brevity)
    uint8_t			rsvd270[242];

    //	NVM Command Set Attributes (Bytes 512-703)
	//	512:     Submission Queue Entry Size
    uint8_t			sqes;
	//	513:     Completion Queue Entry Size
    uint8_t			cqes;
	//	514-515: Maximum Outstanding Commands
    uint16_t		maxcmd;
	//	516-519: Number of Namespaces
    uint32_t		nn;
	//	520-521: Optional NVM Command Support
    uint16_t		oncs;
	//	522-523: Fused Operation Support
    uint16_t		fuses;
	//	524:     Format NVM Attributes
    uint8_t			fna;
	//	525:     Volatile Write Cache
    uint8_t			vwc;
	//	526-527: Atomic Write Unit Normal
    uint16_t		awun;
	//	528-529: Atomic Write Unit Power Fail
    uint16_t		awupf;
	//	530:     NVM Vendor Specific Command Config
    uint8_t			icsvscc;
	//	531-703: Reserved
    uint8_t			rsvd531[173];

	//	704-2047: Reserved / I/O Command Set
    uint8_t			rsvd704[1344];
	//	2048-3071: Power State Descriptors
    uint8_t			psd[1024];
	//	3072-4095: Vendor Specific
    uint8_t			vs[1024];
}__packed NVMeIdentifyController_t;







enumdef(uint8_t, NVMeIOSubmissionQueueOperations){
    NVMeIOFlush = 0x00, NVMeIOWrite = 0x01,
    NVMeIORead = 0x02, NVMeIOWriteUncorrectable = 0x04,
    NVMeIOCompare = 0x05, NVMeIOWriteZeroes = 0x08,
    NVMeIODatasetManagement = 0x09, // Used for TRIM / Deallocate hints
    NVMeIOVerify = 0x0C, NVMeIOGenerateCopy = 0x19
};
enumdef(uint8_t, NVMeIOCompletionStatusCode){
    // Generic Command Status (Type 0x0)
    NVMeLBAOutOfRange = 0x80,
    NVMeCapacityExceeded = 0x81,
    NVMeNamespaceNotReady = 0x82,
    NVMeReservationConflict = 0x83,
    NVMeFormatInProgress = 0x84,

    // Media & Data Integrity Errors (Type 0x2)
    NVMeWriteFault = 0x80,
    NVMeUnrecoveredReadError = 0x81,
    NVMeEndToEndGuardCheckError = 0x82,
    NVMeEndToEndApplicationTagError =0x83,
    NVMeEndToEndReferenceTagError = 0x84,
    NVMeCompareFailure = 0x85,
    NVMeAccessDenied = 0x86,
    NVMeDeallocatedOrUnwrittenLBA = 0x87
};

typedef struct{
	//	DW10: Starting LBA [31:0]
    uint32_t SLBALower;
	//	DW11: Starting LBA [63:32]
    uint32_t SLBAUpper;

    uint16_t NumberOfBlocks;        // DW12 [15:0]: 0-based sector count (0 = 1 sector)
    uint16_t Reserved0						: 10;
    uint16_t DirectiveType					: 4;  // DW12 [29:26]
	//	DW12 bit 30: Bypass volatile cache on write
    uint16_t BypassCache					: 1;
	//	DW12 bit 31: Limit target error recovery
    uint16_t LimitedErrorRecoveryAttempts	: 1;  

	//	DW13 [3:0]: Dataset Management access frequency hint
    uint32_t AccessFrequency	: 4;
	//	DW13 [5:4]: DSM access latency hint
    uint32_t AccessLatency		: 2;
	//	DW13 bit 6: Sequential access optimization hint
    uint32_t SequentialRequest	: 1;
	//	DW13 bit 7: Data compression hint
    uint32_t Incompressible		: 1;
    uint32_t Reserved1			: 24;

	//	DW14: Expected Initial Logical Block Reference Tag
    uint32_t ILBRT;                 
	//	DW15 [15:0]: Logical Block Application Tag
    uint16_t LBAT;                  
	//	DW15 [31:16]: Logical Block Application Tag Mask
    uint16_t LBATM;                 
}NVMeCommandStruct(ReadWrite, 10);

// 16-byte Range Descriptor sent via PRP1 buffer when executing Dataset Management
typedef struct{
	uint32_t ContextAttributes;
	uint32_t LengthInBlocks;
	uint64_t StartingLBA;
}__packed NVMeDatasetManagementRange_t;
typedef struct{
	//	DW10 [7:0]: 0-based range count (0 = 1 range descriptor)
    uint32_t NumberOfRanges		: 8;  
	//	DW10 bit 8: Send TRIM/Deallocate notification
    uint32_t EnableDeallocate	: 1;  
	//	DW10 bit 9
    uint32_t EnableIntegralRead	: 1;  
	//	DW10 bit 10
    uint32_t EnableIntegralWrite: 1;  
    uint32_t Reserved0			: 21;
    uint32_t Reserved1[5];            // DW11-DW15
}NVMeCommandStruct(DatasetManagement, 10);
