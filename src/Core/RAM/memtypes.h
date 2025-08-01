#include <./src/Public/Publics.h>
#include <./src/Core/General.h>

extern uintptr_t _ram_start, _ram_length, _heap_start, _heap_length;
volatile size_t Pointer;

volatile uint8_t *RAM;
volatile uint8_t *RAMMeta;

#define INFO(context) (uint8_t)((context.Extras >> 50) & BIT5_MASK)

#define ID_t _ID
#define hcontext_t headercontext
#define hflags_t headerflags
#define hpeek_t HContextPeekerAttr
#define bound_t Bounds

#define STREAM_IDT(S) if(is_set(S, DEVICE_ID)){set(S, DEVICE_ID, 0);}else{set(S, DEVICE_ID, 1);}
#define DEVICE_ID 5
#define STREAM_IDT(S) if(is_set(S, STREAM_ID)){set(S, STREAM_ID, 0);}else{set(S, STREAM_ID, 1);}
#define STREAM_ID 6
#define EXTENSION_IDT(S) if(is_set(S, EXTENSION_ID)){set(S, EXTENSION_ID, 0);}else{set(S, EXTENSION_ID, 1);}
#define EXTENSION_ID 7
#define MAXMEM_MOVE 1024*50

typedef struct Bounds{uint8_t *addr; ssize_t offset;}Bounds;

//(2^120)-1 representable numbers.
#define MAXINDEXERSIZE 15
typedef struct headercontext{
    ID_t ID;
    size_t addr, size;
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
    HContextPeekerAttr_Checks =0x15,
    HContextPeekerAttr_IsProcess =0x16,
    HContextPeekerAttr_IsThread =0x17,
    HContextPeekerAttr_IsKernel =0x18,
    HContextPeekerAttr_IsPrivate =0x19,
    HContextPeekerAttr_IsStream =0x20,
    HContextPeekerAttr_Extras =0x21,
}HContextPeekerAttr;

void memcpy_unsafe(uint8_t *dest, const size_t offset, const uint8_t *src, const size_t n);
void memmove_unsafe(uint8_t *dest, size_t size, size_t offset, uint8_t *src, size_t size_);
void mem_displace(uint8_t *src, size_t blockaddr, const size_t blocksize, const ssize_t displacement);
void memclear(uint8_t *dest, size_t offset, size_t length);
uint8_t *slice_bytes(uint8_t *src, size_t start, size_t Length);
uint8_t *define_pid();
size_t memsize(void *ptr);

size_t hcontext_encode(uint8_t *array, const hcontext_t h);
void hcontext_store(const hcontext_t H);
size_t get_hcontextaddr(size_t ramaddress);
void hcontext_attrwrite(uint8_t *data, ID_t ID, hpeek_t peeker);
void hcontext_attrwrite_unsafe(uint8_t *data, hpeek_t peeker, size_t hcontextaddr);
void direct_extraswrite(size_t hcontextaddr, uint8_t value, uint8_t index);
uint8_t *hcontext_attrpeekh(const uint8_t hereditary_id[IDSize], const hpeek_t peeker, const hflags_t flags, const hflags_t ignore);
uint8_t *hcontext_attrpeek(ID_t ID, hpeek_t peeker);
uint8_t *hcontext_attrpeek_unsafe(size_t hcontextaddr, hpeek_t peeker);
int headers_underprocess(uint8_t ProcessID[IDSize]);
size_t memorysize_underprocess(uint8_t ProcessID[IDSize]);
bool address_validate(size_t addr);
bool space_validate(size_t address, size_t concurrent_size);
size_t address_pointfree(size_t startfrom, size_t concurrent_size);
