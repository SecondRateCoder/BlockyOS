#pragma once

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"
#include "kernel/libcrt/memory/memory.h"
#include "kernel/libcrt/hardware/ACPI/ACPI.h"

#define XSDT_MCFGCode		"MCFG"
typedef struct{
	//	Physical_Address = MMIO_Starting_Physical_Address + ((Bus) << 20 | Device << 15 | Function << 12)
	uint64_t	ConfigurationBaseAddress;
	uint16_t	SegmentGroup;
	uint8_t		StartingBus, 
				EndingBus;
	uint32_t	Reserved0;
}XSDT_MCFGConfigurationEntry_t;
typedef struct{
	SDTHeader_t	Header;
	uint64_t	Reserved0;
	//	(Header.Length - (8 + sizeof(SDTHeader_t)) / sizeof(RSDT_MCFGConfigurationEntry_t))
	XSDT_MCFGConfigurationEntry_t	Table[];
}XSDT_MCFG_t;