 bits 16
org 0x7C00
; 0xf0000:e05b

; FAT12 Headers:
jmp short start
nop

bdb_oem:                   	db 'MSWIN4.1'
bdb_bytes_per_sector:       dw 128
bdb_sectors_per_cluster:    db 2
bdb_reserved_sectors:       db 10
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
ebr_signature:				db 29h
ebr_volume_id:				db 12h, 99h, 40h, 22h
ebr_volume_label:			db 'BLOCKY OS  '
ebr_system_id:				db 'FAT12   '	; 25 bytes
; custom_boot_record
segment_clusters:			db 128			; 26 bytes
global start

%define ENDL 0x0A, 0x0D, 0x00

; This file will simply load Boot2, Boot2 will be the main booting file

; Push decrements sp
; Pop increments sp
start:
	xchg bx, bx

	db 0x66
	xor ax, ax
	mov ax, 0
	mov es, ax
	mov ds, ax
	mov ax, buffer
	mov ss, ax
	add ax, 512
    mov sp, ax
	; stack pointer is not 512 bytes above ss, which is buffer
	push es
	push word .after
	retf
.after:
	call diskm_read

	; Address to be loaded at
	mov bx, buffer
	add bx, 1024

	; Sectors to be Read.
	mov di, 20

	; LBA Adress
	mov ax, 1

	; Disk read
	call disk_read
	call bt2_srch
	test di, di
	jz .no_ld

	; Load Drive header into Stage 2's space
	mov si, bdb_oem
    ; ES:DI = destination
    mov di, buffer    ; e.g. 1025
	add di, 1025

    mov cx, segment_clusters - bdb_oem + 1   ; 58 bytes total

    cld
    rep movsb

.ld_s:
	push es
	mov bx, segment_clusters
	sub bx, bdb_oem
	add bx, buffer
	add bx, 1024
	push word bx
	retf
.no_ld:
	mov si, msg_bt2f
	call write
	jmp full_restart

; Params:
;	si: Offset of a string ending with the ENDL macro
write:
	push bx
    push ax
    push si
	; xchg bx, bx
	pop bx
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
;	di: TRUE(1) if bt2 was found
;	di: FALSE(0) if bt2 was not found
bt2_srch:
	push bx
	mov bx, buffer
	add bx, 1024
	mov di, 512
.srch_begin:
	sub [bx], byte 103
	jz .ld_s
	add [bx], byte 103
	add bx, 1
	sub di, 1
	test di, di
	jnz .srch_begin
.no_ld:
	mov si, msg_bt2f
	call write
	mov di, 0
	jmp .rett
.ld_s:
	add [bx], byte 103
	mov si, msg_bt2s
	mov di, 1
	call write
.rett:
	pop bx
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
	mov ax, 0
	mov es, ax
	pop cx
	pop dx
	pop ax
	ret

; Params:
;	ax: LBA address(as a counter of 512)
; Returns:
;	ch: Cylinder num ; & 0xff
;	cl: Sector num ; | ((cylinder num >> 2) & 0xC0)
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
;	mov al, ch
;	shr ch, 2
;	and ch, 0xC0
;	or cl, ch
;	mov ch, al
;	and ch, 0xff
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
	; push bx
	; xchg bx, bx
	; pop bx
	call lbatochs
	mov ax, di
	mov di, 11
.begin_retry:
	dec di
	mov ah, 2
	stc
	push ds
	int 13h
	pop ds
	; Reset floppy on every other try.
	test di, di
	jpo .cont1
	call disk_reset
.cont1:
	push di
	; Search for Boot2, jump to finish if so
	call bt2_srch
	; In this case, if di == 1(true) then 0 flag is set, otherwise then subtract underflows and sets the carry flag
	sub di, 1
	pop di
	jz .done
	mov si, msg_diskf
	inc ah
	mov [msg_diskf_err_code], ah
	call write
	jmp .begin_retry
.done:
	mov si, msg_disks
	call write
	ret

disk_reset:
	push ax
	push dx
    mov ah, 00h
    mov dl, 00h
    int 13h
	pop ax
	pop dx
	ret
	
full_restart:
	call disk_reset
    mov ah, 0
    int 16h
    jmp 0FFFFH:0
halt:
	mov si, msg_hlt
	call write
    cli ; Disable interrupts
    hlt

msg_hlt: db 'Halt', ENDL
msg_disks: db 'Read Success', ENDL
msg_diskf: db 'Read Fail, Code: '
msg_diskf_err_code: db '0', ENDL
msg_bt2s: db 'Boot2 found', ENDL
msg_bt2f: db 'Boot2 not found', ENDL
times 510 - ($ - $$) db 0 ;Repeat so the Program can be 512 bytes large.
dw 0xAA55              	; The final 2 bytes will be the boot signature.
buffer:
