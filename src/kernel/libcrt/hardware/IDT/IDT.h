#pragma once

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"

#define IDTLength		256
#define IDTFirstFreeVector	0x21

typedef struct{
	uint16_t	Limit;
	uint64_t	Base;
}__packed IDTR64;

#define IDT64InterruptGateType		(0xE)
#define IDT64TrapGateType			(0xF)

typedef union{
	uint16_t	Raw;
	struct{
		uint16_t	PriviledgeLevel	: 2;
		uint16_t	LocalDescriptor	: 1;
		uint16_t	Index			: 13;
	};
}IDTEntrySegmentSelector64;

#define _64BitInterruptGate		(0xE)
#define _64BitTrapGate			(0xF)
//	Number(No)			Mnem. 	Type		Err. code	Name												Source/Description
//	hex			dec
//	0x00		0		#DE		Fault		No			Divide Error										DIV and IDIV instructions.
//	0x01		1		#DB		Trap		No			Debug Exception										Instruction, data, and I/O breakpoints; single-step; and others.
//	0x02		2		NMI		Interrupt	No			NMI Interrupt										Nonmaskable external interrupt.
//	0x03		3		#BP		Trap		No			Breakpoint											INT3 instruction.
//	0x04		4		#OF		Trap		No			Overflow											INTO instruction.
//	0x05		5		#BR		Fault		No			BOUND Range Exceeded								BOUND instruction.
//	0x06		6		#UD		Fault		No			Invalid Opcode (Undefined Opcode)					UD instruction or reserved opcode.
//	0x07		7		#NM		Fault		No			Device Not Available (No Math Coprocessor)			Floating-point or WAIT/FWAIT instruction.
//	0x08		8		#DF		Abort		Yes			(zero) 	Double Fault								Any instruction that can generate an exception, an NMI, or an INTR.
//	0x09		9		N/A		Fault		No			Coprocessor Segment Overrun (reserved)				Floating-point instruction.
//	0x0A		10		#TS		Fault		Yes			Invalid TSS											Task switch or TSS access.
//	0x0B		11		#NP		Fault		Yes			Segment Not Present									Loading segment registers or accessing system segments.
//	0x0C		12		#SS		Fault		Yes			Stack-Segment Fault									Stack operations and SS register loads.
//	0x0D		13		#GP		Fault		Yes			General Protection									Any memory reference and other protection checks.
//	0x0E		14		#PF		Fault		Yes			Page Fault											Any memory reference.
//	0x0F		15		N/A					No			Intel reserved.										Do not use.
//	0x10		16		#MF		Fault		No			x87 FPU Floating-Point Error (Math Fault)			x87 FPU floating-point or WAIT/FWAIT instruction.
//	0x11		17		#AC		Fault		Yes			(zero) 	Alignment Check								Any data reference in memory.
//	0x12		18		#MC		Abort		No			Machine Check										Error codes (if any) and source are model dependent.
//	0x13		19		#XM		Fault		No			SIMD Floating-Point Exception						SSE/SSE2/SSE3 floating-point instructions
//	0x14		20		#VE		Fault		No			Virtualization Exception							EPT violations
//	0x15		21		#CP		Fault		Yes			Control Protection Exception						RET, IRET, RSTORSSP, and SETSSBSY instructions can generate this exception. When CET indirect branch tracking is enabled, this exception can be generated due to a missing ENDBRANCH instruction at target of an indirect call or jump.
//	0x16-0x1f 	22-31 	N/A								Reserved for future use as CPU exception vectors.
typedef union{
	uint128_t	Raw;
	struct{
		uint64_t					OffsetLow					:	16;
		//	A Segment Selector with multiple fields which must point to a valid code segment in the GDT.
		IDTEntrySegmentSelector64	SegmentSelector;
		//	A 3-bit value which is an offset into the Interrupt Stack Table, 
		//		which is stored in the Task State Segment. 
		//	If the bits are all set to zero, the Interrupt Stack Table is not used.
		uint64_t					InterruptStackTableOffset	:	3;
		uint64_t												:	5;
		uint64_t					GateType					:	4;
		uint64_t												:	1;
		uint64_t					DescriptorPriviledgeLevel	:	2;
		uint64_t					Present						:	1;
		uint64_t					OffsetHigh					:	48;
		uint64_t												:	0;
	};
}__packed IDTEntry64, IDTTable64[IDTLength];

LibAPI extern bool __sysvabi LoadIDTR(IDTR64 *Register);
LibAPI extern bool __sysvabi ReadIDTR(IDTR64 *Register);