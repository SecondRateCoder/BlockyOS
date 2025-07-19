#include "./src/Programs/grippy_cli/types.h"

void handle_tokenlist(tokenlist *tL){
    /*
    Format in:
        Command|Executable path|PATH vatriable,
        OS-controlled arguments (Arguments that are inputted by the User but handled by the OSe e.g Raise Priviledge to MAX, Display Binaries e.t.c),
        "Command|Executable path|PATH vatriable"- dependent arguments.
    */
}

tokenlist_t *parse_with(char *string, size_t len, char parse_id){
    int cc =0, cc_ =0, numofwords =0;
    tokenlist_t out;
    while(cc < len){
        cc_ = 0;
        if(string[cc] != parse_id){
            cc_ = cc;
            while(string[cc_] != parse_id){++cc_;}
        }
        if(cc_ > cc){
            ++numofwords;
            char *str = (char *)alloca(cc_ - cc, 0);
            const int len = cc_ - cc;
            while(cc < cc_){
                str[cc] = string[cc];
                ++cc;
            }
            place_word(&out, str, len, numofwords);
        }
        ++cc;
    }
}

void place_word(tokenlist_t *in, char *str, size_t strlen, int depth){
    if(dept != 0){place_word(&in->tL, str, strlen, depth-1);}
    in->word = str;
    return;
}