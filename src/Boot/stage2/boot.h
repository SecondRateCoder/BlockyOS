#include "kernel/public/public/public.h"

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
#define div64_32(dividend, divisor, result, remainder) div64_32_((uint64_t)(dividend), (uinl32_t)(divisor), (uint64_t __far *)(result), (uinl32_t __far *)(remainder))
extern void __cdecl div64_32_(uint64_t dividend, uinl32_t divisor, uint64_t __far *result, uinl32_t __far *remainder);
extern unsigned short __cdecl __U8LS(unsigned char dividend, unsigned char divisor);
extern short __cdecl __I8LS(signed char value, unsigned char shift);

// Helpers
extern void int16disable(void);
extern void int16enable(void);
extern void switch16_32(gdtDESC_t __far * gdt, void __far * idt, void(__far __cdecl *func)(void));

#define ENDL "\r\n\0"

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
} drive_header;

typedef struct gdtENTRY_t{
    union{
        struct{
            uint16_t limit;
            uint16_t base_upper;      uint8_t base_lower;
            uint8_t access_flags;
            uint8_t gran_limit;
            uint8_t base_high;
        };
        size_t ENTRY[2];
    };
}gdtENTRY_t;

typedef struct gdtDESC_t{
    // Byte size of GDT table
    uint16_t limit;

    // Address of GDT table
    gdtENTRY_t __far *address;
}gdtDESC_t;
#pragma pack(pop)

extern drive_header bt1_drive_header;
