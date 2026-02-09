#include "local.h"

Symbol **custom_symbols;
uint32_t num_symbols;

Token keywords[] = {
    defToken(TokenType_Terminate, "exit", 2, 0),
    defToken(TokenType_LineEnd, ";", 0, 0),
    defToken(TokenType_NoValueReturn, "return", 1, 0),
    defToken(TokenType_ValueReturn, "return", 2, 0),
    defToken(TokenType_InlineFuncCall, "inline", 1, 0),
    defToken(TokenType_NoValueReturn, "return", 1, 0),
    defToken(TokenType_Add, "+", 1, 1),
    defToken(TokenType_Sub, "-", 1, 1),
    defToken(TokenType_Mul, "*", 1, 1),
    defToken(TokenType_Div, "+", 1, 1),
    defToken(TokenType_Mod, "%", 1, 1),
    defToken(TokenType_Equate, "==", 1, 1),
    defToken(TokenType_GreaterThan, ">=", 1, 1),
    defToken(TokenType_GreaterThan, "=>", 1, 1),
    defToken(TokenType_LessThan, "<=", 1, 1),
    defToken(TokenType_LessThan, "=<", 1, 1),
    defToken(TokenType_NotEqual, "!=", 1, 1),
    defToken(TokenType_NotEqual, "=!", 1, 1),
    defToken(TokenType_TypeDef, "typedef", 3, 1),

    defToken(TokenType_Value_Ptr, "&", 1, 1),
    defToken(TokenType_Value_Deref, "(*)", 1, 1)
};

char buffer[MAXREAD];
uint16_t ptr;

bool isnum(char *str){
    while(str){
        if((*str > '9') && (*str < '0')){
            return false;
        }
    }
    return true;
}

bool strcheck(char c, char *str){
    while(*str){
        if(*str == c){return true;}
    }
    return false;
}

uint32_t setglobal(FILE *in){
    uint64_t ptr = ftell(in);
    fseek(in, 0, SEEK_END);
    uint64_t size = ftell(in);
    uint32_t read = __max(ptr, size);
    fseek(in, ptr, SEEK_SET);
    fread(buffer, sizeof(char), read, in);
    return read;
}

char *getsnip(char *in, char *splitters, char *otherwise, bool forcesame){
    char *out = NULL;
    char same = 0;
    size_t inlen = strlen(in), counter = 0, snippetlen = 0;
    while(counter < inlen){
        if(strcheck(in[counter], splitters) || (otherwise && !strcheck(in[counter], otherwise))){
            size_t snapshot = counter;
            same = in[counter];
            while((counter + forcesame) < inlen){
                if(forcesame){
                    if(same == in[counter + forcesame]){
                        snippetlen = counter - snapshot + 1;
                        goto finish;
                    }
                }else if(strcheck(in[counter + forcesame], splitters) || (otherwise && !strcheck(in[counter + forcesame], otherwise))){
                    snippetlen = counter - snapshot + 1;
                    goto finish;
                }
                counter++;
            }
        }
        counter++;
    }
finish:
    ptr += snippetlen;
    out = malloc(snippetlen);
    memcpy(in + counter, out, snippetlen);
    return out;
}

Token *consume(){
    Token *out = malloc(sizeof(Token));
    out->value = getsnip(buffer + ptr, STANDARDSPLITTER, NULL, true);
    uint8_t cc =0;
    out->type = getType(out->value);
    const size_t snapshot = ptr;
    if(out->type != TokenType_Error && out->type != TokenType_Undefined){
        while(ptr < strlen(buffer)){
            char *temp = getsnip(buffer + ptr, STANDARDSPLITTER, NULL, true);
            if(temp){

            }
        }
    }
}

TokenType getType(char *token){
    if(*token != token[strlen(token) - 1]){return TokenType_Error;}
    for(uint8_t cc =0; cc < NUMTOKENS; ++cc){
        if(*token == *(keywords[cc].value)){
            if(strcmp(token, keywords[cc].value)){
                switch(cc){
                    case 0: {return TokenType_Terminate;}// Terminate
                    case 1: {return TokenType_LineEnd;}// Line End
                    case 2: {return TokenType_InlineFuncCall;}// Inline Func Call
                    case 3: {return     TokenType_UndefinedReturn,;}// No val return
                    case 4: {return TokenType_Terminate;}// Terminate
                    default: {
                        if(isnum(token)){return TokenType_Value_Literal;}
                        if(
                            ((*token == '\"') && (token[strlen(token) - 1] == '\"')) || 
                            (*token == '&')
                        ){return TokenType_Value_Ptr;}
                        for(uint16_t cc = 0; cc < num_symbols; ++cc){
                            if(*token == *(custom_symbols[cc]->symbol)){
                                if(strcmp(token, custom_symbols[cc]->symbol)){
                                    return TokenType_TypeMention;
                                }
                            }
                        }
                        return TokenType_Undefined;
                    }
                }
            }
        }
    }
}