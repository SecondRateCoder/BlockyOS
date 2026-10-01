#pragma once

#include "kernel/services/IO/service.h"
#include "kernel/libcrt/hardware/IDT/APIC/LocalAPIC.h"
#include "_NVMe.h"





// unsigned __ = sizeof(NVMeController_t);

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