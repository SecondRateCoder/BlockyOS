#pragma once

#include "kernel/libcrt/hardware/paging/paging.h"
#include "kernel/libcrt/math/int.h"
#include "kernel/libcrt/def.h"

LibAPI void *ReallocatePages(void *Phys, void *Base, PageAllocationFlags Flags, uint8_t ProtectionKey, uint64_t size);

LibAPI void __noinline __visibilitydefault FreeAlignedPages(void *Page);
LibAPI void *AllocateAlignedPages(void *Phys, uint64_t size, PageAllocationFlags Flags, uint8_t ProtectionKey, uint8_t Align);
LibAPI void *ReallocateAlignedPages(void *Phys, void *ptr, PageAllocationFlags Flags, uint8_t ProtectionKey, uint64_t size, uint8_t Align);

LibAPI void *SafeAllocatePages(void *Physical, uint32_t Bytes, PageAllocationFlags Flags, uint8_t ProtectionKey, uint8_t Align);

#define DefaultFixMemFrequency			32

#ifdef _DEBUG
#define MSafePrint(FMT, RET, ...)		/*printf(L"\n" FMT, __VA_ARGS__);*/	return RET;
#define MSafePrintNF(FMT, RET)			/*printf("\n" FMT);*/				return RET;
#define MSafePrintNR(FMT, ...)			/*printf("\n" FMT, __VA_ARGS__);*/	return;
#else
#define MSafePrint(FMT, RET, ...)		return RET;
#define MSafePrintNR(FMT, ...)			return;
#endif


#define BLOCKSIZE		nKB(4)
#define dBLOCKS			nKB(32)
#define dPOOLS			32
#define allocALIGN		8
#define TestAllocRng	nKB(4)
#define TestCmdRng		nKB(3)
#define fragMul(nb)		(__roundup((nb) * 1.15, allocALIGN))

enumdef(uint32_t, memdesc_e){memdesc_e__used = 0x1, memdesc_e__nofree = 0x2};

// These need to account for the Different Device Types.
/*
* @brief Get the end of the Memdesc_t buffer.
* @remarks Do not use this Macro, it is not safe
*/
#define getdescarrtrue(mp)	(memdesc_t *)((mp)->base + ((mp)->nblocks * BLOCKSIZE))
/*
* @brief Get the Beginning of the Descriptor Array Buffer
*/
#define getdescarrn(mp)		(memdesc_t *)((void *)getdescarrtrue(mp) - ((mp)->nDescriptors * sizeof(memdesc_t)))
/*
* @brief Get the Last Entry of the Descriptor Array Buffer
*/
#define getdescarrt(mp)		((getdescarrtrue(mp)) - 1)
typedef struct memdesc_t{
	void *ptr;
	size_t nbytes;
	memdesc_e flags;
	struct memdesc_t *next;
}__attribute__((aligned(allocALIGN))) memdesc_t;
// unsigned _ = sizeof(memdesc_t);
typedef struct mempool_t{
	/// @brief A memory ptr of (.nblocks * BLOCKSIZE) aligned to 8-bytes.
	void *base;
	/// @brief The Number of BLOCKSIZE in .base
	size_t nblocks;
	/// @brief The Number of actual Descriptors from the end of Memory.
	size_t nDescriptors;
}mempool_t;
typedef struct ml_t{
	uint32_t pool;
	bool free;
	memdesc_t mdesc;
	void *ptr;
}ml_t;

enumdef(uint32_t, alloctype_t){
	// The Passed Address is marked as constantly Chainging. Allocations pass a Ptr to a Ptr.
	// Read the Stored Ptr.
	attdynamic, 
	// Non-Immutable Pointer.
	attstatic, 
	// Align to various Boundaries.
	attalign2, attalign3, attalign4, attalign5, attalign6, attalign7, attalign8
};
typedef struct allocstate_t{
	mempool_t *pools;
	uint32_t npools;
	size_t totalNAllocations;
}allocstate_t;

LibAPI volatile void *mmalloc(size_t nbytes);
LibAPI volatile void *mmalloczero(size_t nbytes);
LibAPI volatile void *mcalloc(size_t nItems, size_t itemSize);
LibAPI volatile void *mrealloc(volatile void *__attribute__((restrict)) ptr, size_t new);
LibAPI ml_t descinfo(volatile void *__attribute__((restrict)) ptr);
LibAPI volatile void *mclone_s(void *original, size_t target);
LibAPI volatile void *mclone(void *original);
LibAPI void mfree(void *__attribute__((restrict)) ptr);

// Arbitrary Functions
LibAPI bool mIsMapped(void *m);
LibAPI ml_t *mtest(uint32_t ntries, bool print);