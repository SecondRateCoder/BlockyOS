org 0x7C00            ; If this is a boot sector

MEMINFO_ENTRY_SIZE equ 24
MEMINFO_BUFFER     equ 0x8000 ; Place where we store memory map
MEMINFO_MAX_ENTRIES equ 32    ; Max entries to try

start:
    xor bx, bx                 ; Entry counter
    mov di, MEMINFO_BUFFER     ; DI points to start of memory map buffer

    mov ebx, 0                 ; EBX must be 0 for the first call
e820_loop:
    mov eax, 0xE820            ; E820 function
    mov edx, 'SMAP'            ; Magic value 'SMAP'
    mov ecx, MEMINFO_ENTRY_SIZE; Size of buffer (24 bytes)
    mov es, di >> 4            ; ES:DI point to buffer
    mov edi, di & 0xF
    int 0x15
    jc  e820_done              ; Carry set: done
    cmp eax, 'SMAP'
    jne e820_done              ; Not supported
    add di, MEMINFO_ENTRY_SIZE ; Next entry
    inc bx                     ; Increment entry count
    cmp bx, MEMINFO_MAX_ENTRIES
    jae e820_done              ; Prevent overflow
    test ebx, ebx              ; EBX = 0 means last entry
    jnz e820_loop
e820_done:

    ; Now BX = number of entries found
    ; Memory map is at MEMINFO_BUFFER, each entry is 24 bytes

    ; ... Continue with OS boot process ...
    jmp $

times 510-($-$$) db 0  ; Pad boot sector to 512 bytes
dw 0xAA55              ; Boot signature