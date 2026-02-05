bits 32

%define bdb_oem						byte [_bootDrive]
%define bdb_bytes_per_sector		word [_bootDrive + 1]
%define bdb_sectors_per_cluster		word [_bootDrive + 3]
%define bdb_reserved_sectors		byte [_bootDrive + 5]
%define bdb_fat_count				byte [_bootDrive + 6]
%define bdb_dir_entries_count		word [_bootDrive + 7]
%define bdb_total_sectors		    word [_bootDrive + 9]
%define bdb_media_desciptor_type	byte [_bootDrive + 11]
%define bdb_sectors_per_fat 		word [_bootDrive + 12]
%define bdb_sectors_per_track		word [_bootDrive + 14]
%define bdb_heads					word [_bootDrive + 16]
%define bdb_hidden_sectors			dword [_bootDrive + 18]
%define bdb_large_sector_count		dword [_bootDrive + 22]
; extended boot record:
%define ebr_drive_number			byte [_bootDrive + 26]
%define ebr_signature				byte [_bootDrive + 28]
%macro ebr_volume_id 2				
                                    mov %2, byte [_bootDrive + 30 + %1]
%endmacro
%macro ebr_volume_label 2
                        			mov %2, byte [_bootDrive + 46 + %1]
 %endmacro
%macro ebr_system_id 2
                    				mov %2, byte [_bootDrive + 57 + %1]
 %endmacro
; custom_boot_record:
%define sectormap_entries			byte [_bootDrive + 65]

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
db 103

extern __kernel_start, __kernel_end

extern IDTdesc, GDTdesc

global _bootDrive
extern _setup32
extern _int32enable
extern _main32
extern _idtDesc

extern _MemDetect16

_bootDrive: times 58 db 0
jmp short _start

; Number of sectors to load into RAM, should be the full size of Boot2
%define ENDL 0x0A, 0x0D, 0x00

global _start
; void start(void)
_start:
    [bits 16]
    xchg bx, bx

    ; Setup stack
	call int16disable
	mov ax, 0x7C00
	mov ss, ax
	mov sp, 1024
	call int16enable
    ; LBA2CHS
.LBA2CHS:
    mov ax, 1
    xor dx, dx
    div bdb_sectors_per_track

    inc dx
    mov cx, dx
    xor dx, dx
    div bdb_heads

    mov ch, al
    mov dh, al
    mov dl, ebr_drive_number
.DISKREAD:
    mov ax, word __kernel_end
    sub ax, word __kernel_start
    push dx
    xor dx, dx
    div bdb_bytes_per_sector
    pop dx
    
	mov ah, 2
	stc
	push ds
	int 13h
	pop ds
	; Reset floppy on every other try.
	test di, di
	jpo .done
	call .DISKRESET
.DISKRESET:
	push ax
	push dx
    mov ah, 00h
    mov dl, 00h
    int 13h
	pop ax
	pop dx
	ret
.done:
    ; Get Memory Map
    sub sp, 4   ; Enough space for far bla bla
    push sp     ; To emulate ptr
    add sp, 2   ; Pop ptr
    ; Jump to boot2's .c file
	push word GDTdesc
	push word IDTdesc
	push cs
	push word _main32
	call _switch16_32
; void halt16(void)
.halt16:
    [bits 16]
    cli
.halt_:
    nop
    jmp .halt_

global _halt32
; void halt32(void)
_halt32:
    [bits 32]
    cli
._halt_:
    nop
    jmp ._halt_

_bochs_breakpoint16:
	[bits 32]
    xchg bx, bx
    nop
    ret

global _bochs_breakpoint32
;   void bochs_breakpoint(void)
_bochs_breakpoint32:
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


global _switch16_32
;   void switch16_32(GDTdesc __far *, IDTdesc __far*, void(__far __cdecl *func)(void), function In args)
_switch16_32:
    [bits 16]
    mov bp, sp
    ; Nothing needs to be saved as since, the switcher does NoT revert

    call int16disable
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
	mov es, [bp + 6]		; Segment
	mov bx, [bp + 8]		; Offset
	lgdt [es:bx]
.LoadIDT:
	mov es, [bp + 10]		; Segment
	mov bx, [bp + 12]		; Offset
	lidt [es:bx]
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
    leave
	ret

.finish:
	; Setup to jmp to func
	mov edx, [ebp + 14]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
	jmp dword 0000h:.pmode
.pmode:
	[bits 32]
    add esp, 12
	call _int32enable
	jmp dword edx

KbdCtrlDataPort             	equ 0x60
KbdCtrlCmdPort              	equ 0x64

KbdCtrlDisable              	equ 0xAE
KbdCtrlEnable             		equ 0xAD

KbdCtrlReadOutPort             equ 0xAE
KbdCtrlWriteOutPort            equ 0xAD

BootInfo:
    istruc BootIn
        at BootIn.MemRegion,     dq 0
    iend

buffer: