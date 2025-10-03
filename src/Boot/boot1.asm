org 0x7C00
; 0xf0000:e05b
bits 16


; FAT12 Headers:
jmp short start
nop

bdb_oem:                   	db 'MSWIN4.1'
bdb_bytes_per_sector:       dw 512
bdb_sector_per_cluster:     db 1
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
kernel_cluster: dw 0
kernel_addr: dw 0

global start

%define ENDL 0x0A, 0x00
%define DIRH_SIZE 32

BOOT2: db word 0x7F00
BOOT2_MSG: db word 0x7F03
; This file will simply load Boot2, Boot2 will be the main booting file

start:
    xchg bx, bx
    ; Print hello
    ; Clear ax, ds and es
    db 0x66             ; Operand-size override
    xor ax, ax          ; Actually clears EAX
    ; xor ah, ah
    ; xor al, al
	mov ds, ax
    mov es, ax
    ;Stack set-up
    mov ss, ax
    ; Calculate the offs so that it doesn't overwrite itself
    mov ax, 0x7C00
    add ax, 512
    mov sp, ax
	; Resolve differences in booting
	push es
	push word .after
	retf
.after:
	; Use BIOS to read disk metadata.
	call read_disk_meta

	add ax, 64
	mov es, ax

; Print a string.
; Argument: si => Pointer to string that ends with ENDL macro
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

;! Disk control

; Read drive parameters and push them into the labels: [bdb_heads]: head count and [bdb_sectors_per_track]: sector count
read_disk_meta:
	push es
	push ax
	mov ah, 0x80
	int 13h
	jc full_restart
	pop ax
	pop es

	and cl, 0x3F
	xor ch, ch
	mov [bdb_sectors_per_track], cx	; Sector count.

	inc dh
	mov [bdb_heads], dh				;head count
	ret


; Convert LBA addressing to CHS addressing.
; ax => LBA adressing scheme
;Returns:
	; ch: cylinder number
	; cl: sector number + 1
	; dh: head number
lbatochs:
	; save modified registers,
	push dx
    push ax
    div word [bdb_sectors_per_track]; ax: LBA/bdb_sectors_per_track
	pop ax							; restore ax: LBA/bdb_sectors_per_track
                                	; dx: LBA %(Remainder Division) bdb_sectors_per_track
    inc dx                      	; dx: (LBA%bdb_sectors_per_track) + 1: Sector number
    push dx							; store Sector number
	push ax							; store LBA address
	xor dx, dx
	mov cx, ax						; store cx: LBA/bdb_sectors_per_track
    div  word[bdb_heads]        	; dx: (LBA/bdb_sectors_per_track) % bdb_heads: head number
									; ax: (LBA/bdb_sectors_per_track) / bdb_heads: cylinder number

	;At this point:
		; ax: cylinder number
		; dx: head number
		; cx: LBA/bdb_sectors_per_track
		; Stack:
			; [1st]: sector number
	mov ax, cx
	mov ch, al						; ch: cylinder number
	add sp, 2						; "pop" register
	pop ax							; ax->al: sector number
	sub sp, 2
	mov cl, al						; cl: sector number
	shl dx, 8						; move the head number in dx to the dh register, clearing dl
	; Pull back to original dl
	add sp, 1
	mov bp, sp
	mov dl, byte [ss:bp]
	add sp, 1
	sub sp, 4
	pop ax
	add sp, 4
	ret

; mov ah, 0x02        ; BIOS read sector function
; mov al, 1           ; Number of sectors to read
; mov ch, cylinder    ; Cylinder number
; mov cl, sector      ; Sector number (1-based!)
; mov dh, head        ; Head number
; mov dl, drive       ; Drive number (0x00 = floppy, 0x80 = HDD)
; mov bx, buffer      ; ES:BX points to memory buffer
; int 0x13            ; Call BIOS
; jc error_handler    ; If carry flag set, handle error

; Params:
	; ax: LBA addressing
	; cl: Total sector count
	; es [Location]:bx [Offset]: Memory location
disk_read:
;! Using:
	; ax, cx, dx
	push cx
	call lbatochs
	mov di, 4
.begin_retry:
	mov si, msg_disk_read
	call write
	; mov ah, 0x02        ; BIOS read sector function
	; mov al, 1           ; Number of sectors to read
	; mov ch, cylinder    ; Cylinder number
	; mov cl, sector      ; Sector number (1-based!)
	; mov dh, head        ; Head number
	; mov dl, drive       ; Drive number (0x00 = floppy, 0x80 = HDD)
	; mov bx, buffer      ; ES:BX points to memory buffer
	; int 0x13            ; Call BIOS
	; jc error_handler    ; If carry flag set, handle error
	and ch, 0xff
	pop ax
	; add sp, 1
	mov dl, [ebr_drive_number]
	mov ah, 0x02
	mov al, bh
	pusha
	stc
	int 0x13
	popa
	; carry flag set and ah not 0 if fail, otherwise for success
	jnc .disk_success
	dec di
	test di, di
	jz .disk_unsure
.disk_err:
	mov si, msg_disk_errormsg
	call write
	mov ah, 0x0
	int 0x13
	jmp .begin_retry
.disk_unsure:
	mov si, msg_disk_idk
	call write
.disk_success:
	mov si, msg_disk_readsucc
	call write
	ret




full_restart:
    mov ah, 0
    int 16h
    jmp 0FFFFH:0
halt:
    cli ; Disable interrupts
    hlt

; pmode:
;     mov  bx, 0x10          ; select descriptor 2, instead of 1
;     mov  ds, bx            ; 10h = 10000b

;     and al, 0xFE            ; back to realmode
;     mov  cr0, eax          ; by toggling bit again

msg_disk_idk: db 'Max reads reached.', ENDL, 0
msg_hello: db 'hello there! ', ENDL, 0
msg_bt2lod: db 'Jumping to Bootloader 2. ', ENDL, 0
msg_disk_read: db 'Reading from disk. ', ENDL, 0
msg_disk_errormsg: db 'Error when reading Disk. Resetting Floppy disk. ', ENDL, 0
msg_disk_readsucc: db 'Disk read success...', ENDL, 0
kernel_name: db "kernel0.bin", ENDL, 0


times 510 - ($ - $$) db 0 ;Repeat so the Program can be 512 bytes large.
dw 0xAA55              ; The final 2 bytes will be the boot signature.
buffer: