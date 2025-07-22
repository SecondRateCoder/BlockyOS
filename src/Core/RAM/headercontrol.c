#include "./src/Core/RAM/memtypes.h"

//Stores in the order:
//    ID: ProcessID; ID, addr, size, hflags_t: IsProcess; IsThread; IsKernel; IsPrivate.
size_t hcontext_encode(uint8_t *array, const hcontext_t h){
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
	//Checker.
	array[Offset] = h.checks;
	++Offset;
	//booleans
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

void hcontext_store(const hcontext_t H){
    uint8_t context_encoded[context_size];
    context_encode(context_encoded, H);
    RAMMeta-=context_size;
    memcpy_unsafe((uint8_t *)RAMMeta, 0, context_encoded, context_size);
}

size_t get_hcontextaddr(size_t ramaddress){
	size_t temp = RAMMeta;
	while(temp < _ram_end){
		if(clamp_size_t(hcontext_attrpeek_unsafe(temp, HContextPeekerAttr_Address), 
			hcontext_attrpeek_unsafe(temp, HContextPeekerAttr_Size), 
				ramaddress) == ramaddress){return temp;}
		// if(hcontext_attrpeek_unsafe(temp, HContextPeekerAttr_Address) == ramaddress){return temp;}
		++temp;
	}
	return INVALID_ADDR;
}


void hcontext_attrwrite(uint8_t *data, ID_t ID, hpeek_t peeker){
	size_t temp = RAMMeta;
	while(temp < _ram_end){
		if((compare_array(hcontext_attrpeek_unsafe(temp, HContextPeekerAttr_HeaderID), ID.HeaderID, IDSize) &&
		compare_array(hcontext_attrpeek_unsafe(temp, HContextPeekerAttr_ProcessID), ID.ProcessID, IDSize)) || ID.ProcessID == KERNEL_ID){
			hcontext_attrwrite_unsafe(data, peeker, temp);
		}
		temp+=context_size;
	}
}
void hcontext_attrwrite_unsafe(uint8_t *data, hpeek_t peeker, size_t hcontextaddr){
	size_t temp = RAMMeta;
	if(hcontextaddr < temp || hcontextaddr >= _ram_length){return NULL;}
	switch(peeker){
		case HContextPeekerAttr_ProcessID:
			memcpy_unsafe(RAM, temp + hcontextaddr, data, 8);
			break;
		
		case HContextPeekerAttr_HeaderID:
			memcpy_unsafe(RAM, temp + hcontextaddr + IDSize, data, 8);
			break;

		case HContextPeekerAttr_Address:
			memcpy_unsafe(RAM, temp + hcontextaddr + (IDSize *2), data, sizeof(size_t));
			break;

		case HContextPeekerAttr_Size:
			memcpy_unsafe(RAM, temp + hcontextaddr + (IDSize *2) + sizeof(size_t), data, sizeof(size_t));
			break;

		case HContextPeekerAttr_Checks:
			memcpy_unsafe(RAM, temp + hcontextaddr + (IDSize *2) + (sizeof(size_t)*2), data, 1);
			break;

		case HContextPeekerAttr_IsProcess:
			memcpy_unsafe(RAM, temp + hcontextaddr + (IDSize *2) + (sizeof(size_t)*2) + 1, data, 1);
			break;

		case HContextPeekerAttr_IsThread:
			memcpy_unsafe(RAM, temp + hcontextaddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 2, data, 1);
			break;

		case HContextPeekerAttr_IsKernel:
			memcpy_unsafe(RAM, temp + hcontextaddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 3, data, 1);
			break;

		case HContextPeekerAttr_IsPrivate:
			memcpy_unsafe(RAM, temp + hcontextaddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 4, data, 1);
			break;

		case HContextPeekerAttr_IsStream:
			memcpy_unsafe(RAM, temp + hcontextaddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 5, data, 1);
			break;

		case HContextPeekerAttr_Extras:
			memcpy_unsafe(RAM, temp + hcontextaddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 6, data, CONTEXTEXTRAS_SIZE);
			break;
		
		default: return;
	}
}
void direct_extraswrite(size_t hcontextaddr, uint8_t value, uint8_t index){
	size_t temp = RAMMeta;
	if(address_validate(hcontextaddr) == false){
		RAM[temp + hcontextaddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 5 + index] = value;
	}
	return;
}

uint8_t *hcontext_attrpeekh(const uint8_t hereditary_id[IDSize], const hpeek_t peeker, const hflags_t flags, const hflags_t ignore){
	size_t temp = RAMMeta;
	while(temp < _ram_end){
		if((compare_array(hcontext_attrpeek_unsafe(temp, HContextPeekerAttr_IsKernel), flags.IsKernel, 1) || !ignore.IsKernel) &&
			(compare_array(hcontext_attrpeek_unsafe(temp, HContextPeekerAttr_IsPrivate), flags.IsPrivate, 1) || !ignore.IsPrivate) &&
			(compare_array(hcontext_attrpeek_unsafe(temp, HContextPeekerAttr_IsProcess), flags.IsProcess, 1) || !ignore.IsProcess) &&
			(compare_array(hcontext_attrpeek_unsafe(temp, HContextPeekerAttr_IsStream), flags.IsStream, 1) || !ignore.IsStream) &&
			(compare_array(hcontext_attrpeek_unsafe(temp, HContextPeekerAttr_IsThread), flags.IsThread, 1) || !ignore.IsThread)){
				if((compare_array(hcontext_attrpeek_unsafe(temp, HContextPeekerAttr_HeaderID), hereditary_id, IDSize) ||
				compare_array(hcontext_attrpeek_unsafe(temp, HContextPeekerAttr_ProcessID), hereditary_id, IDSize)) || compare_array(hereditary_id, KERNEL_ID, IDSize)){
					headerMeta_attrpeek_unsafe(peeker, temp);
				}
				temp+=context_size;
		}
	}
}
uint8_t *hcontext_attrpeek(ID_t ID, hpeek_t peeker){
	size_t temp = RAMMeta;
	while(temp < _ram_end){
		if((compare_array(hcontext_attrpeek_unsafe(temp, HContextPeekerAttr_HeaderID), ID.HeaderID, IDSize) &&
		compare_array(hcontext_attrpeek_unsafe(temp, HContextPeekerAttr_ProcessID), ID.ProcessID, IDSize)) || compare_array(ID.ProcessID, KERNEL_ID, IDSize)){
			return headerMeta_attrpeek_unsafe(peeker, temp);
		}
		temp+=context_size;
	}
	return NULL;
}
uint8_t *hcontext_attrpeek_unsafe(size_t hcontextaddr, hpeek_t peeker){
	size_t temp = RAMMeta;
	if(hcontextaddr < temp || hcontextaddr >= _ram_length){return NULL;}
	switch(peeker){
		case HContextPeekerAttr_ProcessID:
			return slice_bytes(RAM, temp + hcontextaddr, IDSize);
		
		case HContextPeekerAttr_HeaderID:
			return slice_bytes(RAM, temp + hcontextaddr + IDSize, IDSize);

		case HContextPeekerAttr_Address:
			return slice_bytes(RAM, temp + hcontextaddr + (IDSize *2), sizeof(size_t));

		case HContextPeekerAttr_Size:
			return slice_bytes(RAM, temp + hcontextaddr + (IDSize *2) + sizeof(size_t), sizeof(size_t));
		
		case HContextPeekerAttr_IsProcess:
			return slice_bytes(RAM, temp + hcontextaddr + (IDSize *2) + (sizeof(size_t)*2), 1);
		
		case HContextPeekerAttr_IsThread:
			return slice_bytes(RAM, temp + hcontextaddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 1, 1);

		case HContextPeekerAttr_IsKernel:
			return slice_bytes(RAM, temp + hcontextaddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 2, 1);
		
		case HContextPeekerAttr_IsPrivate:
			return slice_bytes(RAM, temp + hcontextaddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 3, 1);
	
		case HContextPeekerAttr_IsStream:
			return slice_bytes(RAM, temp + hcontextaddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 4, 1);
		
		case HContextPeekerAttr_Extras:
			return slice_bytes(RAM, temp + hcontextaddr + IDSize + (IDSize *2) + (sizeof(size_t)*2) + 5, CONTEXTEXTRAS_SIZE);

		default: return INVALID_ADDR;
	
	}
}

bool address_validate(size_t addr){
	size_t temp = RAMMeta;
	if(addr < temp && addr > _ram_start){
		while(temp < _ram_length){
			if(addr == hcontext_attrpeek_unsafe(temp, HContextPeekerAttr_Address)){return true;}
			temp += context_size;
		}
	}
	return false;
}
bool space_validate(size_t address, size_t concurrent_size){
    size_t cc = 0;
    while(cc < _ram_length){
        size_t addr = decode_size_t(hcontext_attrpeek_unsafe(cc, HContextPeekerAttr_Address), 0), size = decode_size_t(hcontext_attrpeek_unsafe(cc, HContextPeekerAttr_Size), 0);
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
		if(compare_array(hcontext_attrpeek_unsafe(startFrom, HContextPeekerAttr_ProcessID), ProcessID, IDSize) == true){++counter;}
		startFrom += context_size;
	}
	return counter;
}

size_t memorysize_underprocess(uint8_t ProcessID[IDSize]){
	size_t startFrom = RAMMeta;
	int counter =0;
	while(startFrom < _ram_length){
		if(compare_array(hcontext_attrpeek_unsafe(startFrom, HContextPeekerAttr_ProcessID), ProcessID, IDSize) == true){
			counter += decode_size_t(hcontext_attrpeek_unsafe(startFrom, HContextPeekerAttr_Size), 0);
		}
		startFrom += context_size;
	}
	return counter;
}

size_t address_pointfree(size_t startfrom, size_t concurrent_size){
	size_t prev_end = startfrom > RAMMeta ? startfrom : RAMMeta;
    size_t cc = RAMMeta;
    while (cc < _ram_length) {
        uint8_t *addr_ptr = hcontext_attrpeek_unsafe(cc, HContextPeekerAttr_Address);
        uint8_t *size_ptr = hcontext_attrpeek_unsafe(cc, HContextPeekerAttr_Size);
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
		size_t addr_ = decode_size_t(hcontext_attrpeek_unsafe(addr, HContextPeekerAttr_Address), 0), size = decode_size_t(hcontext_attrpeek_unsafe(addr, HContextPeekerAttr_Size), 0);
		if(space_validate(addr_+size, concurrent_size) == true){return addr;}
		addr+=context_size;
	}
	return 0; // No free space found
}
*/


// bool memclean(){
// 	size_t temp =RAMMeta;
// 	// const size_t snapshot = temp;
// 	/*
// 	Clean Memory, moving similar ProcessIDs to be near each-other, move "re-alloca" blocks next to thier neighbours.
// 	*/
// 	int phase = 0;
// 	while(phase < 3){
// 		switch(phase){
// 			case 0:
// 				while(temp < _ram_end){
// 					if(hcontext_attrpeek_unsafe(temp, HC))
// 					temp += context_size;
// 				}
// 		}
// 	}
// }
	// while(temp < _ram_end){
	// 	size_t addr =decode_size_t(hcontext_attrpeek_unsafe(temp, HContextPeekerAttr_Address));
	// 	if(addr == 0){/*No Header there.*/continue;}else{

	// 	}
	// 	temp += context_size;
	// }