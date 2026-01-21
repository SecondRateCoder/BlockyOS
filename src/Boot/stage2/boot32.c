#include "boot.h"

#define PF_DSTEP_LONG 		1
#define PF_DSTEP_LONG_LONG 	4

char g_hexes32[];
// C Functions
void main32(uint16_t header_ptr);
void printf32(char *str, ...);
// Returns char[32]
char *printarg32(uint16_t *argp, uint8_t doubles, bool sign, uint8_t radix, bool printin, bool attach_sign);
char g_hexes32[] = "0123456789ABCDEF";

void __far main32(void){
	printf(
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
    uinl32_t *argp = (uint16_t *)(&fmt + 1);
    uinl32_t doubles = 0;
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
                    // https://board.flatassembler.net/topic.php?t=19467
                    uinl32_t ofs = argp[doubles];
                    uinl32_t seg = argp[doubles + 1];
                    char *fp = (char *)(((uinl64_t)seg << 32) | ofs);
                    puts(fp);   // Try both
                    doubles += 2;
                    break;
                }
                case 'z': {	// long long
                    printarg32(argp + doubles, 4, sign, radix, true, (radix == 16? false: true));
                    doubles += 2;
                    break;
                }
                case '%': {
                    putc('%');
                    break;
                }
                case 'c': {	// char
                    putc(*(argp + doubles));
                    doubles++;
                    break;
                }
                case 'i': {	// integer
                    printarg32(argp + doubles, 2, sign, radix, true, (radix == 16? false: true));
                    doubles++;
                    break;
                }
                case 'h': 	// short
                case 'a':	// byte
                    printarg32(argp + doubles, 1, sign, radix, true, (radix == 16? false: true));
                    doubles++;
                    break;
                case 'l':	// long
                    printarg32(argp + doubles, 2, sign, radix, true, (radix == 16? false: true));
                    doubles ++;
                    break;
            }
        }else{putc(*fmt);}
		fmt++;
    }
}

char *printarg32(uint16_t *argp, uint8_t doubles, bool sign, uint8_t radix, bool printin, bool attach_sign){
    static char out[32];
    uint64_t num = 0;
    uinl32_t rem;
    int8_t num_sign = 1;
    int8_t pos = 0;
    switch(doubles){
        case PF_DSTEP_LONG: {
            num = (unsigned long)(*argp);
            if(sign){
                signed char n = (signed long)(*argp);
                if(n < 0){n = -n;   num_sign = -1;}
                num = (unsigned long)n;
            }
            break;
        }
        case PF_DSTEP_LONG_LONG: {
            num = ((unsigned long long)argp[3] << 48) | ((unsigned long long)argp[2] << 32) |
                  ((unsigned long long)argp[1] << 16) | (unsigned long long)argp[0];
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
