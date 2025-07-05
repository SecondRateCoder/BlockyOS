#include "./Source/Public/Publics.h"
#include <stdint.h>
#include <stdbool.h>

extern uintptr_t _ram_start, _ram_length, _heap_start, _heap_length;
volatile size_t Pointer;

volatile uint8_t *RAM;
size_t *RAMMeta;
#define IDSize 8
#define context_size sizeof(bool)*4 + IDSize*2 + sizeof(size_t)*2
#define ID_t _ID
#define header_t header
#define hcontext_t headercontext
#define hflags_t headerflags
#define hpeek_t HContextPeekerAttr

#pragma pack(1)
typedef struct header{
    size_t size, *addr;
    hcontext_t context;
}header;
#pragma pop()

typedef struct headercontext{
    ID_t ID;
    hflags_t flags;
}headercontext;

typedef struct _ID{uint8_t ProcessID[IDSize], HeaderID[IDSize];}

typedef struct headerflags{
    bool IsProcess, IsThread, IsKernel, IsPrivate;
}headerflags;

typedef enum HContextPeekerAttr{
    HContextPeekerAttr_HeaderID =0xF1,
    HContextPeekerAttr_ProcessID =0xF2,
    HContextPeekerAttr_Address =0xF3,
    HContextPeekerAttr_Size =0xF4,
    HContextPeekerAttr_IsProcess =0xF5,
    HContextPeekerAttr_IsThread =0xF6,
    HContextPeekerAttr_IsKernel =0xF7,
    HContextPeekerAttr_IsPrivate =0xF8,
}HContextPeekerAttr;

void memcpy_unsafe(uint8_t *dest, const size_t offset, const uint8_t *src, const size_t n);
void memmove_unsafe(uint8_t *dest, size_t size, size_t offset, uint8_t *src, size_t size_);
uint8_t *slice_bytes(uint8_t *src, size_t start, size_t Length);

size_t header_encode(uint8_t *array, header_t h);
size_t context_encode(uint8_t *array, const context_t c);
void headerMeta_store(const header_t H);
uint8_t *headerpeek_unsafe(size_t addrInMeta, hpeek_t peeker);
bool Metaaddress_validate(size_t maddr);
bool address_validate(size_t addr);
