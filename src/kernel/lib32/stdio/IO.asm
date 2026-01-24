bits 32

global _outb
;    void outb(uint16_t port, uint8_t value)
_outb:
    push ebp
    mov ebp, esp
    mov dx, [ebp + 8]
    mov al, [ebp + 12]
    out dx, al
    ret

global _inb
;    void inb(uint16_t port)
_inb:
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret
