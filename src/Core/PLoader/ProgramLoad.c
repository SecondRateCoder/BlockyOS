#include "./src/Core/PLoader/Programs.h"

signed int program_load(uint8_t *program, size_t size){
    size++;
    size_t temp = address_pointfree(0xFF, size);
    if(temp == INVALID_ADDR){return -1;}
    headerMeta_store((hcontext_t){
        .ID = (ID_t){
            .ProcessID = define_pid(),
            .HeaderID = 0x00
        },
        .size = size,
        .addr = temp,
        .checks = CHECK_PROTECT,
        .flags = (hflags_t){
            true,
            false,
            false,
            true
        }
    });
    memcpy_unsafe(RAM, temp, program, size);
    RAM[temp+size] = 0XC3;
    return 0;
}

void program_run(ID_t ID){
    size_t addr = headerpeeksearch_unsafe(ID, HContextPeekerAttr_Address);
    asm volatile ( // Added 'volatile'
    "jmp *%0"
    : // No output operands for jmp
    : "r"(addr) // Input: addr in a general-purpose register
    : "memory", "cc" // Clobbers: potentially all memory and condition codes
    );
    // asm(
    //     "jmp *%0":
    //     "r(addr)":
    //     "memory", "cc"

    // );
}
/*

1:N Treading structure.

*/