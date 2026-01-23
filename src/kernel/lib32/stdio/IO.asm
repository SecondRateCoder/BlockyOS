bits 32

global _outb
;    void outb(uint16_t port, uint8_t value)
global _outb:
    mov dx, [esp + 4]
    mov al, [esp + 5]
    outb dx, al
    ret

global _outb
;    void outb(uint16_t port)
global _outb:
    mov dx, [esp + 4]
    xor eax, eax
    inb al, dx
    ret
