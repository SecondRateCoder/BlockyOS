bits 16

%define NULL 0x00
%define NEWLINE 0x0A
%define RETURN 0x0D
%define ENDL 0x0A, 0x0D, NULL

%define MAXREGIONS					0x0100

%define E820SIGNATURE				0x534D4150
; %define E820TYPE_USABLE 			0x0001
; %define E820TYPE_RESERVED 			0x0002
; %define E820TYPE_ACPI_RECLAIMABLE 	0x0003
; %define E820TYPE_ACPI_NVS 			0x0004
; %define E820TYPE_BAD_MEMORY 		0x0005

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

%macro LINEAR2SEGOFF 3
    ; %1 = linear
    ; %2 = segment register
    ; %3 = offset register

    mov %2, %1 >> 4
    mov %3, %1 & 0xF
%endmacro

extern printf16

global E280GetNextMemBlock16
;   int(eax) E280GetNextMemBlock16(E820MemBlock __far *Block, uint32_t __far *continuationID)
E280GetNextMemBlock16:
	push bp
	mov bp, sp
	push si
	push bx
	push dx
	push cx
	; Get Continuation ID
	push ds
	mov ds, [bp + 8]
	mov si, [bp + 10]
	mov ebx, ds:[si]
	pop ds

	; Get Structure Pointer, In es:di
	push es
	mov ax, [bp + 4]
	mov es, ax
	mov di, [bp + 6]


	mov eax, 0xE820
	mov edx, E820SIGNATURE
	mov ecx, 24

	int 15h
	; Restore es
	pop es

	cmp eax, E820SIGNATURE
	jne .Error
.Success:
	mov eax, ecx	; Size
	push ds			; Restor ContinuationID
	mov ds, [bp + 8]
	mov si, [bp + 10]
	mov ds:[si], ebx
	pop ds
	jmp .EndIf
.Error:
	mov eax, -1
.EndIf:
	pop cx
	pop dx
	pop bx
	pop si
	leave
	ret

global MemDetect16
;   void MemDetect16(MemInfo *out)
MemDetect16:
	push bp
	mov bp, sp
	push di					; Counter
	push es					; Contain out pointer segment
	push si					; Contain out pointer offset

	mov ax, [bp + 4]
	mov es, ax
	mov si, [bp + 6]

	; Use di as an indexer of MemDetectBlock,
	; Write directly to the table, using ax to generate address
.loop_:
	; Push far Continuation ID
	push ds
	push MemDetectContinuationID
	; Push far table pointer
	mov ax, di
	add ax, MemDetectBlock
	push ds
	push ax
	; Call
    call E280GetNextMemBlock16
	add sp, 8					; Pop far pointers
	add di, 24
.print:
	push si
	push dx
	mov si, di
	add si, MemDetectBlock
	mov eax, dword [si + E820MemBlock.Base]		; High
	xor dx, dx
	push bx
    mov bx, 65535
	div bx
    pop bx
	push word 0
	push ax
	push dword [si + E820MemBlock.Base + 4]		; Low
	mov eax, dword [si + E820MemBlock.Limit]		; High
	xor dx, dx
    push bx
    mov bx, 65535
	div bx
    pop bx
	push word 0
	push ax
	push dword [si + E820MemBlock.Limit + 4]	; Low
	push dword [si + E820MemBlock.Type]
	push dword [si + E820MemBlock.ACPI]
	mov si, E820MemBlock
    mov al, 0x00
	call printf16
	add sp, 26
	pop dx
	pop si
.update:
	; Increment out's Count
    cmp eax, 0x0000
	ja .loop_
	cmp [MemDetectContinuationID], dword 0
	jne .loop_
.finish:
    ; Store out table
    mov [es:si + MemInfo.Table], word 0
	mov [es:si + MemInfo.Table + 1], ds
	mov [es:si + MemInfo.Table + 2], word 0
	mov [es:si + MemInfo.Table + 3], word MemDetectBlock
	; Store Out Count
	mov [es:si + MemInfo.Count], word 0
	mov [es:si  + MemInfo.Count + 2], di
	pop si
	pop es
	pop di
	leave
    ret


section .data
E820String: db NEWLINE, 'E820 Block: {', NEWLINE, RETURN,
	.BASE: db '     Base: %d:%d, ', NEWLINE, RETURN,
	.LIMIT: db '    Limit: %d:%d, ', NEWLINE, RETURN,
	.TYPE: db '    Type: %d', NEWLINE, RETURN, 
	.ACPI: db '	ACPI: %d}', ENDL
MemDetectContinuationID: dd 0
MemDetectBlock: 
    times (MAXREGIONS * 24) db 0