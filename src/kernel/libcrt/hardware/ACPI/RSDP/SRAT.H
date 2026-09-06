#pragma once

#include "RSDT.h"

//	The SRAT table is described by the ACPI Specification 5.0a [1] in section 5.2.16 as: 
//		A table providing information that allows an OSPM (Operating System Power Management [System]) 
//		"to associate processors and memory ranges, including ranges of memory provided by hot-added memory devices, 
//		with system localities proximity domains and clock domains." 
//	Basically, the SRAT provides information on relationships of resources in the system - namely the processor(s) and memory. 
//	The SRAT table associates each processor and blocks of memory to a plain integer, named proximity domains in ACPI jargon. 
#define	RSDT_SRATCode		"SRAT"

typedef struct{
	SDTHeader_t			Header;
	uint8_t				Reserved[12];
}__packed RSDT_SRATHeader_t;

enumdef(uint8_t, RSDT_SRATStructType){
	RSDT_SRATProcessorLocalAPIC = 0x0, RSDT_SRATSAPICAffinityStructure = 0x0, 
	RSDT_SRATMemoryAffinityStructure = 0x1, RSDT_SRATProcessorLocalx2APICAffinityStructure = 0x2
};

//	If clear, the OS should ignore the contents of the structure.
enumdef(uint8_t, RSDT_SRAT_proc_lapic_flags){RSDT_SRAT_proc_lapic_enabled};
//	This structure describes the association of a processor's APIC ID (or SAPIC ID/EID) and 
//		the proximity domain (in ACPI jargon) in which it belongs to. 
typedef struct{
	uint8_t					Type;
	uint8_t					Length;
	uint8_t					ProximityDomainLow;
	uint8_t					APICID;
	RSDT_SRAT_proc_lapic_flags	Flags;
	uint8_t					SAPIC_EID;
	uint8_t					ProximityDomainHigh[3];
	uint32_t				ClockDomain;
}__packed RSDT_SRAT_proc_lapic_t;

enumdef(uint32_t, RSDT_SRAT_mem_struct_flags){
	RSDT_SRAT_mem_struct_Enabled = 0x0, RSDT_SRAT_mem_struct_HotPluggable = 0x1, 
	RSDT_SRAT_mem_struct_NonVolatile = 0x2
};
typedef struct{
	uint8_t						Type;
	//	40
	uint8_t						Length;
	//	The domain to which this memory region belongs to
	uint32_t					Domain;
	uint8_t						Reserved0[2];
	//	Low 32 bits of the base address of the memory range
	uint32_t					BaseLow;
	//	High 32 bits of the base address of the memory range
	uint32_t					BaseHigh;
	//	Low 32 bits of the length of the range
	uint32_t					LengthLow;
	//	High 32 bits of the length
	uint32_t					LengthHigh;
	uint8_t						Reserved1[4];
	RSDT_SRAT_mem_struct_flags	Flags;
	uint8_t						Reserved2[8];
}__packed RSDT_SRAT_mem_struct;


typedef struct{
    uint8_t					Type;
	// 24
    uint8_t					Length;
    uint8_t					Reserved0[2];
	// The proximity domain which the logical processor belongs to
    uint32_t				ProximityDomain;
	// Processor's x2APIC ID
    uint32_t				x2APIC_ID;
    uint32_t				Flags;
	// The clock domain which the processor belongs to (more jargon)
    uint32_t				ClockDomain;
    uint8_t					Reserved1[4];
}__packed RSDT_SRAT_proc_lapic2_struct;