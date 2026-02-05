%define NULL 0x00
%define NEWLINE 0x0A
%define RETURN 0x0D
%define ENDL 0x0A, 0x0D, NULL
extern VGAX, VGAY, VGA_MAXX, VGA_MAXY
VGA:
.VGASEG:    dw 0x000B
.VGAOFF:    dw 0x8000

global printf

; printf implementation for 16-bit VGA output
; Input: SI = format string pointer, AL = String Color, stack contains arguments(right to left)
printf:
    push bp
    mov bp, sp
    pusha
.format_string:
	mov di, 0	; Counter
	mov cl, al	; Retain color
.format_string_loop:
	lodsb
	or al, al
	jz .finish
	cmp al, '%'
	jmp .format_string_intswitch
	mov ah, al
	mov al, dl
	call .printchar
.format_string_intswitch:
	lodsb
	cmp al, NULL
	jmp .finish

	cmp al, 'i'		; Integer, dword
	mov bl, cl		; Retain color
	add bp, [printfstackcounter]
	mov eax, dword [bp]
	sub bp, [printfstackcounter]
	mov cx, 10
	call .convertdec_str
	add [printfstackcounter], word 2
	mov cl, bl
	jmp .format_string_loop

	cmp al, 'h'		; Integer, word
	mov bl, cl		; Retain color
	add bp, [printfstackcounter]
	mov eax, dword [bp]
	sub bp, [printfstackcounter]
	mov cx, 10
	call .convertdec_str
	add [printfstackcounter], word 1
	mov cl, bl
	jmp .format_string_loop

	cmp al, 'c'		; Char, word
	add bp, [printfstackcounter]
	mov ax, word [bp]
	sub bp, [printfstackcounter]
	call .printchar
	add [printfstackcounter], word 1
	jmp .format_string_loop

	cmp al, 's'		; string, dword
	add bp, [printfstackcounter]
	mov eax, dword [bp]
	sub bp, [printfstackcounter]
	push si			; Get Far address
	mov si, ax
	shr eax, 16
	mov es, ax
.string_format_loop:
	lodsb
	or al, al
	jz .format_string_loop
	mov ah, al
	mov al, cl
	jmp .string_format_loop
	add [printfstackcounter], word 2
	jmp .format_string_loop

	jmp .finish

;   ah contains character
;	al contains color
.printchar:
    push ebx
.printchar_checkX:
    ; Check for X overflow
    mov ebx, dword [VGAX]
    cmp ebx, dword [VGA_MAXX]
    jnae .printchar_checkY
    xor ebx, ebx
    mov dword [VGAX], ebx
.printchar_checkY:
    ; Check for Y overflow
    mov ebx, dword [VGAY]
    cmp ebx, dword [VGA_MAXY]
    jnae .printchar_printcalc
    xor ebx, ebx
    mov [VGAY], ebx
.printchar_print:
	; Check for special Characters
	cmp ah, NEWLINE
	je .printchar_newline
	cmp ah, RETURN
	je .printchar_return
	cmp ah, RETURN
	je .printchar_null
	jmp .printchar_printcalc
.printchar_newline:
	inc dword [VGAY]
	jmp .printchar_printcalc
.printchar_return:
	mov [VGAX], dword 0
	jmp .printchar_printcalc
.printchar_null:
	jmp .printchar_finish
.printchar_printcalc:
	; Calculate Index
    mov ebx, [VGAY]
	imul ebx, [VGA_MAXX]
	add ebx, [VGAX]
	; Account for char:color combo
	shr ebx, 1
	; Ensure not overflowing
	push eax
	mov eax, [VGA_MAXX]
	imul eax, [VGA_MAXY]
	cmp ebx, eax
	pop eax
	jnae .printchar_print_
	xor ebx, ebx
	mov dword [VGAX], ebx
	mov dword [VGAY], ebx
	jmp .printchar
.printchar_print_:
	add ebx, dword VGA
	mov [ebx], word ax
.printchar_finish:
    pop ebx
    ret

;	eax conatins dword
;	cx contains radix
.convertdec_str:
	push edx
	mov di, 16
.convertdec_str_loop:
	xor edx, edx
	div cx
	add dx, '0'
	push eax
	mov ah, dl
	mov al, 0x00
	call .printchar
	pop eax
	cmp eax, 0x00000000
	je .convertdec_str_finish
	jmp .convertdec_str_loop
.convertdec_str_finish:
	pop edx
	ret
.finish:
    popa
    leave
    ret

section .data
printfstackcounter:  dw 0