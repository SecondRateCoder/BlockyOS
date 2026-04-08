#include "f-rat.h"

char *commands[13] = {
    "Buffer-Create",
    "Buffer-Select",
    "Buffer-Resize",
    "Buffer-Delete",
    "Buffer-Read",
    "Buffer-Write",
    "File-Create",
    "File-Delete",
    "File-Swap",
    "File-Read",
    "File-Write",
    "Directory-Create",
    "Directory-Delete"
};

#define TOGGLE_DUPE(REG) (REG->flags ^= 0x0001)
#define SET_MULTIPLY(REG) (REG->flags |= 0x0002)
#define SET_COMPARE(REG) (REG->flags |= 0x0004)
#define SET_DIVIDE(REG) (REG->flags |= 0x0008)
#define SET_MODULO(REG) (REG->flags |= 0x0010)

#define CLEAR_DUPE(REG) (REG->flags != 0x0001)
#define CLEAR_MULTIPLY(REG) (REG->flags != 0x0002)
#define CLEAR_COMPARE(REG) (REG->flags != 0x0004)
#define CLEAR_DIVIDE(REG) (REG->flags != 0x0008)
#define CLEAR_MODULO(REG) (REG->flags != 0x0010)

#define CHECK_DUPE(REG) (REG->flags & 0x0001)
#define CHECK_MULTIPLY(REG) (REG->flags & 0x0002)
#define CHECK_COMPARE(REG) (REG->flags & 0x0004)
#define CHECK_DIVIDE(REG) (REG->flags & 0x0008)
#define CHECK_MODULO(REG) (REG->flags & 0x0010)
typedef struct Register{
    uint32_t value;
    uint8_t flags;
}__attribute__((packed)) Register;

typedef struct Buffer{
    char *ID;
    void *data;
    size_t size;
}Buffer;

#define MAX_REGS 99

Buffer *buffers;
uint8_t num_buffers;
FILE *file;
Register regs[MAX_REGS];
Register reg_dupe[MAX_REGS];

void main(char **argv, uint32_t argc){
    memset(regs, 0, sizeof(regs));
}


char **parse_tokens(char *input, const char *delims, size_t *count_out) {
    size_t capacity = 8;   // initial capacity
    size_t count = 0;
    char **tokens = malloc(capacity * sizeof(char*));

    char *token = strtok(input, delims);
    while(token != NULL){
        if(count >= capacity){
            capacity *= 2;
            tokens = realloc(tokens, capacity * sizeof(char*));
        }
        tokens[count++] = token;   // store pointer to token
        token = strtok(NULL, delims);
    }

    *count_out = count;
    return tokens;
}

char *str_tolower(char *str){
    for(size_t cc =0; cc < strlen(str); ++cc){str[cc] = tolower(str[cc]);}
    return str;
}

void step(char *line){
    char *copy = strdup(line);
    char *temp = strtok(copy, " ");
	char *ID, *Fail, *Foreach, *Success, *Name, *Path, *Target;
	long Size = 0, Bytes = 0;
	size_t num_tokens = 0;
	char **tokens = parse_tokens(copy, " ", &num_tokens);
    for(uint32_t cc =0; cc < (sizeof(commands)/sizeof(char *)); ++cc){
        if(commands[cc][0] == temp[0]){
            if(strcmp(commands[cc], temp)){
                switch(cc){
                    case 0:	// Buffer-Create
                        for(uint8_t cc_ =0; cc_ < num_tokens; ++cc_){
                            if(*tokens[cc_] == '-'){
                                tokens[cc_][strlen(tokens[cc_++]) - 1] = '\0';
                                if(!strncmp(str_tolower(tokens[cc_] + 1), "id", 2)){
                                    ID = (tokens[cc_] + 1);
                                }else if(!strncmp(str_tolower(tokens[cc_] + 1), "size", 4)){
                                    Size = strtol(tokens[cc_] + 1, NULL, 10);
                                }else if(!strncmp(str_tolower(tokens[cc_] + 1), "fail", 4)){
                                    Fail = (tokens[cc_] + 1);
                                }else if(!strncmp(str_tolower(tokens[cc_] + 1), "foreach", 7)){
                                    Foreach = (tokens[cc_] + 1);
                                }else{printf(ANSI_RED("Invalid flag"));}
                            }
                        }
						buffers = realloc(buffers, (num_buffers++) * sizeof(Buffer));
						for(uint32_t cc_ =0; cc_ < num_buffers; ++cc_){
							if(!strncmp(buffers[cc_].ID, ID, strlen(ID))){
								printf(ANSI_RED("\nFailed to generate Buffer; Buffer already exists, %s"), ID);
								goto finish;
							}
						}
						buffers[num_buffers - 1] = (Buffer){
							.data = malloc(Size),
							.ID = ID,
							.size = Size
						};
						if(!buffers[num_buffers - 1].data){
							printf(ANSI_RED("\nFailed to generate Buffer; Could not allocate %d bytes"), Size);
						}
                        break;
					case 1:	// Buffer-Select
                        for(uint8_t cc_ =0; cc_ < num_tokens; ++cc_){
                            if(*tokens[cc_] == '-'){
                                tokens[cc_][strlen(tokens[cc_++]) - 1] = '\0';
                                if(!strncmp(str_tolower(tokens[cc_] + 1), "id", 2)){
                                    ID = (tokens[cc_] + 1);
                                }else if(!strncmp(str_tolower(tokens[cc_] + 1), "fail", 4)){
                                    Fail = (tokens[cc_] + 1);
                                }else if(!strncmp(str_tolower(tokens[cc_] + 1), "success", 7)){
                                    Success = (tokens[cc_] + 1);
                                }else{printf(ANSI_RED("Invalid flag"));}
                            }
                        }
						uint32_t cc_ =0;
						for(; cc_ < num_buffers; ++cc_){
							if(!strncmp(ID, buffers[cc].ID, strlen(ID))){
								brainfuck_parse(Success, (buffers + cc));
								goto finish;
							}
						}
						printf(ANSI_RED("Could not find Buffer: %s"), ID);
						brainfuck_parse(Fail, NULL);
						break;
					case 2:	// Buffer-Resize
						for(uint8_t cc_ =0; cc_ < num_tokens; ++cc_){
                            if(*tokens[cc_] == '-'){
                                tokens[cc_][strlen(tokens[cc_++]) - 1] = '\0';
                                if(!strncmp(str_tolower(tokens[cc_] + 1), "id", 2)){
                                    ID = (tokens[cc_] + 1);
                                }else if(!strncmp(str_tolower(tokens[cc_] + 1), "fail", 4)){
                                    Fail = (tokens[cc_] + 1);
                                }else if(!strncmp(str_tolower(tokens[cc_] + 1), "success", 7)){
                                    Success = (tokens[cc_] + 1);
                                }else if(!strncmp(str_tolower(tokens[cc_] + 1), "size", 4)){
                                    Size = strtol(tokens[cc_] + 1, NULL, 10);
                                }else{printf(ANSI_RED("Invalid flag"));}
                            }
                        }

						uint32_t cc_ =0;
						for(; cc_ < num_buffers; ++cc_){
							if(!strncmp(ID, buffers[cc].ID, strlen(ID))){
								buffers[cc_].data = realloc(buffers[cc_].data, Size);
								if(buffers[cc_].data){brainfuck_parse(Success, (buffers + cc));}
								goto finish;
							}
						}
						printf(ANSI_RED("Could not find Buffer: %s"), ID);
						brainfuck_parse(Fail, NULL);
						break;
					case 3:	// Buffer-Delete
						for(uint8_t cc_ =0; cc_ < num_tokens; ++cc_){
                            if(*tokens[cc_] == '-'){
                                tokens[cc_][strlen(tokens[cc_++]) - 1] = '\0';
                                if(!strncmp(str_tolower(tokens[cc_] + 1), "id", 2)){
                                    ID = (tokens[cc_] + 1);
                                }else if(!strncmp(str_tolower(tokens[cc_] + 1), "fail", 4)){
                                    Fail = (tokens[cc_] + 1);
                                }else if(!strncmp(str_tolower(tokens[cc_] + 1), "success", 7)){
                                    Success = (tokens[cc_] + 1);
                                }else if(!strncmp(str_tolower(tokens[cc_] + 1), "size", 4)){
                                    Size = strtol(tokens[cc_] + 1, NULL, 10);
                                }else{printf(ANSI_RED("Invalid flag"));}
                            }
                        }

						uint32_t cc_ =0;
						for(; cc_ < num_buffers; ++cc_){
							if(!strncmp(ID, buffers[cc].ID, strlen(ID))){
								buffers[cc_].data = realloc(buffers[cc_].data, Size);
								if(buffers[cc_].data){brainfuck_parse(Success, (buffers + cc));}
								goto finish;
							}
						}
						printf(ANSI_RED("Could not find Buffer: %s"), ID);
						brainfuck_parse(Fail, NULL);
						break;
                }
            }
        }
    }
finish:
	free(copy);	for(uint32_t cc_ =0; cc_ < num_tokens; ++cc_){free(tokens[cc_]);}
	free(tokens);
}

void brainfuck_parse(char *str, Buffer *buffer){
    size_t num_tokens = 0;
    char **tokens = parse_tokens(str, ",:", &num_tokens);
    Register **registers = malloc((num_tokens - 1) * sizeof(Register *));
    Register **prev_run = malloc((num_tokens - 1) * sizeof(Register *));
    for(uint8_t cc = 0; cc < (num_tokens - 1); ++cc){
        long index = strtol(tokens[cc], NULL, 10);
        if(index < 100){
            registers[cc] = (regs + index);
            prev_run[cc] = malloc(sizeof(Register));
			if(*tokens[cc + 1] == '(' && tokens[cc + 1][strlen(tokens[cc + 1]) - 1] == ')' ){
				tokens[cc + 1][strlen(tokens[cc + 1]) - 1] = 0;
				long temp = strtol(tokens[cc + 1], NULL, 10);
				if(temp < 100){*prev_run[cc] = regs[temp];}
				else{printf("Error: Invalid Dupe Reg ID: %d", temp);}
			}
        }
        printf("Error: Invalid Reg ID: %d", index);
    }
    int32_t jmp_base = -1;
    char c = *tokens[num_tokens - 1];
    uint32_t cc = 0;
    for(; cc < strlen(tokens[num_tokens - 1]) && c; ++cc, c = tokens[num_tokens - 1][cc]){
        for(uint32_t cc_ = 0; cc_ < num_tokens - 1 /*Number of registers*/; ++cc_){
            if(USE_DUPE(registers[cc])){
                switch(c){
                    case '+':
                        prev_run[cc_]->value++;
                        break;
                    case '-':
                        prev_run[cc_]->value--;
                        break;
                    case ']':
                        printf("%d", prev_run[cc_]->value);
                        break;
                    case '[':
                        prev_run[cc_]->value = getc(stdin);
                        break;
                    case '>':
                        if(buffer){
                            if(prev_run[cc_]->value > buffer->size){
                                printf(ANSI_RED("\nRegister %d's value %d too large for Buffer size %zu"), 
                                        cc_, prev_run[cc_]->value, buffer->size);
                            }else{printf("%u ", ((uint8_t *)buffer->data)[prev_run[cc_]->value]);}
                        }
                        break;
                    case '<':
                        if(buffer){
                            if(prev_run[cc_]->value > buffer->size){
                                printf(ANSI_RED("\nRegister %d's value %d too large for Buffer size %zu"), 
                                        cc_, prev_run[cc_]->value,buffer->size);
                            }else{((uint8_t *)buffer->data)[prev_run[cc_]->value] = getc(stdin);}
                        }else{printf(ANSI_RED("\nBuffer not attached"));}
                        break;
                    case '_':
                        jmp_base = cc;
                        break;
                    case '^':
						if(prev_run[cc]->value == 0){cc = jmp_base;}
                        break;
                    case '{':
                        registers[cc]->value = prev_run[cc]->value;
                        break;
                    case '}':
                    default:
                        break;
                }
            }else{
                switch(c){
                    case '+':
                        registers[cc_]->value++;
                        break;
                    case '-':
                        registers[cc_]->value--;
                        break;
                    case ']':
                        printf("%d", registers[cc_]->value);
                        break;
                    case '[':
                        registers[cc_]->value = getc(stdin);
                        break;
                    case '>':
                        if(buffer){
                            if(registers[cc_]->value > buffer->size){
                                printf(ANSI_RED("\nRegister %d's value %d too large for Buffer size %zu"), 
                                        cc_, registers[cc_]->value, buffer->size);
                            }else{printf("%u ", ((uint8_t *)buffer->data)[registers[cc_]->value]);}
                        }
                        break;
                    case '<':
                        if(buffer){
                            if(registers[cc_]->value > buffer->size){
                                printf(ANSI_RED("\nRegister %d's value %d too large for Buffer size %zu"), 
                                        cc_, registers[cc_]->value,buffer->size);
                            }else{((uint8_t *)buffer->data)[registers[cc_]->value] = getc(stdin);}
                        }else{printf(ANSI_RED("\nBuffer not attached"));}
                        break;
                    case '_':
                        jmp_base = cc;
                        break;
                    case '^':
                        cc = jmp_base;
                        break;
                    case '*':
                        CLEAR_DUPE(registers[cc]);
                        TOGGLE_DUPE(registers[cc]);
                        SET_MULTIPLY(registers[cc]);
                        cc++;
                        break;
                    case '=':
                        CLEAR_DUPE(registers[cc]);
                        TOGGLE_DUPE(registers[cc]);
                        SET_COMPARE(registers[cc]);
                        cc++;
                        break;
                    case '/':
                        CLEAR_DUPE(registers[cc]);
                        TOGGLE_DUPE(registers[cc]);
                        SET_DIVIDE(registers[cc]);
                        cc++;
                        break;
                    case '}':
                        
                    case '{':
                    default:
                        break;
                }
            }
        }
        if(CHECK_DIVIDE(registers[cc])){
            registers[cc]->value = registers[cc]->value / prev_run[cc]->value;
        }else if(CHECK_MULTIPLY(registers[cc])){
            registers[cc]->value = registers[cc]->value * prev_run[cc]->value;
        }else if(CHECK_COMPARE(registers[cc])){
            registers[cc]->value = registers[cc]->value == prev_run[cc]->value;
        }else if(CHECK_MODULO(registers[cc])){
            registers[cc]->value = registers[cc]->value % prev_run[cc]->value;
        }
    }
}