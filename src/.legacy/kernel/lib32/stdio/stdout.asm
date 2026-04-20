bits 16

global printf16
; 16-bit printf
; Supports: %c, %s, %u, %h, %z
; Args: printf16(si = format, stack: ...)
printf16:
    push bp
    mov bp, sp
    pusha
	jmp .testspecial
.printfstackcounter: dw 0
.testspecial:
	lodsb
	cmp al, 0
	je .return
	push di
	mov di, bp
	add di, [.printfstackcounter]
	add di, 4
	cmp al, '%'
	je .formatter
	call printchar16
	pop di
	jmp .testspecial
.formatter:
	lodsb
	cmp al, 'z'
	je .ull
	cmp al, 'u'
	je .ul
	cmp al, 'h'
	je .uh
	cmp al, 'c'
	je .uc
	cmp al, 's'
	je .us
	cmp al, 0
	je .return
	call printchar16
	jmp .testspecial
.ull:
	push eax
	push edx
	push cx
	mov eax, [di]
	mov edx, [di + 4]
	mov cx, 10
	call printnum16
	add [.printfstackcounter], word 8
	pop cx
	pop edx
	pop eax
	pop di
	jmp .testspecial
.ul:
	push eax
	push edx
	push cx
	mov eax, [di]
	mov edx, 0
	mov cx, 10
	call printnum16
	add [.printfstackcounter], word 4
	pop cx
	pop edx
	pop eax
	pop di
	jmp .testspecial
.uh:
	push eax
	push edx
	push cx
	xor eax, eax
	mov ax, [di]
	mov edx, 0
	mov cx, 10
	call printnum16
	add [.printfstackcounter], word 2
	pop cx
	pop edx
	pop eax
	pop di
	jmp .testspecial
.uc:
	push ax
	mov ax, [di]
	xor ah, 0
	call printchar16
	add [.printfstackcounter], word 1
	pop ax
	pop di
	jmp .testspecial
.us:
	push di
	push ax
	push si
	push ds

	mov si, [di]
	mov ds, [di + 2]
.usloop:
	lodsb
	test al, al
	jz .usfinish
	call printchar16
	jmp .usloop
.usfinish:
	add [.printfstackcounter], word 4
	pop ds
	pop si
	pop ax
	pop di
	jmp .testspecial
.return:
	mov [.printfstackcounter], word 0
    popa
    leave
    ret

;   edx:eax [LOW:HIGH] qword
;   cx: radix
printnum16:
    push ebx
    push si
    mov si, .printbuffer
	push cx
	xor ecx, ecx
	pop cx
.calcloop:
    push eax
    mov eax, edx
    xor edx, edx
    div ecx
    				; eax: High dword out, edx: High dword remainder
    mov ebx, eax    ; ebx: High dword out
    pop eax         ; Restore Low dword
    add eax, ebx    ; Add remainder to low dword
	push edx
	xor edx, edx
	div ecx			; Divide low dword
	add dl, '0'
	mov [si], dl
	pop edx
	mov edx, ebx	; edx is the upper quotient out now

	inc si
	push edx
	or edx, eax
	pop edx
	jz .print
    jmp .calcloop
.print:
	mov si, (.printbufferend - 1)
.printloop:
	mov al, [si]
	test al, al
	je .noprint
	call printchar16
.noprint:
	cmp si, .printbuffer
	je .reset
	dec si
	jmp .printloop
.reset:
    mov si, .printbuffer
    mov cx, 32
.resetloop:
    mov [si], byte 0
    dec cx
    inc si
    test cx, cx
    jz .return
	jmp .resetloop
.return:
	pop si
	pop ebx
	ret
.printbuffer:
    times 32 db 0
.printbufferend:

printchar16:
    push ax
    push bx
    xor bx, bx

    mov ah, 0Eh
    int 10h
%ifdef __DEBUG
    push dx
    ; E9 debug output
    mov dx, 0E9h
    out dx, al
    pop dx
%endif
    pop bx
    pop ax
    ret