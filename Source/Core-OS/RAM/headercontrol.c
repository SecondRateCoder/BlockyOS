#include "./Source/Core-OS/RAM/memtypes.h"


size_t header_encode(uint8_t *array, header_t h){
    context_encode(array, h.context);
	memcpy_unsafe(array, context_size, h.addr, h.size);
    return context_size + h.size;
}
//Stores in the order:
//    ID: ProcessID; ID, addr, size, hflags_t: IsProcess; IsThread; IsKernel; IsPrivate.
size_t context_encode(uint8_t *array, const header_t h){
	size_t Offset =0;
	//ProgramID encoded.
	memcpy_unsafe(array, 0, h.context.ID.ProcessID, IDSize);
	Offset += IDSize;
	//HeaderID encoded.
	memcpy_unsafe(array, Offset, h.context.ID.HeaderID, IDSize);
	Offset += IDSize;
	//Address encoded.
	encode_size_t(array, h.addr, Offset);
	Offset += sizeof(size_t);
	//size encoded.
	encode_size_t(array, h.size, Offset);
	Offset += sizeof(size_t);

	//booleans
	memcpy_unsafe(array, h.context.flags.IsProcess, Offset);
	++Offset;
	memcpy_unsafe(array, h.context.flags.IsThread, Offset);
	++Offset;
	memcpy_unsafe(array, h.context.flags.IsKernel, Offset);
	++Offset;
	memcpy_unsafe(array, h.context.flags.IsPrivate, Offset);
    return context_size;
}

void headerMeta_store(const header_t H){
    uint8_t context_encoded[context_size];
    context_encode(context_encoded, H);
    RAMMeta-=context_size;
    memcpy_unsafe(RAMMeta, 0, context_encoded, context_size);
}

uint8_t *headerpeek_unsafe(size_t addrInMeta, hpeek_t peeker){
    if(!Metaaddress_validate(addrInMeta)){return NULL;}
	if(context_size+Offset>_ram_length){return NULL;}
	switch(peeker){
		case HContextPeekerAttr_ProcessID:
			return slice_bytes(RAM, RAMMeta + addrInMeta, IDSize);
		
		case HContextPeekerAttr_HeaderID:
			return slice_bytes(RAM, RAMMeta + addrInMeta + IDSize, IDSize);

		case HContextPeekerAttr_Address:
			return slice_bytes(RAM, RAMMeta + addrInMeta + (IDSize *2), IDSize);

		case HContextPeekerAttr_Size:
			return slice_bytes(RAM, RAMMeta + addrInMeta + (IDSize *2) + sizeof(size_t), IDSize);
		
		case HContextPeekerAttr_IsProcess:
			return slice_bytes(RAM, RAMMeta + addrInMeta + (IDSize *2) + (sizeof(size_t)*2), IDSize);
		
		case HContextPeekerAttr_IsThread:
			return slice_bytes(RAM, RAMMeta + addrInMeta + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 1, IDSize);

		case HContextPeekerAttr_IsKernel:
			return slice_bytes(RAM, RAMMeta + addrInMeta + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 2, IDSize);
		
		case HContextPeekerAttr_IsPrivate:
			return slice_bytes(RAM, RAMMeta + addrInMeta + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 3, IDSize);	
	}
}

bool Metaaddress_validate(size_t maddr){return ((float)addrInMeta)/((float)context_size) == 0;}

bool address_validate(size_t addr){
	size_t startingaddr = RAMMeta;
	while(startingaddr < _ram_length){
		if(addr == headerpeek_unsafe(startingaddr, HContextPeekerAttr_Address)){return true;}
		startingaddr += context_size;
	}
	return false;
}

bool space_validate(size_t address, size_t concurrent_size){
    size_t cc =0;
    while(cc < NumberOfBlocks){
        
        cc+=context_size;
    }
    return false;
}

int headers_underprocess(uint8_t ProcessID[IDSize]){
	size_t startFrom = RAMMeta;
	int counter =0;
	while(startFrom < _ram_length){
		if(compare_array(headerpeek_unsafe(startFrom, HContextPeekerAttr_ProcessID), ProcessID) == true){++counter;}
		startFrom += context_size;
	}
	return counter;
}

size_t memorysize_underprocess(uint8_t ProcessID[IDSize]){
	size_t startFrom = RAMMeta;
	int counter =0;
	while(startFrom < _ram_length){
		if(compare_array(headerpeek_unsafe(startFrom, HContextPeekerAttr_ProcessID), ProcessID) == true){
			counter += headerpeek_unsafe(startFrom, HContextPeekerAttr_Size);
		}
		startFrom += context_size;
	}
	return counter;
}