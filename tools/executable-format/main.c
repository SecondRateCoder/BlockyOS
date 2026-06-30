#include "convert.h"


int main(uint32_t argcc, char **argv){
    char *inpath = NULL, *outpath = NULL;
    bool dump = false, help = false;
    for(uint32_t cc = 1; cc < argcc; ++cc){
        if(!memcmp(argv[cc], "-d", __min(strlen(argv[cc]), 2))){dump = true;}
        else if(!memcmp(argv[cc], "-h", __min(strlen(argv[cc]), 2))){help = true;}
        else if(cc == (dump + help + 1)){inpath = argv[cc];}
        else if(cc == (dump + help + 2)){outpath = argv[cc];}
        else{continue;}
    }
    if(help){
        printf("\n%s [-h] [-d] <INPUT-FILE-PATH> <OUTPUT-FILE-PATH>\n\t-h:\tPrint this Info\n\t-d:\tDump Executable Info", argv[0]);
    }else{
        uint32_t out;
        if(inpath && outpath){out = convert_exec(inpath, outpath);}
        if(dump){dump_exec(outpath);}
        
    }
    return EXIT_SUCCESS;
}