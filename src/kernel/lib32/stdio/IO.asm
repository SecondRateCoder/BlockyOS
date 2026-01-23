bits 32

global _outb
;    void outb(uint16_t port, uint8_t value)
global _outb:
    push edx
    push eax
    push ebp
    mov ebp, esp
    mov dx, [ebp + 8]
    mov al, [ebp + 12]
    outb dx, al
    pop eax
    pop edx
    ret

global _inb
;    void inb(uint16_t port)
global _inb:
    mov dx, [esp + 4]
    xor eax, eax
    inb al, dx
    ret
