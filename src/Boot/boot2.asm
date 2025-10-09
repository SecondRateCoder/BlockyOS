org 0x7E00
bits 16

jmp short start
; Copy of boot record, overwritten at Run-time...
boot_record_buffer:
bdb_oem:                   	db 'MSWIN4.1'
bdb_bytes_per_sector:       dw 512
bdb_sectors_per_cluster:     db 1
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

%define ENDL 0x00, 0x0A
kernel_name:                db "kernel0.bin", ENDL, 0
kernel_addr:                dw 0
kernel_cluster:             dw 0

%define DIRH_SIZE 32

boot2msg: db 'This is Boot2', ENDL, 0

global start

start:
    ; Copy over boot record
    mov ax, 0x7C00
    mov es, ax
    mov bx, 72
.memcpy:
    test bx, bx
    jz .memcpy_done
    mov ax, [es:bx]

    push es
    push ax
    mov ax, word boot_record_buffer
    mov es, ax
    pop ax
    mov [es:bx], ax
    pop es
    dec di

.memcpy_done:
    ; Compute LBA of Root directory...
	xor ax, ax
	add ax, [bdb_fat_count]
	mov bl, [bdb_sectors_per_fat]
	xor bx, bx
	mul bx
	add ax, [bdb_reserved_sectors]
	push ax

	; Compute size of root directory...
	mov ax, [bdb_dir_entries_count]
	shl ax, DIRH_SIZE
	xor dx, dx
	div word [bdb_bytes_per_sector]


	test dx, dx
	jz read_root
	inc ax

puts:
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

strncmp:
	push si	;Store string locations as to restore them later
	push di
.begin_retry:
	test cx, cx
	jz .done
	push ax
	mov ax, [ds:si]
	push bx
	mov bx, [es:di]
	test ax, bx
	jnz .done
	dec cx
	inc si
	inc di
	jmp .begin_retry
.done:
	pop di
	pop si
	ret

read_root:
    ; Params:
		; ax: LBA addressing
		; cl: Total sector count
		; es [Location]:bx [Offset]: Memory location
	mov cl, al
	pop ax
	mov bx, buffer
	call disk_read
.kernel_search:
	xor bx, bx
	mov di, buffer
	mov si, kernel_name
	mov cx, 11
	push di
	call strncmp
	pop di
	test cx, cx
	jz .kernel_found
	add di, DIRH_SIZE
	inc bx
	test bx, [bdb_dir_entries_count]
	jl .kernel_search

	mov si, msg_kernel_err
	call puts
	jmp full_restart
.kernel_found:
	mov [kernel_addr], di
	mov ax, [di + 26]	; di should have the kernel addr.
						; Use the byte offset 26 to get the uint16 cluster number.
	mov [kernel_cluster], ax
	; Load both FAT allocation tables to Memory, at buffer
	; Params:
		; ax: LBA addressing
		; cl: Total sector count
		; es [Location]:bx [Offset]: Memory location
	mov ax, [bdb_sectors_per_fat]
	mov bl, [bdb_fat_count]
	xor bh, bh
	mul bx
	; ax: num of sectors
	mov cl, al
	mov ax, [bdb_reserved_sectors]
	mov bx, buffer
	call disk_read
	
	; Load main kernel
	mov bx, word [kernel_ld_segment]
	mov es, bx
	mov bx, [kernel_ld_offset]
.ld_kernel:
	; Read next cluster
	mov ax, [kernel_cluster]
	; 1st cluster:
		; ((kernel_cluster - 2) * sectors_per_cluster) + start_sector
			; start_sector: reserved_sectors + fat_sectors + root_sectors
	sub ax, 2
	xor dx, dx
	mul word [bdb_sectors_per_cluster]
	push ax
	mov ax, [bdb_fat_count]
	xor dx, dx
	mul word [bdb_sectors_per_fat]
	push ax
	mov ax, [bdb_dir_entries_count]
	mul word [bdb_bytes_per_sector]
	mov bp, sp
	add ax, [ss:bp]
	add sp, 2
	add ax, [bdb_reserved_sectors]
	mov bp, sp
	add ax, [ss:bp]
	add sp, 2
	; Params:
		; ax: LBA addressing
		; cl: Total sector count
		; es [Location]:bx [Offset]: Memory location
	mov cl, 1
	call disk_read
	mov si, buffer
	add si, ax
	mov ax, [ds:si]

	add bx, [bdb_bytes_per_sector]
	or dx, dx
	jz .even
.odd:
	shr ax, 4
	jmp .next_cluster

.even:
	and ax, 0x0FFF
.next_cluster:
	cmp ax, 0x0FF8
	jae .read_finish

	mov [kernel_cluster], ax
	jmp .ld_kernel
.read_finish:
	mov dl, [ebr_drive_number]

	mov ax, kernel_ld_segment
	mov ds, ax
	mov es, ax

	jmp kernel_ld_segment:kernel_ld_offset
	jmp full_restart

	cli
	hlt

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

kernel_ld_segment	equ 0x20000
kernel_ld_offset	equ 0

msg_disk_idk: db 'Max reads reached.', ENDL, 0
msg_bt2lod: db 'Jumping to Bootloader 2. ', ENDL, 0
msg_disk_read: db 'Reading from disk. ', ENDL, 0
msg_disk_errormsg: db 'Error when reading Disk. Resetting Floppy disk. ', ENDL, 0
msg_disk_readsucc: db 'Disk read success...', ENDL, 0
msg_hello: db 'hello there!', ENDL, 0
msg_kernel_err: db 'Kernel not found...', ENDL, 0

buffer: