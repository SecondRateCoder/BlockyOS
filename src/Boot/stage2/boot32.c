#include "boot.h"

#define PF_DSTEP_LONG 		1
#define PF_DSTEP_LONG_LONG 	4

// https://www.youtube.com/watch?v=db_5skZaneg&list=PLFjM7v6KGMpiH2G-kT781ByCNC_0pKpPN&index=9

const gdtENTRY_t gdtTABLE16[] = {
    /* Null descriptor */
    { .limit = 0x0000, .base_low = 0x0000, .base_mid = 0x00,
      .access = 0x00, .granularity = 0x00, .base_high = 0x00 },

    /* 32-bit Code segment: limit=0xFFFF, base=0x00000000, access=0x9A, gran=0xCF */
    { .limit = 0xFFFF, .base_low = 0x0000, .base_mid = 0x00,
      .access = 0x9A, .granularity = 0xCF, .base_high = 0x00 },

    /* 32-bit Data segment: limit=0xFFFF, base=0x00000000, access=0x92, gran=0xCF */
    { .limit = 0xFFFF, .base_low = 0x0000, .base_mid = 0x00,
      .access = 0x92, .granularity = 0xCF, .base_high = 0x00 },

    /* 16-bit Code segment: limit=0x0000, base=0x00000000, access=0x9A, gran=0x0F */
    { .limit = 0x0000, .base_low = 0x0000, .base_mid = 0x00,
      .access = 0x9A, .granularity = 0x0F, .base_high = 0x00 },

    /* 16-bit Data segment: limit=0xFFFF, base=0x00000000, access=0x92, gran=0x0F */
    { .limit = 0xFFFF, .base_low = 0x0000, .base_mid = 0x00,
      .access = 0x92, .granularity = 0x0F, .base_high = 0x00 }
};

const gdtDESC_t desc16 = {
    .address = gdtTABLE16,
    .limit = (uint16_t)(sizeof(gdtTABLE16) - 1)
};

uint16_t *VGA = (uint16_t *)0x000B8000
uinl32_t VGAX, VGAY;
#define VGAINDEX ((VGAY * VGA_MAXY) + VGAX)
const uinl32_t VGA_MAXX = 80, VGA_MAXY = 25;
#defie TABSIZE (TABSIZE >> 1)
uint8_t TABSIZE_ = 4;

char g_hexes32[] = "0123456789ABCDEF";
// -freestanding
void putc(char c, uint8_t color){
	switch(c){
		case '\0': {					// Backspace
			VGAX--;
			if(VGAX < 0){
				VGAX = VGA_MAXX;
				VGAY--;
			}
			putc(' ', color);
		}
		case '\n': {VGAY++;}			// New-line, No Carriage-Return
		case '\r': {VGAX = 0;}			// Carriage-Return
		case '\t': {					// Tab
			for(uint8_t cc =0; cc < TABSIZE; ++cc){putc(' ', color);}
			VGAX += TABSIZE;
		}
		default: {
			VGA[VGAIDEX] = ((uint16_t)c << 8) | color;
			VGAX++;
		}
	}
	updateCursor();
}

void updateCursor(){
	if(VGAX >= VGA_MAXX){
		VGAX = 0;
		VGAY++;
	}
	if(VGAY >= VGA_MAXY){scrollCursor(VGAY - VGA_MAXY);}
	asm_updateCursor();
}

static inline void scrollCursor(uint8_t lines){
	if(lines < VGAY){
		// Copy back
		memcpy(
			VGA + ((lines * VGA_MAXX) + VGAX), 						// SRC
			VGA,													// Destination
			(((VGA - lines) * VGA_MAXX) + VGAX) * sizeof(uint16_t)	// Size
		);
		memset(
			VGA + VGAINDEX,											// SRC
			(VGA_MAXX * VGA_MAXY) - VGAINDEX,						// SIZE
			0														// VALUE
		);
	}
}

void puts(char *str){
	do{putc(*str);
	}while(str++);
}

void main32(void){
	printf32(
		"Formatted 32-bit string: %a, %c, %h, %l, %i, %z, %s",
		(u8_t)99u, 'H', (short)88u, 66ul, (int)98u, 3334848348ull, "Look, it's a Negro"
	);
}

// Follow with flags, 
// %u.: unsigned:
//      e.g &ul: unsigned long.
// %b.: Use binary radix:
//      e.g %bl: Prints long as bytes, (2).
// %d.: Use decimal radix:
//      e.g %bl: Prints long as decimal, (10).
// %x.: Use hexadecimal radix:
//      e.g %xl: Prints long as hex, (16).
// &a: byte
// %c: char     //! Modifiers do not affect this type.
// %h: short
// %l: long
// %i: integer
// %z: long long    //! A bit dodgy, dont even know why asw
// %s: string
void printf32(char *fmt, ...){
    bool sign = true;
    uint8_t radix = 10;
    uinl32_t *argp = (uint8_t *)(&fmt + 1);
    uinl32_t dwords = 0;
    while(*fmt){
        if(*fmt == '%'){
            fmt++;
            uint8_t offs_ = 0;
            if(*fmt == 'u'){			offs_ = 1;    sign = false;
            }else if(*(fmt + 1) == 'u'){offs_ = 2;    sign = false;
            }else if(*(fmt + 2) == 'u'){offs_ = 3;    sign = false;
            }else if(*(fmt + 3) == 'u'){offs_ = 4;    sign = false;}
            else{sign = true;}

            if(*fmt == 'b'){			offs_ = 1;    radix = 2;
            }else if(*(fmt + 1) == 'b'){offs_ = 2;    radix = 2;
            }else if(*(fmt + 2) == 'b'){offs_ = 3;    radix = 2;
            }else if(*(fmt + 3) == 'b'){offs_ = 4;    radix = 2;}

            if(*fmt == 'd'){			offs_ = 1;    radix = 10;
            }else if(*(fmt + 1) == 'd'){offs_ = 2;    radix = 10;
            }else if(*(fmt + 2) == 'd'){offs_ = 3;    radix = 10;
            }else if(*(fmt + 3) == 'd'){offs_ = 4;    radix = 10;}

            if(*fmt == 'x'){			offs_ = 1;    radix = 16;
            }else if(*(fmt + 1) == 'x'){offs_ = 2;    radix = 16;
            }else if(*(fmt + 2) == 'x'){offs_ = 3;    radix = 16;
            }else if(*(fmt + 3) == 'x'){offs_ = 4;    radix = 16;}
            fmt += offs_;
            switch(*fmt){
                case 's': {	// string
					// Reconstruct
                    puts((char *)(((size_t)argp[dwords] << 32)  | argp[dwords]));
                    dwords += 2;
                    break;
                }
                case 'z': {	// long long
                    printarg32(argp + dwords, 2, sign, radix, true, (radix == 16? false: true));
                    dwords += 2;
                    break;
                }
                case '%': {
                    putc('%');
                    break;
                }
                case 'c': {	// char
                    putc(*(argp + dwords));
                    dwords++;
                    break;
                }
				// All promoted to same size
                case 'i': 	// integer
                case 'h': 	// short
                case 'l':	// long
                case 'a':	// byte
                    printarg32(argp + dwords, 1, sign, radix, true, (radix == 16? false: true));
                    dwords++;
                    break;
            }
        }else{putc(*fmt);}
		fmt++;
    }
}

char *printarg32(uinl32_t *argp, uint8_t dwords, bool sign, uint8_t radix, bool printin, bool attach_sign){
    static char out[32];
    uint64_t num = 0;
    uinl32_t rem;
    int8_t num_sign = 1;
    int8_t pos = 0;
    switch(dwords){
        case PF_DSTEP_LONG: {	// 4 bytes, double word
            num = (unsigned long)(*argp);
            if(sign){
                signed char n = (signed long)(*argp);
                if(n < 0){n = -n;   num_sign = -1;}
                num = (unsigned long)n;
            }
            break;
        }
        case PF_DSTEP_LONG_LONG: {	// 8 bytes quadruple word
            num = ((size_t)argp[1] << 32) | (size_t)argp[0];
            if(sign){
                long long sll = (long long)(
                    ((long long)argp[3] << 48) | ((long long)argp[2] << 32) |
                    ((long long)argp[1] << 16) | (long long)argp[0]);
                if(sll < 0){num_sign = -1;      sll = -sll;}
                num = (unsigned long long)sll;
            }
            break;
        }
    }
    do{
        // uint32_t rem = num % radix;
		// num /= radix;
        div64_32(num, radix, &num, &rem);
        out[pos++] = g_hexes32[rem];
    }while(num);
    if(sign && num_sign < 0 && attach_sign){out[pos++] = '-';}
    if(printin){while(--pos >= 0){putc(out[pos]);}}
    return out;
}
