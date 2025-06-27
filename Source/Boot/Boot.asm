org 0x7C00            ; If this is a boot sector
bits 16

start:
    halt

halt:
    jmp .halt

times 510 - ($ - $$) db 0
dw 0xAA55              ; Boot signature