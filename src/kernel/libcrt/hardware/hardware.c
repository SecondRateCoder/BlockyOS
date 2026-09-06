#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"

LibAPI bool TestMSR(void){
    struct cpuid_result{
        uint32_t eax;
        uint32_t ebx;
        uint32_t ecx;
        uint32_t edx;
    };
    struct cpuid_result r = {0};
    __cpuid(1, r.eax, r.ebx, r.ecx, r.edx);
    // __asm__ volatile(
    //     "cpuid"
    //     : "=a"(r.eax), "=b"(r.ebx), "=c"(r.ecx), "=d"(r.edx)
    //     : "a"(1), "c"(0)
    // );
    return (r.edx & CPUID_FEAT_EDX_MSR) != 0;
}

LibAPI uint64_t ReadMSR(uint32_t msr){
    uint32_t lo, hi;
    __asm__ volatile(
        "rdmsr"
        : "=a"(lo), "=d"(hi)
        : "c"(msr)
    );
    return ((uint64_t)hi << 32) | lo;
}

LibAPI void WriteMSR(uint32_t msr, uint64_t value){
    uint32_t lo = (uint32_t)value, 
            hi = (uint32_t)(value >> 32);
    __asm__ volatile(
        "wrmsr"
        :
        : "c"(msr), "a"(lo), "d"(hi)
        : "memory"
    );
}


#include "PIT.h"
volatile uint128_t pit_ticks = {0x00};
#define PITTick         (1193182 / 1000)
#define MS_TO_TICKS(MS)	(MS * PITTick)

// Call this inside your IRQ0 handler routine
LibAPI ISRCallbackDefinition(_PITTick){pit_ticks[0]++; if(!pit_ticks[0]){pit_ticks[1]++;} ISRCallbackReturn;}
LibAPI void InitializePIT(void *acpibase, uint8_t IOAPIC, uint8_t LocalAPIC, uint32_t Priviledge){
	uint8_t _IST, Vector;
	IDTEntrySegmentSelector64 sselector;
	if(AllocateIST(&_IST, &sselector) && AllocateInterruptVector(&Vector)){
		//  Search the MADT for the InterruptSourceOverride of PIN0
		uint32_t PIN = 0x00;
		void *_MADT = SearchACPITable("APIC", acpibase);
		if(*((uint8_t *)_MADT + __offsetof(RSDP_t, Version0.Revision)) >= XSDPRevision){
			XSDT_MADT_t *MADT = _MADT;
			uint32_t Offset = 0x00;
			for(uint32_t cc = 0; cc < (MADT->Header.Length - sizeof(XSDT_MADT_t) / sizeof(XSDT_MADTEntry)); ++cc){
				XSDT_MADTEntry *Entry = (void *)MADT->Table + Offset;
				if(MADT->Table[cc].Type == XSDT_IOAPICInterruptSourceOverride){
					XSDT_IOAPICInterruptSourceOverride_t *Override = (XSDT_IOAPICInterruptSourceOverride_t *)Entry;
					if(Override->busSource == 0x00){
						PIN = Override->GlobalSystemInterrupt;
						break;
					}
				}
			}
		}
		IoApicSetRedirection(acpibase, IOAPIC, PIN, Vector, LocalAPIC);
		if(!ISRSetCallback(acpibase, Vector, Priviledge, _IST, NULL, sselector, false, (ISRCallback *)&_PITTickISR)){return;}
	}
	uint32_t divisor = PITTick; // 1 ms per tick frequency (1000 Hz)
	
	// Command byte: Channel 0, Access mode: lobyte/hibyte, Mode 2 (rate generator), Binary
	__outb(0x43, 0x34);
	
	// Send divisor low byte, then high byte
	__outb(0x40, (uint8_t)(divisor & 0xFF));
	__outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}


LibAPI void SleepMS(uint32_t milliseconds){
    uint32_t start_ticks = pit_ticks[0], 
    target_ticks = start_ticks + milliseconds;
    
	// Handle potential overflow gracefully if your system runs for a very long time, 
	// or use a simple active/hault wait loop:
	if(target_ticks >= start_ticks){
		//  Wait for next interrupt (saves CPU cycles)
		while(pit_ticks < target_ticks){__asm__ volatile("hlt");}
		return;
	}
	// Handle tick wrap-around edge case
	while(pit_ticks >= start_ticks || pit_ticks < target_ticks){__asm__ volatile("hlt");}
}
LibAPI inline void SleepS(uint32_t seconds){SleepMS(seconds * 1000);}


#include "RTC.h"
#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

uint8_t CMOSRead(uint8_t reg){
	__outb(CMOS_ADDR, (1 << 7) | reg);
	return __inb(CMOS_DATA);
}

static int CMOSUpdateInProgress(void){return (CMOSRead(0x0A) & 0x80);}

LibAPI uint32_t CMOSGetElapsedDays(uint8_t Year, uint8_t Month, uint8_t Days){
	uint32_t out = (Year * 365) + (Year / 4);
	if(Month > 0){out += 31;}
	if(Month > 1){out += (Year % 4 == 0? 29: 28);}
	if(Month > 2){out += 31;}
	if(Month > 3){out += 30;}
	if(Month > 4){out += 31;}
	if(Month > 5){out += 30;}
	if(Month > 6){out += 31;}
	if(Month > 7){out += 31;}
	if(Month > 8){out += 30;}
	if(Month > 9){out += 31;}
	if(Month > 10){out += 30;}
	if(Month > 11){out += 31;}
	return out + Days;
}

LibAPI void CMOSGetTime(CMOS_time_t *time){
	//  Wait until RTC finished updating internal registers
	while (CMOSUpdateInProgress());

	//  Read raw values from RTC registers
	time->second = CMOSRead(0x00);
	time->minute = CMOSRead(0x02);
	time->hour   = CMOSRead(0x04);
	time->day    = CMOSRead(0x07);
	time->month  = CMOSRead(0x08);
	time->year   = CMOSRead(0x09);

	uint8_t register_b = CMOSRead(0x0B);

	//  Convert BCD to binary if Bit 2 of Register B is cleared (0)
	if(!(register_b & 0x04)){
		time->second = (time->second & 0x0F) + ((time->second / 16) * 10);
		time->minute = (time->minute & 0x0F) + ((time->minute / 16) * 10);
		time->hour   = ((time->hour & 0x0F) + (((time->hour & 0x70) / 16) * 10)) | (time->hour & 0x80);
		time->day    = (time->day & 0x0F) + ((time->day / 16) * 10);
		time->month  = (time->month & 0x0F) + ((time->month / 16) * 10);
		time->year   = (time->year & 0x0F) + ((time->year / 16) * 10);
	}

	//  Convert 12-hour format to 24-hour format if needed (Bit 1 of Register B clear = 12-hour)
	if(!(register_b & 0x02) && (time->hour & 0x80)){time->hour = ((time->hour & 0x7F) + 12) % 24;}

	//  Calculate 4-digit year (Assumes 21st century)
	time->year += 2000;
}