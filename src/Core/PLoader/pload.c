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
    RAM[addr+decode_size_t(headerpeeksearch_unsafe(ID, HContextPeekerAttr_Size), 0)] = 0XC3;
    int out;
#if defined(__x86_64__) || defined(_M_X64) // For GCC/Clang on Linux/macOS (x86_64) or MSVC (x64)
    // 64-bit assembly
    asm volatile (
        // Step 1: Prepare the function address for the 'call' instruction.
        // Input operand %1 will be in an available 64-bit register (e.g., RDI, RSI, etc.).
        // We will call the address in that register.
        "call *%1\n\t"      // Call the function pointed to by the input register (%1).
                            // The 'call' instruction pushes a 64-bit return address onto the stack.

        // Step 2: After the function returns, its return value will typically be in RAX.
        // Move the content of RAX into our output variable 'returned_value'.
        "movq %%rax, %0"    // Move the 64-bit value from RAX to the output operand %0.

        : "=r" (out) // Output operand: 'returned_value' mapped to %0.
                                // "=r" means writeable register.
        : "r" (addr) // Input operand: 'function_address' mapped to %1.
                                 // "r" means in a general-purpose register.
        : "rax", // Clobber: RAX is used for the return value.
          "rcx", "rdx", "rsi", "rdi", "r8", "r9", // Common clobbered registers by 64-bit calling conventions.
                                                // These are caller-saved in System V ABI.
          "memory",   // Clobber: The called function might modify memory.
          "cc"        // Clobber: Condition codes (flags) might be changed.
    );

#elif defined(__i386__) || defined(_M_IX86) // For GCC/Clang on Linux (i386) or MSVC (x86)
    // 32-bit assembly
    asm volatile (
        // Step 1: Prepare the function address for the 'call' instruction.
        // On 32-bit x86, arguments are typically pushed onto the stack.
        // However, for an indirect call, we usually push the target address
        // or put it in a register, then call that register.
        // For simplicity with '%1' operand, we'll let GCC put it in a register,
        // then call that register.
        "call *%1\n\t"      // Call the function pointed to by the input register (%1).
                            // The 'call' instruction pushes a 32-bit return address onto the stack.

        // Step 2: After the function returns, its return value will typically be in EAX.
        // Move the content of EAX into our output variable 'returned_value'.
        "movl %%eax, %0"    // Move the 32-bit value from EAX to the output operand %0.

        : "=r" (out) // Output operand: 'returned_value' mapped to %0.
        : "r" (addr) // Input operand: 'function_address' mapped to %1.
        : "eax", // Clobber: EAX is used for the return value.
          "ecx", "edx", // Common clobbered registers by 32-bit calling conventions (caller-saved).
          "memory",   // Clobber: The called function might modify memory.
          "cc"        // Clobber: Condition codes (flags) might be changed.
    );

#else
    #error "Unsupported architecture for inline assembly."
#endif
}
/*

1:N Treading structure.

*/