#pragma once

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"
#include "APIC/LocalAPIC.h"
#include "IDT.h"


typedef struct{
    //	Pushed by common_isr_stub (Lowest Address [RSP + 0])
	uint64_t					R15, R14, R13, R12, R11, R10, R9, R8;
	uint64_t					RDI, RSI, RBP, RDX, RCX, RBX, RAX;
	
	//	Pushed by Macros
	uint64_t					VectorNumber;
	uint64_t					ErrorCode;
	
	//	Pushed by CPU Hardware (Highest Address [RSP + 168])
	uint64_t					RIP;
	uint64_t					CS;
	uint64_t					RFLAGS;
	uint64_t					RSP;
	//	Must be 64-bit in Long Mode
	uint64_t					SS;
	//	Stack Header.
	void						*LocalAPIC;
	uint64_t					_GDT, _IST;
	uint8_t						StackHeader;
	//	For Multiple Faults is followed by
	// struct InterruptStackFrame	Faults[];
}__packed InterruptStackFrame;

typedef void *__sysvabi __naked(*ISRCallback)(InterruptStackFrame *Frame);
typedef ISRCallback ISRCallbackTable[IDTLength];
extern ISRCallbackTable __align(64) InterruptCallbacks;

#define ISRCallbackReturn			WriteAPICRegister(Frame->LocalAPIC, LAR_EndOfInterrupt, (uint32_t)0x00);		return NULL;
#define ISRCallbackDefinition(NAME)	void *__naked __sysvabi NAME##ISR(InterruptStackFrame *Frame)

LibAPI void *ISRGetStackHeader(uint32_t Vector);
LibAPI bool AllocateInterruptVector(uint8_t *Vector);
LibAPI bool QueryVectorByCallback(uint8_t *Vector, ISRCallback *CB);
LibAPI bool ISRUSetCallback(uint32_t Vector);
LibAPI bool AllocateIST(uint8_t *IST, IDTEntrySegmentSelector64 *sselector);
LibAPI ISRCallback *ISRSetCallback(void *acpibase, uint8_t Vector, uint8_t Privilege, uint8_t IST, 
	void *StackHeader, IDTEntrySegmentSelector64 SS, bool TrapGate, ISRCallback *New);