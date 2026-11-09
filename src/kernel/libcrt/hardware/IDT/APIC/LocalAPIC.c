#include "LocalAPIC.h"
#include "kernel/libcrt/def.h"
#include "kernel/libcrt/hardware/PIT.h"
#include "kernel/libcrt/hardware/paging/paging.h"

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
	uint32_t eax = 0x00, ebx = 0x00, ecx = 0x00, edx = 0x00;
	__cpuid(1, eax, ebx, ecx, edx);
	return (uint32_t)((uint8_t)(ebx >> 24) & UINT8_MAX);
}

void *GetLocalAPICBase(void *ACPIBase, bool *_RDST){
	void *MADT = SearchACPITable("APIC", ACPIBase);
	if(!MADT){return NULL;}
	uint32_t currentApicId = GetLocalAPICID();
	//	The MADT and its records are firmware memory: read them at their physical address (identity map), and
	//	walk the records by BYTE OFFSET. They have different sizes (LAPIC 8, IOAPIC 12, ...), so indexing them as an
	//	array and subtracting each Length from RemBytes both drifts and can underflow into an endless loop.
	uint8_t *Cursor, *End = (uint8_t *)MADT + ((ACPIStdHeader_t *)MADT)->Length;
	uint32_t LocalAPICAddress;
	bool XSDT = (((ACPIStdHeader_t *)MADT)->Revision >= XSDPRevision);
	if(_RDST){(*_RDST) = !XSDT;}
	if(XSDT){Cursor = (uint8_t *)(((XSDT_MADT_t *)MADT)->Table);	LocalAPICAddress = (uint32_t)((XSDT_MADT_t *)MADT)->LocalAPICAddress;}
	else{Cursor = (uint8_t *)(((RSDT_MADT_t *)MADT)->Table);		LocalAPICAddress = (uint32_t)((RSDT_MADT_t *)MADT)->LocalAPICAddress;}
	while((Cursor + 2) <= End){
		uint8_t Type = Cursor[0], Length = Cursor[1];
		if(Length < 2){break;}		//	corrupt record: stop instead of spinning
		if(Type == (XSDT? XSDT_ProcessorLocalAPIC: RSDT_ProcessorLocalAPIC)){
			if(XSDT){
				XSDT_ProcessorLocalAPIC_t *Local = (XSDT_ProcessorLocalAPIC_t *)Cursor;
				if((Local->ApicID == currentApicId) && __check(Local->Flags, XSDT_ProcessorEnabled)){return (void *)(uint64_t)LocalAPICAddress;}
			}else{
				RSDT_ProcessorLocalAPIC_t *Local = (RSDT_ProcessorLocalAPIC_t *)Cursor;
				if((Local->ApicID == currentApicId) && __check(Local->Flags, RSDT_ProcessorEnabled)){return (void *)(uint64_t)LocalAPICAddress;}
			}
		}
		Cursor += Length;
	}
	return NULL;	//	the original fell off the end here and returned whatever was in rax
}

bool InitLocalAPIC(void *ACPIBase, uint8_t InterruptBase, bool Enable){
	//	Ensure Higher than 32
	if(InterruptBase < 32){return false;}
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

void SendIPI(void *ACPIBase, uint32_t apic_id, uint32_t icr_low){
	bool _RSDT = false;
	void *Base = GetLocalAPICBase(ACPIBase, &_RSDT);
    // Write target APIC ID to ICR High (bits 24-31)
    WriteAPICRegister(Base, LAR_InterruptCommandRegister, apic_id << 24);
    // Write command to ICR Low (triggers the interrupt)
    WriteAPICRegister(Base, LAR_InterruptCommandRegister, icr_low);
}

void *BootCore(void *ACPIBase, uint8_t ApicID, uint8_t *InterruptVector, void *Code, UINT64 CodeLength){
	//	Initialise the Memory at Location
	void *Tmp = NULL;
	*InterruptVector = 0;
	// EnableInputAddress(true, true);
	while((Tmp = AllocatePages((void *)(*InterruptVector * PAGE_SIZE), CodeLength, (ReadWritable | SupervisorMode | ExecuteEnable), 0x00)) == InvalidPointer){
		(*InterruptVector)++;}
	memcpy(Tmp, Code, CodeLength);
    //	Send INIT IPI (Assert)
    //	Delivery Mode = 101b (INIT), Level = 1, Trigger = Edge
    SendIPI(ACPIBase, ApicID, 0x00004500);
    SleepMS(10);

    //	Send First Startup IPI (SIPI)
    //	Delivery Mode = 110b (Startup), Vector = sipi_vector (e.g. 0x08 for 0x8000)
    SendIPI(ACPIBase, ApicID, 0x00004600 | *InterruptVector);
    SleepMS(10);

    //	Send Second Startup IPI (SIPI) for hardware reliability
    SendIPI(ACPIBase, ApicID, 0x00004600 | *InterruptVector);
    SleepMS(10);

	return Tmp;
}