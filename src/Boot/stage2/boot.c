#include "boot.h"

char g_hexes[] = "0123456789ABCDEF";

void main16(uint16_t header_ptr){
    char *temp = "Hi, this is a test from Boot 2..." ENDL;
    puts(temp);
    bochs_breakpoint();
    printf16((char *)("Freaky Signed: %%, %dl, %di, %da, %c%c, 0x%uxh, %dz, %s." ENDL), -7l, (int)-6, (int8_t)-5, 'H', 'I', (signed short)-16, (signed long long)-8854ll, (char __far *)"Freaking monkey yolo");
    printf16((char *)("Freaky Unsigned: %udl, 0x%uxi, %uba, %c, %ubh, 0x%uxz." ENDL), 749ul, (unsigned int)76, true, 'h', (unsigned short)4u, 885430ull);
    65535ull;
    0x82BF;
    halt();
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
void printf16(char *fmt, ...){
    bool sign = true;
    uint8_t radix = 10;
    uint16_t *argp = (uint16_t *)(&fmt + 1);
    uint32_t words = 0;
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
                    // bochs_breakpoint();
                    uint16_t ofs = argp[words];
                    uint16_t seg = argp[words + 1];
                    char __far *fp = (char __far *)(((uint32_t)seg << 16) | ofs);
                    puts(fp);   // Try both
                    words += 2;
                    break;
                }
                case 'z': {	// long long
                    printarg16(argp + words, 4, sign, radix, true, (radix == 16? false: true));
                    words += 4;
                    break;
                }
                case '%': {
                    putc('%');
                    break;
                }
                case 'c': {	// char
                    putc(*(argp + words));
                    words++;
                    break;
                }
                case 'i':	// integer
                case 'h': 	// short
                case 'a':	// byte
                    printarg16(argp + words, 1, sign, radix, true, (radix == 16? false: true));
                    words++;
                    break;
                case 'l':	// long
                    printarg16(argp + words, 2, sign, radix, true, (radix == 16? false: true));
                    words += 2;
                    break;
            }
        }else{putc(*fmt);}
		fmt++;
    }
}

char *printarg16(uint16_t *argp, uint8_t words, bool sign, uint8_t radix, bool printin, bool attach_sign){
    static char out[32];
    uint64_t num = 0;uint32_t rem;
    int8_t num_sign = 1;
    int8_t pos = 0;
    switch(words){
        case PF_DSTEP_WORD: {
            num = (unsigned char)(*argp);
            if(sign){
                signed char n = (signed char)(*argp);
                if(n < 0){n = -n;   num_sign = -1;}
                num = (unsigned char)n;
            }
            break;
        }
        case PF_DSTEP_LONG: {
            int16_t low = *argp;
            int16_t high = *(argp + 1);
            num = (uinl32_t)(((uinl32_t)high << 16) | low);
            if(sign){
                long sval = (long)(((long)high << 16) | low);
                if(low < 0 || high < 0){sval = -sval;      num_sign = -1;}
                num = (unsigned long)sval;
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
        out[pos++] = g_hexes[rem];
    }while(num);
    if(sign && num_sign < 0 && attach_sign){out[pos++] = '-';}
    if(printin){while(--pos >= 0){putc(out[pos]);}}
    return out;
}