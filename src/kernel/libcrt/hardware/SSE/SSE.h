#pragma once

#include "kernel/libcrt/math/bool.h"
#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"

#define NXmmRegisters			(16)
#define registerBitmapLength	((NXmmRegisters) / 8)
#define NSSEStores				(64)
#define SSEStoreSize			(512)	// FXSAVE area. YMM upper halves need XSAVE (see notes).
#define SSENoStore				(0xFFFFFFFFu)

typedef struct{uint16_t store;	uint16_t Register;}xmmRegister;
typedef struct{
	uint16_t Enabled;
	uint16_t Parent;		// store that was current when this one was pushed (0xFFFF = none)
	uint8_t __align(16) store[SSEStoreSize];
	//	Each bit represents a Register
	uint8_t registerBitmap[registerBitmapLength];
}__align(16) SSEStore;

// ---- Per-register assembly routines (UTILS.asm) ----------------------------
#define DeclareMemoryOps(TypePlural, Letter, N)										\
	extern bool __sysvabi MovAligned##TypePlural##To##Letter##mm##N(void *ptr);		\
	extern bool __sysvabi MovUAligned##TypePlural##To##Letter##mm##N(void *ptr);	\
	extern bool __sysvabi MovAligned##TypePlural##From##Letter##mm##N(void *ptr);	\
	extern bool __sysvabi MovUAligned##TypePlural##From##Letter##mm##N(void *ptr);

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

#define DeclareIntegerOps(N)							\
	DeclareMemoryOps(Integers, X, N)					\
	extern bool __sysvabi AddIntegerXmm##N(void *ptr);	\
	extern bool __sysvabi SubIntegerXmm##N(void *ptr);	\
	extern bool __sysvabi MulIntegerXmm##N(void *ptr);	\
	extern bool __sysvabi AndIntegerXmm##N(void *ptr);	\
	extern bool __sysvabi OrIntegerXmm##N(void *ptr);	\
	extern bool __sysvabi XorIntegerXmm##N(void *ptr);	\
	extern bool __sysvabi ClearIntegerXmm##N(void);

#define DeclareDoubleOps(N)								\
	DeclareMemoryOps(Doubles, X, N)						\
	extern bool __sysvabi AddDoubleXmm##N(void *ptr);	\
	extern bool __sysvabi SubDoubleXmm##N(void *ptr);	\
	extern bool __sysvabi MulDoubleXmm##N(void *ptr);	\
	extern bool __sysvabi DivDoubleXmm##N(void *ptr);	\
	extern bool __sysvabi SqrtDoubleXmm##N(void *ptr);

#define DeclareVectorOps(N)								\
	DeclareMemoryOps(Vectors, Y, N)						\
	extern bool __sysvabi AddVectorYmm##N(void *ptr);	\
	extern bool __sysvabi SubVectorYmm##N(void *ptr);	\
	extern bool __sysvabi MulVectorYmm##N(void *ptr);

#define DECLARE_REGISTER_BLOCK(N) \
	DeclareFloatOps(N) DeclareIntegerOps(N) DeclareDoubleOps(N) DeclareVectorOps(N)

DECLARE_REGISTER_BLOCK(0)  DECLARE_REGISTER_BLOCK(1)  DECLARE_REGISTER_BLOCK(2)  DECLARE_REGISTER_BLOCK(3)
DECLARE_REGISTER_BLOCK(4)  DECLARE_REGISTER_BLOCK(5)  DECLARE_REGISTER_BLOCK(6)  DECLARE_REGISTER_BLOCK(7)
DECLARE_REGISTER_BLOCK(8)  DECLARE_REGISTER_BLOCK(9)  DECLARE_REGISTER_BLOCK(10) DECLARE_REGISTER_BLOCK(11)
DECLARE_REGISTER_BLOCK(12) DECLARE_REGISTER_BLOCK(13) DECLARE_REGISTER_BLOCK(14) DECLARE_REGISTER_BLOCK(15)

typedef bool __sysvabi(*SIMDFuncPtr)(void *ptr);
typedef bool __sysvabi(*SIMDVoidFuncPtr)(void);

// ---- Public register-indexed wrappers (SSE.c) -------------------------------
// These take the xmmRegister returned by AllocateSSERegister. The previous header
// declared them WITHOUT the xmmRegister parameter, which did not match the definitions,
// and re-used the macro names that SSE.c #defines (macro redefinition).
#define SSE_PROTO_SUITE(TypePlural, Op)				bool Op##TypePlural(xmmRegister reg, void *ptr);
#define SSE_PROTO_OP(Op, TypeSingular, RegLetter)	bool Op##TypeSingular##RegLetter(xmmRegister reg, void *ptr);
#define SSE_PROTO_VOID(Op, TypeSingular, RegLetter)	bool Op##TypeSingular##RegLetter(xmmRegister reg);

// "Store*" = write memory INTO the register, "Load*" = read the register OUT to memory.
SSE_PROTO_SUITE(AlignedFloats,   Store)  SSE_PROTO_SUITE(AlignedFloats,   Load)
SSE_PROTO_SUITE(UAlignedFloats,  Store)  SSE_PROTO_SUITE(UAlignedFloats,  Load)
SSE_PROTO_SUITE(AlignedIntegers, Store)  SSE_PROTO_SUITE(AlignedIntegers, Load)
SSE_PROTO_SUITE(UAlignedIntegers,Store)  SSE_PROTO_SUITE(UAlignedIntegers,Load)
SSE_PROTO_SUITE(AlignedDoubles,  Store)  SSE_PROTO_SUITE(AlignedDoubles,  Load)
SSE_PROTO_SUITE(UAlignedDoubles, Store)  SSE_PROTO_SUITE(UAlignedDoubles, Load)
SSE_PROTO_SUITE(AlignedVectors,  Store)  SSE_PROTO_SUITE(AlignedVectors,  Load)
SSE_PROTO_SUITE(UAlignedVectors, Store)  SSE_PROTO_SUITE(UAlignedVectors, Load)

SSE_PROTO_OP(Add, Float, Xmm)   SSE_PROTO_OP(Sub, Float, Xmm)
SSE_PROTO_OP(Mul, Float, Xmm)   SSE_PROTO_OP(Div, Float, Xmm)
SSE_PROTO_OP(Sqrt, Float, Xmm)  SSE_PROTO_OP(And, Float, Xmm)
SSE_PROTO_OP(Or, Float, Xmm)    SSE_PROTO_OP(Xor, Float, Xmm)
SSE_PROTO_VOID(Clear, Float, Xmm)
SSE_PROTO_OP(EqualTo, Float, Xmm)      SSE_PROTO_OP(LessThan, Float, Xmm)
SSE_PROTO_OP(NotEqualTo, Float, Xmm)   SSE_PROTO_OP(GreaterThan, Float, Xmm)
SSE_PROTO_VOID(BroadcastFloatLane0, , Xmm)
SSE_PROTO_OP(InterleaveLow, Float, Xmm)
SSE_PROTO_OP(ConvertFloatToInt32Trunc, , Xmm)

SSE_PROTO_OP(Add, Integer, Xmm) SSE_PROTO_OP(Sub, Integer, Xmm)
SSE_PROTO_OP(Mul, Integer, Xmm) SSE_PROTO_OP(And, Integer, Xmm)
SSE_PROTO_OP(Or, Integer, Xmm)  SSE_PROTO_OP(Xor, Integer, Xmm)
SSE_PROTO_VOID(Clear, Integer, Xmm)

SSE_PROTO_OP(Add, Double, Xmm)  SSE_PROTO_OP(Sub, Double, Xmm)
SSE_PROTO_OP(Mul, Double, Xmm)  SSE_PROTO_OP(Div, Double, Xmm)
SSE_PROTO_OP(Sqrt, Double, Xmm)

SSE_PROTO_OP(Add, Vector, Ymm)  SSE_PROTO_OP(Sub, Vector, Ymm)
SSE_PROTO_OP(Mul, Vector, Ymm)

LibAPI extern bool __sysvabi SIMDSupported(void);
LibAPI extern uint64_t __sysvabi CheckSIMDExtensions(void);
LibAPI extern bool __sysvabi EnableSIMD(void);
LibAPI extern void __sysvabi DisableSIMD(void);
LibAPI extern char *__sysvabi DecodeSIMDFeatureMask(char *out, uint64_t mask);
LibAPI extern char * __sysvabi StoreSSEState(void *ptr, size_t size);
LibAPI extern char * __sysvabi LoadSSEState(void *ptr, size_t size);

// Store (= SIMD scope) management
LibAPI bool pushStore(uint32_t *N);							// save live state into a new store and make it current
LibAPI bool popStore(uint32_t N);							// restore N's saved state and release N
LibAPI bool switchCurrentStore(uint32_t N, uint32_t *Previous);	// save live -> old current, load N, N becomes current
LibAPI bool AllocateSSERegister(xmmRegister *out);			// allocates from the current store
LibAPI bool FreeSSERegister(xmmRegister In);