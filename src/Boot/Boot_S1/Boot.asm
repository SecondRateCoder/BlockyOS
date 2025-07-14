; Not implemented, will do later.

org 0x7C00
bits 16

; nasm boot.asm -o boot.bin
; partcopy boot.bin 0 200 -f0

global _start
_start:
    xor ax, ax 
    LDR R0, 0x80000
    mov  cr0, eax
    jmp 0x8:pmode
pmode:
    mov  bx, 0x10          ; select descriptor 2, instead of 1
    mov  ds, bx            ; 10h = 10000b

    and al,0xFE            ; back to realmode
    mov  cr0, eax          ; by toggling bit again
    jmp 0x0:huge_unreal
huge_unreal:

   ...                    ;As before

gdtinfo:
   dw gdt_end - gdt - 1   ;last byte in table
   dd gdt                 ;start of table

gdt         dd 0,0        ; entry 0 is always unused
flatcode    db 0xff, 0xff, 0, 0, 0, 10011010b, 10001111b, 0
flatdata    db 0xff, 0xff, 0, 0, 0, 10010010b, 11001111b, 0


times 510 - ($ - $$) db 0 ;Repeat so the Program can be 512 bytes large.
dw 0xAA55              ; The final 2 bytes will be the boot signature.