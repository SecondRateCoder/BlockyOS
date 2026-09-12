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


bool ValidateHeader(SDTHeader_t *Header){
	if(!Header || Header->Length < sizeof(SDTHeader_t)){return false;}
	uint8_t checksum = 0, *buffer = (uint8_t *)Header;
	for(uint32_t cc = 0; cc < Header->Length; ++cc){checksum += buffer[cc];}
	//	Overflow into 0
	return (checksum == 0);
}

uint8_t __inline GetACPIRevision(void *ACPIBase){
	return *((uint8_t *)ACPIBase + __offsetof(RSDP_t, Version0.Revision));
}

static void *_SearchTable(char Signature[4], void *ACPIBase, uint8_t Revision){
	if(!Signature || !ACPIBase){return NULL;}
	if(Revision >= XSDPRevision){
		XSDT_t *Table = (XSDT_t *)ACPIBase;
		if(!ValidateHeader(&(Table->Header))){return NULL;/*Table Corrupted*/}
		//	Quickly Verify the Signature
		bool supported = false;
		for(uint32_t cc = 0; cc < XSDT_NSupported; ++cc){
			if(!memcmp(Signature, XSDT_SupportedTables[cc], sizeof(uint32_t))){supported = true; break;}
		}
		if(!supported){return NULL;}
		for(uint32_t cc = 0; cc < ((Table->Header.Length - sizeof(SDTHeader_t)) / sizeof(Table->Pointers[0])); ++cc){
			SDTHeader_t *Entry = MapVirtual((void *)(Table->Pointers[cc]));
			if(Entry && ValidateHeader(Entry)){
				if(!memcmp(Entry->Signature, Signature, sizeof(uint32_t))){return Entry;}
				if(!memcmp((uint32_t *)(Entry->Signature + 1), (uint32_t *)("SDT"), 3)){
					return _SearchTable(Signature, Entry, Entry->Signature[0] == 'X'? XSDPRevision: Entry->Signature[0] == 'R'? RSDPRevision: 0x0);}
			}
		}
	}else{
		RSDT_t *Table = (RSDT_t *)ACPIBase;
		if(!ValidateHeader(&(Table->Header))){return NULL;/*Table Corrupted*/}
		//	Quickly Verify the inputted Signature
		bool supported = false;
		for(uint32_t cc = 0; cc < RSDT_NSupported; ++cc){
			if(!memcmp(Signature, RSDT_SupportedTables[cc], sizeof(uint32_t))){supported = true; break;}
		}
		if(!supported){return NULL;}
		for(uint32_t cc = 0; cc < ((Table->Header.Length - sizeof(SDTHeader_t)) / sizeof(Table->Pointers[0])); ++cc){
			SDTHeader_t *Entry = MapVirtual((void *)(Table->Pointers[cc]));
			if(Entry && ValidateHeader(Entry)){
				if(!memcmp(Entry->Signature, Signature, sizeof(uint32_t))){return Entry;}
				if(!memcmp((uint32_t *)(Entry->Signature + 1), (uint32_t *)("SDT"), 3)){
					return _SearchTable(Signature, Entry, Entry->Signature[0] == 'X'? XSDPRevision: Entry->Signature[0] == 'R'? RSDPRevision: 0x0);}
			}
		}
	}
	return NULL;
}

void *SearchACPITable(char Signature[4], void *ACPIBase){
	if(!Signature || !ACPIBase){return NULL;}
	uint8_t Revision = GetACPIRevision(ACPIBase);
	void *Root = Revision >= XSDPRevision
		? MapVirtual((void *)((XSDP_t *)ACPIBase)->Version1.XsdtAddress)
		: MapVirtual((void *)(uint64_t)((RSDP_t *)ACPIBase)->Version0.RsdtAddress);
	return _SearchTable(Signature, Root, Revision);
}
