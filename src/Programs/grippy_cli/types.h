#include "./src/Public/Publics.h"
// #include "./src/Programs/grippy_cli/types.h"

#define tokenlist_t TokenList
typedef struct TokenList{
    char *word;
    TokenList tL;
}TokenList;