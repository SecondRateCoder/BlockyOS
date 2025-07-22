#include "./src/Core/RAM/memtypes.h"


#define LASTBLOCKINDEX 5
#define NEXTBLOCKINDEX 12

#pragma region Alloca & Re-alloca
void *realloca(void *object, size_t new_size){
	const size_t addr = (size_t)object, hcontextaddr = get_hcontextaddr(addr);
	if(address_validate(addr)){return;}
	const size_t curr_size =hcontext_attrpeek_unsafe((size_t)object, HContextPeekerAttr_Size);
	const uint8_t ProcessID[IDSize] = hcontext_attrpeek_unsafe(hcontextaddr, HContextPeekerAttr_ProcessID), extras[CONTEXTEXTRAS_SIZE] = hcontext_attrpeek_unsafe(hcontextaddr, HContextPeekerAttr_Extras);
	//Re-alloced Header's Extras should have the ID: Extras[0 -> 4]= Num, Extras[5 -> 11]= LastBlockAddr, Extras[12 -> 20]= NextBlockAddr
	//If there is viable space for the new_size then simply write a new size to the header's hcontext_t
	if(new_size > curr_size){
		// //!METHOD 1
		// if(space_validate(addr, new_size) == true){hcontext_attrwrite_unsafe(decode_size_t(new_size), HContextPeekerAttr_Size, hcontextaddr);}else{
		// 	//Allocate new block.
		// 	const uint8_t *freeaddr = (void *)alloca(new_size-curr_size, ProcessID);
		// 	const uint8_t *free_hcontextaddr = get_hcontextaddr(freeaddr);
		// 	uint8_t free_addr_encoded[sizeof(size_t)] = decode_size_t(freeaddr);
		// 	uint8_t curr_addr_decoded[sizeof(size_t)] = decode_size_t(addr);
		// 	uint8_t cc =0;
		// 	while(cc < sizeof(size_t)){
		// 		direct_extraswrite(hcontextaddr, free_addr_encoded[cc], cc+NEXTBLOCKINDEX);
		// 		direct_extraswrite(free_hcontextaddr, curr_addr_decoded[cc], cc+LASTBLOCKINDEX);
		// 		++cc;
		// 	}
		// }
		//!METHOD 2
		//Simply alloca the new block.
		uint8_t *freeaddr = alloca(new_size, ProcessID);
		memmove_unsafe(freeaddr, new_size, 0, object, curr_size);
		dealloca_unsafe(object);
		return freeaddr;
	}else{
		hcontext_attrwrite_unsafe(decode_size_t(new_size), HContextPeekerAttr_Size, hcontextaddr);
		return object;
	}



	// if(new_size > curr_size){
	// 	//alloca new block, store pointer to that header.
	// 	size_t next_addr =(size_t)alloca(new_size-curr_size, KERNEL_ID), next_hcaddr = get_hcontextaddr(next_addr);
	// 	uint8_t addr_encoded[sizeof(size_t)];
	// 	encode_size_t(addr_encoded, next_addr, 0);
	// 	//Write the new size_t address.
	// 	int cc =1;
	// 	for(; cc < sizeof(size_t); cc++){direct_extraswrite(addr, addr_encoded[cc-1], cc);}
	// 	//write the last address in the new header.
	// 	encode_size_t(addr_encoded, addr, 0);
	// 	for(; cc < sizeof(size_t)*2; cc++){direct_extraswrite(next_hcaddr, addr_encoded[cc-1], cc);}
	// }else if(new_size < curr_size){headerMeta_write(new_size, HContextPeekerAttr_Size, (size_t)object);}
}

void *alloca(size_t Size, uint8_t ProcessID[IDSize]){
	//INC Size, to have a NULL value at the end of the block.
	Size++;
	//Get address of the end of Parent Address.
	const hflags_t flags = (hflags_t){true, true, true, true, false}, mask = (hflags_t){true, true, true, true, false};
	size_t finaladdr = decode_size_t(hcontext_attrpeekh(ProcessID, HContextPeekerAttr_Address, flags, mask)) + memorysize_underprocess(ProcessID);
	if(space_validate(finaladdr), Size){
		//Manually store at parent's end location.
		hcontext_store(
			(hcontext_t){
				.ID = (ID_t){ProcessID, headers_underprocess(ProcessID)++},
				.addr = finaladdr+1,
				.size = Size-1,
				.checks = CHECK_PROTECT,
				.flags = (hflags_t){false, false, false, true, false},
				.Extras = (uint8_t[CONTEXTEXTRAS_SIZE]){0}
			}
		);
		finaladdr = parent_endaddr+1;
	}else{
		//Store at any viable Location.
		finaladdr = address_pointfree(RAMMeta, Size);
		if(finaladdr == INVALID_ADDR){
			//Store in Virt-RAM.
		}else{
			if(RAM[finaladdr] != 0){memclear(RAM, finaladdr, Size);}
			hcontext_store(
				(hcontext_t){
					.ID = (ProcessID, headers_underprocess(ProcessID)++),
					.addr = finaladdr,
					.size = Size-1,
					.checks = CHECK_PROTECT,
					.flags = (hflags_t){false, false, false, true, false},
					.Extras = (uint8_t[CONTEXTEXTRAS_SIZE]){0}
				}
			);
		}
	}
	RAM[finaladdr+Size] = NULL;
	return (void *)&RAM[finaladdr];
}
#pragma endregion

#pragma region De-Alloca
void dealloca_unsafe(void *header){
	//Cleaner
	/*
	Should assist in the cleaning of Memory Blocks.
	1st:	Find specific location in Parent Process's heap.
	2nd:	Based on the distance between the location and the end addr of Heap either
				save it's addr and end addr to the memtobefreed Buffer. (So that it's de-allocing can be handled later.).
				or move ProgramData to pver-write itself.
				Starting from RAMMeta all the Headers above it should be moved down and over-write it's data,
				
	*/
	//Get Header location and location and end address of Parent Process.
	const size_t addr = (size_t)header, haddr = get_hcontextaddr(addr), size = hcontext_attrpeek_unsafe(haddr, HContextPeekerAttr_Size);
	const uint8_t ProcessID[IDSize] = hcontext_attrpeek_unsafe(haddr, HContextPeekerAttr_ProcessID);
	const hflags_t flags = (hflags_t){true, true, false, true, false}, mask = (hflags_t){false, true, true, true, true};
	//Search with the same ProcessID but with the Process flag set, so it returns the 1st address pointing to a Process with the same ProcessID.
	const size_t parent_startaddr = hcontext_attrpeekh(ProcessID, HContextPeekerAttr_Address, flags, mask), parent_memlen = memorysize_underprocess(Process_ID), parent_endaddr = parent_memlen+ parent_startaddr;
	if((parent_memlen/2)+parent_startaddr > MAXMEM_MOVE){
		//Too large to be feasible, put to be freed later.
		memtobefreed = realloca(memtobefreed, sizeof(bound_t));
		memtobefreed[memtobefreed] = (bound_t){addr, addr > (parent_memlen/2)+parent_startaddr? -((ssize_t)size): (ssize_t)size};
		memtobefreed_length++;
	}else{mem_displace(RAM, parent_startaddr, parent_memlen, (ssize_t)size);}
	
	//Clean Header Context.
	mem_displace(RAMMeta, 0, ((size_t)haddr)-((size_t)RAMMeta), context_size);
	RAMMeta+=context_size;
}

#pragma endregion