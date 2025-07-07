#include "./Source/Public/Publics.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

extern uintptr_t _ram_start, _ram_length, _heap_start, _heap_length;
volatile size_t Pointer;

volatile uint8_t *RAM;
uint8_t *RAMMeta;
#define IDSize 8
#define _ram_end _ram_length + _ram_start
#define context_size sizeof(bool)*4 + IDSize*2 + sizeof(size_t)*2
#define metanumber_inblocks (_ram_length - *RAMMeta)/context_size
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
    uint8_t checks;
    hflags_t flags;
}headercontext;

typedef struct _ID{uint8_t ProcessID[IDSize], HeaderID[IDSize];}_ID;

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
size_t context_encode(uint8_t *array, const header_t h);
void headerMeta_store(const header_t H);
uint8_t *headerpeek_unsafe(size_t addrInMeta, hpeek_t peeker);
bool Metaaddress_validate(size_t maddr);
bool address_validate(size_t addr);
bool space_validate(size_t address, size_t concurrent_size);
int headers_underprocess(uint8_t ProcessID[IDSize]);
size_t memorysize_underprocess(uint8_t ProcessID[IDSize]);
size_t address_pointfree(size_t startfrom, size_t concurrent_size);

