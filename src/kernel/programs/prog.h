#pragma once
#include "../Public/kernpublic.h"

prog_header **prog_headers;

typedef void *(* sigfunc)(uint32_t arg, ...);

typedef struct progh_scheduler{
    void *prog;
    uint8_t _attributes;
}progh_scheduler;

typedef struct sig_descr{
    uint8_t sig_;
    uint8_t lvl_;
    sigfunc func;
}sig_descr;

typedef enum api_lvl{
    API_LVL_THREAD = 0x0,
    API_LVL_PROG = 0x1,
    API_LVL_KERNEL = 0x2,
    API_LVL_HLPR = 0x3
}api_lvl;

typedef enum api_sig{
    API_SIGTERM = 0x0,
    API_SIGKTERM = 0x1,
    API_SIGABRT = 0x2,
    API_SIGCONTROL = 0x3,
    API_SIGMEMCORRUPTION = 0x4
}api_sig;

typedef struct prog_header{
    uint128_t id;
    uint16_t _attributes;
    _IO file;
    uint64_t sigdr_num, handle_num;
    HANDLE *handles;
    sig_descr *sigdr_table;
}prog_header;

/// @brief Using ASM, get the ID of the Program of calling interrupt.
/// @return 
uint128_t *prog_get();
prog_header *get_prog(uint128_t prog_ID);