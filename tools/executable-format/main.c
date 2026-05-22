#include "convert.h"


int main(uint32_t argcc, char **argv){
    if(argcc >= 3){
        uint32_t out = convert_pe(argv[1], argv[2]);
        dump_exec(argv[2]);
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}