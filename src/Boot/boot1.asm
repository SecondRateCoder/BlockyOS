org 0x7C00
; 0xf0000:e05b
bits 16

; FAT12 Headers:
jmp short start
nop

bdb_oem:                   	db 'MSWIN4.1'
bdb_bytes_per_sector:       dw 512
bdb_sectors_per_cluster:    db 1
bdb_reserved_sectors:       db 2
bdb_fat_count:              db 2
bdb_dir_entries_count:      dw 0F0h
bdb_total_sectors:			dw 2868
bdb_media_desciptor_type:	db 0F0h
bdb_sectors_per_fat:		dw 9
bdb_sectors_per_track:		dw 18
bdb_heads:					dw 2
bdb_hidden_sectors:			dd 0
bdb_large_sector_count:		dd 0

; extended boot record:
ebr_drive_number:			db 0
							db 0
ebr_signature:				db 29h
ebr_volume_id:				db 12h, 99h, 40h, 22h
ebr_volume_label:			db 'BLOCKY OS  '
ebr_system_id:				db 'FAT12   '
cylinder_count: dw 0
global start

%define ENDL 0x0A, 0x00

; This file will simply load Boot2, Boot2 will be the main booting file

; Push decrements sp
; Pop increments sp
start:
	xchg bx, bx

	db 0x66
	xor ax, ax
	mov ss, ax
	mov es, ax
	mov ds, ax
	mov ax, 0x7C00
    add ax, 512
    mov sp, ax
	push es
	push word .after
	retf
.after:
	mov si, msg_hello
	call write

	call diskm_read

	add ax, 512
	mov bx, ax
	mov ax, 2
	mov di, 1
	call disk_read
	; Setup interface for Boot2 to access Boot1 instructions.
	mov bx, buffer
	mov word [bx], start
	add bx, 2
	mov word [bx], write
	add bx, 2
	mov word [bx], diskm_read
	add bx, 2
	mov word [bx], lbatochs
	add bx, 2
	mov word [bx], disk_read
	add bx, 2
	mov word [bx], full_restart
	add bx, 2
	mov word [bx], halt
	jmp buffer

; Params:
;	si: Offset of a string ending with the ENDL macro
write:
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
;	NONE
; Returns:
;	NONE
diskm_read:
	push ax
	push dx
	push cx
	mov ah, 8
	int 13h
	inc dh
	mov [bdb_heads], dh
	and cl, 0x3f
	mov [bdb_sectors_per_track], cl
	pop cx
	pop dx
	pop ax
	ret



; Params:
;	ax: LBA address(as a counter of 512)
; Returns:
;	ch: Cylinder num & 0xff
;	cl: Sector num | ((cylinder num >> 2) & 0xC0)
;	dh: Head number
;	dl: drive number
;	ax: LBA address
lbatochs:
	push ax
	xor dx, dx
	div word [bdb_sectors_per_track]	; ax: LBA / bdb_sectors_per_track
										; dx: LBA % bdb_sectors_per_track
										; ...
	inc dx								; dx: (LBA % bdb_sectors_per_track) + 1: Sector number
	mov cx, dx							; cx: Sector number
	dec dx								; dx: LBA % bdb_sectors_per_track
	xor dx, dx							; ...
	div word [bdb_heads]				; dx: (LBA / bdb_sectors_per_track) % db_heads: Head number
										; ax: (LBA / bdb_sectors_per_track) / db_heads:	Cylinder number
	; cx: Sector number; (ch: ?, cl: Sector number)
	; dx: Head number; (dh: ?, dl: Head number)
	; ax: Cylinder number; (ah: ?, al: Cylinder number)
	mov ch, al
	mov dh, dl
	mov dl, [ebr_drive_number]
	; cx: Sector number; (ch: cylinder number, cl: Sector number)
	; dx: Head number; (dh: head number, dl: Drive number)
	mov al, ch
	shr ch, 2
	and ch, 0xC0
	or cl, ch
	mov ch, al
	and ch, 0xff
	; ch: Cylinder num & 0xff
	; cl: Sector num | ((cylinder num >> 2) & 0xC0)
	; dh: Head number
	; dl: drive number
	pop ax
	ret

; Params:
;	ax: LBA address
;	[es:bx]: Buffer address
;	di: Number of sectors
disk_read:
	call lbatochs
	mov ax, di
	mov di, 5
.begin_retry:
	mov si, msg_diskr
	call write
	mov ah, 2
	stc
	int 13h
	jnc .done
	test di, di
	jz full_restart
	mov si, msg_diskf
	call write
	jmp .begin_retry
.done:
	mov si, msg_disks
	call write
	ret
	
full_restart:
    mov ah, 0
    int 16h
    jmp 0FFFFH:0
halt:
    cli ; Disable interrupts
    hlt

msg_hello: db 'Hello', ENDL, 0
msg_diskr: db 'Reading from disk', ENDL, 0
msg_disks: db 'Disk read success', ENDL, 0
msg_diskf: db 'Disk read failed', ENDL, 0
times 510 - ($ - $$) db 0 ;Repeat so the Program can be 512 bytes large.
dw 0xAA55              	; The final 2 bytes will be the boot signature.
buffer: