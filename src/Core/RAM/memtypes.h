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
#define hcontext_t headercontext
#define hflags_t headerflags
#define hpeek_t HContextPeekerAttr
#define INVALID_ADDR (size_t)-1
#define CHECK_PROTECT 0xFF
#define DEFAULTSTART 0xF1F1
#define CONTEXTEXTRAS_SIZE 55

//(2^120)-1 representable numbers.
#define MAXINDEXERSIZE 15

typedef struct headercontext{
    ID_t ID;
    size_t size, addr;
    uint8_t checks;
    hflags_t flags;
    uint8_t Extras[CONTEXTEXTRAS_SIZE];
}headercontext;

typedef struct _ID{uint8_t ProcessID[IDSize], HeaderID[IDSize];}_ID;

typedef struct headerflags{
    bool IsProcess, IsThread, IsKernel, IsPrivate, IsStream;
}headerflags;

typedef enum HContextPeekerAttr{
    HContextPeekerAttr_ProcessID =0x11,
    HContextPeekerAttr_HeaderID =0x12,
    HContextPeekerAttr_Address =0x13,
    HContextPeekerAttr_Size =0x14,
    HContextPeekerAttr_IsProcess =0x15,
    HContextPeekerAttr_IsThread =0x16,
    HContextPeekerAttr_IsKernel =0x17,
    HContextPeekerAttr_IsPrivate =0x18,
    HContextPeekerAttr_IsStream =0x19,
    HContextPeekerAttr_Extras =0x1A,
}HContextPeekerAttr;

void memcpy_unsafe(uint8_t *dest, const size_t offset, const uint8_t *src, const size_t n);
void memmove_unsafe(uint8_t *dest, size_t size, size_t offset, uint8_t *src, size_t size_);
uint8_t *slice_bytes(uint8_t *src, size_t start, size_t Length);
void memclear(uint8_t *dest, size_t offset, size_t length);
uint8_t *define_hid();
uint8_t *define_pid();


size_t header_encode(uint8_t *array, header_t h);
size_t context_encode(uint8_t *array, const header_t h);
void headerMeta_store(const header_t H);
uint8_t *headerpeek_unsafe(size_t addrInMeta, hpeek_t peeker);
uint8_t *headerpeeksearch_unsafe(ID_t ID, hpeek_t peeker);
bool Metaaddress_validate(size_t maddr);
bool address_validate(size_t addr);
bool space_validate(size_t address, size_t concurrent_size);
int headers_underprocess(uint8_t ProcessID[IDSize]);
size_t memorysize_underprocess(uint8_t ProcessID[IDSize]);
size_t address_pointfree(size_t startfrom, size_t concurrent_size);
