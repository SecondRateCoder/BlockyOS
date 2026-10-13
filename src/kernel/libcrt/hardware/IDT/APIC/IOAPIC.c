#include "IOAPIC.h"

//  The IO APIC uses two registers for most of its operation - an address register at IOAPICBASE+0 and a data register at IOAPICBASE+0x10. 
//  All accesses must be done on 4 byte boundaries. 
//  The address register uses the bottom 8 bits for register select.
void *MapAPIC(void *physical){
	return (MapVirtual(physical) != 0)? AllocatePages(physical, PAGE_SIZE, 
		(GlobalEnable | ReadWritable | SupervisorMode | PageLevelWriteThroughEnable), 0x0): 
		MapVirtual(physical);
}

bool DisableIoApicInterrupt(void *acpibase, uint32_t N, uint32_t Interrupt){
	uint32_t _Version = ReadIoApicRegister(acpibase, N, _IOAPICVER);
	IOAPICVersionRegister Version = *((IOAPICVersionRegister *)(&_Version));
	if(Interrupt < (Version.MaxIRQNumber + 1)){
		IOAPICInterruptRegister r = ReadIoApicInterruptRegister(acpibase, N, Interrupt);
		r.Register.InterruptDisable = true;
		return WriteIoApicInterruptRegister(acpibase, N, Interrupt, r);
	}
	return false;
}
bool DisableIoApic(void *acpibase, uint32_t N){
	uint32_t _Version = ReadIoApicRegister(acpibase, N, _IOAPICVER);
	IOAPICVersionRegister Version = *((IOAPICVersionRegister *)(&_Version));
	uint64_t cc = 0;
	bool temp = false;
	while((temp = DisableIoApicInterrupt(acpibase, N, cc))){cc++;}
	return temp;
}

bool EnableIoApicInterrupt(void *acpibase, uint32_t N, uint32_t Interrupt){
	uint32_t _Version = ReadIoApicRegister(acpibase, N, _IOAPICVER);
	IOAPICVersionRegister Version = *((IOAPICVersionRegister *)(&_Version));
	if(Interrupt < (Version.MaxIRQNumber + 1)){
			IOAPICInterruptRegister r = ReadIoApicInterruptRegister(acpibase, N, Interrupt);
			r.Register.InterruptDisable = false;
			WriteIoApicInterruptRegister(acpibase, N, Interrupt, r);
	}
}
bool EnableIoApic(void *acpibase, uint32_t N){
	uint32_t _Version = ReadIoApicRegister(acpibase, N, _IOAPICVER);
	IOAPICVersionRegister Version = *((IOAPICVersionRegister *)(&_Version));
	uint64_t cc = 0;
	bool temp = false;
	while((temp = EnableIoApicInterrupt(acpibase, N, cc))){cc++;}
	return temp;
}

uint32_t _ReadIoApic(void *addr, uint32_t reg){
	uint32_t volatile *ioapic = (uint32_t volatile *)addr;
	*ioapic = (reg & 0xff);
	return ioapic[4];
}
uint32_t ReadIoApicRegister(void *acpibase, uint32_t N, uint32_t reg){
	void *MADT = SearchACPITable("APIC", acpibase);
	if(((SDTHeader_t *)MADT)->Revision >= XSDPRevision){
		XSDT_MADT_t *Table = (XSDT_MADT_t *)MADT;
		uint32_t RemBytes = Table->Header.Length - sizeof(XSDT_MADT_t);
		for(uint32_t cc = 0; RemBytes != 0; ++cc){
			if(Table->Table[cc].Type == XSDT_IOAPIC){
				XSDT_IOAPIC_t *Local = (XSDT_IOAPIC_t *)(Table->Table + cc);
				if(N){N--;			break;}
				return _ReadIoApic(MapAPIC((void *)Local->IOApicAddress), reg);
				break;
			}
			RemBytes -= Table->Table[cc].RecordTypeLength;
		}
	}else{
		RSDT_MADT_t *Table = (RSDT_MADT_t *)MADT;
		uint32_t RemBytes = Table->Header.Length - sizeof(RSDT_MADT_t);
		for(uint32_t cc = 0; RemBytes != 0; ++cc){
			if(Table->Table[cc].Type == RSDT_ProcessorLocalAPIC){
				RSDT_IOAPIC_t *Local = (RSDT_IOAPIC_t *)(Table->Table + cc);
				if(N){N--;			break;}
				return _ReadIoApic(MapAPIC((void *)Local->IOApicAddress), reg);
			}
			RemBytes -= Table->Table[cc].RecordTypeLength;
			break;
		}
	}
	return 0x0;
}

void _WriteIoApic(void *addr, uint32_t reg, uint32_t value){
	uint32_t volatile *ioapic = (uint32_t volatile *)addr;
	*ioapic = (reg & 0xff);
	ioapic[4] = value;
}
void WriteIoApicRegister(void *acpibase, uint32_t N, uint32_t reg, uint32_t value){
	void *MADT = SearchACPITable("APIC", acpibase);
	if(((SDTHeader_t *)MADT)->Revision >= XSDPRevision){
		XSDT_MADT_t *Table = (XSDT_MADT_t *)MADT;
		uint32_t RemBytes = Table->Header.Length - sizeof(XSDT_MADT_t);
		for(uint32_t cc = 0; RemBytes != 0; ++cc){
			if(Table->Table[cc].Type == XSDT_IOAPIC){
				XSDT_IOAPIC_t *Local = (XSDT_IOAPIC_t *)(Table->Table + cc);
				if(N){N--;			break;}
				_WriteIoApic(MapAPIC((void *)Local->IOApicAddress), reg, value);
				break;
			}
			RemBytes -= Table->Table[cc].RecordTypeLength;
		}
	}else{
		RSDT_MADT_t *Table = (RSDT_MADT_t *)MADT;
		uint32_t RemBytes = Table->Header.Length - sizeof(RSDT_MADT_t);
		for(uint32_t cc = 0; RemBytes != 0; ++cc){
			if(Table->Table[cc].Type == RSDT_ProcessorLocalAPIC){
				RSDT_IOAPIC_t *Local = (RSDT_IOAPIC_t *)(Table->Table + cc);
				if(N){N--;			break;}
				_WriteIoApic(MapAPIC((void *)Local->IOApicAddress), reg, value);
			}
			RemBytes -= Table->Table[cc].RecordTypeLength;
			break;
		}
	}
}

IOAPICInterruptRegister ReadIoApicInterruptRegister(void *acpibase, uint32_t APICN, uint32_t N){
	uint32_t _Version = ReadIoApicRegister(acpibase, N, _IOAPICVER);
	IOAPICVersionRegister Version = *((IOAPICVersionRegister *)(&_Version));
	if(N < (Version.MaxIRQNumber + 1)){
		IOAPICInterruptRegister out = {0};
		((uint32_t *)&out)[0] = ReadIoApicRegister(acpibase, APICN, _IOAPICREDTBL(N, false));
		((uint32_t *)&out)[1] = ReadIoApicRegister(acpibase, APICN, _IOAPICREDTBL(N, true));
		return out;
	}else{return (IOAPICInterruptRegister){0};}
}

bool WriteIoApicInterruptRegister(void *acpibase, uint32_t APICN, uint32_t N, IOAPICInterruptRegister R){
	uint32_t _Version = ReadIoApicRegister(acpibase, N, _IOAPICVER);
	IOAPICVersionRegister Version = *((IOAPICVersionRegister *)(&_Version));
	if(N < (Version.MaxIRQNumber + 1)){
		WriteIoApicRegister(acpibase, APICN, _IOAPICREDTBL(N, false), ((uint32_t *)&R)[0]);
		WriteIoApicRegister(acpibase, APICN, _IOAPICREDTBL(N, true), ((uint32_t *)&R)[1]);
		return true;
	}else{return false;}
}

void IoApicSetRedirection(void *acpibase, uint32_t N, uint8_t InterruptPin, uint8_t InterruptVector, uint8_t LocalAPIC){
	uint32_t reg_low = 0x10 + (InterruptPin * 2);
	uint32_t reg_high = reg_low + 1;

	// Configure Low Dword: Vector, Fixed delivery mode (0), Physical mode (0), Unmasked (0)
	uint32_t low_value = InterruptVector & 0xFF; // Delivery mode, polarity, trigger mode default to 0 (Fixed, Active High, Edge)

	// Configure High Dword: Destination APIC ID (shifted to bits 24-31)
	uint32_t high_value = ((uint32_t)LocalAPIC << 24);

	// Write to IO APIC
	WriteIoApicRegister(acpibase, N, reg_low, low_value);
	WriteIoApicRegister(acpibase, N, reg_high, high_value);
}