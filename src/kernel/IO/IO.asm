global _outl
; outl: void outl(unsigned short port, unsigned int val)
_outl:
    mov edx, [esp+4]   ; port
    mov eax, [esp+8]   ; value
    out dx, eax
    ret

global _inl
; inl: unsigned int inl(unsigned short port)
_inl:
    mov edx, [esp+4]   ; port
    in eax, dx
    ret
