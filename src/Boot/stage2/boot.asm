bits 16
; org 0x8200
section _ENTRY class=CODE
db 103

extern main_
global start

bt1_main: dw 0
; Params:
;	si: Offset of a string ending with the ENDL macro
bt1_write: dw 0
; Params:
;	NONE
; Returns:
;	NONE
bt1_dm_read: dw 0
; Params:
;	ax: LBA address(as a counter of 512)
; Returns:
;	ch: Cylinder num ; & 0xff
;	cl: Sector num ; | ((cylinder num >> 2) & 0xC0)
;	dh: Head number
;	dl: drive number
;	ax: LBA address
bt1_lbatochs: dw 0
; Params:
;	ax: LBA address
;	[es:bx]: Buffer address
;	di: Number of sectors
bt1_diskread: dw 0

bt1_frestart: dw 0
bt1_hlt: dw 0
; bdb_oem:                   1
; bdb_bytes_per_sector:      2
; bdb_sectors_per_cluster:   1
; bdb_reserved_sectors:      1
; bdb_fat_count:             1
; bdb_dir_entries_count:     2
; bdb_total_sectors:		 2
; bdb_media_desciptor_type:	 1
; bdb_sectors_per_fat:		 2
; bdb_sectors_per_track:	 2
; bdb_heads:				 2
; bdb_hidden_sectors:		 4
; bdb_large_sector_count:	 4; 25 bytes

; ; extended boot record:
; ebr_drive_number:			1
; 							1
; ebr_signature:			1
; ebr_volume_id:			4
; ebr_volume_label:			11
; ebr_system_id:			8; 51 bytes
global bt1_drive_header
;uint8_t[51]
bt1_drive_header: times 51 db 0
jmp short start

; Number of sectors to load into RAM, should be the full size of Boot2
%define ENDL 0x0A, 0x00

start:
	cli
	mov ax, ds
	mov ss, ax
	mov sp, 512
	sti
	mov si, msg_bt2
	push es
	push word [bt1_write]
	call call_bt1
	call main_

	push es
	push word [bt1_hlt]
	call call_bt1

; Params:
;	Push segment
;	Push offset
; 	call
call_bt1:
	retf

; global bt1_mainc_, bt1_writec_, bt1_writecl_, bt1_dmreadc_, bt1_lbatochsc_, bt1_diskrc_
; bt1_mainc_:
; 	jmp bt1_mainc
; bt1_writec_:
; 	jmp bt1_writec
; bt1_writecl_:
; 	jmp bt1_writecl
; bt1_dmreadc_:
; 	jmp bt1_dmreadc
; bt1_lbatochsc_:
; 	jmp bt1_lbatochsc
; bt1_diskrc_:
; 	jmp bt1_diskrc


; bt1_mainc:
; 	add sp, 2	; Pop return address ;! Main does not return...
; 	push es
; 	push word [bt1_main]
; 	retf

; ; void bt1_writec(offset[uint16_t])
; bt1_writec:
; 	add sp, 2
; 	pop si	; Retrieve string offset
; 	sub sp, 4
; 	push es
; 	push word [bt1_writec]
; 	retf

; ; void bt1_writecl(segment[uint16_t], offset[uint16_t])
; bt1_writecl:	; For long, [segment:offset] string access
; 	mov ax, es
; 	add sp, 2
; 	pop si
; 	pop es
; 	sub sp, 4
; 	push .finish
; 	push es
; 	push word [bt1_writec]
; 	retf
; .finish:	; Expose .finish on retf, restore es
; 	mov es, ax
; 	ret

; ; void bt1_dmreadc(void)
; bt1_dmreadc:
; 	push es
; 	push word [bt1_dm_read]
; 	retf

; ; uint8_t[4] bt1_lbatochsc(lba[uint16_t])
; bt1_lbatochsc:
; 	add sp, 2
; 	pop ax
; 	sub sp, 4
; 	push .finish
; 	push es
; 	push word [bt1_lbatochs]
; 	retf
; .finish:
; 	mov [.return], ch
; 	mov [.return + 1], cl
; 	mov [.return + 2], dh
; 	mov [.return + 3], dl
; 	mov ax, .return
; 	ret
; .return: dd 0	; 4 byte return

; ; void bt1_diskrc(LBA[uint16_t], segment[uin16_t], offset[uint16_t], sector number[uint16_t])
; bt1_diskrc:
; 	add sp, 2
; 	pop di
; 	pop bx
; 	mov word [.es_temp], es
; 	pop es
; 	pop ax
; 	sub sp, 10
; .es_temp: dw 0
; 	push .finish
; 	push word [.es_temp]
; 	push word [bt1_diskread]
; 	retf
; .finish:
; 	mov es, word [.es_temp]
; 	ret


global bt1_writec_, bt1_writecl_, bt1_mainc_, bt1_dmreadc_, bt1_lbatochsc_, bt1_diskrc_
; void bt1_writec_(uint16_t offset)
bt1_writec_:
    push bp
    mov bp, sp
    ; read first argument (uint16) into AX without changing SP
    mov ax, [bp+4]
    ; save callee-saved registers
    push bx
    push si
    push di
    ; move arg to SI (asm routine expects SI)
    mov si, ax

    ; call existing mechanism that does far dispatch to bt1_write
    push es
    push word [bt1_write]
    call call_bt1

    ; restore registers
    pop di
    pop si
    pop bx
    mov sp, bp
    pop bp
    ret

; void bt1_writecl_(uint16_t seg, uint16_t offs)
bt1_writecl_:
    push bp
    mov bp, sp
    mov ax, [bp+4]    ; seg (first declared param)
    mov dx, [bp+6]    ; offs (second declared param)
    push bx
    push si
    push di

    ; set ES to seg, SI to offs, then call bt1_writec via far dispatch
    mov si, dx
    mov ax, ax        ; seg currently in AX
    mov es, ax        ; be careful: if seg must be moved to ES via other mechanism, adapt
    push es
    push word [bt1_write]
    call call_bt1

    pop di
    pop si
    pop bx
    mov sp, bp
    pop bp
    ret

; void bt1_mainc_(void)
bt1_mainc_:
    push bp
    mov bp, sp
    push bx
    push si
    push di

    push es
    push word [bt1_main]
    call call_bt1

    pop di
    pop si
    pop bx
    mov sp, bp
    pop bp
    ret

; uint8_t[] bt1_lbatochsc_(uint16_t lba)
bt1_lbatochsc_:
    push bp
    mov bp, sp
    mov ax, [bp+4]     ; lba
    push bx
    push si
    push di

    mov si, ax         ; pass LBA in AX/SI as your protocol expects
    push .finish
    push es
    push word [bt1_lbatochs]
    call call_bt1
.finish:
    ; copy results into .return (same as you did)
    mov [.return], ch
    mov [.return + 1], cl
    mov [.return + 2], dh
    mov [.return + 3], dl

    pop di
    pop si
    pop bx
    mov sp, bp
    pop bp
    ret
.return: dd 0

global put_vidteletype_
; void put_vidteletype(c[char]
put_vidteletype_:
	push bp
	mov bp, sp

	push bx
	mov ah, 0Eh
	mov al, [bp + 2]
	mov bh, [bp + 4]

	int 10h
	pop bx
	mov sp, bp

	pop bp
	ret

; void div64_32
global div64_32_
div64_32_:

msg_bt2: db 'This is Boot2', ENDL, 0
buffer: