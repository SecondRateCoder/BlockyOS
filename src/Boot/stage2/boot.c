#include "./int.h"

extern uint8_t bt1_drive_header[51];

extern void put_vidteletype(char c, uint8_t page);
extern void bt1_mainc(void);
extern void bt1_writec(uint16_t offset);
extern void bt1_writecl(uint16_t seg, uint16_t offs);
extern void bt1_dmreadc(void);
extern uint8_t *bt1_lbatochs(uint16_t lba);
extern void bt1_diskrc(uint16_t lba, uint16_t segment, uint16_t offset, uint16_t sector_max);

void _cdecl main(uint16_t header_ptr){
    char *temp = "This is Boot2's C file...";
    bt1_writec((uint16_t)((uint32_t)temp >> 2));
    return;
}

void putc(char c){
    put_vidteletype(c, 00);
}
void puts(char *str){
    while(*str != 0){
        putc(*str);
        ++str;
    }
}

#define PF_DSTEP_LONG 4
#define PF_DSTEP_DEC 4
#define PF_DSTEP_SHRT 2
#define PF_DSTEP_CHAR 1
#define PF_DSTEP_BYTE 1
#define PF_DSTEP_LONG_LONG 8

typedef unsigned char bool;
#define true 1
#define false 0

void printf(char *str, ...){
    bool unsigned_ = false;
    int *argp = (int *)&str;
    argp += sizeof(str) / sizeof(int);
    argp++;
    unsigned int cc_data = 0;
    unsigned int cc = 0;
    while(str[cc] != 0){
        switch(str[cc]){
            default:
                putc(str[cc]);
                break;
            case '%':
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
                    case 's':
                        puts((char *)(argp + cc_data));
                        while(argp[cc_data] != 0){++cc_data;}
                        cc += 1 + unsigned_;
                        break;
                }
                break;
        } // <-- THIS WAS MISSING
        cc++;
    }
    return;
}

/*void _cdecl printf(char *str, ...){
    bool unsigned_ = false;
    int *argp = (int *)&str;
    argp += sizeof(str) / sizeof(int);
    argp++;
    unsigned int cc_data = 0;
    unsigned int cc = 0;
    while(str[cc] != 0){
        switch(str[cc]){
            default:
                putc(str[cc]);
                break;
            case '%':
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
                    case 's':
                        puts((char *)(argp + cc_data));
                        while(argp[cc_data] != 0){++cc_data;}
                        cc += 1 + unsigned_;
                        break;
                }
                break;
        }
        cc++;
    }
    return;
}*/

char g_hexes[] = "0123456789abcdef";
int *print_num(int *argp, int len, bool sign, int radix){
    char buffer[32];
    unsigned long long num;
    int num_sign = 1;
    int pos = 0;
    switch(len){
        case PF_DSTEP_BYTE:
            if(sign){
                signed char n = *(signed char *)argp;
                if(n < 0){n = -n;   num_sign = -1;}
                num = n;
            }else{num  = *(unsigned char *)argp;}
            argp++;
            break;
        case PF_DSTEP_SHRT:
            if(sign){
                int n = *argp;
                if(n < 0){n = -n;     num_sign = -1;}
                num = n;
            }else{num  = *(unsigned int *)argp;}
            argp++;
            break;
        case PF_DSTEP_LONG:
            if(sign){
                long int n = *(long int*)argp;
                if(n < 0){n = -n;     num_sign = -1;}
                num = n;
            }else{num  = *(unsigned long int*)argp;}
            argp+=2;
            break;
        case PF_DSTEP_LONG_LONG:
            if(sign){
                long long int n = *(long long int*)argp;
                if(n < 0){n = -n;     num_sign = -1;}
                num = n;
            }else{num  = *(unsigned long long int*)argp;}
            argp+=4;
            break;
    }
    do{
        uint32_t rem = num % radix;
        num = num / radix;
        buffer[pos++] = g_hexes[rem];
    }while(num > 0);
    if(sign && num_sign < 0){buffer[pos++] = '-';}
    while(--pos >= 0){putc(buffer[pos]);}
    return argp;
}


