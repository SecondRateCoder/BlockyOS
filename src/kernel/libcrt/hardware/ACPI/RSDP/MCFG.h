#pragma once

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"
#include "kernel/libcrt/memory/memory.h"
#include "kernel/libcrt/hardware/ACPI/ACPI.h"

#define RSDT_MCFGCode		"MCFG"
typedef struct{
	//	Physical_Address = MMIO_Starting_Physical_Address + ((Bus) << 20 | Device << 15 | Function << 12)
	uint32_t	ConfigurationBaseAddress;
	uint16_t	SegmentGroup;
	//	Lowest Addressable Bus Number
	uint8_t		StartingBus, 
	//	Highest Addressable Bus Number
				EndingBus;
	uint32_t	Reserved0;
}RSDT_MCFGConfigurationEntry_t;
typedef struct{
	SDTHeader_t	Header;
	uint64_t	Reserved0;
	//	(Header.Length - (8 + sizeof(SDTHeader_t)) / sizeof(RSDT_MCFGConfigurationEntry_t))
	RSDT_MCFGConfigurationEntry_t	Table[];
}RSDT_MCFG_t;