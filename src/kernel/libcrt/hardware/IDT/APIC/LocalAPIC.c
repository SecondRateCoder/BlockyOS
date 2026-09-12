#include "LocalAPIC.h"
#include "kernel/libcrt/def.h"
#include "kernel/libcrt/hardware/PIT.h"

/**	The Local APIC works by:	
 * 		When an Interuupt is Signalled:
 * 			If the Interrupt is Non-Maskable:
 * 				Process it Now, Pause any currently Handled Process if need be.
 * 			If the bit is set in the IRR(Interrupt Request Register) already:
 * 				The bit is set in both the ISR(In-Service Register) and IRR(Interrupt Request Register).
 * 			If the LocalAPIC is not ready to Dispatch it:
 * 				The Bit corresponding to the Interrupt is set in the IRR(Interrupt Request Register)
 * 			If the LocalAPIC discovers a bit higher than the bit of the Currently Handled Interrupt is set:
 * 				The Lower Interrupt Bit is paused(withouth raising an EOI) and the Higher Bit Interrupt is handled
 * 			If an Interrupt is raised but it's bit is set in the ISR(In-Service Register) and IRR(Interrupt Request Register):
 * 				The LocalAPIC will reject it.
 * 			When the LocalAPIC is Ready to Dispatch any Interrupts:
 * 				The Highest Bit in the IRR(Interrupt Request Register) is cleared
 * 				(Thus emulating a Priority System!)and the Corresponding bit is set in the ISR(In-Service Register).
 * 			When the EOI Signal(EOI Register gets 0 WriteAPICRegister) is sent:
 * 				The Highest bit in the ISR(In-Service Register) is cleared and the next Interrupt is Passed.
*/

uint32_t GetLocalAPICID(){
	uint32_t eax, ebx, ecx, edx;
	__cpuid(1, eax, ebx, ecx, edx);
	return (uint32_t)((uint8_t)(ebx >> 24) & UINT8_MAX);
}

void *GetLocalAPICBase(void *ACPIBase, bool *_RDST){
	void *MADT = SearchACPITable("APIC", ACPIBase);
	if(!MADT){return NULL;}

	uint32_t currentApicId = GetLocalAPICID();

	if(((SDTHeader_t *)MADT)->Revision >= XSDPRevision){
		XSDT_MADT_t *Table = (XSDT_MADT_t *)MADT;
		uint32_t RemBytes = Table->Header.Length - sizeof(XSDT_MADT_t);
		for(uint32_t cc = 0; RemBytes != 0; ++cc){
			if(Table->Table[cc].Type == XSDT_ProcessorLocalAPIC){
				XSDT_ProcessorLocalAPIC_t *Local = (XSDT_ProcessorLocalAPIC_t *)(Table->Table + cc);
				if(Local->ApicID == currentApicId){
					if(_RDST){(*_RDST) = false;}
					//	Make sure it's Online Capable, if so then Enable it.
					//	Otherwise, return false. 
					if(__check(Local->Flags, XSDT_ProcessorEnabled)){return MapVirtual((void *)Table->LocalAPICAddress);}
				}
			}
			RemBytes -= Table->Table[cc].RecordTypeLength;
		}
	}else{
		RSDT_MADT_t *Table = (RSDT_MADT_t *)MADT;
		uint32_t RemBytes = Table->Header.Length - sizeof(RSDT_MADT_t);
		for(uint32_t cc = 0; RemBytes != 0; ++cc){
			if(Table->Table[cc].Type == RSDT_ProcessorLocalAPIC){
				RSDT_ProcessorLocalAPIC_t *Local = (RSDT_ProcessorLocalAPIC_t *)(Table->Table + cc);
				(*_RDST) = true;
				//	Make sure it's Online Capable, if so then Enable it.
				//	Otherwise, return false. 
				if(Local->ApicID == currentApicId && __check(Local->Flags, RSDT_ProcessorEnabled)){
					return MapVirtual((void *)(uint64_t)Table->LocalAPICAddress);
				}
			}
			RemBytes -= Table->Table[cc].RecordTypeLength;
		}
	}
}

bool InitLocalAPIC(void *ACPIBase, uint8_t InterruptBase, bool Enable){
	//	Ensure Higher than 32
	if(InterruptBase <= 32){return false;}
	//	Ensure Low 4-bits set
	if((InterruptBase & 0x0F) != 0x0F){return false;}
	bool _RSDT = false;
	void *Base = GetLocalAPICBase(ACPIBase, &_RSDT);
	if(!Base){return false;}
	//	Update the Spurious Register
	WriteAPICRegister(Base, LAR_SpuriousInterruptVector, (uint32_t)InterruptBase | (Enable? LocalAPICEnableBit: 0));
	return true;
}

void EnableLocalAPIC(void *ACPIBase){
	bool _RSDT = false;
	uint32_t Temp = 0;
	void *Base = GetLocalAPICBase(ACPIBase, &_RSDT);
	if(!Base){return;}
	ReadAPICRegister(Base, LAR_SpuriousInterruptVector, Temp);
	WriteAPICRegister(Base, LAR_SpuriousInterruptVector, Temp | LocalAPICEnableBit);
}
void DisableLocalAPIC(void *ACPIBase){
	bool _RSDT = false;
	uint32_t Temp = 0;
	void *Base = GetLocalAPICBase(ACPIBase, &_RSDT);
	if(!Base){return;}
	ReadAPICRegister(Base, LAR_SpuriousInterruptVector, Temp);
	WriteAPICRegister(Base, LAR_SpuriousInterruptVector, Temp & ~LocalAPICEnableBit);
}