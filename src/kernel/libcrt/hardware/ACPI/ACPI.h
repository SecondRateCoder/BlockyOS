#pragma once

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"
#include "kernel/libcrt/hardware/paging/paging.h"
#include "kernel/libcrt/memory/allocator/malloc.h"
#include "kernel/libcrt/memory/memory.h"

#define RSDPRevision		0
#define XSDPRevision		2

typedef struct{
	char			Signature[4];
	uint32_t		Length;
	uint8_t			Revision;
	uint8_t			Checksum;
	char			OEMID[6];
	char			OEMTableID[8];
	uint32_t		OEMRevision;
	uint32_t		CreatorID;
	uint32_t		CreatorRevision;
}__packed SDTHeader_t;

LibAPI bool ValidateHeader(SDTHeader_t *Header);
LibAPI uint8_t __inline GetACPIRevision(void *ACPIBase);
LibAPI void *SearchACPITable(char Signature[4], void *ACPIBase);