#pragma once

#define DEF_H

#define attribute(...)			__attribute__((__VA_ARGS__))
#define attributeN(...)			attribute(__VA_ARGS__)

//*	Calling Conventions
#if defined(__cdecl)
	#undef __cdecl
#endif
#define __cdecl					attribute(cdecl)
#define __msabi					attribute(ms_abi)
#define __sysvabi				attribute(sysv_abi)
#if defined(__stdcall)
	#undef __stdcall
#endif
#define __stdcall				attribute(stdcall)
#if defined(__fastcall)
	#undef __fastcall
#endif
#define __fastcall				attribute(fastcall)
#define __longcall				attribute(longcall)
#define __shortcall				attribute(shortcall)
#define __ARMlongcall			attribute(long_call)
#define __ARMshortcall			attribute(short_call)
#define __function_vector		attribute(function_vector)
#define __interrupt				attribute(interrupt)
#if	defined(__aarch64__) || defined(_M_ARM64) || defined(__arm64__) || defined(__arm__) || defined(_M_ARM) || defined(__thumb__)
	#define __ARMinterrupt(S)	attribute(interrupt(#S))
#else
	#define __ARMinterrupt(S)	__interrupt
#endif
#define __ARMinterruptIRQ		__ARMinterrupt(IRQ)
#define __ARMinterruptFIQ		__ARMinterrupt(FIQ)
#define __ARMinterruptSWI		__ARMinterrupt(SWI)
#define __ARMinterruptABORT		__ARMinterrupt(ABORT)
#define __ARMinterruptUNDEF		__ARMinterrupt(UNDEF)
#define __interrupt_handler		attribute(interrupt_handler)
#define __sp_switch(stack)		attribute(sp_switch(#stack))
#define __trap_exit(trap)		attribute(trap_exit(trap))

//*	Memory model / placement
#define __eightbit_data			attribute(eightbit_data)
#define __tiny_data				attribute(tiny_data)
#define __far					attribute(far)
#define __near					attribute(near)
#define __packed				attribute(packed)
#define __align(N)				attribute(aligned(N))
#define __model(model)			attribute(model(#model))
#define __LARGEmodel			__model(large)
#define __MEDIUMmodel			__model(medium)
#define __SMALLmodel			__model(small)

//*	Code generation / function semantics
#define __saveall				attribute(saveall)
#define __signal				attribute(signal)
#define __naked					attribute(naked)
#define __noinline				attribute(noinline)
#define __inline				attribute(always_inline)
#define __no_instrument			attribute(no_instrument_function)

//*	Diagnostics/optimization hints
#define __deprecated			attribute(deprecated)
#define __noreturn				attribute(noreturn)
#define __pure					attribute(pure)
#define __nothrow				attribute(nothrow)
#define __nonnull(...)			attribute(nonnull(__VA_ARGS__))
#define __warn_result			attribute(warn_unused_result)
#define __malloc				attribute(malloc)
#define __alias(a)				attribute(alias(a))

//*	Linker/symbol control
#define __linkersection(s)		attribute(section(#s))
#define __constructor			attribute(constructor)
#define __destructor			attribute(destructor)
#define __unused				attribute(unused)
#define __used					attribute(used)
#define __weak					attribute(weak)
#define __visibilitydefault		attribute(visibility ("default"))
#define __visibilityhidden		attribute(visibility ("hidden"))
#define __visibilityprotected	attribute(visibility ("protected"))
#define __visibilityinternal	attribute(visibility ("internal"))

#define __noescape				__noreturn

#ifdef __restrict
	#undef __restrict
#endif
#define __restrict				__restrict__

#define DLLExport				__declspec(dllexport)
#define DLLImport				__declspec(dllimport)


#if defined(__DLL) || defined(__EXPORT) || defined(__IMPORT)
#define LibAPIExport			DLLExport
#define LibAPIImport			DLLImport
#else
#define LibAPIExport
#define LibAPIImport
#endif

#ifdef __EXPORT
#define LibAPI					LibAPIExport
#elif defined(__IMPORT)
#define LibAPI					LibAPIImport
#else
#define LibAPI
#endif

// Vendor strings from CPUs.
#define CPUID_VENDOR_AMD			"AuthenticAMD"
#define CPUID_VENDOR_AMD_OLD		"AMDisbetter!" // Early engineering samples of AMD K5 processor
#define CPUID_VENDOR_INTEL			"GenuineIntel"
#define CPUID_VENDOR_VIA			"VIA VIA VIA "
#define CPUID_VENDOR_TRANSMETA		"GenuineTMx86"
#define CPUID_VENDOR_TRANSMETA_OLD	"TransmetaCPU"
#define CPUID_VENDOR_CYRIX			"CyrixInstead"
#define CPUID_VENDOR_CENTAUR		"CentaurHauls"
#define CPUID_VENDOR_NEXGEN			"NexGenDriven"
#define CPUID_VENDOR_UMC			"UMC UMC UMC "
#define CPUID_VENDOR_SIS			"SiS SiS SiS "
#define CPUID_VENDOR_NSC			"Geode by NSC"
#define CPUID_VENDOR_RISE			"RiseRiseRise"
#define CPUID_VENDOR_VORTEX			"Vortex86 SoC"
#define CPUID_VENDOR_AO486			"MiSTer AO486"
#define CPUID_VENDOR_AO486_OLD		"GenuineAO486"
#define CPUID_VENDOR_ZHAOXIN		"  Shanghai  "
#define CPUID_VENDOR_HYGON			"HygonGenuine"
#define CPUID_VENDOR_ELBRUS			"E2K MACHINE "

// Vendor strings from hypervisors.
#define CPUID_VENDOR_QEMU			"TCGTCGTCGTCG"
#define CPUID_VENDOR_KVM			" KVMKVMKVM  "
#define CPUID_VENDOR_VMWARE			"VMwareVMware"
#define CPUID_VENDOR_VIRTUALBOX		"VBoxVBoxVBox"
#define CPUID_VENDOR_XEN			"XenVMMXenVMM"
#define CPUID_VENDOR_HYPERV			"Microsoft Hv"
#define CPUID_VENDOR_PARALLELS		" prl hyperv "
#define CPUID_VENDOR_PARALLELS_ALT	" lrpepyh vr " // Sometimes Parallels incorrectly encodes "prl hyperv" as "lrpepyh vr" due to an endianness mismatch.
#define CPUID_VENDOR_BHYVE			"bhyve bhyve "
#define CPUID_VENDOR_QNX			" QNXQVMBSQG "

#define Write1ToClear

#define enumdef(type, name)			typedef type name;	enum

#define NULL	((void * )0)
#define __typeof(T)					__typeof__(T)
#define __offsetof(TYPE, MEMBER)	__builtin_offsetof (TYPE, MEMBER)

#define nKB(n)						((n) * 1024)
#define nMB(n)						(nKB(n) * 1024)
#define nGB(n)						(nMB(n) * 1024)
#define nTB(n)						(nGB(n) * 1024)

#define WritePTR(base, type, offset, value)	\
	*((type *)((base) + (offset))) = (type)(value);
#define ReadPTR(base, type, offset, value)	\
	(value) = *((type *)((base) + (offset)));

#define SAFEOP(A, B, CompAOp, CompA, CompBOp, CompB, Comp, OP, Alt) (((A) CompAOp (CompA)) Comp ((B) CompBOp (CompB)) ? ((A) OP (B)) : (Alt))
#define __safediv(A, B) SAFEOP((A), (B), ||, true, !=, 0, &&, /, 1)

typedef struct{
	unsigned short	Device;
	unsigned char	Parent;
	unsigned char	Local;
}__packed GenericReference;
typedef unsigned char		GenericChecksum[2];

enumdef(unsigned long, CPUIDFeatures){
	CPUID_FEAT_EBX_BRAND_INDEX        = 0xFF << 0,
	CPUID_FEAT_EBX_CLFLUSH_LINE_SIZE  = 0xFF << 8,
	CPUID_FEAT_EBX_APIC_ID_SPACE      = 0xFF << 16,
	CPUID_FEAT_EBX_INITIAL_APIC_ID    = 0xFF << 24,
	
	CPUID_FEAT_ECX_SSE3               = 1 << 0,
	CPUID_FEAT_ECX_PCLMUL             = 1 << 1,
	CPUID_FEAT_ECX_DTES64             = 1 << 2,
	CPUID_FEAT_ECX_MONITOR            = 1 << 3,
	CPUID_FEAT_ECX_DS_CPL             = 1 << 4,
	CPUID_FEAT_ECX_VMX                = 1 << 5,
	CPUID_FEAT_ECX_SMX                = 1 << 6,
	CPUID_FEAT_ECX_EST                = 1 << 7,
	CPUID_FEAT_ECX_TM2                = 1 << 8,
	CPUID_FEAT_ECX_SSSE3              = 1 << 9,
	CPUID_FEAT_ECX_CID                = 1 << 10,
	CPUID_FEAT_ECX_SDBG               = 1 << 11,
	CPUID_FEAT_ECX_FMA                = 1 << 12,
	CPUID_FEAT_ECX_CX16               = 1 << 13,
	CPUID_FEAT_ECX_XTPR               = 1 << 14,
	CPUID_FEAT_ECX_PDCM               = 1 << 15,
	CPUID_FEAT_ECX_PCID               = 1 << 17,
	CPUID_FEAT_ECX_DCA                = 1 << 18,
	CPUID_FEAT_ECX_SSE4_1             = 1 << 19,
	CPUID_FEAT_ECX_SSE4_2             = 1 << 20,
	CPUID_FEAT_ECX_X2APIC             = 1 << 21,
	CPUID_FEAT_ECX_MOVBE              = 1 << 22,
	CPUID_FEAT_ECX_POPCNT             = 1 << 23,
	CPUID_FEAT_ECX_TSC                = 1 << 24,
	CPUID_FEAT_ECX_AES                = 1 << 25,
	CPUID_FEAT_ECX_XSAVE              = 1 << 26,
	CPUID_FEAT_ECX_OSXSAVE            = 1 << 27,
	CPUID_FEAT_ECX_AVX                = 1 << 28,
	CPUID_FEAT_ECX_F16C               = 1 << 29,
	CPUID_FEAT_ECX_RDRAND             = 1 << 30,
	CPUID_FEAT_ECX_HYPERVISOR         = 1 << 31,

	CPUID_FEAT_EDX_FPU                = 1 << 0,
	CPUID_FEAT_EDX_VME                = 1 << 1,
	CPUID_FEAT_EDX_DE                 = 1 << 2,
	CPUID_FEAT_EDX_PSE                = 1 << 3,
	CPUID_FEAT_EDX_TSC                = 1 << 4,
	CPUID_FEAT_EDX_MSR                = 1 << 5,
	CPUID_FEAT_EDX_PAE                = 1 << 6,
	CPUID_FEAT_EDX_MCE                = 1 << 7,
	CPUID_FEAT_EDX_CX8                = 1 << 8,
	CPUID_FEAT_EDX_APIC               = 1 << 9,
	CPUID_FEAT_EDX_SEP                = 1 << 11,
	CPUID_FEAT_EDX_MTRR               = 1 << 12,
	CPUID_FEAT_EDX_PGE                = 1 << 13,
	CPUID_FEAT_EDX_MCA                = 1 << 14,
	CPUID_FEAT_EDX_CMOV               = 1 << 15,
	CPUID_FEAT_EDX_PAT                = 1 << 16,
	CPUID_FEAT_EDX_PSE36              = 1 << 17,
	CPUID_FEAT_EDX_PSN                = 1 << 18,
	CPUID_FEAT_EDX_CLFLUSH            = 1 << 19,
	CPUID_FEAT_EDX_DS                 = 1 << 21,
	CPUID_FEAT_EDX_ACPI               = 1 << 22,
	CPUID_FEAT_EDX_MMX                = 1 << 23,
	CPUID_FEAT_EDX_FXSR               = 1 << 24,
	CPUID_FEAT_EDX_SSE                = 1 << 25,
	CPUID_FEAT_EDX_SSE2               = 1 << 26,
	CPUID_FEAT_EDX_SS                 = 1 << 27,
	CPUID_FEAT_EDX_HTT                = 1 << 28,
	CPUID_FEAT_EDX_TM                 = 1 << 29,
	CPUID_FEAT_EDX_IA64               = 1 << 30,
	CPUID_FEAT_EDX_PBE                = 1 << 31
};

#define __cpuid(level, a, b, c, d)				\
  __asm__ __volatile__ ("cpuid\n\t"				\
	: "=a" (a), "=b" (b), "=c" (c), "=d" (d)	\
	: "0" (level))

#define va_list			__builtin_va_list
#define va_start(v, l)	__builtin_va_start(v,l)
#define va_end(v)		__builtin_va_end(v)
#define va_arg(v,l)		__builtin_va_arg(v,l)
#define va_copy(d,s)	__builtin_va_copy(d,s)


typedef struct{
	unsigned long long	Breakpoint0Detected				: 1;
	unsigned long long	Breakpoint1Detected				: 1;
	unsigned long long	Breakpoint2Detected				: 1;
	unsigned long long	Breakpoint3Detected				: 1;
	unsigned long long									: 7;
	unsigned long long	BusLockTrapExcpetion			: 1;
	unsigned long long	SMM_ICEMode						: 1;
	unsigned long long	DebugRegisterAccessDetected		: 1;
	unsigned long long	SingleStepTrigger				: 1;
	unsigned long long	TaskSwitchTrigger				: 1;
	unsigned long long	RTMTransactionTrigger			: 1;
	unsigned long long									: 47;
}__packed DR6;

enumdef(unsigned char, DR7BreakpointCondition){InstructionExecution = 0b00, 
	DataWritesOnly = 0b01, IOReadWrites = 0b10, DataReadWrites = 0b11};
enumdef(unsigned char, DR7BreakpointLength){_1Byte = 0b00, 
	_2Bytes = 0b01, _4Bytes = 0b11, _8Bytes = 0b10};
typedef struct{
	unsigned long long	LocalBreakpoint0Enable			: 1;
	unsigned long long	GlobalBreakpoint0Enable			: 1;
	unsigned long long	LocalBreakpoint1Enable			: 1;
	unsigned long long	GlobalBreakpoint1Enable			: 1;
	unsigned long long	LocalBreakpoint2Enable			: 1;
	unsigned long long	GlobalBreakpoint2Enable			: 1;
	unsigned long long	LocalBreakpoint3Enable			: 1;
	unsigned long long	GlobalBreakpoint3Enable			: 1;
	unsigned long long	LocalExactBreakpointEnable		: 1;
	unsigned long long	GlobalEaxctBreakpointEnable		: 1;
	unsigned long long									: 1;
	//	INT 1 (#DB exception, default)
	//	Break to ICE/SMM
	unsigned long long	BreakpointAction				: 1;
	unsigned long long	DebugRegisterProtect			: 1;
	unsigned long long									: 2;
	unsigned long long	Breakpoint0Condition			: 2;
	unsigned long long	Breakpoint0Length				: 2;
	unsigned long long	Breakpoint1Condition			: 2;
	unsigned long long	Breakpoint1Length				: 2;
	unsigned long long	Breakpoint2Condition			: 2;
	unsigned long long	Breakpoint2Length				: 2;
	unsigned long long	Breakpoint3Condition			: 2;
	unsigned long long	Breakpoint3Length				: 2;
	unsigned long long	Breakpoint0ProcessorTraceLog	: 1;
	unsigned long long	Breakpoint1ProcessorTraceLog	: 1;
	unsigned long long	Breakpoint2ProcessorTraceLog	: 1;
	unsigned long long	Breakpoint3ProcessorTraceLog	: 1;
	unsigned long long									: 28;
}__packed DR7;