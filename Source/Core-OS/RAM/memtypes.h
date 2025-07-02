#include <stdint.h>
#include <stdbool.h>

extern uintptr_t _ram_start, _ram_length, _heap_start, _heap_length;
volatile size_t Pointer;

volatile uint8_t *RAM;
#define RAMMeta hMeta
typedef size_t *hMeta;
#define IDSize 8
#define context_size sizeof(bool)*4 + IDSize*2
#define ID_t ID
#define header_t header
#define hcontext_t headercontext
#define hflags_t headerflags


#pragma pack(1)

typedef struct header{
    size_t size, *addr;
    hcontext_t context;
}header;

typedef struct headercontext{
    ID_t ID;
    size_t addr;
    hflags_t flags;
}headercontext;


typedef struct _ID{uint8_t ProcessID[IDSize], ID[IDSize];}

typedef struct headerflags{
    bool IsProcess, IsThread, IsKernel, IsPrivate;
}headerflags;

void memcpy_unsafe(uint8_t *dest, const size_t offset, const uint8_t *src, const size_t n);