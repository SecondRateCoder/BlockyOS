#include "convert.h"


int main(char **argv, uint32_t argcc){
    if(argcc > 3){return convert_pe_to_exec(argv[1], argv[2]);}
    return EXIT_FAILURE;
}