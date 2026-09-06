#pragma once

#include "RSDT.h"

//	The MADT describes all of the interrupt controllers in the system. 
//	It can be used to enumerate the processors currently available. 
#define	RSDT_MADTCode			"APIC"
#define RSDT_LocalAPICSignature	0xFEE

enumdef(uint32_t, RSDT_MADTFlags){
	RSDT_Dual8259LegacyPICInstall = 0x1
};
enumdef(uint8_t, RSDT_MADTEntryType){
	RSDT_ProcessorLocalAPIC = 0x0, RSDT_IOAPIC = 0x1, 
	RSDT_IOAPICInterruptSourceOverride = 0x2, 
	RSDT_IOAPICNonMaskableInterruptSource = 0x3, 
	RSDT_LocalAPICNonMaskableInterrupts = 0x4, 
	RSDT_LocalAPICAddressOverride = 0x5, RSDT_ProcessorLocalx2APIC = 0x9
};
enumdef(uint32_t, RSDT_GenericMADTEntryFlag){
	//	Polarity Flags
	NoPolarityOverride = 0b00, ActiveHighPolarityOverride = 0b01, 
	ActiveLowPolarityOverride = 0b11, 
	//	Trigger Flags
	NoTriggerOverride = ((uint32_t)0b00 << 16), EdgeTrigger = ((uint32_t)0b01 << 16), 
	LevelTrigger = ((uint32_t)0b11 << 16)
};
typedef struct{
	RSDT_MADTEntryType	Type;
	uint8_t				RecordTypeLength;
}__packed RSDT_MADTEntry;
typedef struct{
	SDTHeader_t			Header;
	uint32_t			LocalAPICAddress;
	RSDT_MADTFlags		Flags;
	RSDT_MADTEntry		Table[];
}__packed RSDT_MADT_t;

enumdef(uint32_t, RSDT_ProcessorLocalAPICFlags){RSDT_ProcessorEnabled = 0x0, RSDT_OnlineCapable = 0x1};
typedef struct{
	RSDT_MADTEntry					Header;
	uint8_t							ProcessorID;
	uint8_t							ApicID;
	RSDT_ProcessorLocalAPICFlags	Flags;
}__packed RSDT_ProcessorLocalAPIC_t;

typedef struct{
	RSDT_MADTEntry			Header;
	uint8_t					IOApicID;
	uint8_t					Reserved;
	uint32_t				IOApicAddress;
	//	The first interrupt number that this I/O APIC handles
	//	The number of interrupts it handles can be retrieved by getting the number of redirection entries from register 0x01
	uint32_t				GlobalSystemInterruptBase;
}__packed RSDT_IOAPIC_t;

//	This explains how IRQ sources are mapped to global system interrupts.
typedef struct{
	RSDT_MADTEntry			Header;
	uint8_t					busSource;
	uint8_t					IRQSource;
	uint32_t				GlobalSystemInterrupt;
	uint16_t				Flags;
}__packed RSDT_IOAPICInterruptSourceOverride_t;

//	Specifies which I/O APIC interrupt inputs should be enabled as non-maskable. 
typedef struct{
	RSDT_MADTEntry			Header;
	uint8_t					NMISource;
	uint8_t					Reserved;
	uint32_t				GlobalSystemInterrupt;
	uint16_t				Flags;
}__packed RSDT_IOAPICNonMaskableInterruptSource_t;

//	Configure these with the LINT0 and LINT1 entries in the Local vector table of the relevant processor(')s(') local APIC.
typedef struct{
	RSDT_MADTEntry			Header;
	uint8_t					ACPIProcessorID;
	uint16_t				Flags;
	union{
		uint8_t				LINT0;
		uint8_t				LINT1;
	};
}__packed RSDT_LocalAPICNonMaskableInterrupts_t;

//	Provides 64 bit systems with an override of the physical address of the Local APIC. 
//	There can only be one of these defined in the MADT. 
//	If this structure is defined, the 64-bit Local APIC address stored within it should be used 
//		instead of the 32-bit Local APIC address stored in the MADT header. 
typedef struct{
	RSDT_MADTEntry			Header;
	uint16_t				Reserved;
	uint64_t				LocalAPICAddress;
}__packed RSDT_LocalAPICAddressOverride_t;

typedef struct{
	RSDT_MADTEntry			Header;
	uint16_t				Reserved;
	uint32_t				Localx2APICID;
	uint32_t				APICID;
}__packed RSDT_ProcessorLocalx2APIC_t;