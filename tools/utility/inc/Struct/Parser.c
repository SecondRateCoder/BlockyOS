#include "Struct32.h"

/// @brief Encrypt A ptr for usage.
/// @param ptr The ptr to encrypt.
/// @return The encrypted ptr.
/// @remark ll->hl, ll->hh, hh->lh, hl->lh
ptrEncrypt *encryptPTR(void *ptr, uint16_t seed){
    ptrEncrypt *out = calloc(1, sizeof(ptrEncrypt));
    out->ptr = ptr;
    out->seed = seed;
    while(seed){
        // Isolate low 2 bits
        switch(seed & 0x3){
            case SHL: {
                uint16_t temp = out->hh;
                out->hh = out->hl;
                out->hl = out->lh;
                out->lh = out->ll;
                out->ll = temp;
                break;
            } case SHR: {
                uint16_t temp = out->ll;
                out->ll = out->lh;
                out->lh = out->hl;
                out->hl = out->hh;
                out->hh = temp;
                break;
            } case DWORDSWAP: {
                uint16_t temp[2] = {out->lh, out->ll};
                out->lh = out->hh;
                out->ll = out->hl;
                out->hh = temp[0];
                out->hl = temp[1];
                break;
            } case HIGHSWAP: {
                uint16_t temp = out->hh;
                out->hh = out->hl;
                out->hl = temp;
                break;
            } case LOWSWAP: {
                uint16_t temp = out->lh;
                out->lh = out->ll;
                out->ll = temp;
                break;
            }
        }
        seed = seed >> 2;
    }
    return out;
}

/// @brief Decrypt A ptr for usage.
/// @param ptr The ptr to decrypt.
/// @return The decrypted ptr.
/// @remark ll->hl, ll->hh, hh->lh, hl->lh
void *decryptPTR(ptrEncrypt *ptr){
    uint16_t seed = ptr->seed;
    while(seed){
        // Isolate high 2 bits
        switch((seed & 0xC0) >> 14){
            case SHL: {
                uint16_t temp = ptr->ll;
                ptr->ll = ptr->lh;
                ptr->lh = ptr->hl;
                ptr->hl = ptr->hh;
                ptr->hh = temp;
                break;
            } case SHR: {
                uint16_t temp = ptr->hh;
                ptr->hh = ptr->hl;
                ptr->hl = ptr->lh;
                ptr->lh = ptr->ll;
                ptr->ll = temp;
                break;
            } case DWORDSWAP: {
                uint16_t temp[2] = {ptr->lh, ptr->ll};
                ptr->lh = ptr->hh;
                ptr->ll = ptr->hl;
                ptr->hh = temp[0];
                ptr->hl = temp[1];
                break;
            } case HIGHSWAP: {
                uint16_t temp = ptr->hh;
                ptr->hh = ptr->hl;
                ptr->hl = temp;
                break;
            } case LOWSWAP: {
                uint16_t temp = ptr->lh;
                ptr->lh = ptr->ll;
                ptr->ll = temp;
                break;
            }
        }
        seed = seed << 2;
    }
    void *decrypted = ptr->ptr;
    *ptr = *(encryptPTR(ptr->ptr, ptr->seed));
    return decrypted;
}

/// @brief Parse a single line and populate sustain.
/// @param in The line to parse
/// @param sustain A ptr that NUST be maintained.
/// @remark .<PARAM #0>.<PARAM #N>:\'[ENCODING]|\'[INIT VALUE], ...
/// @return The Generated node for the Line
node *parse(char *in, void **sustain){
    if(*sustain == NULL){
        *sustain = calloc(1, sizeof(ptrEncrypt));
        encryptPTR(TCreate(32, true), rand());
    }
    ptrEncrypt *encrypted = (ptrEncrypt *)*sustain;
    treeroot *tree = decryptPTR(encrypted);

    const char delims[] = ".:|";
    for(size_t cc = 0; cc < strlen(in); ++cc){
        char *token = 0;
        char tokout;
        if(token = strchecksp(in, delims, &tokout)){
            switch(tokout){
                case '.': {

                }
            }
        }
    }
}

treeroot *releasetree(void **sustain){
    void *out = decryptPTR(*sustain);
    free(*sustain);
    return out;
}