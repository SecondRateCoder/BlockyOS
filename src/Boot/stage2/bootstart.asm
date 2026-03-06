bits 32

extern __kernel_sectors, __CODEADDR, __kernel_start
extern int32enable
extern setup32
extern E280GetNextMemBlock16, MemDetect16, setDefaultControllerInfo16

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

struc VbeInfo
    .Signature      resb 4
    .Version        resb 2
    .OemStringPtr   resb 4
    .Capabilities   resb 4
    .VideoModePtr   resb 4
    .TotalMemory    resb 4      ; As 64kb blocks
    .Reserved       resb (236 + 256)
endstruc

struc BootIn
    .MemRegion      resb 12
    .VBEOut         resb 256
endstruc

section .entry
db 0x67

drive_header:
    bdb_oem:                    times 8 db 0     	; anything 8 chars
    ; BIOS-required geometry
    bdb_bytes_per_sector:       dw 0
    bdb_sectors_per_cluster:    db 0
    bdb_reserved_sectors:       dw 0
    bdb_fat_count:              db 0
    bdb_root_entries:           dw 0
    bdb_total_sectors:          dw 0
    bdb_media_desciptor_type:   db 0
    bdb_sectors_per_fat:        dw 0
    bdb_sectors_per_track:      dw 0
    bdb_heads:                  dw 0
    bdb_hidden_sectors:         dd 0
    bdb_large_sector_count:     dd 0
    ; Extended boot record (BIOS doesn't care)
    ebr_drive_number:           db 0
    ebr_signature:              db 0
    ebr_volume_id:              dq 0
    ebr_volume_label:           times 11 db 0		; Anything 11 chars
    ebr_system_id:              times 8 db 0    	; anything 8 chars

    ; Your custom FS metadata
    reservedClusterMapSectors:	dd 0
drive_end:


; Number of sectors to load into RAM, should be the full size of Boot2
%define ENDL 0x0A, 0x0D, 0x00

; void start(void)
start:
    [bits 16]
	nop
	nop
    xchg bx, bx

    ; Setup stack
	nop
	call int16disable
	mov ax, 0
	mov ss, ax
	mov sp, (0x0500 + 2040)
	call int16enable
	mov cl, 0
.DISKREADnoEDD:
	; Read address
	mov eax, dword __CODEADDR
	mov bx, ax
	shr eax, 16
	mov es, ax
	; LBA
    mov ax, 3
	; Sectors to read
	mov di, __kernel_sectors

	call DISKREAD
	cmp [setup32], byte 0
	jne .DISKREADCONTINUE
	mov cl, 1
	je .DISKREADnoEDD
.DISKREADCONTINUE:
	xchg bx, bx
	; Generate Memory Map
	push BootOut
	call MemDetect16
	add sp, 2
	; Generate GDT Descriptor and IDT Descriptor
    mov ax, [GDTSize]
    mov [.GDTLimit], ax
    mov [.GDTTable], dword GDT

    mov ax, [IDTSize]
    mov [.IDTLimit], ax
    mov [.IDTTable], dword IDT

    ; Get Graphics state
    call setDefaultControllerInfo16
    push bx
    push es
    
    push setup32
    push dword .tempIDTdesc
    push dword .tempGDTdesc
	jmp switch16_32
.tempGDTdesc:
	.GDTLimit:  dw 0
	.GDTTable:  dd 0
.tempIDTdesc:
	.IDTLimit:  dw 0
	.IDTTable:  dd 0
.tempMemMap:
	.Table:		dq 0
	.Limit:		dd 0

; void halt16(void)
halt16:
    [bits 16]
    cli
.halt_:
    nop
    jmp .halt_

; Params:
;	ax: LBA address(as a counter of 512)
; Returns:
;	ch: Cylinder num ; cl: Sector num
;	dh: Head number ; dl: drive number
;	ax: LBA address
LBA2CHS:
	push ax
	xor dx, dx
	div word [bdb_sectors_per_track]
	inc dx
	mov cx, dx
	xor dx, dx
	div word [bdb_heads]
	mov ch, al
	mov dh, dl
	mov dl, [ebr_drive_number]
	pop ax
	ret

; Params:
;	eax: LBA address
;	[es:bx]: Buffer address
;	di: Number of sectors
;   cl: Force CHS Path
DISKREAD:
	[bits 16]
    xor dx, dx
    ; Align to 4 bytes
    push eax
    mov eax, .LBAreadPackage
	push cx
    mov cx, 4
    div cx
	pop cx
    mov [.alignment], dx
    pop eax
    ; Construct DAP Packet
    mov si, dx
    mov [si + .size], byte (.paddinglow - .LBAreadPackage)
    mov [si + ._0], byte 0
	mov word [si + .sectors], di
	mov word [si + .outsegment], es
	mov word [si + .outoffset], bx
	mov [si + .lowerLBA], dword eax
    mov [si + .upperLBA], dword 0
    mov di, 10h
    ; Check-Force CHS
    sub cl, 1
    jnc .noEDD
.testEDD:
    mov ah, 041h
    mov bx, word 055AAh
    mov dl, [ebr_drive_number]
    int 13h
    jc .noEDD
    cmp bx, 0AA55h
    jne .noEDD
    test cx, 1
    jz .noEDD
.EDD:
	mov ah, 42h
	mov dl, [ebr_drive_number]
    mov si, .LBAreadPackage
    add si, word [.alignment]
	int 13h
	jnc .finish
	cmp di, 0
	jz .finish
    jpo .continueEDD
    call .DISKRESET
.continueEDD:
	dec di
    call .finish
	jmp .EDD
.noEDD:
    ; Reconstruct params
    mov si, [.alignment]
    mov eax, [si + .lowerLBA]
    mov es, [si + .outsegment]
    mov bx, [si + .outoffset]
    call LBA2CHS
	mov ax, [si + .sectors]
    mov ah, 02h
    int 13h
    jnc .finish
	cmp di, 0
	jz .finish
    jpo .continuenoEDD
    call .DISKRESET
.continuenoEDD:
	dec di
    call .finish
	jmp .noEDD
.finish:
	mov si, msg_disk_code
	mov al, ah
	xor ah, ah
	call writenum
	mov si, msg_sectors_read
	mov ax, [.sectors]
	call writenum
	mov si, msg_read_address
	mov eax, [.lowerLBA]
	call writenum
	mov si, msg_diskread_out
	call write
    ret
.DISKRESET:
	push ax
	push dx
    mov ah, 00h
    mov dl, 00h
    int 13h
	pop ax
	pop dx
	ret
.alignment: 	dw 0
.LBAreadPackage:
.size:			db 0
._0:			db 0
.sectors:		dw 0
.outsegment:	dw 0
.outoffset:		dw 0
.lowerLBA:		dd 0
.upperLBA:		dd 0
.paddinglow:        times 4 db 0    ; Buffer to allow shifting down 4 bytes

; Params:
;	si: Offset of a string ending with the ENDL macro
write:
	[bits 16]
    push ax
    push si
.loop:
    lodsb
    or al, al
    jz .done

    mov ah, 0x0E
    int 0x10

    jmp .loop
.done:
    pop si
    pop ax

    ret

; Params:
;	si: out buffer, from the end
;	eax: In number
writenum:
	push dx
	push bx
	mov bx, 10
	dec si
.write:
	xor dx, dx
	div bx
	add dx, '0'
	mov [si], dl
	cmp ax, 0
	je .finish
	dec si
	jmp .write
.finish:
	pop bx
	pop dx
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
	out 0x70, al
	pop ax
    ret

;	void int16enable(void)
int16enable:
	[bits 16]
	sti
	push ax
	in al, 0x70
	and al, 7Fh
	out 0x70, al
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
;   void switch16_32(GDTdesc __far *, IDTdesc __far*, void(__far __cdecl *func)(void), BootIn in)
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
    call A20WaitIn16
    mov al, KbdCtrlDisable
    out KbdCtrlCmdPort, al
    ; Read Controller Out Port
    call A20WaitIn16
    mov al, KbdCtrlReadOutPort
    out KbdCtrlCmdPort, al
    
    call A20WaitOut16
    in al, KbdCtrlDataPort
    push eax

    ; Write Controller out port
    call A20WaitIn16
    mov al, KbdCtrlWriteOutPort
    out KbdCtrlCmdPort, al

	; Enable A20 Gate bit
    call A20WaitIn16
    pop eax
    or al, 2		; Bit 2 is the A20 flag bit
	out KbdCtrlDataPort, al

	; Re-enable Keyboard
	call A20WaitIn16
	mov al, KbdCtrlEnable
	out KbdCtrlCmdPort, al
.LoadGDT:
	mov bx, [bp]			; Offset
	mov es, [bp + 2]		; Segment
	lgdt [es:bx]
.LoadIDT:
	mov bx, [bp + 4]		; Offset
	mov es, [bp + 6]		; Segment
	lidt [es:bx]
.finish:
	; Setup to jmp to func
    mov eax, cr0
    or eax, 1
    mov cr0, eax
	jmp dword 0008h:.pmode
.pmode:
	[bits 32]
    add esp, 8
	call int32enable
	ret

A20WaitIn16:
	in al, KbdCtrlCmdPort
	test al, 2
	jnz A20WaitIn16
	ret
A20WaitOut16:
	in al, KbdCtrlCmdPort
	test al, 1
	jz A20WaitOut16
	ret

KbdCtrlDataPort             	equ 0x60
KbdCtrlCmdPort              	equ 0x64

KbdCtrlDisable              	equ 0xAD
KbdCtrlEnable             		equ 0xAE

KbdCtrlReadOutPort              equ 0xD0
KbdCtrlWriteOutPort             equ 0xD1

BootOut: times (BootIn) db 0

sectorreads: db 16
msg_diskread_out: db 'Read Code: 0000'
msg_disk_code: db ', 0000'
msg_sectors_read: db ', 000000000'
msg_read_address: db ENDL
times ((1024 + 512) - ($ - $$)) db 0
