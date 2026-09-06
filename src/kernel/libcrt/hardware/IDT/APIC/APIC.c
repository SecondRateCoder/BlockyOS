#include "APIC.h"

bool TestAPIC(){
	uint32_t eax, ebx, ecx, edx;
	__cpuid(1, eax, ebx, ecx, edx);
	return edx & CPUID_FEAT_EDX_APIC;
}