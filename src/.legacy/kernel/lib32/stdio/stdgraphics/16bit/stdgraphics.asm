bits 16

%define NULL 0x00
%define NEWLINE 0x0A
%define RETURN 0x0D
%define ENDL NEWLINE, RETURN, NULL

extern printf16
section .text
global setDefaultControllerInfo16
;   es:di(VBEInfoBlock) getControllerInfo16()
setDefaultControllerInfo16:
    [bits 16]
    xchg bx, bx
    nop
    nop
    nop
    push bp
    mov bp, sp
    xor bx, bx
    push ax
    ; Get Mode Info
    push cx
    mov cx, 0x117
    mov ax, 0x4F01
    mov ebx, VGAModeOut
    mov di, bx
    shr ebx, 16
    mov es, bx
    int 10h
    pop cx
    test ax, 0x004F
    je .success
    jmp .return
.success:   
    ; Set Mode
    mov ax, 0x4F02
    mov bx, 0x4117
    int 10h
    cmp ax, 0x4F
    jne .return

    mov ebx, VGAModeOut
    mov di, bx
    shr ebx, 16
    mov es, bx
.return:
    pop ax
    leave
    ret

VGAModeOut: times 256 db 0