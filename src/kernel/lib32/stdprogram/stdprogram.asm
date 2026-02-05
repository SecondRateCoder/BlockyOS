bits 32

extern getCODEBase
extern getDATAbase

global _getStandardHeader32
;   standardHeader * getStandardHeader32(void *CODE, void *DATA);
_getStandardHeader32:
    push ebp
    mov ebp, esp
    xor eax, eax
.
    cmp 0x0, [ebp + 8]
    jz .checkData
    mov eax, [ebp + 8]
    call getCODEBase
    jmp .finish
.getData:
    cmp 0x0, [ebp + 12]
    jz .finish
    mov eax, [ebp + 12]
    call getDATABase
.finish:
    leave
    ret

global _getEIP
;   void *getEIP(void)
_getEIP:
	push ebp
	mov ebp, esp
	mov eax, [ebp + 8]
	leave
	ret