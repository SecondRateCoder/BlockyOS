#include "kernel/public/public/public.h"

#define PF_DSTEP_LONG 4
#define PF_DSTEP_DEC 4
#define PF_DSTEP_SHRT 2
#define PF_DSTEP_CHAR 1
#define PF_DSTEP_BYTE 1
#define PF_DSTEP_LONG_LONG 8

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

#define putc(c) put_vidteletype(c, 00)
#define puts(ptr) puts_vidteletype((char __far *)ptr)
// Halt and restart
extern void _cdecl __far start(void);
extern void _cdecl __far halt(void);
// Print functions
extern void _cdecl __far put_vidteletype(char c, u8_t page);
extern void _cdecl __far puts_vidteletype(char __far *ptr);
// Division helpers
extern void _cdecl __far _div64_32(uint64_t dividend, uint32_t divisor, uint64_t *quotientOut, uint32_t *remainderOut);
extern unsigned short _cdecl __far __U8DR(unsigned char dividend, unsigned char divisor);
extern unsigned short _cdecl __far __U8DQ(unsigned char dividend, unsigned char divisor);
extern unsigned short _cdecl __far __U8LS(unsigned char dividend, unsigned char divisor);

#define ENDL "\r\n\0"

char g_hexes[];
void main(uint16_t header_ptr);
void printf(char *str, ...);
int *print_num(int *argp, int len, bool sign, int radix);

void main(uint16_t header_ptr){
    printf(("Freaky Signed: %l, %d, %b, %c, %h, %z, %s", ENDL), -7l, (long)-6, (int)-5, (u8_t)-4, 'H', (short)-16, -8854349430430ll, "Freaking monkey yolo");
    printf(("Freaky Unsigned: %l, %d, %b, %c, %h, %z, %s", ENDL), 7ul, (unsigned int)-6, true, 'h', (unsigned short)4u, 8854349430430ull);
    halt();
}

// Follow with flags, 
// %.u: unsigned
//e.g &lu: unsigned long
// %l: long
// %d: integer
// &b: byte
// %c: char
// %h: short
// %z: long long
// %s: string
void printf(char *str, ...){
    bool unsigned_ = false;
    int *argp = (int *)&str;
    argp += sizeof(str) / sizeof(int);
    argp++;
    unsigned int cc_data = 0;
    unsigned int cc = 0;
    while(str[cc]){
        if(str[cc] == '%'){
            unsigned_ = (str[cc + 2] == 'u');
            switch(str[cc + 1]){
                case 'l':    // long
                case 'd':    // integer
                default:
                    cc += 1 + unsigned_;
                    cc_data += PF_DSTEP_LONG;
                    print_num(argp, PF_DSTEP_LONG, unsigned_, 10);
                    break;
                case 'b':    //byte
                    cc += 1 + unsigned_;
                    cc_data += PF_DSTEP_BYTE;
                    print_num(argp, PF_DSTEP_BYTE, unsigned_, 10);
                    break;
                case 'c':   // char
                    putc(argp[cc_data]);
                    cc += 1 + unsigned_;
                    break;
                case 'h':   // short
                    cc += 1 + unsigned_;
                    cc_data += PF_DSTEP_SHRT;
                    print_num(argp, PF_DSTEP_SHRT, unsigned_, 10);
                    break;
                case 'z':   // long long
                    cc += 1 + unsigned_;
                    cc_data += PF_DSTEP_LONG_LONG;
                    break;
                case 's':   // string
                    puts((char __far *)(argp + cc_data));
                    while(argp[cc_data]){++cc_data;}
                    cc += 1 + unsigned_;
                    break;
            }
        }else{putc(str[cc]);}
        cc++;
    }
    return;
}

char g_hexes[] = "0123456789abcdef";
int * print_num(int *argp, int len, bool sign, int radix){
    char buffer[32];
    size_t num = 0;
    int num_sign = 1;
    int pos = 0;
    switch(len){
        case PF_DSTEP_BYTE: {
            int tmp = *argp;
            argp++;
            if(sign){
                signed char n = (signed char)tmp;
                if(n < 0){ n = -n; num_sign = -1; }
                num = (unsigned char)n;
            } else {
                num = (unsigned char)tmp;
            }
            break;
        }
        case PF_DSTEP_SHRT: {
            int tmp = *argp;
            argp++;
            if(sign){
                short n = (short)tmp;
                if(n < 0){ n = -n; num_sign = -1; }
                num = (unsigned short)n;
            } else {
                num = (unsigned short)tmp;
            }
            break;
        }
        case PF_DSTEP_LONG: {
            uint16_t low = (uint32_t)(uint16_t)(*argp);
            uint16_t high = (uint16_t)(*(argp + 1));
            argp += 2;
            uint32_t val = ((uint32_t)high << 16) | low;
            if(sign){
                long sval = (long)val;
                if(sval < 0){ sval = -sval; num_sign = -1; }
                num = (unsigned long)sval;
            } else {
                num = (unsigned long)val;
            }
            break;
        }
        case PF_DSTEP_LONG_LONG: {
            uint32_t p0 = (uint32_t)(uint16_t)(argp[0]);
            uint32_t p1 = (uint32_t)(uint16_t)(argp[1]);
            uint32_t p2 = (uint32_t)(uint16_t)(argp[2]);
            uint32_t p3 = (uint32_t)(uint16_t)(argp[3]);
            argp += 4;
            num = ((unsigned long long)p3 << 48) | ((unsigned long long)p2 << 32) |
                  ((unsigned long long)p1 << 16) | (unsigned long long)p0;
            if(sign){
                long long sll = (long long)num;
                if(sll < 0){ sll = -sll; num_sign = -1; num = (unsigned long long)sll; }
            }
            break;
        }
    }
    do{
        // uint32_t rem = num % radix;
        uint32_t rem;
        _div64_32(num, radix, &num, &rem);
        num = num / radix;
        buffer[pos++] = g_hexes[rem];
    }while(num > 0);
    if(sign && num_sign < 0){buffer[pos++] = '-';}
    while(--pos >= 0){putc(buffer[pos]);}
    return argp;
}


