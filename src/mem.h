// Place-holder pointer that should point past the kernel
extern char *RAM;
extern char *RAMHeaders;

typedef struct mem_header{
    void *real_addr;
    size_t size;
    uint32_t buffer_len;
    char *buffer;
}mem_header;
#define memh_t mem_header

typedef enum BLOCKY_ENUM{
	// Accept any that is nearest to the benchmark.
	BLOCKY_ENUM_CLOSEST = 0x0,
	// Accept any that is the same size as the benchmark or greater.
	BLOCKY_ENUM_EXACTL = 0x1
	// Accept only what is equal to the benchmark.
	BLOCKY_ENUM_EXACT = 0x2
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
	H_ATTRWRITE_RADDR = 0x11,
	H_ATTRWRITE_SIZE = 0x12,
	H_ATTRWRITE_BUFFERL = 0x13,
	H_ATTRWRITE_BUFFER = 0x14
	
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
bool space_validate(void *addr, ssize_t size);
/*
	Point to a free space using the input arguments to allow for a softer control
*/
void *addr_pointfree(void *startfrom, size_t min_size, BLOCKY_ENUM error_handle[], uint8_t num_handles);
