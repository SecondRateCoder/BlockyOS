#include "ACPI.h"

#include "RSDP/RSDT.h"
#include "RSDP/DSDT.h"
#include "RSDP/MADT.h"
#include "RSDP/SRAT.H"
#include "RSDP/SSDT.h"

#include "XSDP/XSDT.h"
#include "XSDP/BGRT.h"
#include "XSDP/DSDT.h"
#include "XSDP/MADT.h"
#include "XSDP/SRAT.H"
#include "XSDP/SSDT.h"


bool ValidateHeader(ACPIStdHeader_t *Header){
	//	Real tables are a few KiB to a few hundred KiB. A Length far beyond that means Header is not a table (wrong
	//	address): summing e.g. 4GiB byte by byte is what looks like a hang under Bochs. 16MiB is a generous upper bound.
	if(!Header || Header->Length < sizeof(ACPIStdHeader_t) || Header->Length > (16U << 20)){return false;}
	uint8_t checksum = 0, *buffer = (uint8_t *)Header;
	for(uint32_t cc = 0; cc < Header->Length; ++cc){checksum += buffer[cc];}
	//	Overflow into 0
	return (checksum == 0);
}

uint8_t __inline GetACPIRevision(void *ACPIBase){return *(uint8_t *)(ACPIBase + __offsetof(RSDP_t, Version0.Revision));}

//	ACPIBase is the PHYSICAL address of the RSDT/XSDT, and every table below it is read at its physical address through
//	the identity map. MapVirtual must NOT be used here: it returns an allocator coordinate, not the table's address
//	(and it was being applied twice: once in SearchACPITable and again here).
static void *_SearchTable(char Signature[4], void *ACPIBase, uint8_t Revision){
	bool supported = false;
	ACPIStdHeader_t *RootTable = (ACPIStdHeader_t *)ACPIBase;
	if(!RootTable || !ValidateHeader(RootTable)){return NULL;}
	if(Revision >= XSDPRevision){
		XSDT_t *Table = (XSDT_t *)RootTable;
		for(uint32_t cc = 0; cc < XSDT_NSupported; ++cc){
			if(!memcmp(Signature, XSDT_SupportedTables[cc], sizeof(uint32_t))){supported = true;	break;}}
		if(!supported){return NULL;}
		
		uint32_t entryCount = (Table->Header.Length - sizeof(ACPIStdHeader_t)) / sizeof(Table->Pointers[0]);
		for(uint32_t cc = 0; cc < entryCount; ++cc){
			ACPIStdHeader_t *Entry = (ACPIStdHeader_t *)(uint64_t)(Table->Pointers[cc]);
			//	Compare the signature first: only the table we want needs its checksum summed.
			if(Entry && !memcmp(Entry->Signature, Signature, sizeof(uint32_t)) && ValidateHeader(Entry)){return Entry;}
		}
	}else{
		RSDT_t *Table = (RSDT_t *)RootTable;
		for(uint32_t cc = 0; cc < RSDT_NSupported; ++cc){
			if(!memcmp(Signature, RSDT_SupportedTables[cc], sizeof(uint32_t))){supported = true;	break;}}
		if(!supported){return NULL;}
		uint32_t entryCount = (Table->Header.Length - sizeof(ACPIStdHeader_t)) / sizeof(Table->Pointers[0]);
		for(uint32_t cc = 0; cc < entryCount; ++cc){
			ACPIStdHeader_t *Entry = (ACPIStdHeader_t *)(uint64_t)(Table->Pointers[cc]);
			if(Entry && !memcmp(Entry->Signature, Signature, sizeof(uint32_t)) && ValidateHeader(Entry)){return Entry;}
		}
	}
	return NULL;
}

void *SearchACPITable(char Signature[4], void *ACPIBase){
	if(!Signature || !ACPIBase){return NULL;}
	RSDP_t *RSDP = (RSDP_t *)ACPIBase;
	uint8_t Revision = RSDP->Version0.Revision;
	uint64_t Root = 0;
	if(Revision >= XSDPRevision){Root = (uint64_t)MapVirtual((void *)(((XSDP_t *)RSDP)->Version1.XsdtAddress));}
	//	No XSDT (or a zero XsdtAddress): fall back to the RSDT.
	if(!Root){Root = (uint64_t)MapVirtual((void *)(RSDP->Version0.RsdtAddress));	Revision = 0;}
	return _SearchTable(Signature, (void *)Root, Revision);
}