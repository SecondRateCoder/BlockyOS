#pragma once

#include "kernel/services/IO/service.h"
#include "kernel/libcrt/hardware/IDT/APIC/LocalAPIC.h"
#include "_NVMe.h"


enumdef(uint8_t, NVMeSubmissionQueueFuseOperationType){
	NVMeNormalOperation = 0x0, NVMeFusedOperationFirstCommand = 0x1, 
	NVMeFusedOperationSecondCommand = 0x2
};
enumdef(uint8_t, NVMeSubmissionQueuePageType){_32AlignedPhysicalRegionPage = 0x00};
enumdef(uint8_t, NVMeSubmissionQueueOperationTransferType){
	NoDataTransfer = 0x00, HostToControllerDataTransfer = 0x01, 
	ControllerToHostDataTransfer = 0x02, BiDirectionalDataTransfer = 0x03
};
typedef union{
	uint128_t			SGLEntry;
	struct{
		uint64_t		PhysicalRegion0, 
						PhysicalRegion1;
					};
}__packed NVMeSubmissionQueueDataPointer;
typedef struct{
	struct{
		union{
			NVMeAdminSubmissionQueueOperations			Operation;
			struct{
				uint8_t	Opcode				: 6;
				uint8_t	Direction			: 2;
			}OperationBits;
		};
		uint8_t			FuseOperationType	: 2;
		uint8_t								: 4;
		uint8_t			PageType			: 2;
		uint16_t		CommandID;
	}__packed			OperationDWORD;
	uint32_t			NamespaceID;
	//	DWORD 3 - 4
	union{
		uint32_t											CommandSpecificDWord0[2];
		NVMeIOAdminAsynchronousEventRequestCompletion0_1_t	AER;
	};
	uint64_t			MetadataPointer;
	union{
		NVMeSubmissionQueueDataPointer						DataPointer;
		NVMeIOAdminDirectiveRecieveCommandPTR_t				DirectiveRecieve;
		NVMeIOAdminDirectiveSendCommandPTR_t				DirectiveSend;
		NVMeIOAdminGetFeaturesCommandPTR_t					GetFeatures;
		NVMeIOAdminGetLogPageCommandPTR_t					GetLogPage;
	}DataDWORD;
	//	Command DWORD 10 - 16
	union{
		uint32_t											DWords[5];
		NVMeIOReadWriteCommand10_t							ReadWrite;
		NVMeIODatasetManagementCommand10_t					DatasetManagement;

		NVMeIOAdminIdentifyNamespaceCommand10_t				IdentifyNamespace;
		NVMeIOAdminCreateControllerDataQueueCommand10_t		CreateDataQueue;
		NVMeIOAdminDeleteControllerDataQueueCommand10_t		DeleteDataQueue;
		NVMeIOAdminDeviceSelfTestCommand10_t				DeviceSelfTest;
		NVMeIOAdminDirectiveRecieveCommand10_t				DirectiveRecieve;
		NVMeIOAdminDirectiveSendCommand10_t					DirectiveSend;
		NVMeIOAdminGetFeaturesCommand10_t					GetFeatures;
		NVMeIOAdminGetLogPageCommand10_t					GetLogpage;
		NVMeIOAdminCreateIOCompletionQueueCommand10_t		CreateCompletionQueue;
		NVMeIOAdminDeleteIOCompletionQueueCommand10_t		DeleteCompletionQueue;
		NVMeIOAdminCreateIOSubmissionQueueCommand10_t		CreateSubmissionQueue;
		NVMeIOAdminDeleteIOSubmissionQueueCommand10_t		DeleteSubmissionQueue;
	}CommandSpecific;
}__packed NVMeSubmissionQueueSlot_t;

enumdef(uint8_t, NVMeCompletionQueueStatusCodeType){
	GenericCommandStatus = 0x00, CommandSpecificStatus = 0x01, 
	MediaIntegrityErrorStatus = 0x02, DataIntegrityErrorStatus = 0x02, 
	//	Indicates that the command specified by the Command and Submission Queue identifier in the completion queue entry has completed. 
	//	These status values are generic across all command types. 
	//	These values may indicate that additional process is required and indicate a status value that is specific to:
	//		The connection between the host and the controller processing the command; or
	//		The characteristics that support Asymmetric Namespace Access Reporting, 
	//			the characteristics of the relationship between the controller processing the command and the specified namespace.
	PathStatus = 0x03
};
enumdef(uint8_t, NVMeCompletionQueueGenericCommandStatus){
	SuccessfulCompletion = 0x00, InvalidCommandOpcode = 0x01, 
	InvalidCommandQueueField = 0x02, CommandIDConflict = 0x03, 
	DataTransferError = 0x04, PowerLossCommandAbortNotification = 0x05, 
	InternalError = 0x06, CommandAbortRequested = 0x07, 
	SubmissionQueueDeleteCommandAbort = 0x08, 
	FailedFuseOperationCommandAbort = 0x09, MissingFuseOperationCommandAbort = 0x0A, 
	InvalidNamespaceError = 0x0B, InvalidFormatError = 0x0B, CommandSequenceError = 0x0C, 
	InvalidSGLSegmentDescriptor = 0x0D, InvalidSGLDescriptorCount = 0x0E, 
	InvalidSGL_Length = 0x0F, MetadataInvalidSGL_Length = 0x10, 
	InvalidSGLDescriptorType = 0x11, InvalidControllerMemoryBufferUsage = 0x12, 
	InvalidPRPOffset = 0x13, AtomicWriteUnitOverflow = 0x14, 
	OperationDeniedError = 0x15, InvalidSGLOffset = 0x16, 
	InconsistentHostIdentifierFormat = 0x18, KeepAliveTimerExpire = 0x19, 
	KeepAliveTimeoutInvalid = 0x1A, CommandPreemptAbort = 0x1B, 
	SanitiseFailError = 0x1C, SanitiseInProgress = 0x1D, 
	InvalidSGLDataBlockGranularity = 0x1E, QueueInCMBError = 0x1F, 
	WriteProtectedNamespaceError = 0x20, CommandInterupted = 0x21, 
	TransientTransportError = 0x22, CommandLockdownProhibition = 0x23, FeatureLockdownProhibition = 0x23, 
	AdminCommandMediaNotReady = 0x24, InvalidKeyTagError = 0x25, HostDispersedNamespaceSupportDisabled = 0x26, 
	UnintialisedHostID = 0x27, IncorrectKeyError = 0x28, FDPDisabled = 0x29, 
	InvalidPlacementHandleList = 0x2A, SanitiseNamespaceFailed = 0x2B, 
	SanitiseNamespaceInProgress = 0x2C, ConfigurationRestoreFailure = 0x2D, 
	OutOfRangeLBAError = 0x80, CapacityExceededError = 0x81, NamespaceNotReadyError = 0x82, 
	ReservationConflictError = 0x83, FormatInProgress = 0x84, InvalidValueSize = 0x85, 
	InvalidKeySize = 0x86, NonExistentKVKey = 0x87, UnrecoveredError = 0x88, KeyExists = 0x89
};
typedef struct{
	union{
		uint32_t											CommandSpecific[2];
		NVMeIOAdminAbortCompletion0_t						Abort;
		NVMeIOAdminAsynchronousEventRequestCompletion0_1_t	AER;
		NVMeIOAdminGetFeaturesCompletion2_t					GetFeatures;
	}CommandSpecific;
	//	The value returned is the value of the SQ Head pointer when the completion queue entry was created. 
	//	By the time a host consumes the completion queue entry, 
	//		the controller may have an SQ Head pointer that has advanced beyond the value indicated.
	uint16_t		SubmissionQueueHeadPtr;
	uint16_t		SubmissionQueueID;
	uint16_t		CommandID;
	//	Indicates the identifier of the command that is being completed. This identifier is assigned by a host when the command is submitted to the Submission Queue. 
	//	The combination of the SQ Identifier and Command Identifier uniquely identifies the command that is being completed. 
	//	The maximum number of requests outstanding for a Submission Queue at one time is 65,535.
	uint16_t		PhaseTag				: 1;
	uint16_t		DoNotRetry				: 1;
	uint16_t		AdditionalLogInfo		: 1;
	//	If the DNR bit is cleared to ‘0’ and the host has set the Advanced Command Retry Enable (ACRE) field to 1h in the Host Behavior Support feature.
	//		A 00b CRD value indicates a command retry delay time of zero (i.e., the host may retry the command immediately).
	//		A 01b CRD value selects the Command Retry Delay Time 1 (CRDT1) field.
	//		A 10b CRD value selects the Command Retry Delay Time 2 (CRDT2) field.
	//		A 11b CRD value selects the Command Retry Delay Time 3 (CRDT3) field.
	uint16_t		CommandRetryDelaySelect	: 2;
	uint16_t		StatusCodeType			: 3;
	uint16_t		StatusCode				: 8;
}__packed NVMeCompletionQueueSlot_t;


enumdef(uint8_t, ControllerPowerScopeType){
	Unreported = 0b00, ControllerScope = 0b01, 
	DomainScope = 0b10, NVMSubsystemScope = 0b11
};
typedef const volatile struct{
	uint64_t		MaximumQueueEntries			: 16;
	uint64_t		ContiguousQueuesRequired	: 1;
	//	Bits	Description
	//	1		Vendor Specific (VS): Vendor Specific arbitration mechanism.
	//	0		Weighted Round Robin with Urgent Priority Class WRRUPC(Weighted Round Robin with Urgent Priority Class arbitration mechanism).
	uint64_t		ArbitrationMechanism		: 2;
	uint64_t									: 5;
	//	This field is in 500 millisecond units.
	uint64_t		WorstCaseTimeout			: 8;
	//	This field indicates the stride between doorbell registers.
	//	The stride is specified as (2 ^ (2 + DSTRD)) in bytes.
	uint64_t		DoorbellStride				: 4;
	uint64_t		NVMCommandSetSupport		: 1;
	uint64_t									: 5;
	uint64_t		IOCommandSetSupport			: 1;
	uint64_t		NoIOCommandSetSupport		: 1;
	uint64_t		BootPartitionSupport		: 1;
	uint64_t		ControllerPowerScope		: 2;
	//	(2 ^ (12 + MinimumHostPageSize)).
	uint64_t		MinimumHostPageSize			: 4;
	//	(2 ^ (12 + MaximumHostPageSize)).
	uint64_t		MaximumHostPageSize			: 4;
	uint64_t		PersistentMemorySupported	: 1;
	uint64_t		ControllerMemoryBuffer		: 1;
	uint64_t		NVMSubsystemShutdown		: 1;
	//	0b10:	The controller supports the Controller Ready Independent of Media mode.
	//	0b01:	The controller supports the Controller Ready With Media mode.
	uint64_t		ControllerReadyModes		: 2;
	uint64_t		ShutdownEnhancement			: 1;
	uint64_t									: 2;
}__packed NVMeControllerCapabilitiesRegister;
typedef volatile struct{
	uint32_t		Enable									: 1;
	uint32_t												: 3;
	//	Set to 111b
	uint32_t		IOCommandSetSelected					: 3;
	uint32_t		MemoryPageSize							: 4;
	uint32_t		SelectedArbitrationMechanism			: 3;
	uint32_t		ShutdownNotification					: 2;
	uint32_t		IOSubmissionQueueSize					: 4;
	uint32_t		IOCompletionQueueSize					: 4;
	uint32_t		ControllerReadyIndependentofMediaMode	: 1;
	uint32_t												: 7;
}__packed NVMeControllerConfigurationRegister;
typedef const volatile struct{
	uint32_t		TertiaryVersion				: 8;
	uint32_t		MinorVersion				: 8;
	uint32_t		MajorVersion				: 16;
}__packed NVMeControllerVersionRegister;
enumdef(uint8_t, NVMeControllerShutdownStatus){NoShutdown = 0b00, ShutdownInProgress = 0b01, ShutdownComplete = 0b10};
typedef volatile struct{
	const uint32_t			Ready						: 1;
	const uint32_t			ControllerFatalStatus		: 1;
	const uint32_t			ShutdownStatus				: 2;
	uint32_t Write1ToClear	NVMSubsystemResetOccured	: 1;
	//	This bit is only valid when CC.EN is set to ‘1’ and CSTS.RDY is set to ‘1’.
	const uint32_t			ProcessingPaused			: 1;
	//	If this bit is set to ‘1’, then CSTS.SHST is reporting the state of an NVM Subsystem
	//		Shutdown and this bit remains set to ‘1’ until an NVM Subsystem Reset occurs.
	//	If this bit is cleared to ‘0’, then CSTS.SHST is reporting the state of a controller shutdown.
	const uint32_t			ShutdownType				: 1;
	uint32_t											: 24;
}__packed NVMeControllerStatus;
typedef volatile struct{
	uint32_t				AdminSubmissionQueueSize	: 12;
	uint32_t											: 4;
	uint32_t				AdminCompletionQueueSize	: 12;
	uint32_t											: 4;
}__packed NVMeControllerAdminQueueAttributes;
//	Enabled by  CAP.CMB
typedef const volatile struct{
	uint32_t			BARIndex					: 3;
	//	Controls whether Memory Restrictions will be propagated.
	uint32_t			MixedMemorySupport			: 1;
	//	If unset then Queues are forced to be contiguous.
	uint32_t			ContiguousQueueEnforcement	: 1;
	//	If unset then Configuration Data has to either be in device Memory or Host Memory, 
	//	If set then Configuration Data can be in either.
	uint32_t			MixedDataEnforcement		: 1;
	//	If set then Pointers can point to Device Memory.
	uint32_t			HostOnlyMemoryEnforcement	: 1;
	//	If this bit is cleared to ‘0’, 
	//		then the I/O Submission Queues and I/O Completion Queues contained in the Controller Memory Buffer are aligned 
	//		as defined by the PRP1 field of a Create I/O Submission Queue command.
	uint32_t			DwordAlignmentEnforcement	: 1;
	uint32_t										: 4;
	//	This is the Offset from the Base Memory that points to the Controller Buffer Memory.
	uint32_t				Offset					: 20;
}__packed NVMeControllerMemoryBufferLocation;
enumdef(uint8_t, NVMeControllerMemoryBufferGranularity){
	_4KiB = 0x0, _64KiB = 0x1, _1MiB = 0x2, _16MiB = 0x3, 
	_256MiB = 0x4, _4GiB = 0x5, _64GiB = 0x6
};
typedef const volatile struct{
	//	If this bit is set to ‘1’, then the controller supports Admin and I/O Completion Queues in the Controller Memory Buffer.
	uint32_t	CompletionQueueStoreSupport		: 1;
	//	If set then the controller supports PRP/SGL Lists being stored in Controller Memory Buffer. 
	uint32_t	ListStoreSupport				: 1;
	uint32_t	WriteSupport					: 1;
	uint32_t	ReadSupport						: 1;
	uint32_t									: 4;
	uint32_t	Granularity						: 4;
	uint32_t	Size							: 20;
}__packed NVMeControllerMemoryBufferSize;
typedef volatile struct{
	uint64_t	CapabilitiesRegisterEnable		: 1;
	uint64_t	ControllerMemorySpaceEnable		: 1;
	uint64_t									: 10;
	uint64_t	ControllerBaseAddrressHighSig	: 52;
}__packed NVMeControllerMemorySpaceControl;
typedef volatile struct{
	NVMeControllerCapabilitiesRegister	Capabilities;
	NVMeControllerVersionRegister		Version;
	uint32_t							InterruptMaskSet;
	uint32_t							InterruptMaskClear;
	NVMeControllerConfigurationRegister	ControllerConfiguration;
	uint32_t							rsv0;
	NVMeControllerStatus				ControllerStatus;
	//	 A write of the value 4E564D65h ("NVMe") to this field initiates an NVM Subsystem Reset.
	union{
		char							NVMeSubsystemResetSTR[4];
		uint32_t						NVMeSubsystemReset;
	};
	NVMeControllerAdminQueueAttributes	AdminQueueAttributes;
	uint64_t							AdminSubmissionQueuePhysicalBase;
	uint64_t							AdminCompletionQueuePhysicalBase;
	union{
		uint8_t							Raw[0x1000 - 0x38];
		struct{
			const NVMeControllerMemoryBufferLocation	MemoryBufferPointer;
			const NVMeControllerMemoryBufferSize		MemoryBufferSize;
			uint8_t										UNUSED0[16];
			NVMeControllerMemorySpaceControl			MemorySpaceControl;
			uint32_t									UNUSED1;
			uint32_t									ControllerBaseAddressInvalid	: 1;
			uint32_t																	: 31;
			uint32_t									UNUSED2;
			//	0x00		Bytes/second
			//	0x01		1KiB/second
			//	0x02		1MiB/second
			//	0x03		1GiB/second
			uint32_t									SustainedWriteThrouputUnits		: 4; 
			uint32_t																	: 4;
			uint32_t									SustainedWriteThroughput		: 24;
		};
	}Vendor;
	uint32_t							Doorbells[];
}__packed NVMeController_t;
// unsigned __ = sizeof(NVMeController_t);

typedef struct NVMeHandle{
	uint32_t					IVector;
	NVMeController_t			*cntrl;
	uint16_t					GlobalQueueCounter, AdminSQ, AdminCQ, *_SQ, *_CQ;
	uint16_t					CommandCounter;
	NVMeSubmissionQueueSlot_t	**Submissions;
	uint32_t					*NPerSQueue, NSubmissions;
	NVMeCompletionQueueSlot_t	**Completions;
	uint32_t					*NPerCQueue, NCompletions;
	uint8_t						FlushFlags;
}NVMeHandle;

typedef struct{
	void							*memory;
	uint64_t						bytes;
	uint16_t						qid, 
									new_head;
	NVMeHandle                      *Hnd;
	CommonMutex						Mutex;
	struct NVMeInterruptStackHeader	*Next;
}__packed NVMeInterruptStackHeader;

// Ring the Submission Queue doorbell to inform hardware of new tail index
#define NVMeRingSQDoorbell(HND, QID, TAIL)																													\
	((volatile uint32_t *)((void *)((HND)->cntrl) + sizeof(NVMeController_t) + (2 * (QID) * (1U << (2 + (HND)->cntrl->Capabilities.DoorbellStride)))))[0] =	\
		(TAIL + 1) % ((HND)->cntrl->AdminQueueAttributes.AdminSubmissionQueueSize + 1);																		\
	TAIL = (TAIL + 1) % ((HND)->cntrl->AdminQueueAttributes.AdminSubmissionQueueSize + 1);																	\
	if(TAIL >= (HND)->NPerSQueue[QID]){TAIL = 0;}
// Ring the Completion Queue doorbell to inform hardware of new head index
#define NVMeRingCQDoorbell(HND, QID, HEAD)																													\
	((volatile uint32_t *)((void *)((HND)->cntrl) + sizeof(NVMeController_t) + (2 * (QID) * (1U << (2 + (HND)->cntrl->Capabilities.DoorbellStride)))))[1] =	\
		(HEAD + 1) % ((HND)->cntrl->AdminQueueAttributes.AdminSubmissionQueueSize + 1);																		\
	HEAD = (HEAD + 1) % ((HND)->cntrl->AdminQueueAttributes.AdminSubmissionQueueSize + 1);																	\
	if(HEAD >= (HND)->NPerSQueue[QID]){HEAD = 0;}

void UpdateNVMeInterruptStackFrame(uint32_t Vector, bool wmemory, void *memory, bool wbytes, uint64_t bytes, 
	bool wqid, uint16_t qid, bool wnew_head, uint16_t new_head, CommonMutex Mutex, NVMeHandle *Hnd
){
	GDTR64 R;
	NVMeInterruptStackHeader *SH = ISRGetStackHeader(Vector);
	uint64_t LocalAPIC = *((uint64_t *)(void *)SH - (sizeof(uint64_t) * 4));
	//	Temporarily Disable Interrupts
	DisableLocalAPIC((void *)LocalAPIC);
	if(SH){
		while(SH->Next && SH->memory){SH = (NVMeInterruptStackHeader *)SH->Next;}
		if(!SH->memory){
			SH->Next = mcalloc(11, sizeof(NVMeInterruptStackHeader));
			SH = (NVMeInterruptStackHeader *)SH->Next;
		}
		if(Mutex){SH->Mutex = Mutex;}else{SH->Mutex = NULL;}
		if(wnew_head){SH->new_head = new_head;}
		if(wmemory){SH->memory = memory;}
		if(wbytes){SH->bytes = bytes;}
		if(wqid){SH->qid = qid;}
        if(Hnd){SH->Hnd = Hnd;}
		EnableLocalAPIC((void *)LocalAPIC);
	}
}

ISRCallbackDefinition(NVMeMarkCompletionQueue){
	//	rdi(cntrl), rsi(qid), rdx(new_tail), rcx, r8, and r9
	static GDTR64 R;
	if(ReadGDTR(&R)){
		GDTSystemSegmentDescriptor64 *D = (void *)R.Base + (sizeof(GDTDescriptor) * Frame->_GDT);
		TSS_t *TSS = (void *)(((uint64_t)D->LinearBaseHigh << 24) + D->LinearBaseLow);

		//	We get the last StackHeader, this allows us to process Interrupts.
		NVMeInterruptStackHeader *SH = (void *)(TSS->IST[Frame->_IST]), *Previous = SH;
		while(SH->Next){Previous = SH;		SH = (NVMeInterruptStackHeader *)SH->Next;}
		mfree(Previous->Next);		Previous->Next = NULL;
		UnlockMutex(SH->Mutex);
		NVMeRingCQDoorbell(SH->Hnd, SH->qid, SH->new_head);
		//	We update the Page to ReadWritable
		static uint32_t Level = 5;
		void *_PT = WalkPageTreeByVirtual(SH->memory, &Level);
		switch(Level){
			case 3: {((PDPTEntryDirectory *)_PT)->ReadWrite = true;		break;} 
			case 2: {((PageDirectoryEntry *)_PT)->ReadWrite = true;		break;} 
			case 1: {((PageTableEntry4KB *)_PT)->ReadWrite = true;		break;} 
			default: {ISRCallbackReturn;}
		}
		InvalidatePages(SH->memory, SH->bytes);
	}
	ISRCallbackReturn;
}

NVMeIdentifyController_t *NVMePollController(NVMeHandle *hnd, CommonMutex Mutex){
    NVMeIdentifyController_t *Controller = mmalloc(sizeof(NVMeIdentifyController_t));
    ((NVMeSubmissionQueueSlot_t *)MapVirtual((void *)hnd->cntrl->AdminSubmissionQueuePhysicalBase))[hnd->AdminSQ] = (NVMeSubmissionQueueSlot_t){
        .OperationDWORD = {.Operation = IdentifyController, .PageType = _32AlignedPhysicalRegionPage, .CommandID = hnd->CommandCounter++}, 
        .MetadataPointer = 0x0, .DataDWORD = {0x0}, .NamespaceID = 0x00, .CommandSpecific.IdentifyNamespace = {
            .ControllerOrNamespaceStructure = IdentifyNamespace, .ControllerID = UINT16_MAX, 
            .CommandSetIdentifier = 0x00, .UUIDIndex = 0x00
        }, .DataDWORD.DataPointer = {.PhysicalRegion0 = (uint64_t)MapPhysical(Controller), .PhysicalRegion1 = 0x00}
    };
    UpdateNVMeInterruptStackFrame(false, true, Controller, true, sizeof(NVMeIdentifyController_t), true, hnd->AdminCQ, true, hnd->AdminCQ, Mutex, hnd);
    LockMutex(Mutex);
    NVMeRingSQDoorbell(hnd, 0, hnd->AdminSQ);
    return Controller;
}

NVMeHandle ConfigureNVMeController(void *acpibase, uint32_t N, uint32_t Priviledge){
	uint32_t Vector, bus, slot;
	PCIDevice *dev = PCIeSearchDevice(acpibase, GetPCIstruct(MassStorage_NVMe_NVMHCI), N, &bus, &slot);
	if(!dev){dev = PCIeSearchDevice(acpibase, GetPCIstruct(MassStorage_NVMe_Express), N, &bus, &slot);}
	if(!dev){dev = PCIeSearchDevice(acpibase, GetPCIstruct(MassStorage_NVMe_Other), N, &bus, &slot);}
	if(dev && AllocateInterruptVector((uint8_t *)Vector)){
		volatile NVMeController_t *cntrl = (volatile NVMeController_t *)PCIeResolveBar(dev, 0x0);
		volatile NVMeControllerStatus *status = (volatile NVMeControllerStatus *)&cntrl->ControllerStatus;

		//	Reset controller if already enabled (CC.EN = 0)
		cntrl->ControllerConfiguration.Enable = false;
		//	Wait for CSTS.RDY == 0
		while(status->Ready){;}

		//	Configure Admin Queue Attributes (0-based entry counts)
		//	64 entries[cite: 2]
		cntrl->AdminQueueAttributes.AdminSubmissionQueueSize = cntrl->Capabilities.MaximumQueueEntries - 1;
		//	64 entries[cite: 2]
		cntrl->AdminQueueAttributes.AdminCompletionQueueSize = cntrl->Capabilities.MaximumQueueEntries - 1;

		//	Set Physical Base Addresses for Admin SQ and CQ
		cntrl->AdminSubmissionQueuePhysicalBase = (uint64_t)MapPhysical(AllocatePages(NULL, (cntrl->Capabilities.MaximumQueueEntries - 1) * sizeof(NVMeSubmissionQueueSlot_t), (ReadWritable | SupervisorMode), 0x0));
		cntrl->AdminCompletionQueuePhysicalBase = (uint64_t)MapPhysical(AllocatePages(NULL, (cntrl->Capabilities.MaximumQueueEntries - 1) * sizeof(NVMeCompletionQueueSlot_t), (ReadWritable | SupervisorMode), 0x0));

		//	Set CC parameters: CSS=0 (NVM Command Set), MPS=0 (4KB Page), EN=1 (Enable)
		// cntrl->ControllerConfiguration = (0 << 16) | (0 << 7) | (0 << 4) | 1U; //[cite: 2]
		cntrl->ControllerConfiguration.IOCommandSetSelected = 0x0;
		cntrl->ControllerConfiguration.MemoryPageSize = 0x0;
		cntrl->ControllerConfiguration.IOCompletionQueueSize = sizeof(NVMeCompletionQueueSlot_t);
		cntrl->ControllerConfiguration.IOSubmissionQueueSize = sizeof(NVMeSubmissionQueueSlot_t);
		cntrl->ControllerConfiguration.SelectedArbitrationMechanism = 0x00;
		cntrl->ControllerConfiguration.ShutdownNotification = 0x00;
		//	We need to create a Stack Object for allowing us to perform the Ringing of the Completion Queue Ring as well as the Local APIC EOI
		
		uint8_t _IST;
		IDTEntrySegmentSelector64 sselector;
		NVMeInterruptStackHeader *_temp = mcalloc(1, sizeof(NVMeInterruptStackHeader));
		if(!AllocateIST(&_IST, &sselector)){return (NVMeHandle){0};}
		if(!ISRSetCallback(acpibase, Vector, Priviledge, _IST, _temp, sselector, false, (ISRCallback *)&NVMeMarkCompletionQueueISR)){return (NVMeHandle){0};}
		cntrl->ControllerConfiguration.Enable = true;
		
		//	Wait for Controller Ready (CSTS.RDY == 1)
		while(!(status->Ready)){;}
		//	Success
		NVMeHandle temp = (NVMeHandle){
			.CommandCounter = 0x00, .AdminCQ = 0x00, .AdminSQ = 0x00, 
			.GlobalQueueCounter = 0x00, .NCompletions = 31, .NSubmissions = 31, 
			._SQ = mcalloc(32, sizeof(uint16_t)), 
			._CQ = mcalloc(32, sizeof(uint16_t)), 
			.NPerCQueue = mcalloc(32, sizeof(uint32_t)), 
			.NPerSQueue = mcalloc(32, sizeof(uint32_t)), 
			.Submissions = mcalloc(32, sizeof(NVMeSubmissionQueueSlot_t)), 
			.Completions = mcalloc(32, sizeof(NVMeSubmissionQueueSlot_t)), 
			.cntrl = cntrl, .FlushFlags = 0x00
		};
		InitMutex(Mtx);
		NVMeIdentifyController_t *IC = NVMePollController(&temp, Mtx);
		MutexPoll(Mtx);
		temp.FlushFlags = (IC->vwc >> 1) & 0x3;
		mfree(IC);
		return temp;
	}
	//	Device Not Found
	return (NVMeHandle){0};
}

NVMeIdentifyNamespaceData_t *NVMePollNamespace(NVMeHandle *hnd, CommonMutex Mutex){
	NVMeIdentifyNamespaceData_t *Namespace = mmalloc(sizeof(NVMeIdentifyNamespaceData_t));
	((NVMeSubmissionQueueSlot_t *)MapVirtual((void *)hnd->cntrl->AdminSubmissionQueuePhysicalBase))[hnd->AdminSQ] = (NVMeSubmissionQueueSlot_t){
		.OperationDWORD = {.Operation = Identify, .PageType = _32AlignedPhysicalRegionPage, .CommandID = hnd->CommandCounter++}, 
		.MetadataPointer = 0x0, .DataDWORD = {0x0}, .NamespaceID = 0x00, .CommandSpecific.IdentifyNamespace = {
			.ControllerOrNamespaceStructure = IdentifyNamespace, .ControllerID = UINT16_MAX, 
			.CommandSetIdentifier = 0x00, .UUIDIndex = 0x00
		}, .DataDWORD.DataPointer = {.PhysicalRegion0 = (uint64_t)MapPhysical(Namespace), .PhysicalRegion1 = 0x00}
	};
	UpdateNVMeInterruptStackFrame(false, true, Namespace, true, sizeof(NVMeIdentifyNamespaceData_t), true, hnd->AdminCQ, true, hnd->AdminCQ, Mutex, hnd);
	LockMutex(Mutex);
	NVMeRingSQDoorbell(hnd, 0, hnd->AdminSQ);
	return Namespace;
}


void NVMeFreeIOSubmissionQueue(NVMeHandle *hnd, uint32_t Queue, CommonMutex Mutex){
	void *Physical = MapPhysical(hnd->Submissions[Queue]);
	FreeAlignedPages(hnd->Submissions[Queue]);
	memcpy(hnd->Submissions + Queue, hnd->Submissions + Queue + 1, sizeof(NVMeSubmissionQueueSlot_t) * (hnd->NPerSQueue[hnd->NSubmissions] - (Queue + 1)));
	hnd->NPerSQueue[hnd->NSubmissions]--;
	((NVMeSubmissionQueueSlot_t *)MapVirtual((void *)hnd->cntrl->AdminSubmissionQueuePhysicalBase))[hnd->AdminSQ] = (NVMeSubmissionQueueSlot_t){
		.OperationDWORD = {
			.Operation = DeleteIOSubmissionQueue,
			.PageType = _32AlignedPhysicalRegionPage,
			.CommandID = hnd->CommandCounter++
		}, .MetadataPointer = 0x0, .DataDWORD = {
			//	Allocate buffer memory for the target I/O Submission Queue depth
			.DataPointer = {.PhysicalRegion0 = (uint64_t)MapPhysical(hnd->Submissions[hnd->NSubmissions]), .PhysicalRegion1 = 0x0}
		}, .CommandSpecific.DeleteSubmissionQueue = {.QueueID = Queue}
	};
	//	We dont create an entry on the first
	UpdateNVMeInterruptStackFrame(hnd->IVector, true, NULL, true, 0x00, true, 0x00, true, hnd->AdminCQ, Mutex, hnd);
	LockMutex(Mutex);
	//	Advance Admin SQ Tail (SQ0) and ring Admin SQ Doorbell (qid = 0)
	hnd->NSubmissions++;
	NVMeRingSQDoorbell(hnd, 0x00, hnd->AdminSQ);
}

void NVMeFreeIOCompletionQueue(NVMeHandle *hnd, uint32_t Queue, CommonMutex Mutex){
	void *Physical = MapPhysical(hnd->Submissions[Queue]);
	FreeAlignedPages(hnd->Submissions[Queue]);
	memcpy(hnd->Submissions + Queue, hnd->Submissions + Queue + 1, sizeof(NVMeCompletionQueueSlot_t) * (hnd->NPerSQueue[hnd->NSubmissions] - (Queue + 1)));
	hnd->NPerSQueue[hnd->NSubmissions]--;
	((NVMeSubmissionQueueSlot_t *)MapVirtual((void *)hnd->cntrl->AdminSubmissionQueuePhysicalBase))[hnd->AdminSQ] = (NVMeSubmissionQueueSlot_t){
		.OperationDWORD = {
			.Operation = DeleteIOCompletionQueue,	
			.PageType = _32AlignedPhysicalRegionPage,
			.CommandID = hnd->CommandCounter++
		}, .MetadataPointer = 0x0, .DataDWORD = {
			//	Allocate buffer memory for the target I/O Submission Queue depth
			.DataPointer = {.PhysicalRegion0 = (uint64_t)MapPhysical(hnd->Submissions[hnd->NSubmissions]), .PhysicalRegion1 = 0x0}
		}, .CommandSpecific.DeleteCompletionQueue = {.QueueID = Queue}
	};
	//	We dont create an entry on the first
	UpdateNVMeInterruptStackFrame(hnd->IVector, true, NULL, true, 0x00, true, 0x00, true, hnd->AdminCQ, Mutex, hnd);
	LockMutex(Mutex);
	//	Advance Admin SQ Tail (SQ0) and ring Admin SQ Doorbell (qid = 0)
	hnd->NSubmissions++;
	NVMeRingSQDoorbell(hnd, 0x00, hnd->AdminSQ);
}

uint32_t NVMeAllocateIOSubmissionQueues(NVMeHandle *hnd, uint16_t NQueues, CommonMutex Mutex){
	if(((hnd->NSubmissions % 32) == 0) ){
		//	We allocate on every 32
		hnd->Submissions = mrealloc(hnd->Submissions, sizeof(void *) * (hnd->NSubmissions + 32));
		hnd->NPerSQueue = mrealloc(hnd->NPerSQueue, sizeof(uint32_t) * (hnd->NSubmissions + 32));
		hnd->_SQ = mrealloc(hnd->_SQ, sizeof(uint16_t) * (hnd->NSubmissions + 32));
		memset(hnd->_SQ + hnd->NCompletions, 0, sizeof(uint16_t) * 32);
	}
	//	Base physical address of new queue
	hnd->Submissions[hnd->NSubmissions] = AllocateAlignedPages(NULL, NQueues * sizeof(NVMeSubmissionQueueSlot_t), (ReadOnly | SupervisorMode), 0x00, 0x20);
	hnd->NPerSQueue[hnd->NSubmissions] = __min(hnd->cntrl->Capabilities.MaximumQueueEntries + 1, NQueues);
	//	Use current Admin SQ slot index as Command ID
	//	Populate Admin Create I/O Submission Queue command (SQ0)
	((NVMeSubmissionQueueSlot_t *)MapVirtual((void *)hnd->cntrl->AdminSubmissionQueuePhysicalBase))[hnd->AdminSQ] = (NVMeSubmissionQueueSlot_t){
		.OperationDWORD = {
			.Operation = CreateIOSubmissionQueue,
			.PageType = _32AlignedPhysicalRegionPage,
			.CommandID = hnd->CommandCounter++
		}, .MetadataPointer = 0x0, .DataDWORD = {
			//	Allocate buffer memory for the target I/O Submission Queue depth
			.DataPointer = {.PhysicalRegion0 = (uint64_t)MapPhysical(hnd->Submissions[hnd->NSubmissions]), .PhysicalRegion1 = 0x0}
		}, .CommandSpecific.CreateSubmissionQueue = {
			.QueueID = hnd->GlobalQueueCounter++,
			.QueueSize = __min(hnd->cntrl->Capabilities.MaximumQueueEntries, NQueues - 1), // 0-based entry count
			.PhysicallyContiguous = true,
			.QueuePriority = QueuePriorityUrgent,
			.CompletionQueueID = hnd->AdminCQ
		}
	};
	//	We dont create an entry on the first
	UpdateNVMeInterruptStackFrame(hnd->IVector, true, hnd->Submissions[hnd->NSubmissions], true, 
		NQueues * sizeof(NVMeSubmissionQueueSlot_t), true, 0x00, true, hnd->AdminCQ, Mutex, hnd);
	LockMutex(Mutex);
	//	Advance Admin SQ Tail (SQ0) and ring Admin SQ Doorbell (qid = 0)
	hnd->NSubmissions++;
	NVMeRingSQDoorbell(hnd, 0x00, hnd->AdminSQ);
    return hnd->NSubmissions - 1;
}

uint32_t NVMeAllocateIOCompletionQueue(NVMeHandle *hnd, uint16_t NQueues, uint16_t IVector, CommonMutex Mutex){
    if(((hnd->NCompletions % 32) == 0) ){
		//	We allocate on every 32
		hnd->Completions = mrealloc(hnd->Completions, sizeof(void *) * (hnd->NCompletions + 32));
		hnd->NPerCQueue = mrealloc(hnd->NPerCQueue, sizeof(uint32_t) * (hnd->NCompletions + 32));
		hnd->_CQ = mrealloc(hnd->_CQ, sizeof(uint16_t) * (hnd->NCompletions + 32));
		memset(hnd->_CQ + hnd->NCompletions, 0, sizeof(uint16_t) * 32);
	}
	//	Base physical address of new queue
	hnd->Completions[hnd->NCompletions] = AllocateAlignedPages(NULL, NQueues * sizeof(NVMeCompletionQueueSlot_t), (ReadOnly | SupervisorMode), 0x00, 0x20);
	hnd->NPerCQueue[hnd->NCompletions] = __min(hnd->cntrl->Capabilities.MaximumQueueEntries + 1, NQueues);
	//	Use current Admin SQ slot index as Command ID
	//	Populate Admin Create I/O Submission Queue command (SQ0)
	((NVMeSubmissionQueueSlot_t *)MapVirtual((void *)hnd->cntrl->AdminSubmissionQueuePhysicalBase))[hnd->AdminSQ] = (NVMeSubmissionQueueSlot_t){
		.OperationDWORD = {
			.Operation = CreateIOCompletionQueue,
			.PageType = _32AlignedPhysicalRegionPage,
			.CommandID = hnd->CommandCounter++
		}, .MetadataPointer = 0x0, .DataDWORD = {
			//	Allocate buffer memory for the target I/O Submission Queue depth
			.DataPointer = {.PhysicalRegion0 = (uint64_t)MapPhysical(hnd->Completions[hnd->NCompletions]), .PhysicalRegion1 = 0x00}
		}, .CommandSpecific.CreateCompletionQueue = {
			.QueueID = hnd->GlobalQueueCounter++, 
			.QueueSize = __min(hnd->cntrl->Capabilities.MaximumQueueEntries, NQueues - 1), 
			.InterruptVector = IVector, 
			.PhysicallyContiguous = true, 
			.InterruptsEnabled = true
		}
	};
	//	We dont create an entry on the first
	UpdateNVMeInterruptStackFrame(hnd->IVector, true, hnd->Completions[hnd->NCompletions], true, 
		NQueues * sizeof(NVMeCompletionQueueSlot_t), true, 0x00, true, hnd->AdminCQ, Mutex, hnd);
	LockMutex(Mutex);
	//	Advance Admin SQ Tail (SQ0) and ring Admin SQ Doorbell (qid = 0)
	hnd->NCompletions++;
	NVMeRingSQDoorbell(hnd, 0x00, hnd->AdminSQ);
    return hnd->NCompletions - 1;
}

void NVMeFlushVolatileData(NVMeHandle *hnd, uint32_t Queue, uint32_t CompletionQueue, CommonMutex Mtx){
	uint32_t NSID = 0x00;
	switch(hnd->FlushFlags){
		case 0b00: {NSID = 0b01;		break;}
		case 0b10: {NSID = 0b01;		break;}
		case 0b01: {NSID = UINT32_MAX;	break;}
	}
	hnd->Submissions[Queue][hnd->_SQ[Queue]].OperationDWORD.Operation = 0x00;
	hnd->Submissions[Queue][hnd->_SQ[Queue]].OperationDWORD.PageType = _32AlignedPhysicalRegionPage;
	hnd->Submissions[Queue][hnd->_SQ[Queue]].OperationDWORD.CommandID = hnd->CommandCounter++;
	hnd->Submissions[Queue][hnd->_SQ[Queue]].MetadataPointer = (uint64_t)NULL;
	hnd->Submissions[Queue][hnd->_SQ[Queue]].NamespaceID = NSID;
	// ((NVMeSubmissionQueueSlot_t *)MapVirtual(hnd->cntrl->AdminSubmissionQueuePhysicalBase))[hnd->AdminSQ] = (NVMeSubmissionQueueSlot_t){
	// 	.OperationDWORD = {
	// 		.Operation = 0x00,
	// 		.PageType = _32AlignedPhysicalRegionPage,
	// 		.CommandID = hnd->CommandCounter++
	// 	}, .MetadataPointer = 0x0, .DataDWORD = {
		// 		//	Allocate buffer memory for the target I/O Submission Queue depth
	// 		.DataPointer = {.PhysicalRegion0 = MapPhysical(hnd->Completions[hnd->NCompletions]), .PhysicalRegion1 = 0x0}
	// 	}, .NamespaceID = NSID
	// };
	UpdateNVMeInterruptStackFrame(hnd->IVector, true, NULL, true, 0x00, true, Queue, true, CompletionQueue, Mtx, hnd);
	LockMutex(Mtx);
	NVMeRingSQDoorbell(hnd, Queue, hnd->_SQ[Queue]);
	return;
}

bool NVMeIOReadBytes(NVMeHandle *hnd, uint32_t Queue, uint32_t CompletionQueue, uint64_t Position, uint64_t Bytes, void *Out, CommonMutex Mutex){
	if(!(Queue && Out)){return false;}
	{	//	Safely handle Data.
		ml_t temp = descinfo(Out);
		if(temp.free && temp.mdesc.nbytes < Bytes){return false;}
	}
	if(Queue >= hnd->NSubmissions){return false;}
	InitMutex(Mtx);
	NVMeIdentifyNamespaceData_t *IDN = NVMePollNamespace(hnd, Mtx);
	FreeMutex(Mtx);
	uint32_t SectorSize = 1U << IDN->LBAF[IDN->ActiveLBAFormat].LBADataSize;
	uint64_t LBA = Position / SectorSize, NBlocks = (Bytes / SectorSize) + (Bytes % SectorSize != 0);
	hnd->Submissions[Queue][hnd->_SQ[Queue]].OperationDWORD.Operation = NVMeIORead;
	hnd->Submissions[Queue][hnd->_SQ[Queue]].DataDWORD.DataPointer.PhysicalRegion0 = (uint64_t)MapPhysical(Out);
	hnd->Submissions[Queue][hnd->_SQ[Queue]].CommandSpecific.ReadWrite = (NVMeIOReadWriteCommand10_t){
		.SLBALower = LBA & UINT32_MAX, .SLBAUpper = (LBA >> 32) & UINT32_MAX, 
		.NumberOfBlocks = NBlocks, .BypassCache = false, .LimitedErrorRecoveryAttempts = 0, 
		.AccessFrequency = 0, .AccessLatency = 0, .SequentialRequest = 0, .Incompressible = 0, 
		.ILBRT = 0, .LBAT = 0, .LBATM = 0
	};
	uint32_t Level = 5;
	void *_PT = WalkPageTreeByVirtual(Out, &Level);
	switch(Level){
		case 3: {((PDPTEntryDirectory *)_PT)->ReadWrite = false;	break;} 
		case 2: {((PageDirectoryEntry *)_PT)->ReadWrite = false;	break;} 
		case 1: {((PageTableEntry4KB *)_PT)->ReadWrite = false;		break;} 
		default:{return false;}
	}
	InvalidatePages(Out, Bytes);
	UpdateNVMeInterruptStackFrame(hnd->IVector, true, Out, true, Bytes, true, CompletionQueue, true, hnd->_CQ[Queue], Mutex, hnd);
	NVMeRingSQDoorbell(hnd, Queue, hnd->_SQ[Queue]);
	return true;
}

bool NVMeIOWriteBytes(NVMeHandle *hnd, uint32_t Queue, uint32_t CompletionQueue, uint64_t Position, uint64_t Bytes, void *In, CommonMutex Mutex){
	if(!(Queue && In)){return false;}
	{	//	Safely handle Data.
		ml_t temp = descinfo(In);
		if(temp.free && temp.mdesc.nbytes < Bytes){return false;}
	}
	if(Queue >= hnd->NSubmissions){return false;}
	InitMutex(Mtx);
	NVMeIdentifyNamespaceData_t *IDN = NVMePollNamespace(hnd, Mtx);
	FreeMutex(Mtx);
	uint32_t SectorSize = 1U << IDN->LBAF[IDN->ActiveLBAFormat].LBADataSize;
	uint64_t LBA = Position / SectorSize, NBlocks = (Bytes / SectorSize) + (Bytes % SectorSize != 0);
	hnd->Submissions[Queue][hnd->_SQ[Queue]].OperationDWORD.Operation = NVMeIORead;
	hnd->Submissions[Queue][hnd->_SQ[Queue]].DataDWORD.DataPointer.PhysicalRegion0 = (uint64_t)MapPhysical(In);
	hnd->Submissions[Queue][hnd->_SQ[Queue]].CommandSpecific.ReadWrite = (NVMeIOReadWriteCommand10_t){
		.SLBALower = LBA & UINT32_MAX, .SLBAUpper = (LBA >> 32) & UINT32_MAX, 
		.NumberOfBlocks = NBlocks, .BypassCache = false, .LimitedErrorRecoveryAttempts = 0, 
		.AccessFrequency = 0, .AccessLatency = 0, .SequentialRequest = 0, .Incompressible = 0, 
		.ILBRT = 0, .LBAT = 0, .LBATM = 0
	};
	uint32_t Level = 5;
	void *_PT = WalkPageTreeByVirtual(In, &Level);
	switch(Level){
		case 3: {((PDPTEntryDirectory *)_PT)->ReadWrite = false;	break;} 
		case 2: {((PageDirectoryEntry *)_PT)->ReadWrite = false;	break;} 
		case 1: {((PageTableEntry4KB *)_PT)->ReadWrite = false;		break;} 
		default:{return false;}
	}
	InvalidatePages(In, Bytes);
	UpdateNVMeInterruptStackFrame(hnd->IVector, true, In, true, Bytes, true, CompletionQueue, true, hnd->_CQ[Queue], Mutex, hnd);
	NVMeRingSQDoorbell(hnd, Queue, hnd->_SQ[Queue]);
	return true;
}