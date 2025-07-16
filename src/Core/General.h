#include <src/Public/stdint.h>
#include <src/Public/stdbool.h>
#include <src/Public/Publics.h>

#define IDSize 8
#define CHECK_PROTECT 0xFF
#define DEFAULTSTART 0xF1F1
#define CONTEXTEXTRAS_SIZE 55
#define INVALID_ADDR (size_t)-1
#define _ram_end _ram_length + _ram_start
#define context_size sizeof(bool)*4 + IDSize*2 + sizeof(size_t)*2
#define metanumber_inblocks (_ram_length - *RAMMeta)/context_size
#define DWORD uint32_t
#define BIT8_MASK (uint8_t)0xFF
#define BIT16_MASK (uint16_t)0xFFFF

//The Program ID will be absolute zero.
#define KERNEL_ID (uint8_t[IDSize]){0}

void outl(uint16_t port, uint32_t value);
uint32_t inl(uint16_t port);

void *alloca(size_t Size, uint8_t ProcessID[IDSize]);
void dealloca_unsafe(void *header);