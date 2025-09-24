#pragma once

#include "../kernel/public/kernpublic.h"

#ifndef RAM_H
#define RAM_H
// Place-holder pointer that should point past the kernel
extern char *RAM;
extern char *RAMHeaders;
extern volatile RAMH_TYPE num_headers;


typedef struct mem_header{
    void *real_addr;
    size_t size;
    uint32_t buffer_len;
	/*
	Buffer format:
		1st 8 bytes should be the headerID
		2nd 8 bytes should be the ProgramID
	*/
    char *buffer;
}mem_header;
#define memh_t mem_header

typedef enum BLOCKY_ENUM{
	// Accept any that is nearest to the benchmark.
	BLOCKY_ENUM_CLOSEST = 0x0,
	// Accept any that is the same size as the benchmark or greater.
	BLOCKY_ENUM_EXACTL = 0x1,
	// Accept only what is equal to the benchmark.
	BLOCKY_ENUM_EXACT = 0x2,
	// Allow for long time range,
	// the next argument should be the acceptable range(unless function already defines presets).
	BLOCKY_ENUM_DO_LONG = 0x3,
	// Use minimum time range that the function pre-defines.
	BLOCKY_ENUM_DO_QUICK = 0x4,
	// Allow for long search distance,
	//  the next argument should be the acceptable range(unless function already defines presets).
	BLOCKY_ENUM_DO_FAR = 0x5,
	// Use minimum distance pre-defined by function.
	BLOCKY_ENUM_DO_SHORT = 0x6,
	H_ATTRPEEK_RADDR = 0x7,
	H_ATTRPEEK_SIZE = 0x8,
	H_ATTRPEEK_BUFFERL = 0x9,
	H_ATTRPEEK_BUFFER = 0x10,
	H_ATTRPEEK_HID = 0x11,
	H_ATTRPEEK_PID = 0x12,

	H_ATTRWRITE_RADDR = 0x13,
	H_ATTRWRITE_SIZE = 0x14,
	H_ATTRWRITE_BUFFERL = 0x15,
	H_ATTRWRITE_BUFFER = 0x16
	
}BLOCKY_ENUM;
#define blockye_t BLOCKY_ENUM

/*
	Returns the header address of the inputted Memory block,
 	NULL if the address doesn't have an associative header block
*/
void *get_haddr(void *mem_block);

/*
	Returns true if the address is within a header range,
 	false if there isnt a header associated with the address.
*/
bool addr_validate(void *addr);

/*
	Returns true if the memory space at address(address) and of size(size) is within a header range,
 	false if there isnt a header associated with the memory space.
*/
bool space_validate(void *addr, size_t size);

//Validate that a pointer points to a free space,
//This is special in that it returns the offset from th closest point of the overlapping and used space 
//So if the addr+size is closest to the addr of the used space then (addr+size)-used_addr,
//and the same otherwise: (addr+size)-(used_addr+used_size), (addr)-used_addr, (addr)-(used_addr+used_size)
/*
	Returns:
		0 on Success,
		-1 on failure,
		Any other value on overlap offs
*/
ssize_t space_svalidate(size_t addr, size_t size);

/*
	Point to a free space using the input arguments to allow for a softer control
*/
bool addr_pointfree(void *addr, size_t min_size);

/*
	Get the size of, filtering by direct address access.
*/
size_t memh_size(void *haddr);

/*
	Get the size of a RAMMeta object, filtering by index.
*/
size_t meta_size(const RAMH_TYPE cc_from, bool count_back);

/*
	Perform a function as defined by the blockye_t enum.
*/
void *hcontext_attr_do(void *blockaddr, void *haddr, blockye_t function, void *value, size_t val_len);

/*
	Get the memsize of a header's pointed block, wrapper for hcontext_attr_do
*/
size_t get_memsize(void *memblock);
/*
	Store or Update a header with an index of: index,
	Possible arguments:
		index: -1; Search for the header and work on the 1st same match,
		header ID in buffer: 0; Generate a new ID for the header
*/
bool hcontext_su(memh_t *nh, RAMH_TYPE index);

/*
	Get the header index of a header.
*/
RAMH_TYPE get_hindex(void *ptr, bool is_mem);

#endif