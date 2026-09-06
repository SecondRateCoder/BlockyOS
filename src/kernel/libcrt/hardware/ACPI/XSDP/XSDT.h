#pragma once

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"
#include "kernel/libcrt/memory/memory.h"
#include "kernel/libcrt/hardware/ACPI/RSDP/RSDT.h"
#include "kernel/libcrt/hardware/ACPI/ACPI.h"

//	"the XSDT provides identical functionality to the RSDT but accommodates physical addresses of DESCRIPTION HEADERs that are larger than 32-bits."
//		- ACPI Specification v5.0. 
//	This means all addresses are now 64 bits. 
//	If the pointer to the XSDT is valid, the OS MUST use the XSDT. 
//	All of the addresses will probably be 32 bits, and if you are not using PAE, 
//		you couldn't use higher than 32 bits, but the spec says you should use this anyway. 
#define XSDTCode		"XSDT"

typedef struct{
	RSDP_t				Version0;
	struct{
		uint32_t		Length;
		uint64_t		XsdtAddress;
		uint8_t			ExtendedChecksum;
		uint8_t			reserved[3];
	}Version1;
}__packed XSDP_t;

typedef char XSDT_Signature[4];
static const XSDT_Signature XSDT_SupportedTables[] = {
	"APIC",	//	Multiple APIC Description Table
	"BGRT",	//	Boot Graphics Resource Table (BGRT; only supported on UEFI systems)
	"BERT",	//	Boot Error Record Table
	"CPEP",	//	Corrected Platform Error Polling Table
	"DSDT",	//	Differentiated System Description Table
	"ECDT",	//	Embedded Controller Boot Resources Table
	"EINJ",	//	Error Injection Table
	"ERST",	//	Error Record Serialization Table
	"FACP",	//	Fixed ACPI Description Table
	"FACS",	//	Firmware ACPPI Control Structure
	"HEST",	//	Hardware Error Source Table
	"MSCT",	//	Maximum System Characteristics Table
	"MPST",	//	Memory Power State Table
	// "OEMx",	//	OEM Specific Information Tables (Any table with a signature beginning with "OEM" falls into this definition)
	"PMTT",	//	Platform Memory Topology Table
	"PSDT",	//	Persistent System Description Table
	"RASF",	//	ACPI RAS FeatureTable
	"RSDT",	//	Root System Description Table (RSDT; 32-bit version of the XSDT)
	"SBST",	//	Smart Battery Specification Table
	"SLIT",	//	System Locality System Information Table
	"SRAT",	//	System Resource Affinity Table
	"SSDT",	//	Secondary System Description Table
	"XSDT",	//	Extended System Description Table
	"MCFG", //	PCI(e) Configuration Space
};
static const uint32_t XSDT_NSupported = sizeof(XSDT_SupportedTables) / sizeof(XSDT_Signature);

typedef struct{
	SDTHeader_t			Header;
	//	Pointers[(Header.Length - sizeof(Header)) / sizeof(uint64_t)]
	uint64_t 			Pointers[];
}XSDT_t;