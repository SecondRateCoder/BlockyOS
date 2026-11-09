#include "SSE.h"
#include "kernel/libcrt/SimpleOutput/SimpleOutput.h"

#define NoParent16 (0xFFFFu)

static __align(16) __linkersection(.bss) SSEStore Stores[NSSEStores];
static uint32_t CurrentStore = SSENoStore;

// ---------------------------------------------------------------------------
// Validation: the register must belong to the *current* store (the hardware only
// holds one context at a time), the store must be live, and the bit must be allocated.
// ---------------------------------------------------------------------------
static bool ValidRegister(xmmRegister reg){
	if(reg.Register >= NXmmRegisters || reg.store >= NSSEStores){return false;}
	if(reg.store != CurrentStore || !Stores[reg.store].Enabled){return false;}
	return (Stores[reg.store].registerBitmap[reg.Register / 8] >> (reg.Register % 8)) & 1u;
}

// 16 entries: prefix0 ... prefix15 (prefix must already be a single pasted token)
#define SSE_TABLE16(prefix, type, n, ...)	\
	switch(n){	\
		case 0:		return ((type)prefix##0)(__VA_ARGS__);	\
		case 1:		return ((type)prefix##1)(__VA_ARGS__);	\
		case 2:		return ((type)prefix##2)(__VA_ARGS__);	\
		case 3:		return ((type)prefix##3)(__VA_ARGS__);	\
		case 4:		return ((type)prefix##4)(__VA_ARGS__);	\
		case 5:		return ((type)prefix##5)(__VA_ARGS__);	\
		case 6:		return ((type)prefix##6)(__VA_ARGS__);	\
		case 7:		return ((type)prefix##7)(__VA_ARGS__);	\
		case 8:		return ((type)prefix##8)(__VA_ARGS__);	\
		case 9:		return ((type)prefix##9)(__VA_ARGS__);	\
		case 10:	return ((type)prefix##10)(__VA_ARGS__);	\
		case 11:	return ((type)prefix##11)(__VA_ARGS__);	\
		case 12:	return ((type)prefix##12)(__VA_ARGS__);	\
		case 13:	return ((type)prefix##13)(__VA_ARGS__);	\
		case 14:	return ((type)prefix##14)(__VA_ARGS__);	\
		case 15:	return ((type)prefix##15)(__VA_ARGS__);	\
		default:	return false;}

#define SSE_IMPL_SUITE(TypePlural, Op, Direction, RegLetter)								\
bool Op##TypePlural(xmmRegister reg, void *ptr){											\
	if(!ValidRegister(reg)){return false;}	/*__bochs();*/									\
	SSE_TABLE16(Mov##TypePlural##Direction##RegLetter##mm, SIMDFuncPtr, reg.Register, ptr);	\
}

#define SSE_IMPL_OP(Op, TypeSingular, RegLetter)								\
bool Op##TypeSingular##RegLetter(xmmRegister reg, void *ptr){					\
	if(!ValidRegister(reg)){return false;}	/*__bochs();*/						\
	SSE_TABLE16(Op##TypeSingular##RegLetter, SIMDFuncPtr, reg.Register, ptr);	\
}

#define SSE_IMPL_VOID(Op, TypeSingular, RegLetter)							\
bool Op##TypeSingular##RegLetter(xmmRegister reg){							\
	if(!ValidRegister(reg)){return false;}	/*__bochs();*/					\
	SSE_TABLE16(Op##TypeSingular##RegLetter, SIMDVoidFuncPtr, reg.Register);\
}

SSE_IMPL_SUITE(AlignedFloats,   Store, To,   X)
SSE_IMPL_SUITE(AlignedFloats,   Load,  From, X)
SSE_IMPL_SUITE(UAlignedFloats,  Store, To,   X)
SSE_IMPL_SUITE(UAlignedFloats,  Load,  From, X)
SSE_IMPL_SUITE(AlignedIntegers, Store, To,   X)
SSE_IMPL_SUITE(AlignedIntegers, Load,  From, X)
SSE_IMPL_SUITE(UAlignedIntegers,Store, To,   X)
SSE_IMPL_SUITE(UAlignedIntegers,Load,  From, X)
SSE_IMPL_SUITE(AlignedDoubles,  Store, To,   X)
SSE_IMPL_SUITE(AlignedDoubles,  Load,  From, X)
SSE_IMPL_SUITE(UAlignedDoubles, Store, To,   X)
SSE_IMPL_SUITE(UAlignedDoubles, Load,  From, X)
SSE_IMPL_SUITE(AlignedVectors,  Store, To,   Y)
SSE_IMPL_SUITE(AlignedVectors,  Load,  From, Y)
SSE_IMPL_SUITE(UAlignedVectors, Store, To,   Y)
SSE_IMPL_SUITE(UAlignedVectors, Load,  From, Y)

SSE_IMPL_OP(Add, Float, Xmm)   SSE_IMPL_OP(Sub, Float, Xmm)
SSE_IMPL_OP(Mul, Float, Xmm)   SSE_IMPL_OP(Div, Float, Xmm)
SSE_IMPL_OP(Sqrt, Float, Xmm)  SSE_IMPL_OP(And, Float, Xmm)
SSE_IMPL_OP(Or, Float, Xmm)    SSE_IMPL_OP(Xor, Float, Xmm)
SSE_IMPL_VOID(Clear, Float, Xmm)
SSE_IMPL_OP(EqualTo, Float, Xmm)      SSE_IMPL_OP(LessThan, Float, Xmm)
SSE_IMPL_OP(NotEqualTo, Float, Xmm)   SSE_IMPL_OP(GreaterThan, Float, Xmm)
SSE_IMPL_VOID(BroadcastFloatLane0, , Xmm)
SSE_IMPL_OP(InterleaveLow, Float, Xmm)
SSE_IMPL_OP(ConvertFloatToInt32Trunc, , Xmm)

SSE_IMPL_OP(Add, Integer, Xmm) SSE_IMPL_OP(Sub, Integer, Xmm)
SSE_IMPL_OP(Mul, Integer, Xmm) SSE_IMPL_OP(And, Integer, Xmm)
SSE_IMPL_OP(Or, Integer, Xmm)  SSE_IMPL_OP(Xor, Integer, Xmm)
SSE_IMPL_VOID(Clear, Integer, Xmm)

SSE_IMPL_OP(Add, Double, Xmm)  SSE_IMPL_OP(Sub, Double, Xmm)
SSE_IMPL_OP(Mul, Double, Xmm)  SSE_IMPL_OP(Div, Double, Xmm)
SSE_IMPL_OP(Sqrt, Double, Xmm)

SSE_IMPL_OP(Add, Vector, Ymm)  SSE_IMPL_OP(Sub, Vector, Ymm)
SSE_IMPL_OP(Mul, Vector, Ymm)

// ---------------------------------------------------------------------------
// Store management
//
// A store = one saved SIMD context + the allocation bitmap for it.
//   pushStore   : snapshot live state into a fresh store, make it current
//   popStore    : restore that snapshot, release the store, fall back to its parent
//   switchCurrentStore : context-switch between live stores
// Registers are allocated from the CURRENT store and stay allocated until freed.
// ---------------------------------------------------------------------------
bool pushStore(uint32_t *N){
	if(!N || !SIMDSupported()){return false;}
	EnableSIMD();
	for(uint32_t cc = 0; cc < NSSEStores; ++cc){
		if(Stores[cc].Enabled){continue;}
		const char *err = StoreSSEState(Stores[cc].store, sizeof(Stores[cc].store));
		// __bochs();
		if(err){
			DebugPrintf("\nSSE save error: %s", err);
			return false;		// store is NOT marked Enabled, so nothing leaks
		}
		for(size_t i = 0; i < registerBitmapLength; ++i){Stores[cc].registerBitmap[i] = 0U;}
		Stores[cc].Parent  = (CurrentStore < NSSEStores) ? (uint16_t)CurrentStore : NoParent16;
		Stores[cc].Enabled = true;
		CurrentStore = cc;
		*N = cc;
		DebugPrintf("\nPushed Store %z", (uint64_t)cc);
		return true;
	}
	return false;
}

bool popStore(uint32_t N){
	if(!SIMDSupported()){return false;}
	if(N >= NSSEStores || !Stores[N].Enabled){return false;}
	EnableSIMD();
	const char *err = LoadSSEState(Stores[N].store, sizeof(Stores[N].store));
	// __bochs();
	if(err){
		DebugPrintf("\nSSE restore error: %s", err);
		return false;			// keep the store so the caller can retry
	}
	Stores[N].Enabled = false;
	if(CurrentStore == N){
		uint16_t p = Stores[N].Parent;
		CurrentStore = ((p < NSSEStores) && Stores[p].Enabled) ? p : SSENoStore;
	}
	DebugPrintf("\nPopped Store %z", (uint64_t)N);
	return true;
}

// Previous receives the store that was current (SSENoStore if none).
bool switchCurrentStore(uint32_t N, uint32_t *Previous){
	if(!SIMDSupported()){return false;}
	if(N >= NSSEStores || !Stores[N].Enabled){return false;}
	EnableSIMD();
	if(N == CurrentStore){
		if(Previous){*Previous = N;}
		return true;
	}
	uint32_t old = CurrentStore;
	if(old < NSSEStores && Stores[old].Enabled){
		if(StoreSSEState(Stores[old].store, sizeof(Stores[old].store))){return false;}
	}
	if(LoadSSEState(Stores[N].store, sizeof(Stores[N].store))){return false;}
	CurrentStore = N;
	if(Previous){*Previous = old;}
	return true;
}

// ---------------------------------------------------------------------------
// Register allocation (from the current store)
// ---------------------------------------------------------------------------
bool AllocateSSERegister(xmmRegister *out){
	if(!out || !SIMDSupported()){return false;}
	EnableSIMD();
	if(!(CurrentStore < NSSEStores && Stores[CurrentStore].Enabled)){
		uint32_t base;
		if(!pushStore(&base)){return false;}	// first use: create the base scope
	}
	SSEStore *s = &Stores[CurrentStore];
	for(uint32_t reg = 0; reg < NXmmRegisters; ++reg){
		uint8_t mask = (uint8_t)(1u << (reg % 8));
		if(!(s->registerBitmap[reg / 8] & mask)){
			s->registerBitmap[reg / 8] |= mask;
			*out = (xmmRegister){ .store = (uint16_t)CurrentStore, .Register = (uint16_t)reg };
			return true;
		}
	}
	return false;	// all 16 registers in use
}

bool FreeSSERegister(xmmRegister In){
	if(!SIMDSupported()){return false;}
	if(In.store >= NSSEStores || In.Register >= NXmmRegisters || !Stores[In.store].Enabled){return false;}
	uint8_t mask = (uint8_t)(1u << (In.Register % 8));
	uint8_t *byte = &Stores[In.store].registerBitmap[In.Register / 8];
	if(!(*byte & mask)){return false;}		// not allocated (double free)
	*byte &= (uint8_t)~mask;
	return true;
}