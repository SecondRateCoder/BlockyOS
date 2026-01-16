#include "boot.h"

char g_hexes[] = "0123456789ABCDEF";

void main16(uint16_t header_ptr){
    char *temp = "Hi, this is a test from Boot 2..." ENDL;
    puts(temp);
    printf16((char *)("Freaky Signed: %l, %d, %b, %c, %h, %z, %s" ENDL), -7l, (long)-6, (int)-5, (u8_t)-4, 'H', (short)-16, -8854349430430ll, "Freaking monkey yolo");
    printf16((char *)("Freaky Unsigned: %l, %d, %b, %c, %h, %z, %s" ENDL), 7ul, (unsigned int)-6, true, 'h', (unsigned short)4u, 8854349430430ull);
    bochs_breakpoint();
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
// %c: char
// %h: short
// %l: long
// %i: integer
// %z: long long
// %s: string
void printf16(char *fmt, ...){
    bochs_breakpoint();
    bool sign = 0;
    uint8_t radix = 10;
    uint16_t *argp = (uint16_t *)(&fmt + 1);
    uint32_t words = 0;
    while(*fmt){
        if(*fmt == '%'){
            fmt++;
            uint8_t offs_ = 0;
            if(*fmt == 'u'){			offs_ = 1;    sign = true;
            }else if(*(fmt + 1) == 'u'){offs_ = 2;    sign = true;
            }else if(*(fmt + 2) == 'u'){offs_ = 3;    sign = true;
            }else if(*(fmt + 3) == 'u'){offs_ = 4;    sign = true;}

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
				case 'c':	// char 
                case 'h': 	// short
                case 'a': {	// byte
                    char *temp = printarg16(argp + words, 1, sign, radix);
                    puts(temp);
					words++;
					break;
                }
                case 'i':	// integer
				case 'l': {	// long
                    char *temp = printarg16(argp + words, 2, sign, radix);
                    puts(temp);
					words += 2;
					break;
                }
				case 'z': {	// long long
                    char *temp = printarg16(argp + words, 4, sign, radix);
                    puts(temp);
					words += 4;
					break;
                }
				case 's': {	// string
                    uint16_t ofs = argp[words];
                    uint16_t seg = argp[words + 1];
                    char __far *fp = (char __far *)(((uint32_t)seg << 16) | ofs);
                    words += 2;
                    puts(fp);
					break;
                }
            }
        }else{putc(*fmt);}
		fmt++;
    }
}

char *printarg16(uint16_t *argp, uint8_t words, bool sign, uint8_t radix){
    bochs_breakpoint();
    static char out[32];
    size_t num = 0;
    int8_t num_sign = 1;
    int8_t pos = 0;
    switch(words){
        case PF_DSTEP_BYTE: {
            bochs_breakpoint();
            int tmp = *argp;
            if(sign){
                signed char n = (signed char)tmp;
                if(n < 0){n = -n;   num_sign = -1;}
                num = (unsigned char)n;
            }else{num = (unsigned char)tmp;}
            break;
        }
        case PF_DSTEP_SHRT: {
            bochs_breakpoint();
            int tmp = *argp;
            if(sign){
                signed short n = (signed short)tmp;
                if(n < 0){n = -n;   num_sign = -1;}
                num = (unsigned short)n;
            }else{num = (unsigned short)tmp;}
            break;
        }
        case PF_DSTEP_LONG: {
            bochs_breakpoint();
            uint16_t low = (uint32_t)(uint16_t)(*argp);
            uint16_t high = (uint16_t)(*(argp + 1));
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
            bochs_breakpoint();
            uint32_t p0 = (uint32_t)(uint16_t)(argp[0]);
            uint32_t p1 = (uint32_t)(uint16_t)(argp[1]);
            uint32_t p2 = (uint32_t)(uint16_t)(argp[2]);
            uint32_t p3 = (uint32_t)(uint16_t)(argp[3]);
            num = ((unsigned long long)p3 << 48) | ((unsigned long long)p2 << 32) |
                  ((unsigned long long)p1 << 16) | (unsigned long long)p0;
            if(sign){
                long long sll = (long long)num;
                if(sll < 0){ sll = -sll; num_sign = -1; num = (unsigned long long)sll; }
            }
            break;
        }
    }
    bochs_breakpoint();
    do{
        // uint32_t rem = num % radix;
        uint32_t rem;
        _div64_32(num, radix, &num, &rem);
        out[pos++] = g_hexes[rem];
    }while(num > 0);
    if(sign && num_sign < 0){out[pos++] = '-';}
    while(--pos >= 0){putc(out[pos]);}
    bochs_breakpoint();
    return out;
}
