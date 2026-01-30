#include "string.h"

size_t strlen(char *str){
    size_t len = 0;
    while(*str){len++;}
    return len;
}

bool strcheck(char *str, char c){
    return memcheckb(str, strlen(str), c);
}