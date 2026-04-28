bits 32

global outb
;    void outb(uint16_t port, uint8_t value)
outb:
    push ebp
    mov ebp, esp
    mov dx, [ebp + 8]
    mov al, [ebp + 12]
    out dx, al
    ret

global inb
;    void inb(uint16_t port)
inb:
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret

global outw
;    void outw(uint16_t port, uint16_t value)
outw:
    push ebp
    mov ebp, esp
    mov dx, [ebp + 8]
    mov ax, [ebp + 12]
    out dx, al
    ret

global inw
;    uint16_t inw(uint16_t port)
inw:
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret

global outl
;    void outl(uint16_t port, uint32_t value)
outl:
    push ebp
    mov ebp, esp
    mov dx, [ebp + 8]
    mov eax, [ebp + 12]
    out dx, eax
    ret

global inl
;    uint32_t inl(uint16_t port)
inl:
    mov dx, [esp + 4]
    xor eax, eax
    in eax, dx
    ret

global iowait
UNUSED_PORT			equ 0x80
;   void iowait(void)
iowait:
    push dx
    push ax
    xor ax, ax
    mov dx, UNUSED_PORT
    out dx, ax
    pop ax
    pop dx
	ret