#include "ISR.h"
#include "kernel/libcrt/math/float.h"

#include "kernel/libcrt/hardware/GDT/GDT.h"
#include "kernel/libcrt/hardware/IDT/APIC/LocalAPIC.h"

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
ISRCallbackTable __align(64) InterruptCallbacks = {
	DebugExceptionISR, NonMaskableExtISR, 
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
	NULL, NULL, NULL, 
	NULL, NULL, NULL, 
	NULL, NULL, NULL, 
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

bool AllocateInterruptVector(uint8_t *Vector){
	for(uint32_t cc = 0; cc < (sizeof(InterruptCallbacks) / sizeof(ISRCallback)); ++cc){
		if(InterruptCallbacks[cc] == GenericISR){
			*Vector = cc;
			return true;
		}
	}
	return false;

}

bool QueryVectorByCallback(uint8_t *Vector, ISRCallback *CB){
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
	if(InterruptCallbacks[Vector] == GenericISR){
		InterruptCallbacks[Vector] = *New;
		IDTR64 IR = {0};
		GDTR64 GR = {0};
		if(ReadIDTR(&IR) && ReadGDTR(&GR)){
			GDTSystemSegmentDescriptor64 *Entry = ((GDTSystemSegmentDescriptor64 *)(GR.Base + (sizeof(GDTDescriptor) * SS.Index)));
			TSS_t *_TSS = MapVirtual((void *)(((uint64_t)Entry->LinearBaseHigh << 24) + Entry->LinearBaseLow));
			if(!_TSS->IST[IST]){
				_TSS->IST[IST] = ((uint64_t)mcalloc(1, DefaultStackSize) + DefaultStackSize) - (sizeof(uint64_t) * 4);
				*((uint64_t *)(_TSS->IST[IST] + (sizeof(uint64_t) * 0))) = (uint64_t)StackHeader;
				*((uint64_t *)(_TSS->IST[IST] + (sizeof(uint64_t) * 1))) = IST;
				*((uint64_t *)(_TSS->IST[IST] + (sizeof(uint64_t) * 2))) = SS.Index;
				*((uint64_t *)(_TSS->IST[IST] + (sizeof(uint64_t) * 3))) = (uint64_t)GetLocalAPICBase(acpibase, NULL);
				((IDTEntry64 *)IR.Base)[Vector] = (IDTEntry64){
					.OffsetHigh = (uint64_t)InterruptCallbacks[Vector] >> 16, 
					.OffsetLow = (uint64_t)InterruptCallbacks[Vector] & UINT16_MAX, 
					.GateType = TrapGate? _64BitTrapGate: _64BitInterruptGate, 
					.DescriptorPriviledgeLevel = Privilege, .SegmentSelector = SS, 
					.InterruptStackTableOffset = IST, .Present = true
				};
			}
		}
	}
	return InterruptCallbacks + Vector;
}

bool ISRUSetCallback(uint32_t Vector){
	if(InterruptCallbacks[Vector] != GenericISR){
		InterruptCallbacks[Vector] = GenericISR;
		IDTR64 IR = {0};
		GDTR64 GR = {0};
		if(ReadIDTR(&IR) && ReadGDTR(&GR)){
			IDTEntry64 *E = (void *)(IR.Base + (sizeof(IDTEntry64) * Vector));
			GDTSystemSegmentDescriptor64 *Entry = ((GDTSystemSegmentDescriptor64 *)(GR.Base + (sizeof(GDTDescriptor) * E->SegmentSelector.Index)));
			TSS_t *_TSS = MapVirtual((void *)(((uint64_t)Entry->LinearBaseHigh << 24) + Entry->LinearBaseLow));
			E->OffsetLow = (uint64_t)GenericISR & UINT16_MAX;
			E->OffsetHigh = (uint64_t)GenericISR >> 16;
			mfree((void *)(_TSS->IST[E->InterruptStackTableOffset] - DefaultStackSize) + (sizeof(uint64_t) * 4));
			_TSS->IST[E->InterruptStackTableOffset] = UINT64_MIN;
		}
	}
}

void *ISRGetStackHeader(uint32_t Vector){
	if(InterruptCallbacks[Vector] != GenericISR){
		IDTR64 IR = {0};
		GDTR64 GR = {0};
		if(ReadIDTR(&IR) && ReadGDTR(&GR)){
			IDTEntry64 *E = (void *)(IR.Base + (sizeof(IDTEntry64) * Vector));
			GDTSystemSegmentDescriptor64 *Entry = ((GDTSystemSegmentDescriptor64 *)(GR.Base + (sizeof(GDTDescriptor) * E->SegmentSelector.Index)));
			TSS_t *_TSS = MapVirtual((void *)(((uint64_t)Entry->LinearBaseHigh << 24) + Entry->LinearBaseLow));
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
			if(((GDTDescriptor *)R.Base)[_GDT].ABSystemSegmentBit){
				GDTSystemSegmentDescriptor64 *SS = (GDTSystemSegmentDescriptor64 *)(R.Base + (_GDT * sizeof(GDTDescriptor)));
				if(SS->ABType == TSS64BitAvailable){
					TSS_t *_TSS = (void *)(((uint64_t)SS->LinearBaseHigh << 24) + SS->LinearBaseLow);
					if(_TSS){_TSS = SafeAllocatePages(_TSS, sizeof(TSS_t), (ReadWritable | SupervisorMode), 0x00, 0x20);}else{
						_TSS = AllocatePages(NULL, sizeof(TSS_t), (ReadWritable | SupervisorMode), 0x00);
						uint64_t Phys = (uint64_t)MapPhysical(_TSS);
						SS->LinearBaseHigh = (uint64_t)_TSS >> 24;
						SS->LinearBaseLow = (uint64_t)_TSS & 0xFFFFFF;
					}
					(*IST) = 8;
					while((*IST)--){
						uint32_t LevelTemp = 5;
						//	If unmapped, then we Allocate and Use it.
						if(!_TSS->IST[(*IST)]){
							sselector->Index = _GDT;
							break;
						}
					}
				}
				_GDT++;
			}else{continue;}
			if(sselector->Index){break;}
		}
		return true;
	}
	return false;
}

ISRCallbackDefinition(Generic){ISRCallbackReturn}

ISRCallbackDefinition(DivideError){
	Frame->RAX = UINT64_MAX;
	Frame->RDX = UINT64_MAX;
	ISRCallbackReturn
}

//	Handle Later
ISRCallbackDefinition(DebugException){ISRCallbackReturn}
ISRCallbackDefinition(NonMaskableExt){ISRCallbackReturn}
ISRCallbackDefinition(Breakpoint){ISRCallbackReturn}
ISRCallbackDefinition(Overflow){ISRCallbackReturn}
ISRCallbackDefinition(BoundRangeException){ISRCallbackReturn}
ISRCallbackDefinition(InvalidOpcode){ISRCallbackReturn}
ISRCallbackDefinition(UnavailableDevice){ISRCallbackReturn}
ISRCallbackDefinition(DoubleFault){ISRCallbackReturn}
ISRCallbackDefinition(CoprocessorSegmentOverrun){ISRCallbackReturn}
ISRCallbackDefinition(InvalidTSS){ISRCallbackReturn}
ISRCallbackDefinition(SegmentNotPresent){ISRCallbackReturn}
ISRCallbackDefinition(StackSegmentFault){ISRCallbackReturn}
ISRCallbackDefinition(GeneralProtection){ISRCallbackReturn}
ISRCallbackDefinition(PageFault){ISRCallbackReturn}
ISRCallbackDefinition(x87FPUError){ISRCallbackReturn}
ISRCallbackDefinition(AlignmentCheck){ISRCallbackReturn}
ISRCallbackDefinition(MachineCheck){ISRCallbackReturn}
ISRCallbackDefinition(SIMDFloatingPointException){ISRCallbackReturn}
ISRCallbackDefinition(VirtualisationException){ISRCallbackReturn}
ISRCallbackDefinition(ControlProtectionException){ISRCallbackReturn}