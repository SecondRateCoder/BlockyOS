#include "./src/Core/RAM/memtypes.h"

#pragma region Alloca & Re-alloca
void realloca(void *object, size_t new_size){
	const size_t addr = (size_t)object, hcontextaddr = get_hcontextaddr(addr);
	if(address_validate(addr)){return;}
	const size_t curr_size =hcontext_attrpeek_unsafe((size_t)object, HContextPeekerAttr_Size);
	const uint8_t ProcessID[IDSize] = hcontext_attrpeek_unsafe(hcontextaddr, HContextPeekerAttr_ProcessID), extras[CONTEXTEXTRAS_SIZE] = hcontext_attrpeek_unsafe(hcontextaddr, HContextPeekerAttr_Extras);
	if(new_size > curr_size){
		//alloca new block, store pointer to that header.
		size_t next_addr =(size_t)alloca(new_size-curr_size, KERNEL_ID), next_hcaddr = get_hcontextaddr(next_addr);
		uint8_t addr_encoded[sizeof(size_t)];
		encode_size_t(addr_encoded, next_addr, 0);
		//Write the new size_t address.
		int cc =1;
		for(; cc < sizeof(size_t); cc++){direct_extraswrite(addr, addr_encoded[cc-1], cc);}
		//write the last address in the new header.
		encode_size_t(addr_encoded, addr, 0);
		for(; cc < sizeof(size_t)*2; cc++){direct_extraswrite(next_hcaddr, addr_encoded[cc-1], cc);}
	}else if(new_size < curr_size){headerMeta_write(new_size, HContextPeekerAttr_Size, (size_t)object);}
}

void *alloca(size_t Size, uint8_t ProcessID[IDSize]){
	Size++;
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
		//Store at random Location.
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
	const size_t addr = (size_t)header, haddr = get_hcontextaddr(addr);
	const uint8_t ProcessID[IDSize] = hcontext_attrpeek_unsafe(haddr, HContextPeekerAttr_ProcessID);
	const hflags_t flags = (hflags_t){true, true, false, true, false}, mask = (hflags_t){false, true, true, true, true};
	//Search for the same ProcessID but with the Proecess flag set, so it returns the parent P.
	const size_t parent_startaddr = hcontext_attrpeekh(ProcessID, HContextPeekerAttr_Address, flags, mask), parent_endaddr = memorysize_underprocess(Process_ID);
	if(addr > )
	/*
		!UNSAFE
			Get the pointer to header,
			find the Metadata location where it lies or is within,
			Clear out the HeaderMetadata.
		size_t temp = RAMMeta, addr = (size_t)header;
		const size_t size =decode_size_t(headerpeek_unsafe(addr, HContextPeekerAttr_Size), 0);
		memclear(RAM, get_maddr(addr), size);
		// write over the de-allocated header.
		memclear(RAM, decode_size_t(headerpeek_unsafe(addr, HContextPeekerAttr_Address), 0), size);
	*/
}

#pragma endregion