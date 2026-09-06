#pragma once

#include "kernel/libcrt/math/bool.h"
#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"

#define NXmmRegisters			(32)
#define registerBitmapLength	((NXmmRegisters) / 8)
#define NSSEStores				(16)
#define SSEStoreSize			(512)

typedef struct{uint16_t store;	uint16_t Register;}xmmRegister;
typedef struct{
	bool Enabled;
	uint8_t store[SSEStoreSize];
	//	Each bit represents a Register
	bool registerBitmap[registerBitmapLength];
}__align(16) SSEStore;

// Core Memory Operations (Supported by ALL types)
#define DeclareMemoryOps(TypePlural, Letter, N)										\
	extern bool __sysvabi MovAligned##TypePlural##To##Letter##mm##N(void *ptr);		\
	extern bool __sysvabi MovUAligned##TypePlural##To##Letter##mm##N(void *ptr);	\
	extern bool __sysvabi MovAligned##TypePlural##From##Letter##mm##N(void *ptr);	\
	extern bool __sysvabi MovUAligned##TypePlural##From##Letter##mm##N(void *ptr);

// Float Operations (Full capability - Memory, Math, Logic, Compares, Shuffles)
#define DeclareFloatOps(N)										\
	DeclareMemoryOps(Floats, X, N)								\
	extern bool __sysvabi AddFloatXmm##N(void *ptr);			\
	extern bool __sysvabi SubFloatXmm##N(void *ptr);			\
	extern bool __sysvabi MulFloatXmm##N(void *ptr);			\
	extern bool __sysvabi DivFloatXmm##N(void *ptr);			\
	extern bool __sysvabi SqrtFloatXmm##N(void *ptr);			\
	extern bool __sysvabi AndFloatXmm##N(void *ptr);			\
	extern bool __sysvabi OrFloatXmm##N(void *ptr);				\
	extern bool __sysvabi XorFloatXmm##N(void *ptr);			\
	extern bool __sysvabi ClearFloatXmm##N(void);				\
	extern bool __sysvabi EqualToFloatXmm##N(void *ptr);		\
	extern bool __sysvabi LessThanFloatXmm##N(void *ptr);		\
	extern bool __sysvabi NotEqualToFloatXmm##N(void *ptr);		\
	extern bool __sysvabi GreaterThanFloatXmm##N(void *ptr);	\
	extern bool __sysvabi BroadcastFloatLane0Xmm##N(void);		\
	extern bool __sysvabi InterleaveLowFloatXmm##N(void *ptr);	\
	extern bool __sysvabi ConvertFloatToInt32TruncXmm##N(void *ptr);

// Integer Operations (Memory, Math, Logic - NO Div, Sqrt, or Compares yet)
#define DeclareIntegerOps(N)							\
	DeclareMemoryOps(Integers, X, N)					\
	extern bool __sysvabi AddIntegerXmm##N(void *ptr);	\
	extern bool __sysvabi SubIntegerXmm##N(void *ptr);	\
	extern bool __sysvabi MulIntegerXmm##N(void *ptr);	\
	extern bool __sysvabi AndIntegerXmm##N(void *ptr);	\
	extern bool __sysvabi OrIntegerXmm##N(void *ptr);	\
	extern bool __sysvabi XorIntegerXmm##N(void *ptr);	\
	extern bool __sysvabi ClearIntegerXmm##N(void);

// Double Operations (Memory, Math - NO Logic or Compares yet)
#define DeclareDoubleOps(N)								\
	DeclareMemoryOps(Doubles, X, N)						\
	extern bool __sysvabi AddDoubleXmm##N(void *ptr);	\
	extern bool __sysvabi SubDoubleXmm##N(void *ptr);	\
	extern bool __sysvabi MulDoubleXmm##N(void *ptr);	\
	extern bool __sysvabi DivDoubleXmm##N(void *ptr);	\
	extern bool __sysvabi SqrtDoubleXmm##N(void *ptr);

// AVX Vector Operations (YMM - Memory, Math)
#define DeclareVectorOps(N)								\
	DeclareMemoryOps(Vectors, Y, N)						\
	extern bool __sysvabi AddVectorYmm##N(void *ptr);	\
	extern bool __sysvabi SubVectorYmm##N(void *ptr);	\
	extern bool __sysvabi MulVectorYmm##N(void *ptr);

// Generate all extern prototypes for Registers 0 through 31 safely
#define DECLARE_REGISTER_BLOCK(N) \
	DeclareFloatOps(N) DeclareIntegerOps(N) DeclareDoubleOps(N) DeclareVectorOps(N)

DECLARE_REGISTER_BLOCK(0)  DECLARE_REGISTER_BLOCK(1)  DECLARE_REGISTER_BLOCK(2)  DECLARE_REGISTER_BLOCK(3)
DECLARE_REGISTER_BLOCK(4)  DECLARE_REGISTER_BLOCK(5)  DECLARE_REGISTER_BLOCK(6)  DECLARE_REGISTER_BLOCK(7)
DECLARE_REGISTER_BLOCK(8)  DECLARE_REGISTER_BLOCK(9)  DECLARE_REGISTER_BLOCK(10) DECLARE_REGISTER_BLOCK(11)
DECLARE_REGISTER_BLOCK(12) DECLARE_REGISTER_BLOCK(13) DECLARE_REGISTER_BLOCK(14) DECLARE_REGISTER_BLOCK(15)
DECLARE_REGISTER_BLOCK(16) DECLARE_REGISTER_BLOCK(17) DECLARE_REGISTER_BLOCK(18) DECLARE_REGISTER_BLOCK(19)
DECLARE_REGISTER_BLOCK(20) DECLARE_REGISTER_BLOCK(21) DECLARE_REGISTER_BLOCK(22) DECLARE_REGISTER_BLOCK(23)
DECLARE_REGISTER_BLOCK(24) DECLARE_REGISTER_BLOCK(25) DECLARE_REGISTER_BLOCK(26) DECLARE_REGISTER_BLOCK(27)
DECLARE_REGISTER_BLOCK(28) DECLARE_REGISTER_BLOCK(29) DECLARE_REGISTER_BLOCK(30) DECLARE_REGISTER_BLOCK(31)

typedef bool __sysvabi(*SIMDFuncPtr)(void *ptr);

#define RegisterSuiteFunction(TypePlural, Op, Direction, RegLetter)		bool Op##TypePlural(xmmRegister reg, void *ptr);

// Instantiate Move Suites
RegisterSuiteFunction(AlignedFloats,   Store, To,   X)
RegisterSuiteFunction(AlignedFloats,   Load,  From, X)
RegisterSuiteFunction(UAlignedFloats,  Store, To,   X)
RegisterSuiteFunction(UAlignedFloats,  Load,  From, X)

RegisterSuiteFunction(AlignedIntegers, Store, To,   X)
RegisterSuiteFunction(AlignedIntegers, Load,  From, X)
RegisterSuiteFunction(UAlignedIntegers,Store, To,   X)
RegisterSuiteFunction(UAlignedIntegers,Load,  From, X)

RegisterSuiteFunction(AlignedDoubles,  Store, To,   X)
RegisterSuiteFunction(AlignedDoubles,  Load,  From, X)
RegisterSuiteFunction(UAlignedDoubles, Store, To,   X)
RegisterSuiteFunction(UAlignedDoubles, Load,  From, X)

RegisterSuiteFunction(AlignedVectors,  Store, To,   Y)
RegisterSuiteFunction(AlignedVectors,  Load,  From, Y)
RegisterSuiteFunction(UAlignedVectors, Store, To,   Y)
RegisterSuiteFunction(UAlignedVectors, Load,  From, Y)

LibAPI extern bool __sysvabi testSSE(void);
LibAPI extern uint64_t __sysvabi testSSEExtensions(void);
LibAPI extern void __sysvabi enableSSE(void);
LibAPI extern void __sysvabi disableSSE(void);
LibAPI extern char *__sysvabi decodeSIMDFeatureMask(char *out, uint64_t mask);
LibAPI extern char * __sysvabi pushSSE(void *ptr, size_t size);
LibAPI extern char * __sysvabi popSSE(void *ptr, size_t size);

LibAPI bool switchCurrentStore(uint32_t N, uint32_t *Previous);
LibAPI bool AllocateSSERegister(xmmRegister *out);
LibAPI bool FreeSSERegister(xmmRegister In);
LibAPI bool pushStore(uint32_t *N);
LibAPI bool popStore(uint32_t N);