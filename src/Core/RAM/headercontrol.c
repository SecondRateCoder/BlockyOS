#include "./Source/Core-OS/RAM/memtypes.h"


size_t header_encode(uint8_t *array, hcontext_t h){
    context_encode(array, h);
	memcpy_unsafe(array, context_size, (uint8_t *)h.addr, h.size);
    return context_size + h.size;
}
//Stores in the order:
//    ID: ProcessID; ID, addr, size, hflags_t: IsProcess; IsThread; IsKernel; IsPrivate.
size_t context_encode(uint8_t *array, const hcontext_t h){
	size_t Offset =0;
	//ProgramID encoded.
	memcpy_unsafe(array, 0, h.ID.ProcessID, IDSize);
	Offset += IDSize;
	//HeaderID encoded.
	memcpy_unsafe(array, Offset, h.ID.HeaderID, IDSize);
	Offset += IDSize;
	//Address encoded.
	encode_size_t(array, h.addr, Offset);
	Offset += sizeof(size_t);
	//size encoded.
	encode_size_t(array, h.size, Offset);
	Offset += sizeof(size_t);

	//booleans
	array[Offset] = h.Checks;
	++Offset;
	array[Offset] = h.flags.IsProcess;
	++Offset;
	array[Offset] = h.flags.IsThread;
	++Offset;
	array[Offset] = h.flags.IsKernel;
	++Offset;
	array[Offset] = h.flags.IsPrivate;
	++Offset;
	array[Offset] = h.flags.IsStream;
	++Offset;
	memcpy_unsafe(array, Offset, h.Extras, CONTEXTEXTRAS_SIZE);
    return context_size;
}

void headerMeta_store(const hcontext_t H){
    uint8_t context_encoded[context_size];
    context_encode(context_encoded, H);
    RAMMeta-=context_size;
    memcpy_unsafe((uint8_t *)RAMMeta, 0, context_encoded, context_size);
}
void headerMeta_swrite(uint8_t *data, ID_t ID, hpeek_t peeker){
	size_t temp = RAMMeta;
	while(temp < _ram_end){
		if(headerpeek_unsafe(temp, HContextPeekerAttr_HeaderID) == ID.HeaderID &&
		headerpeek_unsafe(temp, HContextPeekerAttr_ProcessID) == ID.ProcessID){
			headerMeta_write(temp, peeker);
		}
		temp+=context_size;
	}
}

void headerMeta_write(uint8_t *data, hpeek_t peeker, size_t maddr){
	size_t temp = RAMMeta;
	if(maddr < temp || maddr >= _ram_length){return NULL;}
    if(!Metaaddress_validate(maddr)){return NULL;}
	switch(peeker){
		case HContextPeekerAttr_ProcessID:
			memcpy_unsafe(RAM, temp + maddr, data, 8);
		
		case HContextPeekerAttr_HeaderID:
			memcpy_unsafe(RAM, temp + maddr + IDSize, data, 8);

		case HContextPeekerAttr_Address:
			memcpy_unsafe(RAM, temp + maddr + (IDSize *2), data, sizeof(size_t));

		case HContextPeekerAttr_Size:
			memcpy_unsafe(RAM, temp + maddr + (IDSize *2) + sizeof(size_t), data, sizeof(size_t));
		
		case HContextPeekerAttr_IsProcess:
			memcpy_unsafe(RAM, temp + maddr + (IDSize *2) + (sizeof(size_t)*2), data, 1);
		
		case HContextPeekerAttr_IsThread:
			memcpy_unsafe(RAM, temp + maddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 1, data, 1);

		case HContextPeekerAttr_IsKernel:
			memcpy_unsafe(RAM, temp + maddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 2, data, 1);
		
		case HContextPeekerAttr_IsPrivate:
			memcpy_unsafe(RAM, temp + maddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 3, data, 1);
		
		case HContextPeekerAttr_IsStream:
			memcpy_unsafe(RAM, temp + maddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 4, data, 1);

		case HContextPeekerAttr_IsStream:
			memcpy_unsafe(RAM, temp + maddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 5, data, CONTEXTEXTRAS_SIZE);
		default: return INVALID_ADDR;
	}
}

uint8_t *headerpeeksearch_unsafe(ID_t ID, hpeek_t peeker){
	size_t temp = RAMMeta;
	while(temp < _ram_end){
		if(headerpeek_unsafe(temp, HContextPeekerAttr_HeaderID) == ID.HeaderID &&
		headerpeek_unsafe(temp, HContextPeekerAttr_ProcessID) == ID.ProcessID){
			return headerpeek_unsafe(temp, peeker);
		}
		temp+=context_size;
	}
}

uint8_t *headerpeek_unsafe(size_t maddr, hpeek_t peeker){
	size_t temp = RAMMeta;
	if(maddr < temp || maddr >= _ram_length){return NULL;}
    if(!Metaaddress_validate(maddr)){return NULL;}
	switch(peeker){
		case HContextPeekerAttr_ProcessID:
			return slice_bytes(RAM, temp + maddr, IDSize);
		
		case HContextPeekerAttr_HeaderID:
			return slice_bytes(RAM, temp + maddr + IDSize, IDSize);

		case HContextPeekerAttr_Address:
			return slice_bytes(RAM, temp + maddr + (IDSize *2), sizeof(size_t));

		case HContextPeekerAttr_Size:
			return slice_bytes(RAM, temp + maddr + (IDSize *2) + sizeof(size_t), sizeof(size_t));
		
		case HContextPeekerAttr_IsProcess:
			return slice_bytes(RAM, temp + maddr + (IDSize *2) + (sizeof(size_t)*2), 1);
		
		case HContextPeekerAttr_IsThread:
			return slice_bytes(RAM, temp + maddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 1, 1);

		case HContextPeekerAttr_IsKernel:
			return slice_bytes(RAM, temp + maddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 2, 1);
		
		case HContextPeekerAttr_IsPrivate:
			return slice_bytes(RAM, temp + maddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 3, 1);
	
		case HContextPeekerAttr_IsStream:
			return slice_bytes(RAM, temp + maddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 4, 1);
		case HContextPeekerAttr_IsStream:
			return slice_bytes(RAM, temp + maddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 5, CONTEXTEXTRAS_SIZE);
		default: return INVALID_ADDR;
	
	}
}

bool Metaaddress_validate(size_t maddr){return ((float)maddr)/((float)context_size) == 0;}

bool address_validate(size_t addr){
	size_t startingaddr = RAMMeta;
	while(startingaddr < _ram_length){
		if(addr == headerpeek_unsafe(startingaddr, HContextPeekerAttr_Address)){return true;}
		startingaddr += context_size;
	}
	return false;
}

bool space_validate(size_t address, size_t concurrent_size){
    size_t cc = 0;
    while(cc < _ram_length){
        size_t addr = decode_size_t(headerpeek_unsafe(cc, HContextPeekerAttr_Address), 0), size = decode_size_t(headerpeek_unsafe(cc, HContextPeekerAttr_Size), 0);
        if((address < addr+size) && (addr < address+concurrent_size)){
            // regions overlap
            return false;
        }
        cc+=context_size;
    }
    return true; // No overlaps found, region is free
}

int headers_underprocess(uint8_t ProcessID[IDSize]){
	size_t startFrom = RAMMeta;
	int counter =0;
	while(startFrom < _ram_length){
		if(compare_array(headerpeek_unsafe(startFrom, HContextPeekerAttr_ProcessID), IDSize, ProcessID, IDSize) == true){++counter;}
		startFrom += context_size;
	}
	return counter;
}

size_t memorysize_underprocess(uint8_t ProcessID[IDSize]){
	size_t startFrom = RAMMeta;
	int counter =0;
	while(startFrom < _ram_length){
		if(compare_array(headerpeek_unsafe(startFrom, HContextPeekerAttr_ProcessID), IDSize, ProcessID, IDSize) == true){
			counter += decode_size_t(headerpeek_unsafe(startFrom, HContextPeekerAttr_Size), 0);
		}
		startFrom += context_size;
	}
	return counter;
}

size_t address_pointfree(size_t startfrom, size_t concurrent_size) {
	size_t prev_end = startfrom > RAMMeta ? startfrom : RAMMeta;
    size_t cc = RAMMeta;
    while (cc < _ram_length) {
        uint8_t *addr_ptr = headerpeek_unsafe(cc, HContextPeekerAttr_Address);
        uint8_t *size_ptr = headerpeek_unsafe(cc, HContextPeekerAttr_Size);
        if (addr_ptr && size_ptr) {
            size_t region_addr = decode_size_t(addr_ptr, 0);
            size_t region_size = decode_size_t(size_ptr, 0);
            // Check if the gap before this region is big enough
            if (region_addr >= prev_end && (region_addr - prev_end) >= concurrent_size){return prev_end;}
            // Move prev_end to the end of this region if it's further
            if (region_addr + region_size > prev_end){prev_end = region_addr + region_size;}
        }
        cc += context_size;
    }
    // Check for space after the last region
    if (_ram_end > prev_end && (_ram_end - prev_end) >= concurrent_size) {return prev_end;}
    return (size_t)-1; // No free space found
}
/*
size_t address_pointfree(size_t startfrom, size_t concurrent_size){
	size_t addr;
	if(!address_validate(startfrom)){
		addr = startfrom;
	}else{addr =0;}
	while(addr < _ram_end){
		size_t addr_ = decode_size_t(headerpeek_unsafe(addr, HContextPeekerAttr_Address), 0), size = decode_size_t(headerpeek_unsafe(addr, HContextPeekerAttr_Size), 0);
		if(space_validate(addr_+size, concurrent_size) == true){return addr;}
		addr+=context_size;
	}
	return 0; // No free space found
}
*/


