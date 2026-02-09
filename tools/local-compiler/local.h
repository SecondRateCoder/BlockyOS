#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define MAXREAD 512
#define STANDARDSPLITTER " \"[](){}"
#define NUMTOKENS 18

#define defToken(type, value, proreq, prereq) {     \
    type,                                           \
    value,                                          \
    proreq,                                         \
    prereq                                          \
}

typedef enum TokenType{
    TokenType_Undefined,
    TokenType_Error,

    TokenType_Terminate,
    TokenType_LineEnd,
    TokenType_InlineFuncCall,
    TokenType_UndefinedReturn,
    TokenType_NoValueReturn,
    TokenType_ValueReturn,
    TokenType_Add,
    TokenType_Sub,
    TokenType_Mul,
    TokenType_Div,
    TokenType_Mod,
    TokenType_Equate,
    TokenType_GreaterThan,
    TokenType_LessThan,
    TokenType_NotEqual,
    TokenType_TypeDef,

    TokenType_Value_Ptr,
    TokenType_Value_Deref,

    TokenType_FuncCall,
    TokenType_FuncParam,
    TokenType_FuncArg,
    TokenType_TypeMention,
    TokenType_ParamMention,
    TokenType_Value,
    TokenType_Value_Literal,
}TokenType;

typedef struct Symbol{
    TokenType type;
    char *symbol;
}Symbol;

typedef struct Token{
    TokenType type;
    char *value;
    uint8_t pro_requisite;
    uint8_t pre_requisite;
}Token;

uint32_t setglobal(FILE *in);
char *getsnip(char *in, char *splitters, char *otherwise, bool forcesame);
bool strcheck(char c, char *str);
bool isnum(char *str);