#pragma once

#include "XSDT.h"

//	The MADT describes all of the interrupt controllers in the system. 
//	It can be used to enumerate the processors currently available. 
#define	XSDT_MADTCode			"APIC"
#define XSDT_LocalAPICSignature	0xFEE

enumdef(uint32_t, XSDT_MADTFlags){
	XSDT_Dual8259LegacyPICInstall = 0x1
};
enumdef(uint8_t, XSDT_MADTEntryType){
	XSDT_ProcessorLocalAPIC = 0x0, XSDT_IOAPIC = 0x1, 
	XSDT_IOAPICInterruptSourceOverride = 0x2, 
	XSDT_IOAPICNonMaskableInterruptSource = 0x3, 
	XSDT_LocalAPICNonMaskableInterrupts = 0x4, 
	XSDT_LocalAPICAddressOverride = 0x5, XSDT_ProcessorLocalx2APIC = 0x9
};
enumdef(uint32_t, XSDT_GenericMADTEntryFlag){
	//	Polarity Flags
	XSDT_NoPolarityOverride = 0b00, XSDT_ActiveHighPolarityOverride = 0b01, 
	XSDT_ActiveLowPolarityOverride = 0b11, 
	//	Trigger Flags
	XSDT_NoTriggerOverride = ((uint32_t)0b00 << 16), XSDT_EdgeTrigger = ((uint32_t)0b01 << 16), 
	XSDT_LevelTrigger = ((uint32_t)0b11 << 16)
};
typedef struct{
	XSDT_MADTEntryType	Type;
	uint8_t				RecordTypeLength;
}__packed XSDT_MADTEntry;
typedef struct{
	SDTHeader_t			Header;
	uint64_t			LocalAPICAddress;
	XSDT_MADTFlags		Flags;
	XSDT_MADTEntry		Table[];
}__packed XSDT_MADT_t;

enumdef(uint32_t, XSDT_ProcessorLocalAPICFlags){XSDT_ProcessorEnabled = 0x1, XSDT_OnlineCapable = 0x2};
typedef struct{
	XSDT_MADTEntry					Header;
	uint8_t							ProcessorID;
	uint8_t							ApicID;
	XSDT_ProcessorLocalAPICFlags	Flags;
}__packed XSDT_ProcessorLocalAPIC_t;

typedef struct{
	XSDT_MADTEntry			Header;
	uint8_t					IOApicID;
	uint8_t					Reserved;
	uint64_t				IOApicAddress;
	//	The first interrupt number that this I/O APIC handles
	//	The number of interrupts it handles can be retrieved by getting the number of redirection entries from register 0x01
	uint32_t				GlobalSystemInterruptBase;
}__packed XSDT_IOAPIC_t;

//	This explains how IRQ sources are mapped to global system interrupts.
typedef struct{
	XSDT_MADTEntry			Header;
	uint8_t					busSource;
	uint8_t					IRQSource;
	uint32_t				GlobalSystemInterrupt;
	uint16_t				Flags;
}__packed XSDT_IOAPICInterruptSourceOverride_t;

//	Specifies which I/O APIC interrupt inputs should be enabled as non-maskable. 
typedef struct{
	XSDT_MADTEntry			Header;
	uint8_t					NMISource;
	uint8_t					Reserved;
	uint32_t				GlobalSystemInterrupt;
	uint16_t				Flags;
}__packed XSDT_IOAPICNonMaskableInterruptSource_t;

//	Configure these with the LINT0 and LINT1 entries in the Local vector table of the relevant processor(')s(') local APIC.
typedef struct{
	XSDT_MADTEntry			Header;
	uint8_t					ACPIProcessorID;
	uint16_t				Flags;
	union{
		uint8_t				LINT0;
		uint8_t				LINT1;
	};
}__packed XSDT_LocalAPICNonMaskableInterrupts_t;

//	Provides 64 bit systems with an override of the physical address of the Local APIC. 
//	There can only be one of these defined in the MADT. 
//	If this structure is defined, the 64-bit Local APIC address stored within it should be used 
//		instead of the 32-bit Local APIC address stored in the MADT header. 
typedef struct{
	XSDT_MADTEntry			Header;
	uint16_t				Reserved;
	uint64_t				LocalAPICAddress;
}__packed XSDT_LocalAPICAddressOverride_t;

typedef struct{
	XSDT_MADTEntry			Header;
	uint16_t				Reserved;
	uint32_t				Localx2APICID;
	uint32_t				APICID;
}__packed XSDT_ProcessorLocalx2APIC_t;