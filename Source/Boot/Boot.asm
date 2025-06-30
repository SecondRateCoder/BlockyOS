org 0x7C00            ; If this is a boot sector
bits 16
global _start
_start:
    LDR   R0, =0x8000      // Load stack top address into R0
    MOV   SP, R0           // Set stack pointer (SP) to R0
    halt

halt:
    jmp .halt

times 510 - ($ - $$) db 0 ;Repeat so the Program can be 512 bytes large.
dw 0xAA55              ; The final 2 bytes will be the boot signature.
