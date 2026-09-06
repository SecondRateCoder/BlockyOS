#pragma once

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"

#include "kernel/libcrt/hardware/ACPI/RSDP/MADT.h"
#include "kernel/libcrt/hardware/ACPI/XSDP/MADT.h"
#include "kernel/libcrt/hardware/ACPI/ACPI.h"
#include "kernel/libcrt/hardware/paging/paging.h"
#include "kernel/libcrt/hardware/hardware.h"


//*	Source Adapted from:
//*		https://wiki.osdev.org/APIC

/**	How does the Local APIC work?
 *	The local APIC's registers are memory-mapped in physical page FEE00xxx (as seen in table 8-1 of Intel P4 SPG). 
 *	This address is the same for each local APIC that exists in a configuration, 
 *		meaning you are only able to directly access the registers of the local APIC of the core that your code is currently executing on. 
 *	Note that there is a MSR that specifies the actual APIC base (only available on CPUs with family >5). 
 *	The MADT contains the local APIC base and on 64-bit systems it may also contain a field specifying a 64-bit base address override which you ought to use instead. 
 *	You can choose to leave the Local APIC base just where you find it, or to move it at your pleasure. 
 *		Note: I don't think you can move it any further than the 4th Gb. 
 *	
 *	To enable the Local APIC to receive interrupts it is necessary to configure the "Spurious Interrupt Vector Register". 
 *	The correct value for this field is the IRQ number that you want to map the spurious interrupts to within the lowest 8 bits, 
 *		and the 8th bit set to 1 to actually enable the APIC (see the specification for more details). 
 *	You should choose an interrupt number that has its lowest 4 bits set and is above 32 (as you might guess); easiest is to use 0xFF. 
 *	This is important on some older processors because the lowest 4 bits for this value must be set to 1 on these. 
 */

#define LocalAPICRegisterBase			0xFEE00000
#define LocalAPICEnableBit				0x100
#define LocalAPICDefaultBaseInterrupt	(0xFF)

#define WriteAPICRegister(BASE, REGISTER, VALUE)	if(LARAM_##REGISTER == LARAT_ReadWrite || LARAM_##REGISTER == LARAT_WriteOnly){*(__typeof(VALUE) *)((BASE) + (REGISTER)) = VALUE;}
#define ReadAPICRegister(BASE, REGISTER, OUT)		if(LARAM_##REGISTER == LARAT_ReadWrite || LARAM_##REGISTER == LARAT_ReadOnly){OUT = *((__typeof(OUT) *)((BASE) + (REGISTER)));}

enumdef(uint8_t, LocalAPICRegisterAccessType){LARAT_ReadOnly = 0x0, LARAT_ReadWrite = 0x1, LARAT_WriteOnly = 0x2};
enumdef(uint8_t, LocalAPICTriggerMode){LATM_LevelTriggerMode = 0x1, LATM_EdgeTriggerMode = 0x0};
enumdef(uint8_t, LocalAPICRegisterAccessMode){
	LARAM_LAR_ID = LARAT_ReadWrite, LARAM_LAR_VERSION = LARAT_ReadOnly, 
	LARAM_LAR_TaskPriority = LARAT_ReadWrite, LARAM_LAR_ArbitrationPriority = LARAT_ReadOnly, 
	LARAM_LAR_ProcessorPriority = LARAT_ReadOnly, LARAM_LAR_EndOfInterrupt = LARAT_WriteOnly, 
	LARAM_LAR_RemoteRead = LARAT_ReadOnly, LARAM_LAR_LogicalDestination = LARAT_ReadWrite, 
	LARAM_LAR_DestinationFormat = LARAT_ReadWrite, 
	LARAM_LAR_SpuriousInterruptVector = LARAT_ReadWrite, 
	LARAM_LAR_InService = LARAT_ReadOnly, LARAM_LAR_TriggerMode = LARAT_ReadOnly, 
	LARAM_LAR_InterruptRequest = LARAT_ReadOnly, LARAM_LAR_ErrorStatus = LARAT_ReadOnly, 
	LARAM_LAR_LVTCorrectedMachineCheckInterrupt = LARAT_ReadWrite, 
	LARAM_LAR_InterruptCommandRegister = LARAT_ReadWrite, 
	LARAM_LAR_LVTTimer = LARAT_ReadWrite, LARAM_LAR_LVTThermalSensor = LARAT_ReadWrite, 
	LARAM_LAR_LVTPerformanceMonitoringCounters = LARAT_ReadWrite, 
	LARAM_LAR_LVT_LINT0 = LARAT_ReadWrite, LARAM_LAR_LVT_LINT1 = LARAT_ReadWrite, 
	LARAM_LAR_LVTError = LARAT_ReadWrite, LARAM_LAR_TimerInitialCount = LARAT_ReadWrite, 
	LARAM_LAR_TimerCurrentCount = LARAT_ReadOnly, 
	LARAM_LAR_TimerDivideConfiguration = LARAT_ReadOnly
};
enumdef(uint32_t, LocalAPICRegisters){
	LAR_ID = 0x20, LAR_Version = 0x30, 
	//	The Task Priority Register (TPR) is a 32-bit register used to control the minimum priority an interrupt needs to have for it to be delivered. 
	//	It is divided into two 4-bit fields: task-priority class and task-priority sub-class. 
	//	Since vector interrupts range from 0-255 (8 bits) the priority of an interrupt is based on bits its 7:4 and its sub-priority is based on its bits 3:0. 
	//	This making the lowest (sub-)priority 0 and the highest (sub-)priority 15 (Note that because intel reserves 0-31 interrupts, 
	//		interrupts delivered to the APIC should have a priority of 2 and above).
	//!	The minimum interrupt priority required for the delivery of an interrupt is not fully dependent on the TPR but rather the PPR
	LAR_TaskPriority = 0x80, 
	LAR_ArbitrationPriority = 0x90, 
	//	The Process Priority Register (PPR) is a 32-bit register used to determine the minimum priority an interrupt needs to have for it to be delivered. 
	//	It is divided into two 4-bit fields: processor-priority class and processor-priority sub-class. 
	//	The processor-priority class is determined by comparing the TPR[7:4] and ISRV[7:4] and the greatest of the two will be the processor-priority class (ISRV explained at the bottom). 
	//	The processor-priority sub-class is determined as follows: 
	//		If TPR[3:0] > ISRV[3:0]; then PPR[3:0] (processor-priority) = TPR[3:0]
	//		If TPR[3:0] < ISRV[3:0]; then PPR[3:0] (processor-priority) = 0
	//		If TPR[3:0] == ISRV[3:0]; then PPR[3:0] can be either TPR[3:0] or 0 depending on the processor model.
	//!	Only interrupts with a priority higher than the processor-priority class can be delivered. 
	//!		That is, if processor-priority class is 0 every interrupt with priority 1 and higher can be delivered or if processor-priority class is 15 no interrupt can be delivered (since 15 is maximum priority). 
	//!		It is important to note, though, that this mechanism does NOT affect the following types of interrupts: NMI, SMI, INIT, ExtINIT, INIT-deassert, and start-up delivery modes.
	//!	The processor does not use the processor-priority sub-class to determine if an interrupt should be delivered or not. 
	//!		The processor uses the processor-priority sub-class only to satisfy reads of the PPR.
	//!	ISRV is the highest interrupt in-service (so ISRV[7:4] is the highest priority of the highest interrupt currently in-service) 
	LAR_ProcessorPriority = 0xA0, 
	//	Write to the register with offset 0xB0 using the value 0 to signal an end of interrupt. 
	//	A non-zero value may cause a general protection fault.
	LAR_EndOfInterrupt = 0xB0, 
	LAR_RemoteRead = 0xC0, LAR_LogicalDestination = 0xD0, 
	LAR_DestinationFormat = 0xE0, 
	//	The low byte contains the number of the spurious interrupt. 
	//	As noted above, you should probably set this to 0xFF. 
	//	To enable the APIC, set bit 8 (or 0x100) of this register. 
	//		If bit 12 is set then EOI messages will not be broadcast. 
	//		All the other bits are currently reserved. 
	LAR_SpuriousInterruptVector = 0xF0, 
	
	//	Larger Registers(32-byte)
	
	//	The In-Service Register (ISR) is a 256-bit LAPIC register. 
	//	It is one of the two pending registers the LAPIC uses to queue accepted fixed interrupts. 
	//	Each of the 256 bits of the ISR correspond to one interrupt vector.
	//	Intel reserves the first 32 vector interrupts for architecture-defined exceptions and interrupts. 
	//	After the LAPIC receives an EOI signal (after writing to the EOI register), 
	//		it will clear the highest bit that is set within the ISR. After that, it will look at the highest bit that is set within the IRR, 
	//		clear it, and set the corresponding bit within the ISR. 
	//	Finally, the LAPIC will look at the highest bit that is set within the ISR (which means it will look at the interrupt with the highest priority that needs to be handled) 
	//		and handle the corresponding interrupt request. 
	//	If an interrupt with higher priority than the currently executing one is detected then 
	//		the execution of the current one will be paused without firing an EOI signal and this new interrupt with higher priority will be handled, 
	//		once it is handled execution of the previous paused interrupt will resume. 
	LAR_InService = 0x100, 
	//	The Trigger Mode Register (TMR) is a 256-bit LAPIC register. 
	//	Each of the 256 bits of the TMR correspond to one interrupt vector. 
	//	Consequently, the first 16 bits are reserved because Intel reserves the first 32 vector interrupts for architecture-defined exceptions and interrupts. 
	//	After an interrupt is accepted into the IRR, the corresponding TMR bit is cleared for edge-triggered interrupts and set for level-triggered interrupts. 
	//	If a TMR bit is set when an EOI is issued for the corresponding interrupt, then the processor will send the EOI signal to all I/O LAPICs. 
	LAR_TriggerMode = 0x180, 
	//	Similar to the ISR, the Interrupt Request Register (IRR) is a 256-bit LAPIC register. 
	//	It is one of the two pending registers the LAPIC uses to queue accepted fixed interrupts. 
	//	Each of the 256 bits of the IRR correspond to one interrupt vector. 
	//		Consequently, the first 16 bits are reserved because Intel reserves the first 32 vector interrupts for architecture-defined exceptions and interrupts. 
	//	An IRR bit is set after the LAPIC receives and accepts an interrupt request but is not ready to dispatch it yet. 
	//	When the processor core is ready to handle the next interrupt, the LAPIC clears the highest bit within the IRR and sets the corresponding bit in the ISR. 
	//	If the LAPIC detects an interrupt signal but its corresponding bit within the IRR was already set then the LAPIC can set it the bit in both the ISR and the IRR. 
	//	In case that an interrupt signal is detected but the corresponding bit is set in the ISR and the IRR, then the LAPIC will reject it. 
	LAR_InterruptRequest = 0x200, 
	LAR_ErrorStatus = 0x280, 
	
	//	Local Vector Table Registers
	//	There are some special interrupts that the processor and LAPIC can generate themselves. 
	//	While external interrupts are configured in the I/O APIC, these interrupts must be configured using registers in the LAPIC. 
	//	The most interesting registers are: 
	//		0x320 = lapic timer, 
	//		0x350 = lint0, 
	//		0x360 = lint1. 
	//		See the Intel SDM vol 3 for more info. 

	LAR_LVTCorrectedMachineCheckInterrupt = 0x2F0, 
	LAR_InterruptCommandRegister = 0x300, LAR_LVTTimer = 0x320, LAR_LVTThermalSensor = 0x330, 
	LAR_LVTPerformanceMonitoringCounters = 0x340, LAR_LVT_LINT0 = 0x350, LAR_LVT_LINT1 = 0x360, 
	LAR_LVTError = 0x370, 
	//	The Initial Count Register is a 32-bit register. 
	//	When a value is written to the register that same value will automatically be written to the Current Count Register and in case the value is greater than 0 it will initialize the APIC Timer. 
	//	In case that the APIC Timer already started but the Initial Count Register is rewritten then the APIC Timer will restart (without triggering a timer interrupt) 
	//		by copying the new value to Current Count Register.
	//!	If the APIC Timer already started but the Initial Count Register is rewritten with a value of 0 then the APIC timer will stop and it will NOT trigger a timer interrupt. 
	LAR_TimerInitialCount = 0x380, 
	//	The Current Count Register is a 32-bit register that as its name suggests holds the current count. 
	//	That is, it holds the amount of clock ticks before a timer interrupt will be triggered. 
	//	It's value is set when Initial Count Register is written and it starts decrementing by 1 every clock tick. 
	//	When it's value reaches 0 it will trigger a timer interrupt. 
	LAR_TimerCurrentCount = 0x390, 
	//	The Divide Configuration Register is a 32-bit register used to calculate the frequency of clock ticks of the LAPIC, 
	//		which is obtained dividing the processor's bus clock or core crystal clock frequency (when TSC/core crystal clock ratio is enumerated in CPUID.15H) 
	//		by the value specified in the Divide Configuration Register. 
	//	Here is how the Divide Configuration Register looks like:
	//		Bits	Value
	//		0		1 or 0
	//		1		1 or 0
	//		2		0
	//		3		1 or 0
	//		4-31	Reserved
	//	Here is the value of Divide Configuration Register that will be used to calculate the frequency based on the first 4 bits (the rest are reserved):
	//		Bits 0-3	Divide Configuration Register Value
	//		0000		2
	//		0001		4
	//		0010		8
	//		0011		16
	//		1000		32
	//		1001		64
	//		1010		128
	//		1011		1 
	LAR_TimerDivideConfiguration = 0x3E0
};


typedef struct{
	uint32_t		TaskPrioriySubClass	:	4, 
					TaskPrioriyClass	:	4;
	uint32_t							:	24;
}__packed LocalAPICTaskPriorityRegister_t;
typedef struct{
	uint32_t		ProcessorPrioritySubClass	:	4, 
					ProcessorPriorityClass		:	4;
	uint32_t									:	24;
}__packed LocalAPICProcessPriorityRegister_t;
typedef struct{
	uint32_t		VectorNumber				: 8;
	uint32_t									: 3;
	uint32_t									: 1;
	uint32_t		InterruptPending			: 1;
	uint32_t		Polarity					: 1;
	uint32_t		RemoteIRREnabled			: 1;
	uint32_t		TriggerMode					: 1;
	uint32_t		SetToMask					: 1;
}__packed LocalAPICLocalVectorTableRegister_t;

enumdef(uint8_t, LocalAPICInterruptCommandRegisterDeliveryMode){
	LAICRDM_Normal = 0x0, LAICRDM_LowestPriority = 0x1, 
	LAICRDM_SystemManagementInterrupt = 0x2, LAICRDM_NonMaskableInterrupt = 0x4, 
	LAICRDM_InitialisatioInterrupt = 0x5, LAICRDM_InitialisatioInterruptLevelDeAssert = 0x5, 
	LAICRDM_StartupInterProcessorInterrupt = 0x6
};
enumdef(uint8_t, LocalAPICInterruptCommandRegisterDestinationMode){LAICRDM_PhysicalDestination = 0x0, LAICRDM_LogicalDestination = 0x1};
enumdef(uint8_t, LocalAPICInterruptCommandRegisterDestinationType){
	LAICRDT_SendToMyself = 0x1, LAICRDT_SendToAllProcessors = 0x2, LAICRDT_SendToAllProcessorsExceptCurrent = 0x3
};

typedef struct{
	uint32_t		VectorNumber						: 8;
	uint32_t		DeliveryMode						: 1;
	uint32_t		DestinationMode						: 1;
	//	 Cleared when the interrupt has been accepted by the target.
	uint32_t		DeliveryStatus						: 1;
	uint32_t											: 1;
	uint32_t		InitialisationInterruptSetEnable	: 1;
	uint32_t		InitialisationInterruptSetDisable	: 1;
	//	If this is > 0 then the destination field in 0x310 is ignored.
	uint32_t		DestinationType						: 3;
}__packed LocalAPICInterruptCommandRegister_t;

LibAPI void *GetLocalAPICBase(void *ACPIBase, bool *_RSDT);
LibAPI bool InitLocalAPIC(void *ACPIBase, uint8_t InterruptBase, bool Enable);
LibAPI void DisableLocalAPIC(void *ACPIBase);
LibAPI void EnableLocalAPIC(void *ACPIBase);