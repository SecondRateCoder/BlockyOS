#pragma once

#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"

#include "kernel/libcrt/hardware/ACPI/RSDP/MADT.h"
#include "kernel/libcrt/hardware/ACPI/XSDP/MADT.h"
#include "kernel/libcrt/hardware/ACPI/ACPI.h"
#include "kernel/libcrt/hardware/paging/paging.h"
#include "kernel/libcrt/hardware/hardware.h"

enumdef(uint8_t, IOAPICRegisters){
	IORegisterSelect = 0x00, IORegisterData = 0x10, 
	_IOAPICID = 0x00, _IOAPICVER = 0x01, _IOAPICArbitrationID = 0x02, 
};
#define _IOAPICREDTBL(N, SN)	(0x10 + ((N) * 2) + (SN != 0))

enumdef(uint8_t, IOAPICDeliveryMode){
	Fixed = 0x0, LowestPriority = 0x1, SystemManagedInterrupt = 0x2, 
	NonMaskableInterrupt = 0x4, ExtendedInterrupt = 0x7
};
enumdef(uint8_t, IOAPICDestinationMode){PhysicalDestination = 0x0, LogicalDestination = 0x0};
enumdef(uint8_t, IOAPICPinPolarity){ActiveHigh = 0x0, ActiveLow = 0x1};
enumdef(uint8_t, IOAPICTriggerMode){EdgeTriggerMode = 0x0, LevelTriggerMode = 0x1};

typedef union{
	uint64_t	Raw;
	struct{
		uint64_t	Vector							: 8;
		uint64_t	DeliveryMode					: 3;
		uint64_t	DestinationMode					: 1;
		uint64_t	PinPolarity						: 1;
		uint64_t	RemoteInterruptRequestRegister	: 1;
		uint64_t	InterruptDisable				: 1;
		uint64_t									: 40;
		uint64_t	DestinationAPICID				: 8;
	}Register;
}__packed IOAPICInterruptRegister;

typedef struct{
	uint32_t	Version		: 8;
	//	Max - 1
	uint32_t				: 8;
	uint32_t	MaxIRQNumber: 8;
	uint32_t				: 8;
}__packed IOAPICVersionRegister;

LibAPI uint32_t ReadIoApicRegister(void *acpibase, uint32_t N, uint32_t reg);
LibAPI void WriteIoApicRegister(void *acpibase, uint32_t N, uint32_t reg, uint32_t value);
LibAPI IOAPICInterruptRegister ReadIoApicInterruptRegister(void *acpibase, uint32_t APICN, uint32_t N);
LibAPI bool WriteIoApicInterruptRegister(void *acpibase, uint32_t APICN, uint32_t N, IOAPICInterruptRegister R);

LibAPI bool DisableIoApicInterrupt(void *acpibase, uint32_t N, uint32_t Interrupt);
LibAPI bool DisableIoApic(void *acpibase, uint32_t N);

LibAPI bool EnableIoApicInterrupt(void *acpibase, uint32_t N, uint32_t Interrupt);
LibAPI bool EnableIoApic(void *acpibase, uint32_t N);

LibAPI void IoApicSetRedirection(void *acpibase, uint32_t N, uint8_t InterruptPin, uint8_t InterruptVector, uint8_t LocalAPIC);