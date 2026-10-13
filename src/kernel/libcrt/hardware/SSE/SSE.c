#include "SSE.h"

static __align(16) SSEStore Stores[NSSEStores];

#define RegisterSuiteFunction(TypePlural, Op, Direction, RegLetter)									\
bool Op##TypePlural(xmmRegister reg, void *ptr){													\
	static const SIMDFuncPtr dispatchTable[16] = {													\
		Mov##TypePlural##Direction##RegLetter##mm0,  Mov##TypePlural##Direction##RegLetter##mm1,	\
		Mov##TypePlural##Direction##RegLetter##mm2,  Mov##TypePlural##Direction##RegLetter##mm3,	\
		Mov##TypePlural##Direction##RegLetter##mm4,  Mov##TypePlural##Direction##RegLetter##mm5,	\
		Mov##TypePlural##Direction##RegLetter##mm6,  Mov##TypePlural##Direction##RegLetter##mm7,	\
		Mov##TypePlural##Direction##RegLetter##mm8,  Mov##TypePlural##Direction##RegLetter##mm9,	\
		Mov##TypePlural##Direction##RegLetter##mm10, Mov##TypePlural##Direction##RegLetter##mm11,	\
		Mov##TypePlural##Direction##RegLetter##mm12, Mov##TypePlural##Direction##RegLetter##mm13,	\
		Mov##TypePlural##Direction##RegLetter##mm14, Mov##TypePlural##Direction##RegLetter##mm15	\
	};																								\
	if(reg.Register >= 15){return false;}															\
	return dispatchTable[reg.Register](ptr);														\
}

// Macro for Standard Math/Logic/Comparison Operations taking (xmmRegister, void*)
#define RegisterOpFunction(Op, TypeSingular, RegLetter)						\
bool Op##TypeSingular##RegLetter(xmmRegister reg, void *ptr){				\
	static const SIMDFuncPtr dispatchTable[16] = {							\
		Op##TypeSingular##RegLetter##0,  Op##TypeSingular##RegLetter##1,	\
		Op##TypeSingular##RegLetter##2,  Op##TypeSingular##RegLetter##3,	\
		Op##TypeSingular##RegLetter##4,  Op##TypeSingular##RegLetter##5,	\
		Op##TypeSingular##RegLetter##6,  Op##TypeSingular##RegLetter##7,	\
		Op##TypeSingular##RegLetter##8,  Op##TypeSingular##RegLetter##9,	\
		Op##TypeSingular##RegLetter##10, Op##TypeSingular##RegLetter##11,	\
		Op##TypeSingular##RegLetter##12, Op##TypeSingular##RegLetter##13,	\
		Op##TypeSingular##RegLetter##14, Op##TypeSingular##RegLetter##15	\
	};																		\
	if(reg.Register >= 16){return false;}									\
	return dispatchTable[reg.Register](ptr);								\
}

// Macro for Operations without pointer arguments
#define RegisterVoidOpFunction(Op, TypeSingular, RegLetter)					\
bool Op##TypeSingular##RegLetter(xmmRegister reg){							\
	typedef bool __sysvabi(*SIMDVoidFuncPtr)(void);							\
	static const SIMDVoidFuncPtr dispatchTable[16] = {						\
		Op##TypeSingular##RegLetter##0,  Op##TypeSingular##RegLetter##1,	\
		Op##TypeSingular##RegLetter##2,  Op##TypeSingular##RegLetter##3,	\
		Op##TypeSingular##RegLetter##4,  Op##TypeSingular##RegLetter##5,	\
		Op##TypeSingular##RegLetter##6,  Op##TypeSingular##RegLetter##7,	\
		Op##TypeSingular##RegLetter##8,  Op##TypeSingular##RegLetter##9,	\
		Op##TypeSingular##RegLetter##10, Op##TypeSingular##RegLetter##11,	\
		Op##TypeSingular##RegLetter##12, Op##TypeSingular##RegLetter##13,	\
		Op##TypeSingular##RegLetter##14, Op##TypeSingular##RegLetter##15	\
	};																		\
	if(reg.Register >= 16){return false;}									\
	return dispatchTable[reg.Register]();									\
}

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

// Single-Precision Float Math / Operations (XMM)
RegisterOpFunction(Add, Float, Xmm)
RegisterOpFunction(Sub, Float, Xmm)
RegisterOpFunction(Mul, Float, Xmm)
RegisterOpFunction(Div, Float, Xmm)
RegisterOpFunction(Sqrt, Float, Xmm)
RegisterOpFunction(And, Float, Xmm)
RegisterOpFunction(Or, Float, Xmm)
RegisterOpFunction(Xor, Float, Xmm)
RegisterVoidOpFunction(Clear, Float, Xmm)
RegisterOpFunction(EqualTo, Float, Xmm)
RegisterOpFunction(LessThan, Float, Xmm)
RegisterOpFunction(NotEqualTo, Float, Xmm)
RegisterOpFunction(GreaterThan, Float, Xmm)
RegisterVoidOpFunction(BroadcastFloatLane0, , Xmm)
RegisterOpFunction(InterleaveLow, Float, Xmm)
RegisterOpFunction(ConvertFloatToInt32Trunc, , Xmm)

// Packed Integer Math / Logic Operations (XMM)
RegisterOpFunction(Add, Integer, Xmm)
RegisterOpFunction(Sub, Integer, Xmm)
RegisterOpFunction(Mul, Integer, Xmm)
RegisterOpFunction(And, Integer, Xmm)
RegisterOpFunction(Or, Integer, Xmm)
RegisterOpFunction(Xor, Integer, Xmm)
RegisterVoidOpFunction(Clear, Integer, Xmm)

// Double-Precision Float Math Operations (XMM)
RegisterOpFunction(Add, Double, Xmm)
RegisterOpFunction(Sub, Double, Xmm)
RegisterOpFunction(Mul, Double, Xmm)
RegisterOpFunction(Div, Double, Xmm)
RegisterOpFunction(Sqrt, Double, Xmm)

// 256-Bit Vector Math Operations (YMM)
RegisterOpFunction(Add, Vector, Ymm)
RegisterOpFunction(Sub, Vector, Ymm)
RegisterOpFunction(Mul, Vector, Ymm)


bool AllocateSSERegister(xmmRegister *out){
	enableSSE();
	if(!out){return false;}
	for(uint32_t cc = 0; cc < NSSEStores; ++cc){
		if(Stores[cc].Enabled){
			for(uint32_t byteIdx = 0; byteIdx < registerBitmapLength; ++byteIdx){
				uint8_t currentByte = Stores[cc].registerBitmap[byteIdx];
				if(currentByte == 0xFF){continue;}
				for(uint32_t bitIdx = 0; bitIdx < 8; ++bitIdx){
					if(!(currentByte & (1 << bitIdx))){
						uint16_t regID = (byteIdx * 8) + bitIdx;
						if(regID >= NXmmRegisters){return false;}
						Stores[cc].registerBitmap[byteIdx] |= (1 << bitIdx);
						*out = (xmmRegister){ .Register = regID, .store = cc };
						return true;
					}
				}
			}
		}
	}
	return false;
}

bool FreeSSERegister(xmmRegister In){
	uint32_t byteIdx = In.Register / 8;
	uint32_t bitIdx  = In.Register % 8;
	if(In.store < NSSEStores && In.Register < NXmmRegisters && byteIdx < registerBitmapLength){
		Stores[In.store].registerBitmap[byteIdx] &= ~(1 << bitIdx);
		return true;
	}
	return false;
}

bool pushStore(uint32_t *N){
	enableSSE();
	if(!N){return false;}

	for(uint32_t cc = 0; cc < NSSEStores; ++cc){
		if(!Stores[cc].Enabled){
			*N = cc;
			Stores[cc].Enabled = true;
			for(size_t i = 0; i < sizeof(Stores[cc].registerBitmap); ++i){Stores[cc].registerBitmap[i] = 0;}
			const char* err = pushSSE(Stores[cc].store, sizeof(Stores[cc].store));
			return (err == 0); 
		}
	}
	return false;
}

bool popStore(uint32_t N){
	enableSSE();
	if(N < NSSEStores && Stores[N].Enabled){
		Stores[N].Enabled = false;
		const char* err = popSSE(Stores[N].store, sizeof(Stores[N].store));
		return (err == 0);
	}
	return false;
}

bool switchCurrentStore(uint32_t N, uint32_t *Previous){
	enableSSE();
	if(N < NSSEStores && Stores[N].Enabled){
		if(Previous){pushStore(Previous);}
		return popStore(N);
	}
	return false;
}