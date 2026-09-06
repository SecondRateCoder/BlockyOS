#include "kernel/services/IO/service.h"


#define ATA_DEV_BUSY            0x80
#define ATA_DEV_DRQ             0x08
#define AHCI_TRANSFERBLOCKSIZE	nKB(8)
#define HBA_PxCMD_ST    	    0x0001
#define HBA_PxCMD_FRE   	    0x0010
#define HBA_PxCMD_FR    	    0x4000
#define HBA_PxCMD_CR    	    0x8000

//	Data payload is sent and received through PRDT (Physical Region Descriptor Table)
typedef volatile struct{
	//	0x00, command list base address, 1K-byte aligned
	uint32_t CommandListBaseLow, CommandListBaseHigh;
	//	0x08, FIS base address, 256-byte aligned
	uint32_t FISBaseLow, FISBaseHigh;
	//	0x10, interrupt status
	uint32_t InterruptStatus;
	//	0x14, interrupt enable
	uint32_t InterruptEnable;
	//	0x18, command and status
	uint32_t CommandStatus;
	//	0x1C, Reserved
	uint32_t rsv0;
	//	0x20, task file data
	uint32_t TaskFileData;
	//	0x24, signature
	uint32_t Signature;
	//	0x28, SATA status (SCR0:SStatus)
	uint32_t SATAStatus;
	//	0x2C, SATA control (SCR2:SControl)
	uint32_t SATAControl;
	//	0x30, SATA error (SCR1:SError)
	uint32_t SATAError;
	//	0x34, SATA active (SCR3:SActive)
	uint32_t SATAActive;
	//	0x38, command issue
	uint32_t CommandIssue;
	//	0x3C, SATA notification (SCR4:SNotification)
	uint32_t SATANotification;
	//	0x40, FIS-based switch control
	uint32_t FISBaseSwitchControl;
	//	0x44 ~ 0x6F, Reserved
	uint32_t rsv1[11];
	//	0x70 ~ 0x7F, vendor specific
	uint32_t Vendor[4];
}__packed HBADevicePort;

enumdef(uint8_t, HBAHostCapabilityISS){
	//	1.5 GBps
	Gen1ISS = 0x1, 
	//	3 GBps
	Gen2ISS = 0x2, 
	//	6 GBps
	Gen3ISS = 0x3
};
typedef const volatile struct{
	uint32_t		NumberOfPorts					: 5;
	uint32_t		ExternalSATA					: 1;
	uint32_t		EnclosureManagement				: 1;
	uint32_t		CoalescingCommandCompletion		: 1;
	uint32_t		NumberOfCommandSlots			: 6;
	uint32_t		PartialStateCapable				: 1;
	uint32_t		SlumberStateCapable				: 1;
	uint32_t		PIOMultipleDRQBlock				: 1;
	uint32_t		FISbaseSwitchingSupport			: 1;
	uint32_t		PortMultiplier					: 1;
	uint32_t		SupportsAHCIModeOnly			: 1;
	uint32_t		InterfaceSpeedSupport			: 4;
	uint32_t		CommandListOverride				: 1;
	uint32_t		ActivityLED						: 1;
	uint32_t		AggressiveLinkPowerManagement	: 1;
	uint32_t		StaggeredSpinUp					: 1;
	uint32_t		MechanicalPresenceSwitch		: 1;
	uint32_t		SNotificationRegister			: 1;
	uint32_t		NativeCommandQueueing			: 1;
	uint32_t		_64BitAddressing				: 1;
}__packed HBAHostCapabilityRegister;
typedef const volatile struct{
	uint32_t		HandoffCapable					: 1;
	uint32_t		NVMHCIPresent					: 1;
	uint32_t		AutoPartialSlumberTransition	: 1;
	uint32_t		DeviceSleepCapable				: 1;
	uint32_t		AggressiveDeviceSleepManagement	: 1;
	uint32_t		DevSleepEntranceSlumberOnly		: 1;
	uint32_t										: 26;
}__packed HBAHostCapabilitiesExtended;
typedef volatile struct{
	uint32_t Write1ToClear	HBAReset				: 1;
	uint32_t 				InterruptEnable			: 1;
	const uint32_t			MSI_SingleMessageRevert	: 1;
	uint32_t										: 28;
	//	If CAP.SupportsAHCIModeOnly is '0', then GHC.AE shall be read-write and shall have a reset value of '0'. 
	//	If CAP.SupportsAHCIModeOnly is '1', then AE shall be read-only and shall have a reset value of '1'
	uint32_t				AHCIEnable				: 1;
}__packed HBAGlobalHostControlRegister;
typedef volatile struct{
	uint32_t	Enable				: 1;
	uint32_t						: 2;
	//	Specifies the interrupt used by the CCC feature. This interrupt must be marked as unused in the Ports Implemented (PI) register by the corresponding bit being set to ‘0’. 
	//	Thus, the CCC interrupt corresponds to the interrupt for an unimplemented port on the controller. 
	//	When a CCC interrupt occurs, the IS.IPS[INT] bit shall be asserted to ‘1’. This field also specifies the interrupt vector used for MSI.
	uint32_t	Interrupt			: 5;
	uint32_t	CommandCompletions	: 8;
	//	Accuracy will be within 5%.
	uint32_t	TimeoutValue1MS		: 16;
}__packed HBACommandCompletionCoalescingControl;
typedef volatile struct{
	uint32_t Write1ToClear	MessageRecieved		: 1;
	uint32_t									: 7;
	uint32_t				TransmitMessage		: 1;
	uint32_t				Reset				: 1;
	uint32_t									: 6;
	uint32_t				LEDMessageType		: 1;
	uint32_t				SAFTEMessageType	: 1;
	uint32_t				SES2MessageType		: 1;
	uint32_t				SGPIOMessageType	: 1;
	uint32_t				SingleMessageBuffer	: 1;
	uint32_t				TransmitOnly		: 1;
	uint32_t				HardwareActivityLED	: 1;
	uint32_t				PortMultiplierEnable: 1;
	uint32_t									: 4;
}__packed HBAEnclosureManagementControl;

//	A controller can support up to 32 ports. 
//	HBA memory registers can be divided into two parts: Generic Host Control registers and Port Control registers. 
//	Generic Host Control registers controls the behavior of the whole controller, while each port owns its own set of Port Control registers. 
//	The actual ports a controller supported and implemented can be calculated from the Capacity register (HBA_MEM.cap) and the Port Implemented register (HBA_MEM.pi). 
//		Each port can attach a single SATA device. 
//		Host sends commands to the device using Command List and device delivers information to the host using Received FIS structure. 
//		They are located at HBA_PORT.clb/clbu, and HBA_PORT.fb/fbu. 
//		The most important part of AHCI initialization is to set correctly these two pointers and the data structures they point to. 
//		Each port points to a Command List which contains the actual command to process.
typedef volatile struct{
	// 0x00 - 0x2B, Generic Host Control
	// 0x00, Host capability
	HBAHostCapabilityRegister				HostCapability;
	// 0x04, Global host control
	HBAGlobalHostControlRegister			GlobalHostControl;
	// 0x08, Interrupt status
	//		There are four kinds of FIS which may be sent to the host by the device. 
	//		When an FIS has been copied into the host specified memory, an according bit will be set.
	uint32_t								InterruptStatus;
	// 0x0C, Port implemented
	uint32_t								PortImplementedBitmask;
	// 0x10, Version
	uint32_t								Version;
	// 0x14, Command completion coalescing control
	HBACommandCompletionCoalescingControl	CommandCompletionCoalescingControl;
	// 0x18, Command completion coalescing ports
	uint32_t								CommandCompletionCoalescingPorts;
	// 0x1C, Enclosure management location
	uint32_t								EnclosureManagementLocation;
	// 0x20, Enclosure management control
	HBAEnclosureManagementControl			EnclosureManagementControl;
	// 0x24, Host capabilities extended
	HBAHostCapabilitiesExtended				HostCapabilitiesExt;
	// 0x28, BIOS/OS handoff control and status
	uint32_t								HandoffControlStatus;
	
	// 0x2C - 0x9F, Reserved
	uint8_t									rsv[0xA0-0x2C];
	
	// 0xA0 - 0xFF, Vendor specific registers
	uint8_t									VendorRegisters[0xFF-0xA0];
	
	// 0x100 - 0x10FF, Port control registers
	// 1 ~ 32
	HBADevicePort							Ports[];
}__packed HBAMemorySpace;

typedef volatile struct{
	uint8_t		CommandFISDWORDLength	: 5;		
	// ATAPI
	uint8_t		ATAPIbit				: 1;		
	// Write, 1: H2D, 0: D2H
	uint8_t		WriteFlag				: 1;		
	// Prefetchable
	uint8_t		Prefetchable			: 1;		
	// Reset
	uint8_t		Reset					: 1;		
	// BIST
	uint8_t		BIST					: 1;		
	// Clear busy upon R_OK
	uint8_t		ClearBusy				: 1;		
	// Reserved
	uint8_t  							: 1;		
	// Port multiplier port
	uint8_t		PortMultiplier			: 4;		
	// Physical region descriptor table length in entries
	uint16_t	PRDTableLength;
	// Physical region descriptor byte count transferred
	uint32_t	PRDTableByteCount;		
	// Command table descriptor base address
	uint64_t	CommandTableBase;
	// Reserved
	uint32_t rsv1[4];	
}__packed HBACommandHeader;

typedef struct{
	uint64_t 		DataBaseAddress;
	//	Reserved
	uint32_t		rsv;
	struct{
		//	Byte count, 4M max
		uint32_t	DataByteCount				: 22;
		//	Reserved
		uint32_t								: 9;
		//	Interrupt on completion
		uint32_t	CompletionInterruptEnable	: 1;
	}DWORD3;
}__packed HBAPhysicalRegionDescriptorEntry;
typedef struct{
	//	0x00
	//	The structure that describes a Command/Function.
	//	Command FIS
	uint8_t								CommandFISStruct[64];

	//	0x40
	//	ATAPI command, 12 or 16 bytes
	uint8_t								ATAPICommand[16];

	//	0x50
	//	Reserved
	uint8_t								rsv[48];

	// 0x80
	HBAPhysicalRegionDescriptorEntry	Table[];	// Physical region descriptor table entries, 0 ~ 65535
}__packed HBACommandTableEntry;


void HBAStartCmd(HBADevicePort *port){
	// Wait until CR (bit15) is cleared
	while(port->CommandStatus & HBA_PxCMD_CR){;}

	// Set FRE (bit4) and ST (bit0)
	port->CommandStatus |= (HBA_PxCMD_FRE | HBA_PxCMD_ST);
}

// Stop command engine
void HBAStopCmd(HBADevicePort *port){
	// Clear ST (bit0)
	port->CommandStatus &= ~HBA_PxCMD_ST;
	// Clear FRE (bit4)
	port->CommandStatus &= ~HBA_PxCMD_FRE;

	// Wait until FR (bit14), CR (bit15) are cleared
	while(true){
		if(port->CommandStatus & HBA_PxCMD_FR){continue;}
		if(port->CommandStatus & HBA_PxCMD_CR){continue;}
		break;
	}
}

enumdef(uint8_t, AHCIFrameInformationStructureType){
	//	Register FrameInformationStructure - host to device
	AHCIFISHostToDeviceRegister					= 0x27, 
	//	Register FrameInformationStructure - device to host
	AHCIFISDeviceToHostRegister					= 0x34, 
	//	DMA activate FrameInformationStructure - device to host
	AHCIFISDeviceToHostDirectMemoryAccess		= 0x39, 
	//	DMA setup FrameInformationStructure - bidirectional
	AHCIFISBidirectionalDirectMemoryAccess		= 0x41, 
	//	Data FrameInformationStructure - bidirectional
	AHCIFISBidirectionalData					= 0x46, 
	//	BIST activate FrameInformationStructure - bidirectional
	AHCIFISBidirectionalBootInfoSelfTest		= 0x58, 
	//	PIO setup FrameInformationStructure - device to host
	AHCIFISDeviceToHostProgrammedInputOutput	= 0x5F, 
	//	Set device bits FrameInformationStructure - device to host
	AHCIFISDeviceToHostDeviceBits				= 0xA1, 
};
#define MaxRegisterSize	\
	__max(sizeof(AHCIARISetDeviceBits), __max(sizeof(AHCIBistActivate), \
		__max(sizeof(AHCIHostToDeviceRegister), __max(sizeof(AHCIDeviceToHostRegister), \
			__max(sizeof(AHCIBidirectionalData), __max(sizeof(AHCIDeviceToHostProgrammableInputOutput), \
				sizeof(AHCIDeviceToHostDirectMemoryAccess)))))))

typedef struct{
    struct{
        AHCIFrameInformationStructureType   Type;
        uint8_t                             PortMultiplier  : 4;
        uint8_t                                             : 2;
        uint8_t                             Notification    : 1; // N_B (Notification) bit
        uint8_t                             Interrupt       : 1;
        uint8_t                             StatusLow       : 3; // ATA Status nibble low
        uint8_t                                             : 1;
        uint8_t                             StatusHigh      : 3; // ATA Status nibble high
        uint8_t                                             : 1;
    }DWORD0;
    uint32_t                                ErrorAndActiveBits; // Error / SActive data mask
} __packed AHCIARISetDeviceBits;

// 2. BIST Activate FIS (Type 0x58) - Bidirectional / Diagnostic
typedef struct{
    struct{
        AHCIFrameInformationStructureType   Type;
        uint8_t                             PortMultiplier  : 4;
        uint8_t                                             : 4;
        uint16_t                            BistFlags;          // BIST pattern/control flags
    }DWORD0;
    uint32_t                                DataWord1;          // Diagnostic pattern data
    uint32_t                                DataWord2;          // Diagnostic pattern data
} __packed AHCIBistActivate;

//	A host to device register FIS is used by the host to send command or control to a device. 
//	As illustrated in the following data structure, it contains the IDE registers such as command, 
//		LBA, device, feature, count and control. An ATA command is constructed in this structure and issued to the device. 
//	All reserved fields in an FIS should be cleared to zero. 
typedef struct{
	struct{
		AHCIFrameInformationStructureType	Type;
		uint8_t								PortMultiplier	: 4;
		uint8_t												: 3;
		//	1: Command, 0: Control
		uint8_t								CommandOrControl: 1;
		UsableDiskCommand					Command;
		uint8_t								FeatureLow;
	}DWORD0;
	struct{
		uint8_t  							LBA0;
		uint8_t								LBA1;
		uint8_t								LBA2;
		uint8_t								Device;
	}DWORD1;
	struct{
		uint8_t								LBA3;
		uint8_t								LBA4;
		uint8_t								LBA5;
		uint8_t								FeatureHigh;
	}DWORD2;
	struct{
		uint16_t							Count;
		uint8_t								IsochronousCommandCompletion;
		uint8_t								Control;
	}DWORD3;
	uint32_t								Reserved;
}__packed AHCIHostToDeviceRegister;

//	A device to host register FIS is used by the device to notify the host that some ATA register has changed. 
//	It contains the updated task files such as status, error and other registers. 
typedef struct{
	struct{
		AHCIFrameInformationStructureType	Type;
		uint8_t								PortMultiplier	: 4;
		uint8_t												: 2;
		uint8_t								Interrupt		: 1;
		uint8_t												: 1;
		uint8_t								Status;
		uint8_t								Error;
	}DWORD0;
	struct{
		uint8_t  							LBA0;
		uint8_t								LBA1;
		uint8_t								LBA2;
		uint8_t								Device;
	}DWORD1;
	struct{
		uint8_t								LBA3;
		uint8_t								LBA4;
		uint8_t								LBA5;
		uint8_t								FeatureHigh;
	}DWORD2;
	struct{
		uint8_t								CountLow;
		uint8_t								CountHigh;
		uint8_t								IsochronousCommandCompletion;
		uint16_t							Reserved;
	}DWORD3;
	uint32_t								Reserved;
}__packed AHCIDeviceToHostRegister;

//	This FIS is used by the host or device to send data payload. The data size can be varied. 
typedef struct{
	AHCIFrameInformationStructureType		Type;
	uint8_t									PortMultiplier	: 4;
	uint8_t													: 4;
	uint16_t  								Reserved;
	uint32_t 								Payload[];
}__packed AHCIBidirectionalData;

//	This FIS is used by the device to tell the host that it’s about to send or ready to receive a PIO data payload. 
typedef struct{
	struct{
		AHCIFrameInformationStructureType	Type;
		uint8_t								PortMultiplier	: 4;
		uint8_t												: 1;
		//	Device to Host.
		uint8_t								Direction		: 1;
		uint8_t								Interrupt		: 1;
		uint8_t												: 1;
		uint8_t								Status;
		uint8_t								Error;
	}DWORD0;
	struct{
		uint8_t  							LBA0;
		uint8_t								LBA1;
		uint8_t								LBA2;
		uint8_t								Device;
	}DWORD1;
	struct{
		uint8_t								LBA3;
		uint8_t								LBA4;
		uint8_t								LBA5;
		uint8_t								Reserved;
	}DWORD2;
	struct{
		uint8_t								CountLow;
		uint8_t								CountHigh;
		uint8_t								Reserved;
		uint8_t								Status;
	}DWORD3;
	uint16_t								TransferCount;
	uint16_t								Reserved;
}__packed AHCIDeviceToHostProgrammableInputOutput;

typedef struct{
	struct{
		AHCIFrameInformationStructureType	Type;
		uint8_t								PortMultiplier	: 4;
		uint8_t												: 1;
		uint8_t								Direction		: 1;
		uint8_t								Interrupt		: 1;
		//	Auto-activate. Specifies if DMA Activate FIS is needed.
		uint8_t								AutoActivated	: 1;
		uint16_t							Reserved;
	}DWORD0;
	//	DMA Buffer Identifier. Used to Identify DMA buffer in host memory.
	//	SATA Spec says host specific and not in Spec. Trying AHCI spec might work.
	uint64_t							DMABufferID;
	uint32_t							Reserved;
	//	Aligned to 4 bytes.
	uint32_t							PayloadOffset;
	//	Aligned to 2 Bytes.
	uint32_t							PayloadSize;
	uint32_t							Reserved1;
}__packed AHCIDeviceToHostDirectMemoryAccess;

typedef struct{
	// Word 0: General configuration flags
    uint16_t general_configuration;
	// Word 1: Number of logical cylinders
    uint16_t logical_cylinders;
	// Word 2: Specific configuration
    uint16_t specific_configuration;
	// Word 3: Number of logical heads
    uint16_t logical_heads;
	// Words 4-5
    uint16_t retired1[2];
	// Word 6: Sectors per track
    uint16_t logical_sectors_per_track;
	// Words 7-9
    uint16_t vendor_unique1[3];
	// Words 10-19: Serial number (20 ASCII chars)
    char     serial_number[20];
	// Words 20-21
    uint16_t retired2[2];
	// Word 22
    uint16_t obsolete1;
	// Words 23-26: Firmware revision (8 ASCII chars)
    char     firmware_revision[8];
	// Words 27-46: Model number (40 ASCII chars)
    char     model_number[40];
	// Word 47: Maximum sectors per interrupt
    uint16_t sectors_per_interrupt;
	// Word 48
    uint16_t double_word_not_supported;
	// Word 49: Capabilities (LBA, DMA support)
    uint16_t capabilities;
	// Word 50
    uint16_t capabilities2;
	// Word 51
    uint16_t pio_data_transfer_cycle;
	// Word 52
    uint16_t dma_data_transfer_cycle;
	// Word 53: Validity of extended data fields
    uint16_t field_validity;
	// Word 54
    uint16_t current_cylinders;
	// Word 55
    uint16_t current_heads;
	// Word 56
    uint16_t current_sectors;
	// Words 57-58: Current capacity in sectors (28-bit)
    uint32_t current_capacity_sectors;
	// Word 59
    uint16_t multi_sector_setting;
	// Words 60-61: Total user-addressable sectors (28-bit LBA)
    uint32_t lba_user_sectors_28bit;
	// Word 62
    uint16_t single_word_dma;
	// Word 63
    uint16_t multi_word_dma;
	// Word 64
    uint16_t advanced_pio_modes;
	// Word 65
    uint16_t min_mw_dma_cycle_time;
	// Word 66
    uint16_t rec_mw_dma_cycle_time;
	//	Word 67
    uint16_t min_pio_cycle_time_no_flow;
	// Word 68
    uint16_t min_pio_cycle_time_flow;
	// Words 69-74
    uint16_t reserved1[6];
	// Word 75: Maximum queue depth (NCQ)
    uint16_t queue_depth;
	// Word 76: SATA capabilities (Gen 1/2/3 speeds)
    uint16_t sata_capabilities;
	// Word 77
    uint16_t sata_additional_caps;
	// Word 78
    uint16_t sata_features_supported;
	// Word 79
    uint16_t sata_features_enabled;
	// Word 80: Major ATA version supported
    uint16_t major_version_number;
	// Word 81: Minor version
    uint16_t minor_version_number;
	// Word 82
    uint16_t command_set_supported1;
	// Word 83: Bit 10 indicates 48-bit LBA support
    uint16_t command_set_supported2;
	// Word 84
    uint16_t command_set_supported3;
	// Word 85
    uint16_t command_set_enabled1;
	// Word 86
    uint16_t command_set_enabled2;
	// Word 87
    uint16_t command_set_default;
	// Word 88: Ultra DMA modes supported/enabled
    uint16_t udma_modes;
	// Word 89
    uint16_t time_security_erase;
	// Word 90
    uint16_t time_enhanced_security;
	// Word 91
    uint16_t advanced_power_management;
	// Word 92
    uint16_t master_password_revision;
	// Word 93
    uint16_t hardware_reset_result;
	// Word 94
    uint16_t acoustic_management;
	// Word 95
    uint16_t stream_minimum_request;
	// Word 96
    uint16_t streaming_transfer_time;
	// Word 97
    uint16_t streaming_access_latency;
	// Words 98-99
    uint32_t streaming_performance;
	// Words 100-103: Total user sectors for 48-bit LBA
    uint64_t lba_user_sectors_48bit;
	// Words 104-127
    uint16_t reserved2[24];
	// Word 128: Drive security lock status
    uint16_t security_status;
	// Words 129-159
    uint16_t vendor_specific[31];
	// Word 160
    uint16_t cfa_power_mode;
	// Words 161-255
    uint16_t reserved3[95];
}__packed ATAIdentifyData;

#include "kernel/libcrt/hardware/IDT/APIC/LocalAPIC.h"

ISRCallbackDefinition(AHCIDefaultHandler){ISRCallbackReturn;}

AHCIConfigureReturn ConfigureAHCIController(void *acpibase, bool *DeviceEnable, uint32_t *LocalAPICs, uint32_t N){
	uint32_t bus = 0, slot = 0, capfunc = 0;
	PCIDevice *Header = PCIeSearchDevice(acpibase, GetPCIstruct(MassStorage_SCSI), N, &bus, &slot);
	if(!Header){Header = PCIeSearchDevice(acpibase, GetPCIstruct(MassStorage_SATA_SerialBus), N, &bus, &slot);}
	if(!Header){Header = PCIeSearchDevice(acpibase, GetPCIstruct(MassStorage_SAS_SerialBus), N, &bus, &slot);}
	if(!Header){Header = PCIeSearchDevice(acpibase, GetPCIstruct(MassStorage_SATA_Vendor), N, &bus, &slot);}
	if(!Header){Header = PCIeSearchDevice(acpibase, GetPCIstruct(MassStorage_SATA_AHCI), N, &bus, &slot);}
	if(!Header){Header = PCIeSearchDevice(acpibase, GetPCIstruct(MassStorage_SAS), N, &bus, &slot);}
	//	No Header Exists
	if(!Header){return (AHCIConfigureReturn){.Data = 0x0, .Flags = CouldntFindDevice};}
	HBAMemorySpace *Space = NULL;
	if(Header->HeaderType.Type == PCIGeneralDevice){
		PCIHeader0x0 *_0x0 = (PCIHeader0x0 *)Header;
		Space = SafeAllocatePages((void *)(((uint64_t)*(uint32_t *)(_0x0->BAR + 5) << 32) + (uint64_t)PCIeGetBar(_0x0, 4)), sizeof(HBAMemorySpace), (ReadWritable | SupervisorMode), 0x0, 0x20);
		// if((Space->HostCapability.NumberOfPorts + 1) > NVectors){return (AHCIConfigureReturn){.Data = Space->HostCapability.NumberOfPorts, .Flags = NotEnoughVectors};}
		if(!Space->HostCapability.SupportsAHCIModeOnly){Space->GlobalHostControl.AHCIEnable = 0x1;}
		UndefinedPCIeCapability *_MSIx = SearchPCIeCapabilitiesN(acpibase, 0, bus, slot, &capfunc, PCIeMSIxCapability, 0x0);
        if(!_MSIx && _MSIx->Header.ExtCAPID != PCIeMSIxCapability){return (AHCIConfigureReturn){.Data = 0x0, .Flags = CouldntFindDevice};}
        PCIeMSIxCapability_t *MSIx = (PCIeMSIxCapability_t *)_MSIx;
		//	We let the Integer overflow
		uint32_t cc = UINT32_MAX, Vector;
		if(MSIx){
			MSIxTableEntry_t *MSIxTable = SafeAllocatePages((void *)(PCIGetBar(_0x0, MSIx->TableControl.MSIxBARIndex) + MSIx->TableControl.MSIxByteOffset), 
				sizeof(MSIxTableEntry_t) * (MSIx->MessageControl.TableSize + 1), (ReadWritable | SupervisorMode), 0x0, 0x20);
			// NVectors = Space->HostCapability.NumberOfPorts + 1;
			while((cc++) < (Space->HostCapability.NumberOfPorts + 1) && AllocateInterruptVector((uint8_t *)&Vector)){
				bool _RSDT = false;
				void *LocalProcessor = GetLocalAPICBase(acpibase, &_RSDT);
				uint32_t APICID = _RSDT? ((RSDT_ProcessorLocalAPIC_t *)LocalProcessor)->ApicID: 
										((XSDT_ProcessorLocalAPIC_t *)LocalProcessor)->ApicID;
				uint8_t _IST;
				IDTEntrySegmentSelector64 sselector;
				AllocateIST(&_IST, &sselector);
				ISRSetCallback(acpibase, Vector, 0x00, _IST, NULL, sselector, false, (ISRCallback *)&AHCIDefaultHandlerISR);
				MSIxTable[cc] = (MSIxTableEntry_t){
					.MessageAddressHigh = /*(uint64_t)LocalProcessor >> 32*/0x00, 
					.MessageAddressLow = {
						.DestinationID = APICID, .LogicalAddressMode = true, .RedirectionHint = false, 
						.Signature = _RSDT? RSDT_LocalAPICSignature: XSDT_LocalAPICSignature
					}, .MessageData = Vector, .VectorControl = {
						.Masked = DeviceEnable? DeviceEnable[cc]: true}
				};
			}
		}else{return (AHCIConfigureReturn){.Data = 0x0, .Flags = CouldntFindMSIxCapabilities};}
		// NVectors = Space->HostCapability.NumberOfPorts + 1;
		cc = UINT32_MAX;
		while((cc++) < (Space->HostCapability.NumberOfPorts + 1)){
			HBADevicePort *Device = Space->Ports + cc;
			Device->InterruptEnable = DeviceEnable? DeviceEnable[cc]: true;
			//	We pre-allocate the Maximum Size needed.
			uint64_t temp = (uint64_t)MapPhysical(AllocatePages(NULL, 
				(Space->HostCapability.NumberOfCommandSlots + 1) * sizeof(HBACommandHeader), (ReadWritable | SupervisorMode), 0x0));
			Device->CommandListBaseHigh = (temp >> 32) & UINT32_MAX;
			Device->CommandListBaseLow = temp & UINT32_MAX;
			temp = (uint64_t)MapPhysical(AllocatePages(NULL, MaxRegisterSize, (ReadWritable | SupervisorMode), 0x0));
			Device->FISBaseHigh = (temp >> 32) & UINT32_MAX;
			Device->FISBaseLow = temp &UINT32_MAX;
		}
	}else{return (AHCIConfigureReturn){.Data = 0x0, .Flags = UnusablePCIDevice};}
	return (AHCIConfigureReturn){.Data = (uint64_t)Space, .Flags = NoError};
}

uint32_t AHCISelfTest(void *acpibase, uint32_t N){
	uint32_t bus = 0, slot = 0;
	PCIDevice *Header = PCIeSearchDevice(acpibase, GetPCIstruct(MassStorage_SCSI), N, &bus, &slot);
	if(!Header){Header = PCIeSearchDevice(acpibase, GetPCIstruct(MassStorage_SATA_SerialBus), N, &bus, &slot);}
	if(!Header){Header = PCIeSearchDevice(acpibase, GetPCIstruct(MassStorage_SAS_SerialBus), N, &bus, &slot);}
	if(!Header){Header = PCIeSearchDevice(acpibase, GetPCIstruct(MassStorage_SATA_Vendor), N, &bus, &slot);}
	if(!Header){Header = PCIeSearchDevice(acpibase, GetPCIstruct(MassStorage_SATA_AHCI), N, &bus, &slot);}
	if(!Header){Header = PCIeSearchDevice(acpibase, GetPCIstruct(MassStorage_SAS), N, &bus, &slot);}
	if(Header){
		if(Header->BIST.BISTCapable){
			Header->BIST.StartBIST = 0x1;
			while(Header->BIST.StartBIST){;}
			return Header->BIST.CompletionCode;
		}
	}
	return 0xFF;
}

HBACommandHeader *AHCISearchOpenCommandSlot(HBAMemorySpace *MS, HBADevicePort *Port, uint32_t *slot){
	uint32_t temp = 0;
	if(!slot){slot = &temp;}
	for(; slot < MS->HostCapability.NumberOfCommandSlots; ++(*slot)){
		if(__check(Port->SATAActive >> *slot, false) && __check(Port->CommandIssue >> *slot, false)){
			return MapVirtual((HBACommandHeader *)((uint64_t)Port->CommandListBaseHigh << 32) + Port->CommandListBaseLow + *slot);
		}
	}
}

ATAIdentifyData *AHCIGetDriveInfo(HBAMemorySpace *MS, HBADevicePort *Port){
	static ATAIdentifyData Out = {0};
	HBACommandHeader *Slot = AHCISearchOpenCommandSlot(MS, Port, NULL);
	HBACommandTableEntry *Table = SafeAllocatePages((void *)Slot->CommandTableBase, 
		sizeof(HBACommandTableEntry), (ReadWritable | SupervisorMode), 0x00, 0x20);
    //	Set up the Host-to-Device Register FIS for IDENTIFY DEVICE (0xEC)
    AHCIHostToDeviceRegister *cfis = SafeAllocatePages(&Table->CommandFISStruct, 
		sizeof(AHCIHostToDeviceRegister), (ReadWritable | SupervisorMode), 0x00, 0x20);
    memset(cfis, 0, sizeof(AHCIHostToDeviceRegister));
    
    cfis->DWORD0.Type = AHCIFISHostToDeviceRegister;
    cfis->DWORD0.CommandOrControl = 1;
    cfis->DWORD0.Command  = IDENTIFY_DEVICE; // IDENTIFY DEVICE

    //	Configure Command Header for a PIO/Data-In transfer
    Slot->CommandFISDWORDLength = sizeof(AHCIHostToDeviceRegister) / sizeof(uint32_t);
    Slot->WriteFlag = false; // Data-in (reading from device)
    Slot->PRDTableLength = 1;

    //	Point the single PRDT entry to our 512-byte identification buffer
    Table->Table[0].DataBaseAddress = (uint32_t)(uintptr_t)MapPhysical(&Out);
    Table->Table[0].DWORD3.CompletionInterruptEnable = true;
    Table->Table[0].DWORD3.DataByteCount = 512 - 1; // 512 bytes minus 1
	return &Out;
}

uint32_t AHCIReadBytes(HBAMemorySpace *Controller, uint32_t Device, uint64_t Position, uint64_t Bytes, void *Out){
	{	//	Safely handle Data.
		ml_t temp = descinfo(Out);
		if(temp.free && temp.mdesc.nbytes < Bytes){return Bytes;}
	}
	if(Device > Controller->HostCapability.NumberOfPorts){return Bytes;}
	if(!__check(Controller->PortImplementedBitmask >> Device, true)){return Bytes;}else{
		HBADevicePort *This = SafeAllocatePages(Controller->Ports + Device, 
            sizeof(HBADevicePort), (ReadWritable | SupervisorMode), 0x00, 0x20);
		uint32_t slot = 0;
		HBACommandHeader *CMDHeader = AHCISearchOpenCommandSlot(Controller, This, &slot);
		ATAIdentifyData *ID = AHCIGetDriveInfo(Controller, This);
		const uint32_t SectorSize = (__check(ID->field_validity, 1 << 12)? (ID->multi_sector_setting * 2): 512);
		const uint64_t Count = (Bytes / SectorSize) + ((Bytes % SectorSize) != 0), LBA = Position / SectorSize;
		if(!CMDHeader){return Bytes;}
		CMDHeader->CommandFISDWORDLength = sizeof(AHCIHostToDeviceRegister) / sizeof(uint32_t);
		CMDHeader->PRDTableLength = (uint16_t)((Bytes - 1) >> 4) + 1;
		CMDHeader->WriteFlag = false;
		HBACommandTableEntry *Table = SafeAllocatePages((void *)CMDHeader->CommandTableBase, 
		    sizeof(HBACommandTableEntry), (ReadWritable | SupervisorMode), 0x00, 0x20);
		for(uint32_t cc = 0; cc < CMDHeader->PRDTableLength; ++cc){
			Table->Table[cc].DataBaseAddress = (uint64_t)MapPhysical(Out + (cc * AHCI_TRANSFERBLOCKSIZE));
			Table->Table[cc].DWORD3.CompletionInterruptEnable = true;
			Table->Table[cc].DWORD3.DataByteCount = __min(AHCI_TRANSFERBLOCKSIZE - 1, Bytes);
			Bytes = Bytes > AHCI_TRANSFERBLOCKSIZE? Bytes - AHCI_TRANSFERBLOCKSIZE: 0;
		}
		*((AHCIHostToDeviceRegister *)SafeAllocatePages(Table->CommandFISStruct, sizeof(AHCIHostToDeviceRegister), 
			(ReadWritable | SupervisorMode), 0x00, 0x20)) = (AHCIHostToDeviceRegister){
				.DWORD0 = {
					.Command = READ_DMA_EXT, 
					.CommandOrControl = true, 
					.Type = AHCIFISHostToDeviceRegister, 
				}, .DWORD1 = {
					.Device = 1 << 6, 
					.LBA0 = (uint8_t)LBA, 
					.LBA1 = (uint8_t)(LBA >> 8), 
					.LBA2 = (uint8_t)(LBA >> 16)
				}, .DWORD2 = {
					.LBA3 = (uint8_t)(LBA >> 24), 
					.LBA4 = (uint8_t)(LBA >> 32), 
					.LBA5 = (uint8_t)(LBA >> 40)
				}, .DWORD3 = {.Count = Count}
			};
		while(This->TaskFileData & (ATA_DEV_BUSY | ATA_DEV_DRQ)){;}
		This->CommandIssue = (1 << slot);
		while(true){
			// In some longer duration reads, it may be helpful to spin on the DPS bit 
			// in the PxIS port field as well (1 << 5)
			if((This->CommandIssue & (1 << slot)) == 0){break;}
		}
	}
	return Bytes;
}

uint32_t AHCIWriteBytes(HBAMemorySpace *Controller, uint32_t Device, uint64_t Position, uint64_t Bytes, void *Out){
	{	//	Safely handle Data.
		ml_t temp = descinfo(Out);
		if(temp.free && temp.mdesc.nbytes < Bytes){return Bytes;}
	}
	if(Device > Controller->HostCapability.NumberOfPorts){return Bytes;}
	if(!__check(Controller->PortImplementedBitmask >> Device, true)){return Bytes;}else{
		HBADevicePort *This = SafeAllocatePages(Controller->Ports + Device, sizeof(HBADevicePort), 
			(ReadWritable | SupervisorMode), 0x00, 0x20);
		uint32_t slot = 0;
		HBACommandHeader *CMDHeader = AHCISearchOpenCommandSlot(Controller, This, &slot);
		ATAIdentifyData *ID = AHCIGetDriveInfo(Controller, This);
		const uint32_t SectorSize = (__check(ID->field_validity, 1 << 12)? (ID->multi_sector_setting * 2): 512);
		const uint64_t Count = (Bytes / SectorSize) + ((Bytes % SectorSize) != 0), LBA = Position / SectorSize;
		if(!CMDHeader){return Bytes;}
		CMDHeader->CommandFISDWORDLength = sizeof(AHCIHostToDeviceRegister) / sizeof(uint32_t);
		CMDHeader->PRDTableLength = (uint16_t)((Bytes - 1) >> 4) + 1;
		CMDHeader->WriteFlag = true;
		HBACommandTableEntry *Table = SafeAllocatePages((void *)CMDHeader->CommandTableBase, 
			sizeof(HBACommandTableEntry), (ReadWritable | SupervisorMode), 0x00, 0x20);
		for(uint32_t cc = 0; cc < CMDHeader->PRDTableLength; ++cc){
			Table->Table[cc].DataBaseAddress = (uint64_t)MapPhysical(Out + (cc * AHCI_TRANSFERBLOCKSIZE));
			Table->Table[cc].DWORD3.CompletionInterruptEnable = true;
			Table->Table[cc].DWORD3.DataByteCount = __min(AHCI_TRANSFERBLOCKSIZE - 1, Bytes);
			Bytes = Bytes > AHCI_TRANSFERBLOCKSIZE? Bytes - AHCI_TRANSFERBLOCKSIZE: 0;
		}
		*((AHCIHostToDeviceRegister *)SafeAllocatePages(Table->CommandFISStruct, 
            sizeof(AHCIHostToDeviceRegister), (ReadWritable | SupervisorMode), 0x00, 0x20)) = (AHCIHostToDeviceRegister){
				.DWORD0 = {
					.Command = READ_DMA_EXT, 
					.CommandOrControl = true, 
					.Type = AHCIFISHostToDeviceRegister, 
				}, .DWORD1 = {
					.Device = 1 << 6, 
					.LBA0 = (uint8_t)LBA, 
					.LBA1 = (uint8_t)(LBA >> 8), 
					.LBA2 = (uint8_t)(LBA >> 16)
				}, .DWORD2 = {
					.LBA3 = (uint8_t)(LBA >> 24), 
					.LBA4 = (uint8_t)(LBA >> 32), 
					.LBA5 = (uint8_t)(LBA >> 40)
				}, .DWORD3 = {.Count = Count}
        };
		while(This->TaskFileData & (ATA_DEV_BUSY | ATA_DEV_DRQ)){;}
		This->CommandIssue = (1 << slot);
		while(true){
			// In some longer duration reads, it may be helpful to spin on the DPS bit 
			// in the PxIS port field as well (1 << 5)
			if((This->CommandIssue & (1<<slot)) == 0){break;}
		}
	}
	return Bytes;
}