bits 16
org 0x7C00
%define BOOT2ID 0x67
%define BOOT2ADDR 0x0D00
jmp short start
nop

drive_header:
	bdb_oem:                   	db 'MSWIN4.1'
	bdb_bytes_per_sector:       dw 512						; +1
	bdb_sectors_per_cluster:    dw 4						; +3
	bdb_reserved_sectors:       db 20						; +5
	bdb_fat_count:              db 2						; +6
	bdb_root_entries:           dw 0						; +7
	bdb_total_sectors:			dw 0						; +9
	bdb_media_desciptor_type:	db 0F8h						; +11
	bdb_sectors_per_fat:		dw 8						; +12
	bdb_sectors_per_track:		dw 63						; +14
	bdb_heads:					dw 16						; +16
	bdb_hidden_sectors:			dd 0						; +18
	bdb_large_sector_count:		dd 2880						; +22
	; extended boot record:
	ebr_drive_number:			db 80h						; +26
	ebr_signature:				db 29h						; +28
	ebr_volume_id:				db 12h, 99h, 40h, 22h		; +30
	ebr_volume_label:			db 'BLOCKY OS  '			; +46
	ebr_system_id:				db 'FAT12   '				; +57
	; custom_boot_record
	sectormap_entries:			db 128						; +65
drive_end:
%define ENDL 0x0A, 0x0D, 0x00

; This file will simply load Boot2, Boot2 will be the main booting file

global start
start:
	xchg bx, bx
	mov eax, 0
	mov es, ax
	mov ds, ax
	mov eax, BOOT2ADDR
	mov ss, ax
	mov ax, 2048
    mov sp, ax
	; stack pointer is not 512 bytes above ss, which is buffer
	push dword .after
	retf
.after:
	call diskm_read
	; Address to be loaded at
	mov bx, BOOT2ADDR
	; Sectors to be Read.
	mov di, 1
	; LBA Adress
	mov ax, 1

	; Disk read
	call disk_read
	call bt2srch
	test di, di
	jz .no_ld
	; Load Drive header into Stage 2's space
	mov si, drive_header
    ; ES:DI = destination
    mov di, (BOOT2ADDR + 1)
	; Calculate bytes
    mov cx, (drive_end - drive_header)
	; Perform move
    cld
    rep movsb
.ld_s:
	push es
	push word (BOOT2ADDR + 1 + (drive_end - drive_header))
	retf
.no_ld:
	mov si, msg_bt2f
	call write
	mov ah, 0
    int 16h
    jmp 0FFFFH:0

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
;	di: TRUE(1) if bt2 was found
;	di: FALSE(0) if bt2 was not found
bt2srch:
	push bx
	mov bx, BOOT2ADDR
.srch_begin:
	cmp [bx], byte 103
	jz .rett
	add bx, 1
	sub di, 1
	test di, di
	jnz .srch_begin
.no_ld:
	mov si, msg_bt2f
	call write
	mov di, 0
.rett:
	pop bx
	ret

; Params:
;	NONE
; Returns:
;	NONE
diskm_read:
	mov ah, 8
	int 13h
	inc dh
	mov [bdb_heads], dh
	and cl, 0x3f
	mov [bdb_sectors_per_track], cl
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
	xor dx, dx							; ...
	div word [bdb_heads]				; dx: (LBA / bdb_sectors_per_track) % db_heads: Head number
										; ax: (LBA / bdb_sectors_per_track) / db_heads:	Cylinder number
	; cx: Sector number; (ch: ?, cl: Sector number)
	; dx: Head number; (dh: ?, dl: Head number)
	; ax: Cylinder number; (ah: ?, al: Cylinder number)
	mov ch, al
	mov dh, dl
	mov dl, [ebr_drive_number]
	; ch: Cylinder num & 0xff
	; cl: Sector num | ((cylinder num >> 2) & 0xC0)
	; dh: Head number
	; dl: drive number
	pop ax
	ret

; Params:
;	eax: LBA address
;	[es:bx]: Buffer address
;	di: Number of sectors
disk_read:
	; call lbatochs
    xchg bx, bx
    xor dx, dx
    ; Align to 4 bytes
    push eax
    mov eax, .LBAreadPackage
    mov cx, 16
    div cx
    mov [.alignment], dl
    pop eax
    ; Construct DAP Packet
    mov si, [.alignment]
    mov [si + .size], byte 16
    mov [si + .0], byte 0
	mov [si + .sectors], di
	mov [si + .outsegment], es
	mov [si + .outoffset], bx
	mov [si + .lowerLBA], dword eax
    mov di, 16
.testEDD:
    xor ax, ax
    mov ah, 41h
    mov bx, 55AAh
    mov dl, [ebr_drive_number]
    int 13h
    jc .noEDD
    cmp bx, 0AA55h
    jne .noEDD
.EDD:
    xor ax, ax
	mov ah, 0x42
	mov dl, [ebr_drive_number]
    mov si, .LBAreadPackage
    add si, [.alignment]
	int 13h
	jnc .done
	cmp di, 0
	jz .done
    jpo .continueEDD
    call disk_reset
.continueEDD:
	dec di
    call .fail
	jmp .EDD
.noEDD:
    ; Reconstruct params
    mov si, [.alignment]
    mov eax, [si + .lowerLBA]
    mov es, [si + .outsegment]
    mov bx, [si + .outoffset]
    call lbatochs
    mov ah, 02h
    int 13h
    jnc .done
	cmp di, 0
	jz .done
    jpo .continue_noEDD
    call disk_reset
.continue_noEDD:
	dec di
    call .fail
    jmp .noEDD
.done:
	mov si, msg_disks
	call write
	ret
.fail:
    mov si, msg_diskf
    add ah, '0'
    mov [msg_diskf_err_code], ah
    call write
    ret
.alignment: db 0
.LBAreadPackage:
.size:		db 0
.0:			db 0
.sectors:	dw 0
.outsegment:dw 0
.outoffset:	dw 0
.lowerLBA:	dd 0
.upperLBA:	dd 0
.paddinglow:        times 16 db 0    ; Buffer to allow shifting down 3 bytes

disk_reset:
	push ax
	push dx
    mov ah, 00h
    mov dl, 00h
    int 13h
	pop ax
	pop dx
	ret

msg_disks: db 'Read Finish', ENDL
msg_diskf: db 'Read Fail Code: '
msg_diskf_err_code: db '0', ENDL
msg_bt2f: db 'Boot2 missing', ENDL

times (510 - ($ - $$)) db 0 ;Repeat so the Program can be 512 bytes large.
dw 0xAA55              	; The final 2 bytes will be the boot signature.