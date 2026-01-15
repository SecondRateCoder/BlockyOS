bits 16
; org 0x8200
section _ENTRY class=CODE
db 103

global bt1_drive_header_
extern main16_

bt1_drive_header_: times 58 db 0
jmp short start_

; Number of sectors to load into RAM, should be the full size of Boot2
%define ENDL 0x0A, 0x0D, 0x00

global start_
; void start(void)
start_:
    [bits 16]
    xchg bx, bx
    ; Setup stack
	cli
	mov ax, ds
	mov ss, ax
	mov sp, 512
	sti

    ; Jump to boot2's .c file
	call main16_

global _halt
; void halt(void)
_halt:
    [bits 16]
    cli
    jmp _halt

global _bochs_breakpoint
; void _bochs_breakpoint(void)
_bochs_breakpoint:
    xchg bx, bx
    nop
    ret

; global _puts_vidteletype
; ; void puts_vidteletype((uint32_t)(char __far *ptr))
; _puts_vidteletype:
;     push bp
;     mov bp, sp

;     push ax
;     push si
; .loop:
;     lodsb
;     or al, al
;     jz .done

;     mov ah, 0x0E
;     int 0x10

;     jmp .loop
; .done:
;     pop si
;     pop ax
;     mov sp, bp
;     pop bp
;     ret
global _puts_vidteletype
; void puts_vidteletype(char __far *ptr)
_puts_vidteletype:
    [bits 16]
    call _bochs_breakpoint
    push bp
    mov bp, sp

    ; push bx
    push ax
    push si
    push ds

    ; Load far pointer from stack
    mov si, [bp + 6]     ; offset
    mov ax, [bp + 4]     ; segment
    mov ds, ax         ; DS:SI -> string
.loop:
    lodsb               ; AL = [DS:SI], SI++
    or al, al
    jz .done

    mov ah, 0Eh
    int 10h
    jmp .loop

.done:
    pop ds
    pop si
    pop ax
    mov sp, bp
    pop bp
    ret

global _put_vidteletype
; void put_vidteletype(char c, char page)
;   Pushed right arg 1st, starting from [bp + 4]
_put_vidteletype:
    [bits 16]
    push bp
    mov bp, sp

    push bx
    mov bh, [bp + 4]     ; page number
    mov ax, [bp + 6]     ; character
    test al, al
    jz .finish
    
    mov ah, 0Eh
    int 10h
.finish:
    pop bx
    mov sp, bp
    pop bp
    ret

global __div64_32
;   void _div64_32(uint64_t dividend, uint32_t divisor, uint64_t __far *quotientOut, uint32_t __far *remainderOut)
;   Pushed right arg 1st, starting from [bp + 4]
__div64_32:
    [bits 16]
    call _bochs_breakpoint

    ; Generate new call frame.
    push bp
    mov bp, sp
    
    push bx

    mov eax, [bp + 20]       ; eax: dividend upper 32 bits
    mov ecx, [bp + 12]      ; ecx: divisor
    xor edx, edx
    div ecx                 ; eax - quot, edx - rem

    ;   Store upper 32 bits of quot
    mov bx, [bp + 8]
    push es
    mov es, [bp + 10]
    mov [bx], eax
    pop es

    ; Divide lower 32 bits
    mov eax, [bp + 16]

    div ecx

    ; Store results
    push es
    mov es, [bp + 6]
    mov [bx + 4], eax

    mov bx, [bp + 4]
    mov [bx], edx
    pop es

    pop bx

    ; Restore old call frame.
    mov sp, bp
    pop bp
    ret



global __U8DR
; unsigned short __U8DR(unsigned char dividend, unsigned char divisor)
;   Pushed right arg 1st, starting from [bp + 4]
__U8DR:
    [bits 16]
    push bp
    mov  bp, sp

    mov  bl, [bp + 4]     ; divisor
    mov  al, [bp + 6]     ; dividend
    xor  ah, ah         ; clear high byte for 8-bit divide
    div  bl             ; AL = quotient, AH = remainder

    ; Watcom wants the result in AX (unsigned short)
    ; We'll return the quotient in AX (low 8 bits used)
    ; If you want remainder instead, swap AL/AH here.
    mov  ah, 0          ; ensure high byte is clean

    mov  sp, bp
    pop  bp
    ret                 ; cdecl: caller cleans stack

global __U8DQ
; unsigned short __U8DQ(unsigned char dividend, unsigned char divisor)
;   Pushed right arg 1st, starting from [bp + 4]
__U8DQ:
    [bits 16]
    push bp
    mov  bp, sp

    mov  bl, [bp + 4]     ; divisor
    mov  al, [bp + 6]     ; dividend
    xor  ah, ah
    div  bl             ; AL = quotient, AH = remainder

    mov  ah, 0          ; AX = quotient (zero-extend)

    mov  sp, bp
    pop  bp
    ret


global __U8LS
; unsigned short __U8LS(unsigned char value, unsigned char shift)
;   Pushed right arg 1st, starting from [bp + 4]
__U8LS:
    [bits 16]
    push bp
    mov  bp, sp

    mov  cl, [bp + 4]     ; shift
    mov  al, [bp + 6]     ; value
    shl  al, cl         ; AL <<= CL

    xor  ah, ah         ; zero-extend to 16-bit (AX)
    mov  sp, bp
    pop  bp
    ret

msg_bt2: db 'This is Boot2', ENDL, 0
buffer: