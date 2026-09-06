#pragma once

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"
#include "kernel/libcrt/memory/memory.h"
#include "kernel/libcrt/hardware/ACPI/ACPI.h"

typedef struct{
	struct{
		char		Signature[8];
		uint8_t		Checksum;
		char		OEMID[6];
		uint8_t		Revision;
		uint32_t	RsdtAddress;
	}Version0;
}__packed RSDP_t;

typedef char RSDT_Signature[4];
static const RSDT_Signature RSDT_SupportedTables[] = {
	"APIC",	//	Multiple APIC Description Table
	"BERT",	//	Boot Error Record Table
	"CPEP",	//	Corrected Platform Error Polling Table
	"DSDT",	//	Differentiated System Description Table
	"ECDT",	//	Embedded Controller Boot Resources Table
	"EINJ",	//	Error Injection Table
	"ERST",	//	Error Record Serialization Table
	"FACP",	//	Fixed ACPI Description Table
	"FACS",	//	Firmware ACPI Control Structure
	"HEST",	//	Hardware Error Source Table
	"MSCT",	//	Maximum System Characteristics Table
	"MPST",	//	Memory Power State Table
	"OEMx",	//	OEM Specific Information Tables (Any table with a signature beginning with "OEM" falls into this definition)
	"PMTT",	//	Platform Memory Topology Table
	"PSDT",	//	Persistent System Description Table
	"RASF",	//	ACPI RAS Feature Table
	"RSDT",	//	Root System Description Table
	"SBST",	//	Smart Battery Specification Table
	"SLIT",	//	System Locality System Information Table
	"SRAT",	//	System Resource Affinity Table
	"SSDT",	//	Secondary System Description Table
	"XSDT",	//	Extended System Description Table (XSDT; 64-bit version of the RSDT)
	"MCFG", //	PCI(e) Configuration Table
};
static const uint32_t RSDT_NSupported = sizeof(RSDT_SupportedTables) / sizeof(RSDT_Signature);

typedef struct{
	SDTHeader_t		Header;
	//	uint32_t entries[(Header.Length - sizeof(Header)) / 4]
	uint32_t		Pointers[];
}__packed RSDT_t;