#include "local.h"

#define UnresolvedToken 1

Symbol *types;
uint32_t num_types;
Symbol *names;
uint32_t num_names;

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
	defToken(TokenType_Assign, "=", 1, 1),
    defToken(TokenType_Mod, "%", 1, 1),
    defToken(TokenType_Equate, "==", 1, 1),
    defToken(TokenType_GreaterThan, ">=", 1, 1),
    defToken(TokenType_GreaterThan, "=>", 1, 1),
    defToken(TokenType_LessThan, "<=", 1, 1),
    defToken(TokenType_LessThan, "=<", 1, 1),
    defToken(TokenType_NotEqual, "!=", 1, 1),
    defToken(TokenType_NotEqual, "=!", 1, 1),
    defToken(TokenType_TypeDef, "typedef", 4, 1),

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

char **parseBuffer(size_t *count){
	char **out = NULL;
	size_t available = 0;
	(*count) = 0;
	for(uint32_t cc = 0; cc < strlen(buffer); ++cc){
		register char c = buffer[cc];
		if(isalnum(c)){
			uint32_t cc_ = (cc + 1);
			while(cc_ < strlen(buffer) && isalnum(buffer[cc_]){++cc_;}
			if(count == available){
				availble += 5;
				if(available = 0){out = malloc(sizeof(char *) * available);}
				else{out = realloc(out, sizeof(char *) * available);}
			}
			out[count] = malloc((cc_ - cc) * sizeof(c));
			memcpy(out[count], buffer[cc], cc_ - cc);
			cc += cc_ - cc;
		}
	}
}

Symbol *generateSymbol(char **in, size_t *length){
	Symbol *out = calloc(1, sizeof(Symbol);
	out->symbol = strdup(*in);
	size_t symbolcounter = 0;
	foreach(size_t cc = 0; cc < *length; ++cc){
		while(symbolcounter < num_names){
			if(*(names[symbolcounter].symbol) == *(in[cc])){
				if(strcmp(names[symbolcounter].symbol, in[cc]){
					switch(names[cc].type){
						case TokenType_FuncDef: {out->type = TokenType_FuncMention;	return out;}
						case TokenType_GlobalVarDef: {out->type = TokenType_GlobalVarMention;	return out;}
						case TokenType_LocalVarDef: {
							out->type = TokenType_LocalVarMention;
							out->parent = names[symbolcounter].parent;
							return out;
						}
					}
				}
			}
			++symbolcounter;
		}
		symbolcounter = 0;
		while(symbolcounter < num_types){if(!strcmp(*in[cc], types[symbolcounter].symbol){
			out->type = TokenType_TypeMention;
			symbolcounter++;
		}}
		if(out->type == 0){if(cc > 0){if(!strcmp(in[cc], SYNTAX_TYPEDEF)){
			types = realloc(types, sizeof(Symbol) * (num_types++));
			types[num_types - 1] = {
				.symbol = strdup(in[cc]),
				.parent = strdup(g_Parent),
				.type = TokenType_TypeDef
			};
			// Get Properties as full strings, each string is a token.
			size_t tempcounter = 0; uint8_t _5s = 0;
			while(in[cc +  tempcounter] != '}'){
				if((tempcounter % 5) != 0){_5s++;
					types[num_types - 1].tokens = realloc(types[num_types - 1].tokens, _5s * 5 * sizeof(Token));
				}
				// Get byte size and number of items until end of line.
				uint8_t items = 0; size_t blocksize = 0, bytecounter = 0;
				do{items++;		blocksize += strlen(in[items + tempcounter + cc]);
				}while(!strcheck(in[items + tempcounter + cc], SYNTAK_LINEEND) && !strcheck(in[items + tempcounter + cc], SYNTAX_ALTERNATELINEEND));
				types[num_types - 1].tokens[tempcounter] = {.value = malloc(blocksize + items/*For spaces*/), .token = TokenType_Property};
				while(items){
					types[num_types - 1].tokens[tempcounter].value[bytecounter++] = ' ';
					uint8_t charcounter = 0;
					while(in[items + tempcounter + cc][charcounter]){
						types[num_types - 1].tokens[tempcounter].value[bytecounter++] = in[items + tempcounter + cc][charcounter];
					}
					items--;
				}
				tempcounter++;
			}
		}
	 }return out;}
	return out;
}
