typedef struct TOKEN{
    char *word;
    uint128_t token_hash;
    uint32_t index;
}TOKEN;

typedef struct FILE_TOKEN{
    const char *word;
    const uint128_t token;
    const uint8_t type;
    const size_t child_addrlen;
    uint128_t *child_addresses;
}FILE_TOKEN;

#define FILE_SEPERATOR '\\'

void link_device(int Device_config[2]){return;}

FILE *fopen(char *path){
    TOKEN *tokens = full_tokenise(path, strlen(path), FILE_SEPERATOR);
    
}
/*
    File metadata is stored in the format of
    a str name,
    a uint128_t ID,
    a uint8_t encoded type specifier,
    A uint128_t File address(or different depending on the Drive type),
    (If Directory), A list of uint128_t addresses, (If file) A uint128_t pointer.
*/
FILE_TOKEN fmeta_read(uint128_t token){
    FILE_TOKEN PTR = fmeta_point(token);
    fvalidate(PTR);
}

bool fvalidate(FILE_TOKEN *PTR){
    //Check the copy of the Drive Metadata for the existence of thge file,
    //Check the Drive data for the existence of this file, in the input token's faddr.
}

//Temporarily here

TOKEN *full_tokenise(char *str, size_t len, char breaker){
    TOKEN *tokens = tokenise(str, len, breaker);
    for(size_t cc =0; cc < memsize(tokens); ++cc){
        tokens[cc].token_hash = str_hash(tokens[cc].word);
    }
}

TOKEN *tokenise(char *str, size_t len, char breaker){
    TOKEN *list = alloca(sizeof(TOKEN)* 1);
    uint32_t list_cc = 0, list_len = 1;
    for(size_t cc =0; cc < strlen(str); ++cc){
        if(str[cc] != ' '){for(size_t c_ = 1; c_ < strlen(str) && str[c_ + cc] != breaker; ++c_);}
        list[list_cc].word = malloc(sizeof(char)* c_);
        memcpy(&str[cc], list[list_cc].word, c_);
        list[list_cc].index = list_cc;
        list = realloca(list, sizeof(TOKEN)* list_len++);
        list_cc++;
        
    }
}

size_t* str_hash(const char *str) {
    if(!str){return NULL;}
    size_t *hash = malloc(sizeof(uint128_t));
    if(!hash){return NULL;}
    hash[0] = 5381;
    hash[1] = 0;
    size_t cc = 0;
    int c;
    while((c = *str++)){
        ++cc;
        if(cc > HASH_64BIT_LIMIT){hash[1] = ((hash[1] << 5) + hash[1]) + c;
        }else{hash[0] = ((hash[0] << 5) + hash[0]) + c;}
    }
    return hash;
}
