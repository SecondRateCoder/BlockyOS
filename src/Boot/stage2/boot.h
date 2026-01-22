#pragma once
#include "./kernel/public/public/public.h"

// Halt and restart
extern void __attribute__((cdecl)) start(void);
extern void __attribute__((cdecl)) halt(void);
extern void __attribute__((cdecl)) bochs_breakpoint(void);
extern void __attribute__((cdecl)) asm_updateCursor32(uinl32_t x, uinl32_t y);
// Division helpers
#define div64_32(dividend, divisor, result, remainder) div64_32_((uint64_t)(dividend), (uinl32_t)(divisor), (uint64_t  *)(result), (uinl32_t  *)(remainder))
extern void __attribute__((cdecl)) div64_32_(uint64_t dividend, uinl32_t divisor, uint64_t  *result, uinl32_t  *remainder);
extern unsigned short __attribute__((cdecl)) __U8LS(unsigned char dividend, unsigned char divisor);
extern short __attribute__((cdecl)) __I8LS(signed char value, unsigned char shift);

// C Types
char g_hexes32[];
extern drive_header bt1_drive_header;
// C Functions
void main32(void);

void updateCursor();
void printf32(char *str, ...);
static inline void scrollCursor32(uint8_t lines);
char *printarg32(uinl32_t *argp, uint8_t doubles, bool sign, uint8_t radix, bool printin, bool attach_sign);

#define ENDL "\r\n\0"

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
    uinl32_t 	hidden_sectors,
				large_sector_count;
    // Extended boot record.
    uint8_t 	drive_number,
				signature;
union{
    uinl32_t 	volume_id;
    uint8_t     volume_id_bytes[4];
};
    uint8_t 	volume_label[11];
    uint8_t 	sys_id[8];
	// Custom boot record
	uint8_t segment_clusters;
}__attribute__((packed)) drive_header;

extern void int32disable(void);
extern void int32enable(void);
extern void switch16_32(void *gdt, void *idt, void( __attribute__((cdecl)) *func)(void));
