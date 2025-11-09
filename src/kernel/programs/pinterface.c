#include "../kernel/programs/prog.h"

#define API_HOOK(LEVEL, SIG, FUNC)

bool api_hook_(uint8_t lvl_, uint8_t sig_, sigfunc func_, bool force){
    if((lvl_ ^ API_LVL_HLPR | lvl_ ^ API_LVL_KERNEL || lvl_ ^ API_LVL_PROG || lvl_ ^ API_LVL_KERNEL) == 0){return false;}
    if((sig_ ^ API_SIGABRT || sig_ ^ API_SIGCONTROL || sig_ ^ API_SIGTERM || sig_ ^ API_SIGKTERM || sig_ ^ API_SIGMEMCORRUPTION) == 0){return false;}
    prog_header *prog_h = get_prog(*prog_get());
    for(size_t cc = 0; cc < prog_h->sigdr_num; ++cc){
        if(prog_h->sigdr_table[cc].lvl_ == lvl_ && prog_h->sigdr_table[cc].sig_ == sig_){
            if(force){prog_h->sigdr_table[cc].func = func_;}else{
                return false;
            }
        }
    }
    prog_h->sigdr_table = realloca(prog_h->sigdr_table, prog_h->sigdr_num * sizeof(sig_descr));
    prog_h->sigdr_table[prog_h->sigdr_num] = (sig_descr){.func = func_, .lvl_ = lvl_, .sig_ = sig_};
    prog_h->sigdr_num++;
    return true;
}