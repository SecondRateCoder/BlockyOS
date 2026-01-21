bits 32
; org 0x8200
section _ENTRY class=CODE
db 103

global bt1_drive_header
extern main32
extern gdtDesc
extern idtDesc

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
	push word gdtDesc
	push word idtDesc
	push cs
	push word main32
	call _switch16_32
; void halt16(void)
halt16:
    [bits 16]
    cli
.halt_
    nop
    jmp .halt_

global _halt
; void halt(void)
_halt:
    [bits 32]
    cli
._halt_
    nop
    jmp ._halt_

global _bochs_breakpoint
; void _bochs_breakpoint(void)
_bochs_breakpoint:
	[bits 32]
    xchg bx, bx
    nop
    ret

global _puts_vidteletype
; void puts_vidteletype(char page, char __far *ptr)
_puts_vidteletype:
    [bits 32]
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
    [bits 32]
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
    [bits 32]

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
    [bits 32]
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
    [bits 32]
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

global _int16disable
;	void int16disable(void)
_int16disable:
	[bits 16]
	cli
	push ax
	in al, 0x70
	or al, 80h
	out  0x70, al
	pop ax
    ret
global _int16enable
;	void int16enable(void)
_int16enable:
	[bits 16]
	sti
	push ax
	in   al, 0x70
	and  al, 7Fh
	out  0x70, al
	pop ax
    ret


global _switch16_32
;   void switch16_32(GDTdesc __far *, IDTdesc __far*, void(__far __cdecl *func)(void))
_switch16_32:
    [bits 16]
    ; Save registers
    push bp
    mov bp, sp
    push ax

    call _int16disable
.TestA20:
	; Test for Memory Wrap-around
    mov ax, 0x0500
    mov ds, ax
    mov word [0], 0xBEEF

    ; Read from 0x100500 (wraps to 0x0500 if A20 is disabled)
    mov ax, 0x1005
    mov ds, ax
    cmp word [0], 0xBEEF
	; Memory Wrap-around failed, 0xBEEF and [edx] are not the same
	jnz .LoadGDT
	; Memory Wrap-around worked, 0xBEEF and [edx] are the same
.EnableA20:
    ; Disable Keyboard
    call .A20WaitIn16
    mov al, KbdCtrlDisable
    out KbdCtrlCmdPort, al
    ; Read Controller Out Port
    call .A20WaitIn16
    mov al, KbdCtrlReadOutPort
    out KbdCtrlCmdPort, al
    
    call .A20WaitOut16
    in al, KbdCtrlDataPort
    push eax

    ; Write Controller out port
    call .A20WaitIn16
    mov al, KbdCtrlWriteOutPort
    out KbdCtrlCmdPort, al

	; Enable A20 Gate bit
    call .A20WaitIn16
    pop eax
    or al, 2		; Bit 2 is the A20 flag bit
	out KbdCtrlDataPort, al

	; Re-enable Keyboard
	call .A20WaitIn16
	mov al, KbdCtrlEnable
	out KbdCtrlCmdPort, al
.LoadGDT:
	push es
	mov es, [bp + 6]		; Segment
	push bx
	mov bx, [bp + 8]		; Offset
	lgdt [es:bx]
	pop bx
	pop es
.LoadIDT:
	push es
	mov es, [bp + 10]		; Segment
	push bx
	mov bx, [bp + 12]		; Offset
	lidt [es:bx]
	pop bx
	pop es
	jmp .finish
.A20WaitIn16:
	; Wait till bit 2 is 0
	in al, KbdCtrlCmdPort
	test al, 2
	jnz .A20WaitIn16
	ret
.A20WaitOut16:
	; Wait until Bit 1 is not 0
	in al, KbdCtrlCmdPort
	test al, 1
	jz .A20WaitOut16
	ret

.finish:
	; Setup to jmp to func
	mov ebx, [bp + 14]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
	jmp dword 0000h:.pmode
.pmode:
	[bits 32]
	jmp dword edx

KbdCtrlDataPort             	equ 0x60
KbdCtrlCmdPort              	equ 0x64

KbdCtrlDisable              	equ 0xAE
KbdCtrlEnable             		equ 0xAD

KbdCtrlReadOutPort             equ 0xAE
KbdCtrlWriteOutPort            equ 0xAD

msg_bt2: db 'This is Boot2', ENDL, 0
buffer:
