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
    }
    fhandle *handle = fsloadh(fr, "attempt\0", "fc");
    printf("Ftest Output: %s", (__ftest(handle) ? "TRUE": "FALSE"));
}

void shell(char *image, char *partition){
    conf_fsroot *root;
    if(!(root = fmount(image))){
        GPTeNSTR *str = makeGPTeNSTR(partition);
        formatpart(*str);
        free(str);
        if(!(root = fmount(image))){
            printf("Drive Error! Killing Program");
            exit(EXIT_FAILURE);
        }
    }
    bool exit = 0;
    do{
        char *in = readbuf(CMDiMAX);
        strtok_t *tstate = strtok_i(in, " \",");
        char *tok = NULL;
        do{
            tok = strtok_k(tstate);
        }while(tok);
    }while(!exit);
}