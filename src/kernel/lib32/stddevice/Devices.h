#include "../Public/Publics.h"

extern size_t MAP_MAX;


#define HASH_64BIT_LIMIT 12

typedef struct TOKEN{
    char *word;
    uint128_t token_hash;
    uint32_t index;
}TOKEN;

typedef struct FILE{
    const size_t ID;
    char *buffer;
    const size_t buffer_len;
}FILE;

typedef struct FILE_TOKEN{
    const char *word;
    const uint128_t token;
    const uint8_t type;
    const size_t child_addrlen;
    uint128_t *child_addresses;
}FILE_TOKEN;

#define FILE_SEPERATOR '\\'