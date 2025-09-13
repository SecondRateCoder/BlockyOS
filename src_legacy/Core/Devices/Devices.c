#include "../Devices.h"

//!CHANGE TO BE MORE MALLEABLE
size_t MAP_MAX = 2;



void link_device(int Device_config[2]){return;}

FILE *fopen(char *path){
    TOKEN *tokens = full_tokenise(path, strlen(path), FILE_SEPERATOR);
    for(size_t cc =0; cc < )
}

FILE_TOKEN *fmeta_point(uint128_t token, uint8_t map){
    //Read off the FILE metadata at a specific map.
    //Should start from the 0 address on the drive
    return NULL;
}

/*
    File metadata is stored in the format of
    a str name,
    a uint128_t ID,
    a uint8_t encoded type specifier,
    A uint128_t File address(or different depending on the Drive type),
    (If Directory), A list of uint128_t addresses, (If file) A uint128_t pointer.
*/
   
bool fvalidate(FILE_TOKEN *PTR){
    //Check the copy of the Drive Metadata for the existence of thge file,
    //Check the Drive data for the existence of this file, in the input token's faddr.
    return true;
}

FILE_TOKEN *fmeta_read(uint128_t token, uint8_t map){
    FILE_TOKEN *PTR = fmeta_point(token, map);
    if(fvalidate(PTR) == true){return PTR;}else{
        for(size_t cc =0; cc < MAP_MAX; ++cc){
            const FILE_TOKEN *fmeta_point(token, cc);
        }
        //Check the copy drive metadatas.
    }
}


//Temporarily here

TOKEN *full_tokenise(char *str, size_t len, char breaker){
    TOKEN *tokens = tokenise(str, len, breaker);
    for(size_t cc =0; cc < memsize(tokens); ++cc){
        const size_t *_temp = str_hash(tokens[cc].word);
        tokens[cc].token_hash[0] = _temp[0];
        tokens[cc].token_hash[1] = _temp[1];
    }
    return tokens;
}

TOKEN *tokenise(char *str, size_t len, char breaker){
    TOKEN *list = alloca(sizeof(TOKEN)* 1);
    uint32_t list_cc = 0, list_len = 1;
    for(size_t cc =0; cc < strlen(str); ++cc){
        size_t c_ = 1;
        if(str[cc] != ' '){for(;c_ < strlen(str) && str[c_ + cc] != breaker; ++c_);}
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
