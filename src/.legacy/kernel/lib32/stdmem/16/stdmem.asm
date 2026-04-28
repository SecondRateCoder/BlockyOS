bits 16

struc E820MemBlock
    .Base       resb 8
    .Limit      resb 8
    .Type       resb 4
    .ACPI       resb 4
endstruc

struc MemInfo
    .Table      resb 8
    .Count      resb 4
endstruc

section .text

%define NULL 0x00
%define NEWLINE 0x0A
%define RETURN 0x0D
%define ENDL NEWLINE, RETURN, NULL

%define MAXREGIONS			0x0100

%define E820SIGNATURE		534d4150h
%define E820BLOCKSIZE       24

extern printf16

global E280GetNextMemBlock16
;   length(eax) E280GetNextMemBlock16(E820MemBlock __far *Block, uint32_t __far *continuationID, uint16_t StructSize)
E280GetNextMemBlock16:
	xchg bx, bx
    clc
	push bp
	mov bp, sp
	push di
    push es
    push ebx
    push edx
    push ecx
    ; Get args
    ; Get Continuation ID w/ es:di
    mov di, [bp + 8]
    mov es, [bp + 10]
    mov ebx, [es:di]
    ; Get Block Address
    mov di, [bp + 4]
    mov es, [bp + 6]
    ; Block Size
    xor ecx, ecx
    mov cx, [bp + 12]

    ; True Params
    mov eax, 0xE820
    mov edx, E820SIGNATURE

    int 15h

    ; Check complete, Dont check Continuation ID as it doesnt matter allat much
    jc .End
    cmp eax, E820SIGNATURE
    jne .End
.Success:
    mov di, [bp + 8]
	mov es, [bp + 10]
    mov [es:di], ebx
    mov al, ah  ; Non 0 ax will be an error
.End:
	mov eax, ecx
    pop ecx
    pop edx
    pop ebx
    pop es
	pop di
	leave
	ret

global MemDetect16
;   void MemDetect16(MemInfo __far *out)
MemDetect16:
	push bp
	mov bp, sp
	push di					; Counter
	push es
	push bx
    mov ebx, dword MemDetectBlock
    shr ebx, 16
    mov es, bx
    mov bx, word MemDetectBlock
    mov di, MAXREGIONS
.loop:
    push E820BLOCKSIZE
    push dword MemDetectContinuationID
    push es
    push bx
    call E280GetNextMemBlock16
	add sp, 10
    cmp [MemDetectContinuationID], dword 0
	je .finish
    test di, di
    jz .finish
.printf:
%ifdef __DEBUG
    push dword [bx + E820MemBlock.ACPI]
    push dword [bx + E820MemBlock.Type]
    push dword [bx + E820MemBlock.Limit + 4]
    push dword [bx + E820MemBlock.Limit]
    push dword [bx + E820MemBlock.Base + 4]
    push dword [bx + E820MemBlock.Base]
	sub bx, MemDetectBlock
	push word bx
	add bx, MemDetectBlock
	push word es
	push dword [MemDetectContinuationID]
    mov si, E820String
    call printf16
	add sp, (E820BLOCKSIZE + 4)
%endif

	push eax
	push ebx
	mov eax, [bx + E820MemBlock.Limit]
	mov ebx, [bx + E820MemBlock.Limit + 4]
	or eax, ebx
	pop eax
	pop ebx
	jz .loop	; Retry if limit-high and limit-low
	add bx, E820BLOCKSIZE
    dec di
    jmp .loop
.finish:
	xchg bx, bx
	; Load Decriptor Block
    mov bx, [bp + 4]
    mov es, [bp + 6]
    mov [bx], word 0					; High High Address word
	mov [bx + 2], ds					; High Low Address word
    mov [bx + 4], dword MemDetectBlock	; Low Address dword
	; ax: Block Size, di: Number of Items
    push ax
	push dx
	xor dx, dx

	push bx
	mov bx, di
	mov di, MAXREGIONS
	sub di, bx
	pop bx

	mul di
    mov [bx + 8], eax					; Write Size
	pop dx
    pop ax
%ifdef __DEBUG
	push dword [bx + 8]
	push dword [bx]
	push dword [bx + 4]
	mov si, E820Desc
	call printf16
%endif
    pop bx
    pop es
    pop di
    leave
    ret

E820Desc:
	db NEWLINE, 'E820Desc: {', NEWLINE, RETURN,
	db '	Table: 	%z,', NEWLINE, RETURN,
	db '	Size: 	%u', NEWLINE, RETURN,
	db '}', ENDL
E820String: 
	db NEWLINE, 'E820Block [%u]: {', NEWLINE, RETURN,
	db '    Offset:	%h:%h,', NEWLINE, RETURN,
	db '    Base: 	%z,', NEWLINE, RETURN,
	db '    Limit: 	%z,', NEWLINE, RETURN,
	db '    Type: 	%u,', NEWLINE, RETURN,
	db '    ACPI: 	%u', NEWLINE, RETURN,
	db '}', ENDL
MemDetectContinuationID: dd 0
MemDetectBlock: 
    times (MAXREGIONS * 3) dq 0