org 0x7C00
bits 16

gloabal _start
_start:
    LDR R0, 0x80000

times 510 - ($ - $$) db 0 ;Repeat so the Program can be 512 bytes large.
dw 0xAA55              ; The final 2 bytes will be the boot signature.
