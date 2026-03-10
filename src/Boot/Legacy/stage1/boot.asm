bits 16
org 0x7C00
%define BOOT2ID 0x67
%define BOOT2ADDR 0x0D00
jmp short start
nop

drive_header:
    bdb_oem:                    db 'BLOCKYOS'     ; anything 8 chars
    ; BIOS-required geometry
    bdb_bytes_per_sector:       dw 512
    bdb_sectors_per_cluster:    db 0              ; unused (custom FS)
    bdb_reserved_sectors:       dw 1              ; keep 1 for bootloader
    bdb_fat_count:              db 0              ; unused
    bdb_root_entries:           dw 0              ; unused
    bdb_total_sectors:          dw 0              ; must be 0 for large disks
    bdb_media_desciptor_type:   db 0F8h           ; HDD
    bdb_sectors_per_fat:        dw 0              ; unused
    bdb_sectors_per_track:      dw 63             ; BIOS geometry
    bdb_heads:                  dw 16             ; BIOS geometry
    bdb_hidden_sectors:         dd 0              ; no partition offset
    bdb_large_sector_count:     dd 1032192        ; 512MB disk
    ; Extended boot record (BIOS doesn't care)
    ebr_drive_number:           db 80h
    ebr_signature:              db 29h
    ebr_volume_id:              db 12h, 99h, 40h, 22h
    ebr_volume_label:           db 'BLOCKY OS  '
    ebr_system_id:              db 'CUSTOMFS'     ; anything 8 chars

    ; Your custom FS metadata
    reservedClusterMapSectors:	dd 2048
drive_end:

%define ENDL 0x0A, 0x0D, 0x00

; This file will simply load Boot2, Boot2 will be the main booting file

global start
start:
	xchg bx, bx
	mov eax, 0
	mov es, ax
	mov ds, ax
	mov ss, ax
	mov ax, (BOOT2ADDR - 8)
    mov sp, ax
	; stack pointer is not 512 bytes above ss, which is buffer
	push dword .after
	retf
    ; Don't force noEDD
    mov cl, 1
.after:
	call diskm_read
	; Address to be loaded at
	mov bx, BOOT2ADDR
	; Sectors to be Read.
	mov di, 2
	; LBA Adress
	mov ax, 1

	; Disk read
	call DISKREAD
	call bt2srch
	test di, di
	jnz .ld_s
.ld_s:
	; Load Drive header into Stage 2's space
	mov si, drive_header
    ; ES:DI = destination
    mov di, (BOOT2ADDR + 1)
	; Calculate bytes
    mov cx, (drive_end - drive_header)
	; Perform move
    cld
    rep movsb
	push dword (BOOT2ADDR + 1 + (drive_end - drive_header))
	retf
.no_ld:
    ; set cl to force CHS
    mov cl, 0
    jmp short .after
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
;   cl: Force CHS Path
DISKREAD:
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
    ; Force CHS
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
    call DISKRESET
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
    call lbatochs
    mov ax, [si + .sectors]
    mov ah, 02h
    int 13h
    jnc .finish
	cmp di, 0
	jz .finish
    jpo .continuenoEDD
    call DISKRESET
.continuenoEDD:
	dec di
    call .finish
    jmp .noEDD
.finish:
	mov si, msg_diskcode
    add ah, '0'
    mov [msg_disk_code], ah
    add al, '0'
    mov [msg_sectors_read], al
    call write
    ret
.alignment: dw 0
.LBAreadPackage:
.size:		db 0
._0:		db 0
.sectors:	dw 0
.outsegment:dw 0
.outoffset:	dw 0
.lowerLBA:	dd 0
.upperLBA:	dd 0
.paddinglow:        times 4 db 0    ; Buffer to allow shifting down 4 bytes

DISKRESET:
	push ax
	push dx
    mov ah, 00h
    mov dl, 00h
    int 13h
	pop ax
	pop dx
	ret

msg_diskcode: db 'Read Code: '
msg_disk_code: db '0, '
msg_sectors_read: db '0', ENDL
msg_bt2f: db 'Boot2 missing', ENDL

times (510 - ($ - $$)) db 0 ;Repeat so the Program can be 512 bytes large.
dw 0xAA55              	; The final 2 bytes will be the boot signature.