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

global _outw
;    void outw(uint16_t port, uint16_t value)
_outw:
    push ebp
    mov ebp, esp
    mov dx, [ebp + 8]
    mov ax, [ebp + 12]
    out dx, al
    ret

global _inw
;    uint16_t inw(uint16_t port)
_inw:
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret

global _outl
;    void outl(uint16_t port, uint32_t value)
_outl:
    push ebp
    mov ebp, esp
    mov dx, [ebp + 8]
    mov eax, [ebp + 12]
    out dx, eax
    ret

global _inl
;    uint32_t inl(uint16_t port)
_inl:
    mov dx, [esp + 4]
    xor eax, eax
    in eax, dx
    ret

global _iowait
UNUSED_PORT			equ 0x80
;   void iowait(void)
_iowait:
    push dword UNUSED_PORT
	push dword 0
	call _outb
	ret