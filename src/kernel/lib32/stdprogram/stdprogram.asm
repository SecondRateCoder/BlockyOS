bits 32

extern getCODEBase, getDATABase

global getStanderdHeader32
;   standardHeader * getStandardHeader32(void *CODE, void *DATA);
getStanderdHeader32:
    push ebp
    mov ebp, esp
    xor eax, eax

    mov eax, dword [ebp + 8]
    cmp eax, 0x00
    jz .checkData
    mov eax, [ebp + 8]
    call getCODEBase
    jmp .finish
.checkData:
    mov eax, dword [ebp + 12]
    cmp eax, 0x00
    jz .finish
    mov eax, [ebp + 12]
    call getDATABase
.finish:
    leave
    ret

global getEIP
;   void *getEIP(void)
getEIP:
	push ebp
	mov ebp, esp
	mov eax, [ebp + 4]
	leave
	ret