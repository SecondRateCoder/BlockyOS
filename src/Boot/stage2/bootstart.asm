bits 32

extern __kernel_start, __kernel_end, __kernel_sectors
extern int32enable
extern setup32
extern MemDetect16

; extern GDTdesc, IDTdesc
extern GDTSize, IDTSize, GDT, IDT

global bootDrive
global start

%define MAXREADS 16
%define E820TYPE_USABLE 			0x0001
%define E820TYPE_RESERVED 			0x0002
%define E820TYPE_ACPI_RECLAIMABLE 	0x0003
%define E820TYPE_ACPI_NVS 			0x0004
%define E820TYPE_BAD_MEMORY 		0x0005

struc E820MemBlock
    .Base           resb 8
    .Limit          resb 8
    .Type           resb 4
    .ACPI           resb 4
endstruc

struc BootIn
    .MemRegion      resb 8
endstruc

section .entry
db 0x67

drive_header:
	bdb_oem:                   	times 8 db 0
	bdb_bytes_per_sector:       dw 0						; +1
	bdb_sectors_per_cluster:    dw 0						; +3
	bdb_reserved_sectors:       db 0						; +5
	bdb_fat_count:              db 0						; +6
	bdb_dir_entries_count:      dw 0						; +7
	bdb_total_sectors:			dw 0						; +9
	bdb_media_desciptor_type:	db 0						; +11
	bdb_sectors_per_fat:		dw 0						; +12
	bdb_sectors_per_track:		dw 0						; +14
	bdb_heads:					dw 0						; +16
	bdb_hidden_sectors:			dd 0						; +18
	bdb_large_sector_count:		dd 0						; +22
	; extended boot record:
	ebr_drive_number:			db 0						; +26
	ebr_signature:				db 0						; +28
	ebr_volume_id:				times 4 db 0				; +30
	ebr_volume_label:			times 11 db 0				; +46
	ebr_system_id:				times 8 db 0				; +57
	; custom_boot_record
	sectormap_entries:			db 0						; +65
drive_end:

; Number of sectors to load into RAM, should be the full size of Boot2
%define ENDL 0x0A, 0x0D, 0x00

; void start(void)
start:
    [bits 16]
	nop
    xchg bx, bx

    ; Setup stack
	call int16disable
	mov ax, 0
	mov ss, ax
	mov sp, (0x0500 + 2040)
	call int16enable
	; Read address
	mov bx, buffer
	; LBA
    mov ax, 2
    ; LBA2CHS
.LBA2CHS:
    xor dx, dx
    div word [bdb_sectors_per_track] 	; ax: LBA / bdb_sectors_per_track
                                		; dx: LBA % bdb_sectors_per_track

    inc dx                      		; dx: (LBA % bdb_sectors_per_track) + 1: Sector number
    mov cx, dx							; cx: Sector number
    xor dx, dx							; dx: (LBA / bdb_sectors_per_track) % db_heads: Head number
										; ax: (LBA / bdb_sectors_per_track) / db_heads:	Cylinder number
    div word [bdb_heads]
	; cx: Sector number; (ch: ?, cl: Sector number)
	; dx: Head number; (dh: ?, dl: Head number)
	; ax: Cylinder number; (ah: ?, al: Cylinder number)

    mov ch, al
    mov dh, dl
    mov dl, byte [ebr_drive_number]
	;	ch: Cylinder num    ; & 0xff
	;	cl: Sector num      ; | ((cylinder num >> 2) & 0xC0)
	;	dh: Head number
	;	dl: drive number

	mov ax, __kernel_sectors
	call DISKREAD
.done:
    xchg bx, bx
    mov ax, [GDTSize + 2]
    mov [.GDTLimit], ax
    mov [.GDTTable], dword GDT

    mov ax, [IDTSize]
    mov [.IDTLimit], ax
    mov [.IDTTable], dword IDT

    push setup32
    push dword .tempIDTdesc
    push dword .tempGDTdesc
	call switch16_32
.tempGDTdesc:
.GDTLimit:  dw 0
.GDTTable:  dd 0
.tempIDTdesc:
.IDTLimit:  dw 0
.IDTTable:  dd 0

; void halt16(void)
halt16:
    [bits 16]
    cli
.halt_:
    nop
    jmp .halt_

; INPUT:
;   AX = total sectors to read
;   ES:BX = destination buffer
;   CH = cylinder
;   DH = head
;   CL = sector
;   DL = drive number
DISKREAD:
    mov di, ax                ; DI = sectors remaining
.read_loop:
	; Determine how many sectors to read safely
    mov ax, di                ; AX = sectors remaining
    cmp ax, MAXREADS
    jbe .use_ax
    mov ax, MAXREADS
.use_ax:
    mov [sectorreads], ax
    ; BIOS read
	; Reset Params
    mov al, [sectorreads]
	mov ah, 0x02              ; read sectors
	mov dl, byte [ebr_drive_number]
    stc
    int 13h
    jc halt16
    ; Advance buffer pointer
    mov ax, [bdb_bytes_per_sector]
    mul word [sectorreads]   ; DX:AX = bytes read
    add bx, ax
	push ax
    adc ax, dx
	pop ax
	jnc ._advance_chs
	push ax
	mov ax, es
	inc ax
	mov es, ax
	pop ax
    ; Advance CHS
._advance_chs:
    mov ax, [sectorreads]
.advance_chs:
    dec ax
    js .chs_done

    inc cl
    cmp cl, [bdb_sectors_per_track]
    jbe .advance_chs
    ; sector rolled over
    mov cl, 1
    inc dh
    cmp dh, [bdb_heads]
    jb .advance_chs
    ; head rolled over
    mov dh, 0
    inc ch
    jmp .advance_chs
.chs_done:
    ; xchg bx, bx
    ; Loop until all sectors read
    push ax
    xor ax, ax
    mov al, byte [sectorreads]
    sub di, ax
    pop ax
    jz .finish
    jc .finish  ; More likely to overflow than to actually reach 0
    jmp .read_loop
.finish:
    ret

global halt32
; void halt32(void)
halt32:
    [bits 32]
    cli
.halt:
    nop
    jmp .halt

global bochs_breakpoint32
;   void bochs_breakpoint(void)
bochs_breakpoint32:
	[bits 32]
    xchg bx, bx
    nop
    ret

;	void int16disable(void)
int16disable:
	[bits 16]
	cli
	push ax
	in al, 0x70
	or al, 80h
	out  0x70, al
	pop ax
    ret

;	void int16enable(void)
int16enable:
	[bits 16]
	sti
	push ax
	in   al, 0x70
	and  al, 7Fh
	out  0x70, al
	pop ax
    ret

global TestA20
;   void TestA20(void)
TestA20:
	; Test for Memory Wrap-around
    push ds
    mov ax, 0x0500
    mov ds, ax
    mov word [0], 0xBEEF

    ; Read from 0x100500 (wraps to 0x0500 if A20 is disabled)
    mov ax, 0x1005
    mov ds, ax
    mov eax, 1
    cmp word [0], 0xBEEF
    jz .finish
    mov eax, 0
	; Memory Wrap-around failed, 0xBEEF and [edx] are not the same
.finish:
    pop ds
    ret

global switch16_32
;   void switch16_32(GDTdesc __far *, IDTdesc __far*, void(__far __cdecl *func)(void), function In args)
switch16_32:
    [bits 16]
    mov bp, sp
    ; Nothing needs to be saved as since, the switcher does NoT revert

    call int16disable
    call TestA20
    test eax, eax
    jnz .LoadGDT
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
    jmp .LoadGDT
.A20WaitIn16:
	in al, KbdCtrlCmdPort
	test al, 2
	jnz .A20WaitIn16
	ret
.A20WaitOut16:
	in al, KbdCtrlCmdPort
	test al, 1
	jz .A20WaitOut16
	ret
.LoadGDT:
    xchg bx, bx
	mov bx, [bp + 2]		; Offset
	mov es, [bp + 4]		; Segment
	lgdt [es:bx]
.LoadIDT:
	mov bx, [bp + 6]		; Offset
	mov es, [bp + 8]		; Segment
	lidt [es:bx]
.finish:
	; Setup to jmp to func
    mov eax, cr0
    or eax, 1
    mov cr0, eax
	jmp dword 0008h:.pmode
.pmode:
	[bits 32]
    add esp, 10
	call int32enable
	ret

KbdCtrlDataPort             	equ 0x60
KbdCtrlCmdPort              	equ 0x64

KbdCtrlDisable              	equ 0xAD
KbdCtrlEnable             		equ 0xAE

KbdCtrlReadOutPort              equ 0xD0
KbdCtrlWriteOutPort             equ 0xD1

BootInfo:
    istruc BootIn
        at BootIn.MemRegion,     dq 0
    iend

sectorreads: db 16

times (513 - ($ - $$)) db 0
buffer: