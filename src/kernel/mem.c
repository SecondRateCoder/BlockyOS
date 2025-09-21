char *RAM;
volatile uint num_headers;
char *RAMHeaders;

bool addr_validate(void *addr){return (get_haddr(addr) == NULL);}

bool space_validate(void *addr, size_t size){
	for(size_t cc =0; cc < num_headers; cc+=sizeof(mem_header)){
		const memh_t *temp = (memh_t *)(&RAMHeaders[cc]);
		if(addr_validate(addr) &&
			capl((size_t)addr+size, (size_t)temp.real_addr, temp.size) != (size_t)addr+size){continue;}
		else{return true;}
	}
	return false;
}

void *get_haddr(void *mem_block){
	for(size_t cc =0; cc < num_headers; cc+=sizeof(mem_header)){
		const memh_t *temp = (memh_t *)(&RAMHeaders[cc]);
		if(addr_validate(temp.real_addr) == true){
			return temp;
		}
	}
	return NULL;
}

void *hcontext_attr_do(void *blockaddr, void *haddr, blockye_t function, void *value, size_t val_len){
	if(blockaddr == NULL && haddr != NULL){
		//Haddr was input.
	}else if(blockaddr != NULL && haddr == NULL){
		//Block address was input
		haddr = get_haddr(blockaaddr);
	}else{return;}
		switch(function){
			case H_ATTRPEEK_RADDR:
				return ((memh_t *)haddr).real_addr;
			case H_ATTRPEEK_SIZE:
				return &(((memh_t *)haddr).size);
			case H_ATTRPEEK_BUFFERL:
				return &(((memh_t *)haddr).buffer_len);
			case H_ATTRPEEK_BUFFER:
				return &(((memh_t *)haddr).buffer);
			case H_ATTRWRITE_RADDR:
				((memh_t *)haddr).real_addr = &RAM[decode_64(value)];
				return NULL;
			case H_ATTRWRITE_SIZE:
				((memh_t *)haddr).size = decode_64(value);
				return NULL;
			case H_ATTRWRITE_BUFFER:
				((memh_t *)haddr).buffer = memcpy(value, val_len);
				return NULL;
			case H_ATTRWRITE_BUFFERL:
				((memh_t *)haddr).buffer_len = decode_32(value);
				return NULL;
		}
}

size_t get_memsize(void *memblock){return hcontext_attr_do(memblock, NULL, H_ATTRPEEK_SIZE, NULL, 0);}

bool hcontext_store(memh_t *header, int index){
	if(index > num_headers){index = num_headers +1;}
	if(index == -1){
		// Place at any location, basically append the header.
		if(space_validate(&RAMHeaders[index* sizeof(memh_t)+5], sizeof(memh_t) == true){
			RAMHeaders[index* sizeof(memh_t)+5] = header;
			return true;
		}else{
			if(blockdisplace(&RAMHeaders[index* sizeof(memh_t)+5], {BLOCKY_ENUM_DO_LONG, 20}) == true){
				RAMHeaders[index* sizeof(memh_t)+5] = header;
				return true;
			}else{return false;}
		}
	}else{
		// Update the header at the specified index
		RAMHeaders[index* sizeof(memh_t)+5] = header;
		return true;
	}
}
