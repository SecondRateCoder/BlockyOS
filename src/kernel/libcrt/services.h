// #pragma once

// #include "kernel/libcrt/def.h"
// #include "kernel/libcrt/math/int.h"
// #include "kernel/libcrt/memory/string.h"


// enumdef(uint64_t, ImportServiceReference){UndefinableService = 0};
// //	Pass NonZero Value
// #define ImportServiceDefinition(Table, Service, N)						\
// 	const ImportServiceReference ImportServiceReference__##Service = N;	\
// 	Table[N - 1] = {.Address = NULL, .Name = #Service};
// 	typedef struct{
// 	uint64_t	Address;
// 	char		*Name;
// }LocalLoadedServiceImport;

// typedef struct{
// 	char *Name;
// 	void *Base;
// }LocallyLoadedServiceEntry;
// typedef struct{
// 	uint32_t NLoadedServices;
// 	LocallyLoadedServiceEntry *Table;
// }LocallyLoadedService;

// inline bool InstantiateService(LocalLoadedServiceImport *Table, uint64_t N, LocallyLoadedService *Service){
// 	bool FullInstantiate = true;
// 	while(N--){
// 		uint32_t cc = Service->NLoadedServices;
// 		for(; cc > 0; --cc){
// 			if(!strcmp(Table[N].Name, Service->Table[cc].Name)){
// 				Table[N].Address = Service[cc].Table;
// 				break;
// 			}
// 		}
// 		if(!cc){FullInstantiate = false;}
// 	}
// 	return FullInstantiate;
// }