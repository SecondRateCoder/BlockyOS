#include "frat.h"

void main(uint32_t largs, char **arg){
    conf_fsroot *fr;
    if(!(fr = fmount(arg[1]))){
        GPTeNSTR *str = makeGPTeNSTR("Root");
        formatpart(*str);
        if(!(fr = fmount(arg[1]))){
            printf("Drive Error! Killing Program");
            exit(EXIT_FAILURE);
        }
        printf("\n\n");
        free(str);
        fshandle *handle = fsloadh(fr, "attempt\0", "fc");
        void *_block = malloc(getblocksize());
        for(uint32_t cc = 0; cc < getblocksize(); ++cc){printf("%c:%u    ", ((uint8_t *)_block)[cc], ((uint8_t *)_block)[cc]);}
        _fpush1(handle, _block);
        free(_block);
        _block = _fread1(handle);
        for(uint32_t cc = 0; cc < getblocksize(); ++cc){printf("%c:%u    ", ((uint8_t *)_block)[cc], ((uint8_t *)_block)[cc]);}
    }
}
