bits 32

%define FRATINTERRUPTVECTOR (0x20 + 0)

extern int32disable, int32enable, FRATHANDLER

; global FRATINTERRUPT
; FRATINTERRUPT:
;     push dword 0	                ; Push dummy error
;     push dword FRATINTERRUPTVECTOR 	; Push Interrupt Code
; 	push esp

;     call int32disable
;     pusha                       ; save general-purpose registers
;     xor eax, eax

;     ; Save all data segments
;     mov ax, ds
;     push eax
;     mov ax, es
;     push eax
;     mov ax, fs
;     push eax
;     mov ax, gs
;     push eax

;     mov ax, 0x10               ; kernel data segments
;     mov ds, ax
;     mov es, ax
;     mov fs, ax
;     mov gs, ax

;     push esp
;     call FRATHANDLER

;     pop eax
;     mov gs, ax
;     pop eax
;     mov fs, ax
;     pop eax
;     mov es, ax
;     pop eax
;     mov ds, ax

;     add esp, 8                  ; Pop interrupt vector and error code
; 	popa
;     call int32enable
; 	iret

; global g_fcreate
; ;   void g_fcreate(char *name, char *mode)
; g_fcreate:
;     push ebp
;     mov ebp, esp

;     push dword [ebp + 8]
;     push dword [ebp + 12]
; .flush:
; .interrupt:
;     int FRATINTERRUPTVECTOR

;     leave
;     ret