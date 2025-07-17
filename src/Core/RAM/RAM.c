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
	size_t freeaddr = address_pointfree(RAMMeta, Size);
	if(freeaddr == INVALID_ADDR){
		/*Define Virt-RAM in File-system.*/
	}
	else{memclear(RAM, freeaddr, Size);}
	hcontext_store((hcontext_t){
		.addr = freeaddr,
		.checks = CHECK_PROTECT,
		.flags = (hflags_t){
			.IsKernel = (ProcessID == KERNEL_ID),
			.IsPrivate = true,
			.IsProcess = false, 
			.IsStream = false,
			.IsThread = false
		},
		.ID = (ID_t){
			.ProcessID = ProcessID,
			.HeaderID = define_hid()
		},
		.size = Size,
	});
}
#pragma endregion

#pragma region De-Alloca
void dealloca_unsafe(void *header){
	/*
		Get the pointer to header,
		find the Metadata location where it lies or is within,
		Clear out the HeaderMetadata.
	*/
	size_t temp = RAMMeta, addr = (size_t)header;
	const size_t size =decode_size_t(headerpeek_unsafe(addr, HContextPeekerAttr_Size), 0);
	memclear(RAM, get_maddr(addr), size);
	// write over the de-allocated header.
	memclear(RAM, decode_size_t(headerpeek_unsafe(addr, HContextPeekerAttr_Address), 0), size);
}

#pragma endregion