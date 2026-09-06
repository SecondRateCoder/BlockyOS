#pragma once
#include "Boot/UEFI/drivers/socket/socket.h"

typedef struct{
	char *Name;
	void *Base, *Entry;
}ExecutableSection;

typedef struct{
	char *Name;
	void *Base;
}LoadedServiceEntry;
typedef struct{
	uint32_t NLoadedServices, NLoadedSections;
	LoadedServiceEntry *Table;
	ExecutableSection *Sections;
}LoadedService;

LoadedService LoadService(const socket_t *drive, const socket_t *manifest);
ExecutableSection *LoadExecutable(const socket_t *drive, const socket_t *file, UINT32 *Length);