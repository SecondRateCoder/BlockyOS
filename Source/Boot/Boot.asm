org 0x7C00            ; If this is a boot sector
bits 16

start:
    halt

halt:
    jmp .halt

times 510 - ($ - $$) db 0 ;Repeat so the Program can be 512 bytes large.
dw 0xAA55              ; The final 2 bytes will be the boot signature.