#include "./src/Core/RAM/memtypes.h"

signed int program_load(uint8_t *program, size_t size){
    size_t temp = address_pointfree(0xFF, size);
    if(temp == INVALID_ADDR){return -1;}
    headerMeta_store((header_t){
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
    return 0;
}

void program_run(ID_t ID){
    size_t addr = headerpeeksearch_unsafe(ID, HContextPeekerAttr_Address);
    asm(
        "jmp *%0":
        "r(addr)":
        "memory", "cc"

    );
}