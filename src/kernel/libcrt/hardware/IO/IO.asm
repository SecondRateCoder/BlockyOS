bits 64
section .text

global __outb
; eax(ret)__outb(rdi(port), rsi(value))
__outb:
    mov dx, di
    mov ax, si
    out dx, al
    mov rax, 1
    ret

global __inb
; eax(ret)__inb(rdi(port))
__inb:
    xor rax, rax   ; Clear upper bits to return only the byte
    mov dx, di
    in al, dx
    ret

global __outw
; eax(ret)__outw(rdi(port), rsi(value))
__outw:
    mov dx, di
    mov ax, si
    out dx, ax
    mov rax, 1
    ret

global __inw
; eax(ret)__inw(rdi(port))
__inw:
    xor rax, rax
    mov dx, di
    in ax, dx
    ret

global __outl
; eax(ret)__outl(rdi(port), rsi(value))
__outl:
    mov dx, di
    mov eax, esi
    out dx, eax
    mov rax, 1
    ret

global __inl
; eax(ret)__inl(rdi(port))
__inl:
    mov dx, di
    in eax, dx
    ret

	
global __outsb
; eax(ret)__outsb(rdi(port), rsi(out), rdx(count))
__outsb:
    mov rcx, rdx
    mov dx, di
    rep outsb
    mov rax, 1
    ret

global __insb
; eax(ret)__insb(rdi(port), rsi(out), rdx(count))
__insb:
    mov rcx, rdx
    mov dx, di
    mov rdi, rsi
    rep insb
    mov rax, 1
    ret

global __outsw
; eax(ret)__outsw(rdi(port), rsi(out), rdx(count))
__outsw:
    mov rcx, rdx
    mov dx, di
    rep outsw
    mov rax, 1
    ret

global __insw
; eax(ret)__insw(rdi(port), rsi(out), rdx(count))
__insw:
    mov rcx, rdx
    mov dx, di
    mov rdi, rsi
    rep insw
    mov rax, 1
    ret

global __outsl
; eax(ret)__outsl(rdi(port), rsi(out), rdx(count))
__outsl:
    mov rcx, rdx
    mov dx, di
    rep outsd
    mov rax, 1
    ret

global __insl
; eax(ret)__insl(rdi(port), rsi(out), rdx(count))
__insl:
    mov rcx, rdx
    mov dx, di
    mov rdi, rsi
    rep insd
    mov rax, 1
    ret