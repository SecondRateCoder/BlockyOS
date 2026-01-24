#include "ram.h"
#include "./kernel/public/kernpublic.h"

char *RAM;
volatile RAMH_TYPE num_headers;
//! IMPORTANT: HEADER METADATA IS ARRANGED LINEARLY (WITH THIER BUFFERS TRAILING AFFRONT THEM) USE 1 OF THE VALIDATED FUNCTIONS TO INDEX THIS POINTER.
//	BECAUSE SIZES ARE NOT UNIFORM, TO MEASURE THE SIZE OF A HEADER META USE "memh_size" (IT DOES NOT HAVE ANY VALIDATION AND TAKES ANY ADDRESS) NOT "sizeof(X)"
char *RAMHeaders;

size_t memh_size(void *haddr){
	const memh_t *header = (memh_t *)haddr;
	return (sizeof(size_t)* 2) + header->buffer_len;
}

size_t memh_sizei(RAMH_TYPE index){
	//Get size of current length, get size at point
	size_t lest = meta_size(index - 1, true), most = meta_size(index, true);
	return most - lest;
}

size_t meta_size(const RAMH_TYPE cc_from, bool count_back){
	if(cc_from > num_headers){return 0;}
	RAMH_TYPE cc = (cc_from);
	switch(count_back){
		case true:
			if(cc_from != 0){
				size_t out = 0;
				for(; cc > 0; out+=memh_size(&RAMHeaders[out]), cc--){} //Find size of RAMMeta up to this point
				return cc;
			}
			return 0;
		case false:
			for(; cc < num_headers; cc+=memh_size(&RAMHeaders[cc])){}
			return cc;

	}
}

bool addr_validate(void *addr){return (get_haddr(addr) == NULL);}

ssize_t space_svalidate(size_t addr, size_t size){
	if(space_validate(addr, size) == false /* Not used */){
		return 0; //Success, no overlap
	}else{
		void *haddr = get_haddr((void *)addr);
		if(haddr == NULL){
			haddr = get_haddr((void *)(addr + size));
			if(haddr == NULL){
				return -1; //RAMH_TYPE MAX
			}
		}else{
			const memh_t *mhaddr = ((memh_t *)haddr);
			size_t oaddr = (size_t)mhaddr->real_addr;
			size_t osize = mhaddr->size;
			return min(min((addr+size)-oaddr, (addr+size)-(oaddr+osize)), min((addr)-oaddr, (addr)-(oaddr+osize)));
		}
	}
	return -1;
}

bool space_validate(void *addr, size_t size){
	for(RAMH_TYPE cc =0; cc < num_headers; cc+=sizeof(mem_header)){
		const memh_t *temp = (memh_t *)(&RAMHeaders[meta_size(cc, true)]);
		if(addr_validate(addr) &&
			clamp_sizet((size_t)addr+size, (size_t)temp->real_addr, temp->size) != (size_t)addr+size){continue;}
		else{return true;}
	}
	return false;
}

void *get_haddr(void *mem_block){
	for(RAMH_TYPE cc =0; cc < num_headers; cc+=sizeof(mem_header)){
		const memh_t *temp = (memh_t *)(&RAMHeaders[meta_size(cc, true)]);
		if(addr_validate(temp->real_addr) == true){
			return temp;
		}
	}
	return NULL;
}

RAMH_TYPE get_hindex(void *ptr, bool is_mem){
	const void *hptr = (is_mem == true? get_haddr(ptr): ptr);
	//Index through RAMHeaders, if equal return index.
	size_t acc = 0;
	for(RAMH_TYPE cc = 0; cc < num_headers; ++cc){
		acc = meta_size(cc, true);
		if(h_equ(hptr, RAMHeaders+acc) == true){
			return cc;
		}
	}
	return ((RAMH_TYPE)0)-1;
}

void *hcontext_attr_do(void *blockaddr, void *haddr, blockye_t function, void *value, size_t val_len){
	if(blockaddr == NULL && haddr != NULL){
		//Haddr was input.
	}else if(blockaddr != NULL && haddr == NULL){
		//Block address was input
		haddr = get_haddr(blockaddr);
	}else{return;}
		switch(function){
			case H_ATTRPEEK_HID:
				return decode64(((memh_t *)haddr)->buffer);
			case H_ATTRPEEK_PID:
				return decode64(((memh_t *)haddr)->buffer + sizeof(size_t));
			case H_ATTRPEEK_RADDR:
				return ((memh_t *)haddr)->real_addr;
			case H_ATTRPEEK_SIZE:
				return &(((memh_t *)haddr)->size);
			case H_ATTRPEEK_BUFFERL:
				return &(((memh_t *)haddr)->buffer_len);
			case H_ATTRPEEK_BUFFER:
				return &(((memh_t *)haddr)->buffer);
			case H_ATTRWRITE_RADDR:
				((memh_t *)haddr)->real_addr = &RAM[decode_64(value)];
				return NULL;
			case H_ATTRWRITE_SIZE:
				((memh_t *)haddr)->size = decode_64(value);
				return NULL;
			case H_ATTRWRITE_BUFFER:
				memcpy(((memh_t *)haddr)->buffer, value, val_len);
				return NULL;
			case H_ATTRWRITE_BUFFERL:
				((memh_t *)haddr)->buffer_len = decode_32(value);
				return NULL;
		}
}

size_t get_memsize(void *memblock){return hcontext_attr_do(memblock, NULL, H_ATTRPEEK_SIZE, NULL, 0);}

bool h_equ(memh_t *a, memh_t *b){return decode64(a->buffer) == decode64(b->buffer) && decode64(a->buffer + sizeof(size_t)) == decode64(b->buffer + sizeof(size_t));}

bool hcontext_su(memh_t *nh, RAMH_TYPE index){
	size_t acc = 0;
	if(nh == NULL){
		//Get the cc of the nh header
		for(RAMH_TYPE cc =0; cc < num_headers; ++cc){
			acc = meta_size(cc, true);
			if(h_equ(nh, RAMHeaders+acc) == true){
				const size_t frws = meta_size(cc+1, false);
				memdisplace(RAMHeaders+acc, frws, memh_sizei(cc));
			}
		}
		return true;
	}
	if(index == -1){
		//Search for a header with the specified data
		for(RAMH_TYPE cc =0; cc < num_headers; ++cc){
			acc = meta_size(acc, true);
			if(h_equ(nh, RAMHeaders+acc) == true){
				//If Header IDs are the same then store at location.
				if(memh_size(RAMHeaders+acc) != memh_size(nh)){
					const ssize_t defs = memh_size(RAMHeaders+acc),
					nhs = memh_size(nh),
					frws = meta_size(acc, false);
					//Displace front-most headers to remove unused space.
					memdisplace(RAMHeaders + acc, frws, defs-nhs);
				}
				RAMHeaders[acc] = nh;
				return true;
			}
		}
		//If reaching this point then this header has a new or unseen ID
		if(decode64(nh->buffer) == 0x0){encode64(nh->buffer, new_hid(decode64(nh->buffer + sizeof(size_t))));}
		acc = meta_size(0, false); //Get size of metadata array.
		num_headers++;
		RAMHeaders[acc] = nh;
	}else{
		acc = meta_size(acc, true);
		if(h_equ(nh, RAMHeaders+acc) == true){RAMHeaders[acc] = nh;}
	}
}

//10K bytes, 10 Kilobytes.
#define MAX_SEARCH 10000
//Search in 4 byte chunks
#define SEARCH_CHNK 4
//Simple function to point to a new address within range of min_size etc
bool addr_pointfree(void *addr, size_t min_size){
	for(RAMH_TYPE cc =(RAMH_TYPE)addr; cc <= ((size_t)addr)+MAX_SEARCH; cc+=SEARCH_CHNK){
		if(space_svalidate(cc, min_size) == false){
			encode64(addr, cc);
			return true;
		}
	}
	return false;
}