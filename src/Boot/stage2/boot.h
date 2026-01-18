#include "kernel/public/public/public.h"

// #define __far
// #define __cdecl

#define PF_DSTEP_WORD 1
#define PF_DSTEP_LONG 2
#define PF_DSTEP_LONG_LONG 4

//* Assembly Functions
#define putc(c) put_vidteletype((c), 00)
#define puts(ptr) puts_vidteletype(00, (char __far *)(ptr))
// Halt and restart
extern void __cdecl start(void);
extern void __cdecl halt(void);
extern void __cdecl bochs_breakpoint(void);
// Print functions
extern void __cdecl put_vidteletype(uint16_t c, uint16_t page);
extern void __cdecl puts_vidteletype(char page, char __far *ptr);
// Division helpers
#define div64_32(dividend, divisor, result, remainder) div64_32_((uint64_t)(dividend), (uinl32_t)(divisor), (uint64_t __far *)(result), (uint32_t __far *)(remainder))
extern void __cdecl div64_32_(uint64_t dividend, uinl32_t divisor, uint64_t __far *result, uint32_t __far *remainder);
extern unsigned short __cdecl __U8LS(unsigned char dividend, unsigned char divisor);
extern short __cdecl __I8LS(signed char value, unsigned char shift);

#define ENDL "\r\n\0"

// C Types
char g_hexes[];
// C Functions
void main16(uint16_t header_ptr);
void printf16(char *str, ...);
// Returns char[32]
char *printarg16(uint16_t *argp, uint8_t words, bool sign, uint8_t radix, bool printin, bool attach_sign);

#pragma pack(push, 1)
typedef struct drive_header{
    uint8_t 	OEM_ID[8];
    uint16_t 	bytes_per_sector;
    uint8_t 	sectors_per_cluster,
				reserved_sectors,
				fat_count;
    uint16_t 	dir_entries_count,
				total_sectors;
    uint8_t 	media_descriptor_type;
    uint16_t 	sectors_per_fat,
				sectors_per_track,
				heads;
    uint32_t 	hidden_sectors,
				large_sector_count;
    // Extended boot record.
    uint8_t 	drive_number,
				signature;
union{
    uint32_t 	volume_id;
    uint8_t     volume_id_bytes[4];
};
    uint8_t 	volume_label[11];
    uint8_t 	sys_id[8];
	// Custom boot record
	uint8_t segment_clusters;
} drive_header;
#pragma pack(pop)

extern drive_header bt1_drive_header;
