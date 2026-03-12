#include "string.h"
#include "kernel/lib32/stdmath/math.h"

size_t strlen(char *str){
    size_t len = 0;
    while(*str){len++;}
    return len;
}

bool strchecks(char *str, char *snippet){
    const size_t temp = (strlen(str) - strlen(snippet));
    for(size_t cc =0;cc < temp; ++cc){
        if(memcmp(str + cc, snippet, strlen(str))){return true;}
    }
    return false;
}

size_t strcmpc(char *a, char *b){
    size_t cc =0, max_ = max(strlen(a), strlen(b));
    for(; cc < max_; ++cc){
        if(a[cc] != b[cc]){return cc;}
    }
    return cc;
}

bool isdigit(char c){return (c > '0') && (c < '9');}

int8_t tobyte(char *str){
    unsigned len = strlen(str);
    int8_t out = 0;
    while(*str){
        out += (tobit(*str));
        str++;
    }
    return out;
}

int8_t tobitcomplex(char c){
    if(c > '0' && c < '9'){return c - '0';}
    return -1;
}

int16_t toword(char *str){
    bool negative;
    int16_t out = 0;
    uint8_t _10 = 0;
    if(*str == '-'){negative = true;    str++;}
    while(*str){
        if(!isdigit(*str)){break;}
        out += (tobit(*str) * powll(10, _10));
        str++;_10++;
    }
    return (negative? -out: out);
}

signed long tolong(char *str){
    bool negative = false;
    signed long out = 0;
    uint8_t _10 = 0;
    if(*str == '-'){negative = true;    str++;}
    while(*str){
        if(!isdigit(*str)){break;}
        out += (tobit(*str) * powll(10, _10));
        str++;_10++;
    }
    return (negative? -out: out);
}

ssize_t tolonglong(char *str){
    bool negative = false;
    ssize_t out = 0;
    uint8_t _10 = 0;
    if(*str == '-'){negative = true;    str++;}
    while(*str != 0){
        if(!isdigit(*str)){break;}
        out += (tobit(*str) * powll(10, _10));
        str++;_10++;
    }
    return (negative? -out: out);
}