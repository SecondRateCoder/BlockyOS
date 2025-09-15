// Place-holder pointer that should point past the kernel
char *RAM;

typedef struct mem_header{
    void *real_addr;
    size_t size;
    uint32_t buffer_len;
    char *buffer;
}mem_header;

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
	
}BLOCKY_ENUM;

void *get_haddr(void *mem_block);
bool addr_validate(void *addr);
bool space_validate(void *addr, size_t size);
void addr_pointfree(void *startfrom, size_t min_size, BLOCKY_ENUM error_handle[], uint8_t num_handles);
