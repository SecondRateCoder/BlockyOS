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
        free(str);
        fshandle *handle = fsloadh(fr, "attempt\0", "fc");
    }
}