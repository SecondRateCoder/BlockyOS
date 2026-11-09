#include "ISR.h"
#include "kernel/libcrt/math/float.h"

#include "kernel/libcrt/hardware/GDT/GDT.h"
#include "kernel/libcrt/hardware/IDT/APIC/LocalAPIC.h"

#include "kernel/libcrt/SimpleOutput/SimpleOutput.h"

ISRCallbackDefinition(Generic);
ISRCallbackDefinition(DivideError);
ISRCallbackDefinition(DebugException);
ISRCallbackDefinition(NonMaskableExt);
ISRCallbackDefinition(Breakpoint);
ISRCallbackDefinition(Overflow);
ISRCallbackDefinition(BoundRangeException);
ISRCallbackDefinition(InvalidOpcode);
ISRCallbackDefinition(UnavailableDevice);
ISRCallbackDefinition(DoubleFault);
ISRCallbackDefinition(CoprocessorSegmentOverrun);
ISRCallbackDefinition(InvalidTSS);
ISRCallbackDefinition(SegmentNotPresent);
ISRCallbackDefinition(StackSegmentFault);
ISRCallbackDefinition(GeneralProtection);
ISRCallbackDefinition(PageFault);
ISRCallbackDefinition(x87FPUError);
ISRCallbackDefinition(AlignmentCheck);
ISRCallbackDefinition(MachineCheck);
ISRCallbackDefinition(SIMDFloatingPointException);
ISRCallbackDefinition(VirtualisationException);
ISRCallbackDefinition(ControlProtectionException);

//  0x00        Divide Error
//  0x01        Debug Exception
//  0x02        Non-Maskable External Interrupt
//  0x03        Breakpoint
//  0x04        Overflow
//  0x05        BOUND Range Excpetion
//  0x06        Invalid Execution Opcode
//  0x07        Unavailable Device
//  0x08        Double Fault
//  0x09        Coprocessor Segment Overrun
//  0x0A        Invalid TSS
//  0x0B        Segment Not Present
//  0x0C        Stack-Segment Fault
//  0x0D        General Protection
//  0x0E        Page Fault
//  0x10        x87 FPU Floating-Point Error
//  0x11        Alignment Check
//  0x12        Machine Check
//  0x13        SIMD Floating-Point Exception
//  0x14        Virtualization Exception
//  0x15        Control Protection Exception
//  0x20        Generic Input-Output Software Interrupt
//	0x16-0x1f	Reserved
//	0x20		PIT Interrupt
//	For each Controller/Device, an Interrupt is Allocated.
//	0x21-...	MassStorageController#N Interrupt
volatile ISRCallbackTable __align(4096) InterruptCallbacks = {
	DivideErrorISR, DebugExceptionISR, NonMaskableExtISR, 
	BreakpointISR, OverflowISR, 
	BoundRangeExceptionISR, InvalidOpcodeISR, 
	UnavailableDeviceISR, DoubleFaultISR, 
	CoprocessorSegmentOverrunISR, 
	InvalidTSSISR, SegmentNotPresentISR, 
	StackSegmentFaultISR, GeneralProtectionISR, 
	PageFaultISR, x87FPUErrorISR, 
	AlignmentCheckISR, MachineCheckISR, 
	SIMDFloatingPointExceptionISR, 
	VirtualisationExceptionISR, 
	ControlProtectionExceptionISR, 
	GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, 
	//	Starts from Interrupt 0x20
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR, GenericISR, 
	GenericISR, GenericISR, GenericISR
};

void InitialiseInterruptCallbacks(){
	InterruptCallbacks[0] = DivideErrorISR;
	InterruptCallbacks[1] = DebugExceptionISR;
	InterruptCallbacks[2] = NonMaskableExtISR;
	InterruptCallbacks[3] = BreakpointISR;
	InterruptCallbacks[4] = OverflowISR;
	InterruptCallbacks[5] = BoundRangeExceptionISR;
	InterruptCallbacks[6] = InvalidOpcodeISR;
	InterruptCallbacks[7] = UnavailableDeviceISR;
	InterruptCallbacks[8] = DoubleFaultISR;
	InterruptCallbacks[9] = CoprocessorSegmentOverrunISR;
	InterruptCallbacks[10] = InvalidTSSISR;
	InterruptCallbacks[11] = SegmentNotPresentISR;
	InterruptCallbacks[12] = StackSegmentFaultISR;
	InterruptCallbacks[13] = GeneralProtectionISR;
	InterruptCallbacks[14] = PageFaultISR;
	InterruptCallbacks[15] = x87FPUErrorISR;
	InterruptCallbacks[16] = AlignmentCheckISR;
	InterruptCallbacks[17] = MachineCheckISR;
	InterruptCallbacks[18] = SIMDFloatingPointExceptionISR;
	InterruptCallbacks[19] = VirtualisationExceptionISR;
	InterruptCallbacks[20] = ControlProtectionExceptionISR;
	for(uint64_t cc = 21; cc < IDTLength; ++cc){InterruptCallbacks[cc] = GenericISR;}
}

bool __noinline __used AllocateInterruptVector(uint8_t *Vector){
	if(!Vector){return false;}
	for(uint32_t cc = 0; cc < (sizeof(InterruptCallbacks) / sizeof(ISRCallback)); ++cc){
		if(InterruptCallbacks[cc] == GenericISR){
			*Vector = cc;
			return true;
		}
	}
	return false;

}

bool QueryVectorByCallback(uint8_t *Vector, ISRCallback *CB){
	if(!Vector || !CB){return false;}
	for(uint32_t cc = 0; cc < (sizeof(InterruptCallbacks) / sizeof(ISRCallback)); ++cc){
		if(InterruptCallbacks[cc] == *CB){
			*Vector = cc;
			return true;
		}
	}
	return false;
}

//	Initialise the Callback with a Preset StackHeader.
ISRCallback *ISRSetCallback(void *acpibase, uint8_t Vector, uint8_t Privilege, uint8_t IST, 
	void *StackHeader, IDTEntrySegmentSelector64 SS, bool TrapGate, ISRCallback *New
){
	if(Vector >= IDTLength || !New){return NULL;}
	if(InterruptCallbacks[Vector] == GenericISR){
		InterruptCallbacks[Vector] = *New;
		IDTR64 IR = {0};
		GDTR64 GR = {0};
		if(ReadIDTR(&IR) && ReadGDTR(&GR)){
			GDTSystemSegmentDescriptor64 *Entry = ((GDTSystemSegmentDescriptor64 *)(GR.Base + (sizeof(GDTDescriptor) * SS.Segment)));
			TSS_t *_TSS = MapVirtual((void *)(((uint64_t)Entry->BaseHigh << 24) + Entry->BaseLow));
			if(!_TSS->IST[IST]){
				_TSS->IST[IST] = ((uint64_t)mcalloc(1, DefaultStackSize) + DefaultStackSize) - (sizeof(uint64_t) * 4);
				*((uint64_t *)(_TSS->IST[IST] + (sizeof(uint64_t) * 0))) = (uint64_t)StackHeader;
				*((uint64_t *)(_TSS->IST[IST] + (sizeof(uint64_t) * 1))) = IST;
				*((uint64_t *)(_TSS->IST[IST] + (sizeof(uint64_t) * 2))) = SS.Segment;
				*((uint64_t *)(_TSS->IST[IST] + (sizeof(uint64_t) * 3))) = (uint64_t)GetLocalAPICBase(acpibase, NULL);
				SetIDTe(*(IDTTable64 *)&(IR.Base), Vector, (uint64_t)InterruptCallbacks[Vector], SS, IST, _64BitInterruptGate, true);
			}
		}
	}
	return InterruptCallbacks + Vector;
}

bool ISRUSetCallback(uint32_t Vector){
	if(Vector >= IDTLength){return false;}
	if(InterruptCallbacks[Vector] != GenericISR){
		InterruptCallbacks[Vector] = GenericISR;
		IDTR64 IR = {0};
		GDTR64 GR = {0};
		if(ReadIDTR(&IR) && ReadGDTR(&GR)){
			IDTEntry64 *E = (void *)(IR.Base + (sizeof(IDTEntry64) * Vector));
			GDTSystemSegmentDescriptor64 *Entry = ((GDTSystemSegmentDescriptor64 *)(GR.Base + (sizeof(GDTDescriptor) * E->SegmentSelector)));
			TSS_t *_TSS = MapVirtual((void *)(((uint64_t)Entry->BaseHigh << 24) + Entry->BaseLow));
			E->OffsetLow = (uint64_t)GenericISR & UINT16_MAX;
			E->OffsetHigh = (uint64_t)GenericISR >> 16;
			mfree((void *)(_TSS->IST[E->InterruptStackTableOffset] - DefaultStackSize) + (sizeof(uint64_t) * 4));
			_TSS->IST[E->InterruptStackTableOffset] = UINT64_MIN;
		}
	}
}

void *ISRGetStackHeader(uint32_t Vector){
	if(Vector >= IDTLength){return NULL;}
	if(InterruptCallbacks[Vector] != GenericISR){
		IDTR64 IR = {0};
		GDTR64 GR = {0};
		if(ReadIDTR(&IR) && ReadGDTR(&GR)){
			IDTEntry64 *E = (void *)(IR.Base + (sizeof(IDTEntry64) * Vector));
			GDTSystemSegmentDescriptor64 *Entry = ((GDTSystemSegmentDescriptor64 *)(GR.Base + (sizeof(GDTDescriptor) * E->SegmentSelector)));
			TSS_t *_TSS = MapVirtual((void *)(((uint64_t)Entry->BaseHigh << 24) + Entry->BaseLow));
			return (void *)(_TSS->IST[E->InterruptStackTableOffset] + (sizeof(uint64_t) * 4));
		}
	}
	return NULL;
}

bool AllocateIST(uint8_t *IST, IDTEntrySegmentSelector64 *sselector){
	GDTR64 R = {0};
	if(ReadGDTR(&R)){
		*IST = 0x00;
		*sselector = (IDTEntrySegmentSelector64){0};
		uint32_t _GDT = 0;
		for(; _GDT < (R.Limit / sizeof(GDTDescriptor)); ++_GDT){
			if(!((GDTDescriptor *)R.Base)[_GDT].ABSystemSegmentBit){
				GDTSystemSegmentDescriptor64 *SS = (GDTSystemSegmentDescriptor64 *)(R.Base + (_GDT * sizeof(GDTDescriptor)));
				if(SS->ABType == TSS64BitAvailable){
					TSS_t *_TSS = (void *)(((uint64_t)SS->BaseHigh << 24) + SS->BaseLow);
					if(_TSS){//	EnableInputAddress(true, true);
						_TSS = SafeAllocatePages(_TSS, sizeof(TSS_t), (ReadWritable | SupervisorMode), 0x00, 0x20);
					}else{
						_TSS = AllocatePages(NULL, sizeof(TSS_t), (ReadWritable | SupervisorMode), 0x00);
						uint64_t Phys = (uint64_t)MapPhysical(_TSS);
						SS->BaseHigh = (uint64_t)Phys >> 24;
						SS->BaseLow = (uint64_t)Phys & 0xFFFFFF;
					}
					(*IST) = 8;
					while((*IST)--){
						//	If unmapped, then we Allocate and Use it.
						if(!_TSS->IST[(*IST)]){
							sselector->Segment = _GDT;
							break;
						}
					}
				}
				_GDT++;
			}else{continue;}
			if(sselector->Segment){break;}
		}
		return true;
	}
	return false;
}

static uint32_t ErrorCountdown = 64;

ISRCallbackDefinition(Generic){DebugPrintf("\nGeneric Interrupt at 0x%0z", Frame->RIP);	ISRCallbackReturn;}
ISRCallbackDefinition(DivideError){
	DebugPrintf("\nDivide Error at 0x%0z", Frame->RIP);
	Frame->RAX = 0x00;	Frame->RDX = 0x00;
	ISRCallbackReturn;
}
//	Handle Later
ISRCallbackDefinition(DebugException){DebugPrintf("\nDebug Exception at 0x%0z", Frame->RIP);	ISRCallbackReturn;}
ISRCallbackDefinition(NonMaskableExt){DebugPrintf("\nNon-maskable External Interrupt at 0x%0z", Frame->RIP);	ISRCallbackReturn;}
ISRCallbackDefinition(Breakpoint){DebugPrintf("\nBreakpoint Exception at 0x%0z", Frame->RIP);	ISRCallbackReturn;}
ISRCallbackDefinition(Overflow){if(!ErrorCountdown){DebugPrintf("\nError Rate Unbounded, Halting CPU");	__chalt(true);}	ErrorCountdown--;
	DebugPrintf("\nOverflow Exception at 0x%0z", Frame->RIP);	ISRCallbackReturn;}
ISRCallbackDefinition(BoundRangeException){if(!ErrorCountdown){DebugPrintf("\nError Rate Unbounded, Halting CPU");	__chalt(true);}	ErrorCountdown--;
	DebugPrintf("\nBound Range Exception at 0x%0z", Frame->RIP);	ISRCallbackReturn;}
ISRCallbackDefinition(InvalidOpcode){if(!ErrorCountdown){DebugPrintf("\nError Rate Unbounded, Halting CPU");	__chalt(true);}	ErrorCountdown--;
	DebugPrintf("\nInvalid Opcode Exception at 0x%0z", Frame->RIP);	ISRCallbackReturn;}
ISRCallbackDefinition(UnavailableDevice){if(!ErrorCountdown){DebugPrintf("\nError Rate Unbounded, Halting CPU");	__chalt(true);}	ErrorCountdown--;
	DebugPrintf("\nUnavailable FPU Exception at 0x%0z", Frame->RIP);	ISRCallbackReturn;}
ISRCallbackDefinition(DoubleFault){if(!ErrorCountdown){DebugPrintf("\nError Rate Unbounded, Halting CPU");	__chalt(true);}	ErrorCountdown--;
	DebugPrintf("\nDouble Fault Exception at 0x%0z", Frame->RIP);	ISRCallbackReturn;}
ISRCallbackDefinition(CoprocessorSegmentOverrun){if(!ErrorCountdown){DebugPrintf("\nError Rate Unbounded, Halting CPU");	__chalt(true);}	ErrorCountdown--;
	DebugPrintf("\nCoprocessor Segment Overrun Exception at 0x%0z", Frame->RIP);	ISRCallbackReturn;}
ISRCallbackDefinition(InvalidTSS){if(!ErrorCountdown){DebugPrintf("\nError Rate Unbounded, Halting CPU");	__chalt(true);}	ErrorCountdown--;
	char *Src = NULL;	switch((Frame->ErrorCode >> 1) & 0x3){
		case 0b00:	{Src = "GDT";	break;}
		case 0b10:	{Src = "LDT";	break;}
		default:	{Src = "IDT";	break;}
	}
	DebugPrintf("\nInvalid TSS Exception at 0x%0z\t{External: %z, Source: %s, Selector:  0x%0z}", Frame->RIP, 
		(uint64_t)(Frame->ErrorCode & 0x1), Src, (uint64_t)(Frame->ErrorCode >> 3) & 0x1FFFULL);
	ISRCallbackReturn;
}
ISRCallbackDefinition(SegmentNotPresent){if(!ErrorCountdown){DebugPrintf("\nError Rate Unbounded, Halting CPU");	__chalt(true);}	ErrorCountdown--;
	DebugPrintf("\nSegment Not Present Exception at 0x%0z\t{Segment: 0x%0z}", Frame->RIP, (uint64_t)Frame->ErrorCode);	ISRCallbackReturn;}
ISRCallbackDefinition(StackSegmentFault){if(!ErrorCountdown){DebugPrintf("\nError Rate Unbounded, Halting CPU");	__chalt(true);}	ErrorCountdown--;
	DebugPrintf("\nSegment Not Present Exception at 0x%0z\t{Segment: 0x%0z}", Frame->RIP, (uint64_t)Frame->ErrorCode);	ISRCallbackReturn;}
ISRCallbackDefinition(GeneralProtection){if(!ErrorCountdown){DebugPrintf("\nError Rate Unbounded, Halting CPU");	__chalt(true);}	ErrorCountdown--;
	DebugPrintf("\nGeneral Protection Fault at 0x%0z\t{Segment-Related: %s, Segment:  0x%0z}", Frame->RIP, (Frame->ErrorCode? "TRUE": "FALSE"), Frame->ErrorCode);	ISRCallbackReturn;}
ISRCallbackDefinition(PageFault){if(!ErrorCountdown){DebugPrintf("\nError Rate Unbounded, Halting CPU");	__chalt(true);}	ErrorCountdown--;
	DebugPrintf("\nPage Fault at 0x%0z(VirtualAddress: 0x%0z)\t0b%2z{Present: %z, WriteOperation: %z, CPLEq3: %z, InvalidPageTableBits: %z, ExecuteViolation %z, ProtectionKeyViolation %z, ShadowStackAccessViolation %z, SoftwareGuardExtViolation %z}", 
		Frame->RIP, Frame->CR2, Frame->ErrorCode, (uint64_t)(Frame->ErrorCode & 0x1), (uint64_t)((Frame->ErrorCode >> 1) & 0x1), (uint64_t)((Frame->ErrorCode >> 2) & 0x1), (uint64_t)((Frame->ErrorCode >> 3) & 0x1), (uint64_t)((Frame->ErrorCode >> 4) & 0x1), 
		(uint64_t)((Frame->ErrorCode >> 5) & 0x1), (uint64_t)((Frame->ErrorCode >> 6) & 0x1), (uint64_t)((Frame->ErrorCode >> 15) & 0x1));	ISRCallbackReturn;}
ISRCallbackDefinition(x87FPUError){if(!ErrorCountdown){DebugPrintf("\nError Rate Unbounded, Halting CPU");	__chalt(true);}	ErrorCountdown--;
	DebugPrintf("\nx87 FPU Exception at 0x%0z", Frame->RIP);	ISRCallbackReturn;}
ISRCallbackDefinition(AlignmentCheck){DebugPrintf("\nAlignment Check Exception at 0x%0z", Frame->RIP);	ISRCallbackReturn;}
ISRCallbackDefinition(MachineCheck){DebugPrintf("\nMachine Check Exception at 0x%0z", Frame->RIP);	ISRCallbackReturn;}
ISRCallbackDefinition(SIMDFloatingPointException){if(!ErrorCountdown){DebugPrintf("\nError Rate Unbounded, Halting CPU");	__chalt(true);}	ErrorCountdown--;
	DebugPrintf("\nSIMD Floating Point Exception at 0x%0z", Frame->RIP);	ISRCallbackReturn;}
ISRCallbackDefinition(VirtualisationException){DebugPrintf("\nVirtualisation Exception at 0x%0z", Frame->RIP);	ISRCallbackReturn;}
ISRCallbackDefinition(ControlProtectionException){DebugPrintf("\nControl Protection Exception at 0x%0z", Frame->RIP);	ISRCallbackReturn;}