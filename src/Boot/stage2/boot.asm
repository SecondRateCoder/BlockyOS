bits 16
; org 0x8200
section _ENTRY class=CODE
db 103

global bt1_drive_header_
extern main16_

bt1_drive_header_: times 58 db 0
jmp short _start

; Number of sectors to load into RAM, should be the full size of Boot2
%define ENDL 0x0A, 0x0D, 0x00

global _start
; void start(void)
_start:
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
._halt_
    nop
    jmp ._halt_

global _bochs_breakpoint
; void _bochs_breakpoint(void)
_bochs_breakpoint:
    xchg bx, bx
    nop
    ret

global _puts_vidteletype
; void puts_vidteletype(char page, char __far *ptr)
_puts_vidteletype:
    [bits 16]
    push bp
    mov bp, sp

    ; push bx
    push ax
    push si
    push ds
    push bx

    ; Load page number
    mov bx, [bp + 4]
    ; Load far pointer from stack
    mov ax, [bp + 6]     ; segment
    mov si, [bp + 8]     ; offset
    mov ds, ax         ; DS:SI -> string
.loop:
    lodsb               ; AL = [DS:SI], SI++
    or al, al
    jz .done

    mov ah, 0Eh
    int 10h
    jmp .loop

.done:
    pop bx
    pop ds
    pop si
    pop ax
    mov sp, bp
    pop bp
    ret

global _put_vidteletype
; void put_vidteletype(uint16_t c, uint16_t page)
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

global _div64_32_
;   void _div64_32(uint64_t dividend, uinl32_t divisor, uint64_t __far *result, uinl32_t __far *remainder)
_div64_32_:
    [bits 16]

    ; Generate new call frame.
    push bp
    mov bp, sp

    ; Save modified registers
    push bx
    push es

    mov es, [bp + 20]   ; Grab segment
    mov bx, [bp + 18]   ; Grab offset
    mov eax, [bp + 6]   ; eax: dividend upper 32 bits
    mov ecx, [bp + 14]  ; ecx: divisor

    xor edx, edx
    ; Divide upper 32-bits of dividend
    div ecx             ; eax: result, edx: rem, ecx: divisor

    ; Store lower 32 bits of quotient
    mov [bx], eax

    ; Divide upper 32 bits of quotient
    mov eax, [bp + 10]   ; Grab lower 32-bits
    add eax, edx

    ; Divide lower 32 bits of quotient
    xor edx, edx
    div ecx             ; edx contains remainder

    ; Store lower 32-bit result
    ; bx still unmodified so no need to change/re-assign it
    mov [bx + 4], eax

    ; Store remainder
    mov bx, [bp + 22]
    mov es, [bp + 24]
    mov [bx], edx

    ; Restore modified registers
    pop es
    pop bx

    ; Restore old call frame.
    leave
    ret

global __U8LS
; unsigned short __U8LS(unsigned char value, unsigned char shift)
__U8LS:
    [bits 16]
    push bp
    mov bp, sp

    push cx              ; preserve callee-saved

    mov al, [bp+4]      ; value
    mov cl, [bp+6]      ; shift
    shl al, cl

    xor ah, ah          ; AX = result

    pop cx
    mov sp, bp
    pop bp
    ret

global __I8LS
; short __I8LS(signed char value, unsigned char shift);
__I8LS:
    [bits 16]
    push bp
    mov  bp, sp

    push cx              ; preserve callee-saved

    mov  al, [bp+4]      ; value (signed 8-bit)
    mov  cl, [bp+6]      ; shift count
    sal  al, cl          ; arithmetic left shift on AL
    cbw                  ; sign-extend AL -> AX (result)

    pop  cx
    mov  sp, bp
    pop  bp
    ret



msg_bt2: db 'This is Boot2', ENDL, 0
buffer: