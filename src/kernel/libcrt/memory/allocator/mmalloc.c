#include "malloc.h"
#include "kernel/libcrt/hardware/RNG/RNG.h"
#include "kernel/libcrt/memory/memory.h"
#include "kernel/libcrt/SimpleOutput/SimpleOutput.h"

static volatile allocstate_t *getastate(){
	static __linkersection(.bss) allocstate_t out = {0};
    return &out;
}

/*
* @remarks This Function modifies the Pool ptr, losing the ptr will severly damage the Integrity of the Allocator
*/
void allocatepool(uint64_t nblocks){
	volatile allocstate_t *astate = getastate();
	nblocks = __roundupt(nblocks << 1, allocALIGN);
	astate->npools++;
	if(astate->pools){
		astate->pools = ReallocatePages(NULL, astate->pools, (GlobalEnable | ReadWritable | SupervisorMode | PageLevelCacheEnable), 0x0, 
			sizeof(mempool_t) * astate->npools);
	}else{astate->pools = AllocatePages(NULL, astate->npools * sizeof(mempool_t), (GlobalEnable | ReadWritable | SupervisorMode | PageLevelCacheEnable), 0x0);}
	astate->pools[astate->npools - 1] = (mempool_t){
		.base = AllocateAlignedPages(NULL, nblocks * BLOCKSIZE, (GlobalEnable | ReadWritable | SupervisorMode | PageLevelCacheEnable), 0x0, allocALIGN),
		.nblocks = nblocks, .nDescriptors = 0
	};
	memset(astate->pools[astate->npools - 1].base, 0, nblocks * BLOCKSIZE);
}

/*
* @remarks This Function Updates the Allocator State, whilst i'd like to lock it. 
*			I cant really do that as it needs to initialise the state.
*/
static void initastate(uint64_t nblocks, uint32_t npools){
	volatile allocstate_t *astate = getastate();
	if(!(astate->pools)){
		*astate = (allocstate_t){.pools = NULL, .npools = 0, .totalNAllocations = 0};
		while(npools--){allocatepool(nblocks);}
	}
}

/*
* @remarks This Function only Read the Memory Pool, No need to lock.
*/
memdesc_t *querydescu(volatile mempool_t *__restrict mp, uint64_t nbytes){
	memdesc_t *md = getdescarrn(mp);
	//		md points to the end of the memdesc_t array, walk from the start of the array backwards.
	//		whilst we could alternatively walk forward, i will keep the standard for consistency.
	for(uint64_t cc = (mp->nDescriptors? mp->nDescriptors - 1: UINT64_MAX); cc != UINT64_MAX; --cc){
		if(!__check(md[cc].flags, memdesc_e__used) && (md[cc].nbytes == nbytes)){
			return (md + cc);
		}
	}
	return NULL;
}

/*
* @remarks This Function Modifies the Memory Pool, Lock it.
*/
void fixmdarray(mempool_t *__restrict mp, uint64_t *__restrict ndescs){
	memdesc_t *md = getdescarrn(mp);
	uint64_t min = 0;
	for(uint64_t cc = (*ndescs? *ndescs - 1: UINT64_MAX); (cc != UINT64_MAX) && (cc > min); --cc){
		if(md[cc].nbytes == 0){
			// Remove 0-byte entries
			memmove_s(md + 1, sizeof(memdesc_t) * (*ndescs), md, sizeof(memdesc_t) * ((*ndescs) - cc));
			ndescs--;
			md++;
		}else{
			// Fix Next ptrs, Simply find the Closest Entry that comes after the current one.
			uint64_t next = (uint64_t)md[cc].ptr + md[cc].nbytes, diff = UINT64_MAX;
			memdesc_t *nextdesc = NULL;
			for(uint64_t cc_ = (*ndescs? *ndescs - 1: UINT64_MAX); cc_ != UINT64_MAX; --cc_){
				if(((uint64_t)md[cc_].ptr == (uint64_t)md[cc].ptr) || ((uint64_t)md[cc_].ptr < (uint64_t)md[cc].ptr)){continue;}
				if(((uint64_t)md[cc_].ptr < next) || (((uint64_t)md[cc_].ptr - next) > diff)){continue;}
				diff = (uint64_t)md[cc_].ptr - next;
				nextdesc = (md + cc_);
			}
			md[cc].next = nextdesc;
		}
	}
}

/*
* @remarks This Function modifies the Memory Descriptor Array.
*/
void fragmemory(
	volatile mempool_t *__restrict mp, 
	volatile memdesc_t *__restrict target, 
	volatile uint64_t *__restrict ndescs, uint64_t nbytes
){
	// Fragment Memory by splitting a Memory Descriptor into nbytes(to the 8-byte boundary) and a Overflow
	if(nbytes){nbytes = __roundup(nbytes, allocALIGN);}
	memdesc_t *table = getdescarrn(mp);
	if(nbytes < target->nbytes && table && target && ndescs){
		(getastate())->totalNAllocations++;
		(*ndescs)++;
		// Fragment Standardly.
		table--;
		*table = (memdesc_t){
			.ptr = target->ptr + nbytes,
			.nbytes = target->nbytes - nbytes,
			.next = target->next,
			.flags = 0
		};
		target->nbytes = nbytes;
		target->next = table + (*ndescs) - 1;
		fixmdarray((mempool_t *)mp, (uint64_t *)ndescs);
		return;
	}
}

/*
* @remarks This Function doesn't modify the Memory Descriptor Array.
*/
volatile memdesc_t *chaintip(volatile memdesc_t *__restrict md){
	return (md->next ? chaintip(md->next): md);
}

/*
* @remarks This Function doesn't modify the Memory Descriptor Array.
*/
uint64_t memusedsize(volatile mempool_t *__restrict mp){
	memdesc_t *md = getdescarrn(mp);
	uint64_t out = 0;
	for(uint64_t cc = (mp->nDescriptors? mp->nDescriptors - 1: UINT64_MAX); cc != UINT64_MAX; --cc){
		// Additionally check for Padding with this Entry and the Next.
		out += md[cc].nbytes + /*Ensure that we account for Padding/Gaps*/(md[cc].next? (md[cc].next->ptr - (md[cc].ptr + md[cc].nbytes)): 0);
	}
	return out;
}

/*
* @remarks This Function modifies The Memory Descriptor Array
*/
memdesc_t *allocatedesc(volatile mempool_t *__restrict pool, uint64_t nbytes){
	// Get the end of the array
	memdesc_t *md = getdescarrn(pool);
	nbytes = __roundup(nbytes, allocALIGN);
	// Ensure there's enough bytes for the aligned Memory and Memory Descriptor
	if((pool->nblocks * BLOCKSIZE) >= (memusedsize(pool) + nbytes + sizeof(memdesc_t))){
		(getastate())->totalNAllocations++;
		pool->nDescriptors++;
		volatile memdesc_t *tip = chaintip(md);
		md--;
		if((uint64_t)tip != ((uint64_t)pool->base + (pool->nblocks * BLOCKSIZE))){tip->next = md;}
		*md = (memdesc_t){
			.flags = 0,
			.nbytes = nbytes,
			.next = NULL,
			.ptr = (void *)__roundup(
				(uint64_t)((tip->ptr >= pool->base) && (tip->ptr < (pool->base + (pool->nblocks * BLOCKSIZE)))? 
					((uint64_t)tip->ptr + tip->nbytes): (uint64_t)pool->base), allocALIGN)
		};
		return md;
	}else{return NULL;}
}

/*
* @remarks This Function modifies The Memory Descriptor Array
*/
bool fusememory(
	volatile mempool_t *__restrict mp, volatile uint64_t *__restrict ndescs, 
	volatile memdesc_t *__restrict chainstart, uint64_t targetbytes, 
	memdesc_e targetflags
){
	// Validation Check
	memdesc_t *table = getdescarrn(mp);
	volatile memdesc_t *temp = chainstart;
	uint64_t metbytes = 0;
	bool out = 0;
	do{
		metbytes -= temp->nbytes;
		temp = temp->next;
	}while(temp && __check(temp->flags, targetflags));
	if(metbytes < targetbytes){out = false;}else{// Validation Fail
		// Validation No-Fail
		// Now just Fuse the Memory and remap it all.
		memdesc_t *temp = (memdesc_t *)chainstart, *next = NULL;
		while(temp->next && __check(temp->flags, targetflags)){
			bool break_ = false;
			// Copy nEntries to fill the Space
			chainstart->nbytes += temp->nbytes;
			next = temp->next;
			if((uint64_t)temp < ((uint64_t)table + *ndescs + 1)){	// Ensure there's enough space for the Copy
				uint64_t index = ((uint64_t)temp - (uint64_t)table) / sizeof(memdesc_t);
				memmove_s(table + index, (*ndescs - index) * sizeof(memdesc_t), table + index + 1, (*ndescs - (index + 1)) * sizeof(memdesc_t));
				(getastate())->totalNAllocations--;
			}else{break_ = true;}
			ndescs--;
			out = true;
			temp = next;
			if(break_){break;}
		}
		fixmdarray((mempool_t *)mp, (uint64_t *)ndescs);
	}
	return out;
}

/*
* @remarks This Function already uses Mutex Locked Dependencies
*/
volatile void *mrealloc(volatile void *__restrict ptr, uint64_t new){
	if(!ptr){return mmalloc(new);}
	if(!new){mfree((void *)ptr); return NULL;}
	uint64_t originalsize = 0;
	volatile allocstate_t *astate = getastate();
	for(uint32_t cc = 0; cc < astate->npools; cc++){
		memdesc_t *md = getdescarrn(astate->pools + cc);
		for(uint64_t cc_ = (astate->pools[cc].nDescriptors? astate->pools[cc].nDescriptors - 1: UINT64_MAX); cc_ != UINT64_MAX; --cc_){
			if((ptr >= md[cc_].ptr) && (ptr <= (md[cc_].ptr + md[cc_].nbytes))){
				originalsize = md[cc_].nbytes;
				break;
			}
		}
		if(originalsize){break;}
	}
	if(originalsize){
		volatile void *out = mmalloc(new);
		if(!out){return NULL;}
		memcpy((void *)out, ptr, __min(originalsize, new));
		mfree((void *)ptr);
		return out;
	}
	return NULL;
}

/*
* @remarks This Function already uses Mutex Locked Dependencies
*/
volatile void *mcalloc(uint64_t nItems, uint64_t itemSize){return mmalloczero(nItems * itemSize);}

/*
* @remarks This Function already uses Mutex Locked Dependencies
*/
volatile void *mmalloczero(uint64_t nbytes){
	volatile void *out = mmalloc(nbytes);
	memset((void *)out, 0, nbytes);
	return out;
}

volatile void *mclone(void *original){
	uint64_t size = 0;
	volatile allocstate_t *astate = getastate();
	for(uint32_t cc = 0; cc < astate->npools; cc++){
		memdesc_t *md = getdescarrn(astate->pools + cc);
		for(uint64_t cc_ = (astate->pools[cc].nDescriptors? astate->pools[cc].nDescriptors - 1: UINT64_MAX); cc_ != UINT64_MAX; --cc_){
			if((original >= md[cc_].ptr) && (original <= (md[cc_].ptr + md[cc_].nbytes))){
				size = md[cc_].nbytes;
				break;
			}
		}
		if(size){break;}
	}
	if(size){
		void *out = (void *)mmalloczero(size);
		if(!out){return NULL;}
		memcpy(out, original, size);
		DebugPrintf("Cloning Memory 0x0x%0z, outputting 0x0x%0z of %z bytes", out, original, out, size);
		return out;
	}
	return NULL;
}

bool mIsMapped(void *m){
	volatile allocstate_t *astate = getastate();
	for(uint32_t cc = 0; cc < astate->npools; cc++){
		memdesc_t *md = getdescarrn(astate->pools + cc);
		for(uint64_t cc_ = (astate->pools[cc].nDescriptors? astate->pools[cc].nDescriptors - 1: UINT64_MAX); cc_ != UINT64_MAX; --cc_){
			if(((uint64_t)m >= (uint64_t)md[cc_].ptr) && ((uint64_t)m <= ((uint64_t)md[cc_].ptr + md[cc_].nbytes))){
				return true;
			}
		}
	}
	return MemoryIsMapped(m);
}

volatile void *mclone_s(void *original, uint64_t target){
	uint64_t size = 0;
	volatile allocstate_t *astate = getastate();
	for(uint32_t cc = 0; cc < astate->npools; cc++){
		memdesc_t *md = getdescarrn(astate->pools + cc);
		for(uint64_t cc_ = (astate->pools[cc].nDescriptors? astate->pools[cc].nDescriptors - 1: UINT64_MAX); cc_ != UINT64_MAX; --cc_){
			if((original >= md[cc_].ptr) && (original <= (md[cc_].ptr + md[cc_].nbytes))){
				size = md[cc_].nbytes;
				break;
			}
		}
		if(size){break;}
	}
	void *out = (void *)mmalloczero(target);
	if(!out){return NULL;}
	if(size){memcpy(out, original, __min(size, target));}
	else{memcpy(out, original, target);}
	return out;
}

/*
* @remarks This Function already uses Mutex Locked Dependencies
*/
volatile void *mmalloc(uint64_t nbytes){
	if(!nbytes){return NULL;}else{nbytes = __roundup(nbytes, allocALIGN);}
	volatile allocstate_t *astate = getastate();
	if(!astate->pools){initastate(__min(dBLOCKS, __roundup(nbytes, allocALIGN)), dPOOLS);}
	for(uint32_t cc = 0; cc < astate->npools; ++cc){
		if((astate->totalNAllocations % DefaultFixMemFrequency) == 0){fixmdarray(astate->pools + cc, &(astate->pools[cc].nDescriptors));}
		memdesc_t *md = querydescu((astate->pools + cc), nbytes);
		if(md){
			__set(md->flags, memdesc_e__used);
			DebugPrintf("Allocated Memory of %z bytes (Free Allocation), Output: 0x0x%0z", md->ptr, nbytes, md->ptr);
		}else{
			md = getdescarrn(astate->pools + cc);
			for(uint64_t cc_ = (astate->pools[cc].nDescriptors? astate->pools[cc].nDescriptors - 1: UINT64_MAX); cc_ != UINT64_MAX; --cc_){
				if(!__check(md[cc_].flags, memdesc_e__used)){
					if(md[cc_].nbytes > fragMul(nbytes)){
						// The descriptor is too large to be worth the allocation, fragment the Descriptor
						fragmemory(astate->pools + cc, (md + cc_), &astate->pools[cc].nDescriptors, nbytes);
						__set(md[cc_].flags, memdesc_e__used);
						DebugPrintf("Allocated %z bytes\t(Fragmented Allocation), Output: 0x0x%0z", md[cc_].ptr, nbytes, md[cc_].ptr);
					}else if(md[cc_].nbytes >= nbytes){
						__set(md[cc_].flags, memdesc_e__used);
						DebugPrintf("Allocated %z bytes\t(Standard Allocation), Output: 0x0x%0z", md[cc_].ptr, nbytes, md[cc_].ptr);
					}else{
						if(fusememory(astate->pools + cc, &astate->pools[cc].nDescriptors, (md + cc_), nbytes, ~memdesc_e__used)){
							void *out = (void *)mmalloc(nbytes);
							DebugPrintf("Allocating %z bytes\t(Internal Allocation), Output: 0x0x%0z", out, nbytes, out);
						}
					}
				}
			}
			// Failed to do all other Memory Ops, Allocate Memory Descriptor
			memdesc_t *out;
			if((out = allocatedesc(astate->pools + cc, nbytes))){
				__set(out->flags, memdesc_e__used);
				DebugPrintf("Allocating %z bytes\t(Internal Allocation), Output: 0x0x%0z", out->ptr, nbytes, out->ptr);
			}// Dont return NULL here because it wont let the allocator search across multiple pools.
		}
	}
	// Allocate Pool to allow for more space
	allocatepool(__roundup(nbytes, dBLOCKS * BLOCKSIZE) / BLOCKSIZE);
	return mmalloc(nbytes);
}

/*
* @remarks This Function only performs a small write, make sure that only the write is locked.
*/
void mfree(void *__restrict ptr){
	if(!ptr){return;}
	bool inPool = false;
	bool freed = false;
	volatile allocstate_t *state = getastate();
	if(!state->pools){initastate(dBLOCKS, dPOOLS);}else{
		for(uint32_t cc = 0; cc < state->npools; ++cc){
					inPool = ptr >= state->pools[cc].base && ptr < (state->pools[cc].base + (BLOCKSIZE * state->pools[cc].nblocks));
			memdesc_t *md = getdescarrn((state->pools + cc));
			for(uint64_t cc_ = (state->pools[cc].nDescriptors? state->pools[cc].nDescriptors - 1: UINT64_MAX); cc_ != UINT64_MAX; --cc_){
				if(
					(ptr >= md[cc_].ptr) && (ptr < (md[cc_].ptr + md[cc_].nbytes)) && 
					!__check(md[cc_].flags, memdesc_e__nofree) && 
					__check(md[cc_].flags, memdesc_e__used)
				){
					__uset(md[cc_].flags, memdesc_e__used);
					freed = true;
					DebugPrintf("Freeing Ptr 0x0x%0z", ptr);
				}
			}
		}
	}
	if(!inPool && !freed){FreePages(ptr);}
}

ml_t descinfo(volatile void *__restrict ptr){
	volatile allocstate_t *state = getastate();
	if(!state->pools){initastate(dBLOCKS, dPOOLS);}else{
		for(uint32_t cc = 0; cc < state->npools; ++cc){
			memdesc_t *md = getdescarrn((state->pools + cc));
			for(uint64_t cc_ = (state->pools[cc].nDescriptors? state->pools[cc].nDescriptors - 1: UINT64_MAX); cc_ != UINT64_MAX; --cc_){
				if(((uint64_t)ptr >= (uint64_t)md[cc_].ptr) && ((uint64_t)ptr <= ((uint64_t)md[cc_].ptr + md[cc_].nbytes))){
					return (ml_t){
						.free = !__check(md[cc_].flags, memdesc_e__used),
						.mdesc = md[cc_],						.pool = cc
					};
				}
			}
		}
	}
	return (ml_t){0};
}

ml_t *mtest(uint32_t ntries, bool print){
	typedef union fui{float f;	unsigned u;	signed i;}fui;
	ml_t *out = (ml_t *)mcalloc(ntries, sizeof(ml_t));
	while(ntries--){
		uint64_t size = GetTrueRandomRange64(UINT64_MIN, TestAllocRng),
				cmp = GetTrueRandomRange64(UINT64_MIN, TestCmdRng);
		volatile void *temp = mmalloc(size);
		out[ntries] = descinfo(temp);
		if(cmp >= (size / 2.5)){
			out[ntries].free = true;
			mfree((void *)temp);
		}else{out[ntries].ptr = temp;}
	}
	// if(print){printmem();}
	return out;
}

memdesc_t *mdump(uint32_t pool){
	volatile allocstate_t *astate = getastate();
	memdesc_t *md = (memdesc_t *)mcalloc(astate->pools[pool].nDescriptors, sizeof(memdesc_t));
	if(astate->pools){
		void *clone = (void *)mcalloc(astate->pools[pool].nDescriptors, sizeof(memdesc_t));
		memcpy(clone, getdescarrn(astate->pools + pool), astate->pools[pool].nDescriptors * sizeof(memdesc_t));
		return (memdesc_t *)clone;
	}
	return (memdesc_t *)NULL;
}

// void printmem(){
// 	volatile allocstate_t *astate = getastate();
// 	printf("\n# of Allocations: %z", astate->totalNAllocations);
// 	for(uint32_t cc = 0; cc < astate->npools; ++cc){
// 		printf("\n  Pool #%u", cc);
// 		memdesc_t *md = getdescarrn(astate->pools + cc);
// 		for(uint64_t cc_ = (astate->pools[cc].nDescriptors? astate->pools[cc].nDescriptors - 1: UINT64_MAX); cc_ != UINT64_MAX; --cc_){
// 			printf(
// 				"\n    Descriptor #%u"
// 				"\n      .ptr: 	0x0x%0z"
// 				"\n      .nbytes: %z"
// 				"\n      .next: 0x0x%0z"
// 				"\n      .flags:"
// 				"\n        :Used? [%s]"
// 				"\n        :Un-Freeable? [%s]", 
// 				cc_, md[cc_].ptr, md[cc_].nbytes, md[cc_].next, 
// 				(__check(md[cc_].flags, memdesc_e__used)? "TRUE": "FALSE"), 
// 				(__check(md[cc_].flags, memdesc_e__nofree)? "TRUE": "FALSE")
// 			);
// 		}
// 		if(!astate->pools[cc].nDescriptors){printf("\n    No Mapped Memory");}
// 	}
// 	// Analyse Memory
// 	// For each Memory Descriptor, check if the adjacent Memory is filled by a Descriptor
// 	printf("\nMap Dump:");
// 	for(uint32_t cc = 0; cc < astate->npools; ++cc){
// 		fixmdarray(astate->pools + cc, &astate->pools[cc].nDescriptors);
// 		memdesc_t *tmp = getdescarrt(astate->pools + cc);
// 		printf(
// 			"\n  Pool #%u\tUsed: %z/%z\t(%%%z)", 
// 			cc, memusedsize(astate->pools + cc), astate->pools[cc].nblocks * BLOCKSIZE, 
// 			// Shift the 100 up front to perform pure, precise integer math
// 			// Calculate the usage ratio as a float, multiply by 100, then cast the final result to uint64_t
// 			(uint64_t)(((float)memusedsize(astate->pools + cc) / (float)(astate->pools[cc].nblocks * BLOCKSIZE)) * 100.0f)
// 		);
// 		// Ensure all Next ptrs are set.
// 		do{
// 			// Walk Next
// 			// Foreach Verify if the Next is adjacent
// 			// If not print the difference.
// 			printf(
// 				"\n    <  [%s:#%z]:  0x0x%0z  ->  0x0x%0z    >", 
// 				__check(tmp->flags, memdesc_e__used)? "Allocated": "Freed    ", 
// 				((uint64_t)tmp - (uint64_t)getdescarrn(astate->pools + cc)) / sizeof(memdesc_t), 
// 				tmp->ptr, tmp->ptr + tmp->nbytes
// 			);
// 			if(tmp->next){
// 				if(((uint64_t)(tmp->ptr + tmp->nbytes) - (uint64_t)tmp->next->ptr) > 0){
// 					printf("\n    <  [Un-Mapped]:  0x0x%0z  ->  0x%0z    >", tmp->ptr + tmp->nbytes, tmp->next->ptr);
// 				}
// 				tmp = tmp->next;
// 			}
// 		}while(tmp->next);
// 		if((((uint64_t)tmp->ptr + tmp->nbytes) - (uint64_t)astate->pools[cc].base + (astate->pools[cc].nblocks * BLOCKSIZE)) > 0){
// 			printf("\n    <  [Un-Mapped]: 0x0x%0z  ->  0x0x%0z    >", 
// 				PTRCHECK(tmp->ptr)? ((uint64_t)tmp->ptr + tmp->nbytes): (uint64_t)astate->pools[cc].base, 
// 				(uint64_t)astate->pools[cc].base + (astate->pools[cc].nblocks * BLOCKSIZE)
// 			);
// 		}
// 	}
// 	MUTEX_ULOCK(astate->mutex);
// }