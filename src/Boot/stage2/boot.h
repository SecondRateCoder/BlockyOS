#include "kernel/public/public/public.h"

#define PF_DSTEP_LONG_LONG 8
#define PF_DSTEP_LONG 4
#define PF_DSTEP_SHRT 2
#define PF_DSTEP_BYTE 1

//* Assembly Functions
#define putc(c) put_vidteletype(c, 00)
#define puts(ptr) puts_vidteletype((char __far *)(ptr))
// Halt and restart
extern void _cdecl start(void);
extern void _cdecl halt(void);
extern void _cdecl bochs_breakpoint(void);
// Print functions
extern void _cdecl put_vidteletype(char c, u8_t page);
extern void _cdecl puts_vidteletype(char __far *ptr);
// Division helpers
extern void _cdecl _div64_32(uint64_t dividend, uint32_t divisor, uint64_t __far *quotientOut, uint32_t __far *remainderOut);
extern unsigned short _cdecl __U8DR(unsigned char dividend, unsigned char divisor);
extern unsigned short _cdecl __U8DQ(unsigned char dividend, unsigned char divisor);
extern unsigned short _cdecl __U8LS(unsigned char dividend, unsigned char divisor);

#define ENDL "\r\n\0"
char __ = '\r';

// C Types
char g_hexes[];
// C Functions
void main16(uint16_t header_ptr);
void printf16(char *str, ...);
// Returns char[32]
char *printarg16(uint16_t *argp, uint8_t words, bool sign, uint8_t radix);

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