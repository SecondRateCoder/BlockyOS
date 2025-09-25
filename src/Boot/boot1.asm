org 0x7C00
; 0xf0000:e05b
bits 16


; FAT12 Headers:
jmp short start
nop

bdb_open:                   db 'MSWIN4.1'
bdb_bytes_per_sector:       dw 512
dbd_sector_per_cluster:     db 1
dbd_reserved_sectors:       db 1
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

global start

%define ENDL 0x00, 0x0A
BOOT2: db word 0x7F00
; This file will simply load Boot2, Boot2 will be the main booting file

start:
    xchg bx, bx
    ; Print hello
    mov si, msg_hello
    call puts
    ; Clear ax, ds and es
    db 0x66             ; Operand-size override
    xor ax, ax          ; Actually clears EAX
    xor ah, ah
    xor al, al
    
    mov ds, ax
    mov es, ax

    ;Stack set-up
    mov ss, ax
    ; Calculate the offs so that it doesn't overwrite itself
    mov ax, 0x7C00
    add ax, 512
    mov sp, ax

    ;Attempt reading from the disk.
    mov dl, [ebr_drive_number]
    xor ax, ax
	mov ax, 1
	mov cl, 1
    mov ch, 0
    ; Location to load Boot2
	mov bx, [BOOT2]
    mov ah, 0x02
    ;* Why is it starting here?
    mov al, 2
	call disk_read
    mov si, msg_bt2lod
    call puts
    jmp [BOOT2]
    mov si, [BOOT2]
    call puts
    hlt

.halt:
    cli
    jmp .halt

; Print a string.
; Argument: si => Pointer to string that ends with ENDL macro
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

;! Disk control

; Convert LBA addressing to CHS addressing.
; ax => LBA adressing scheme

;Returns:
    ; cx [0 - 5]: Sector index
    ; cx [6 - 15]: cylinder index
    ; dh: head index
lbatochs:
	; save modified registers,
	push ax
	push dx
	; push cl
	; push ch
	; push dh

    ; DIV performs an integer division, using ax[Operand] and dx[Remainder] as well as the operand as the argument
    xor dx, dx
    div word [bdb_sectors_per_track] ; Divide [dx,ax] by [bdb_sectors_per_track], ax = LBA / [bdb_sectors_er_track]
                                     ; && dx = LBA % [bdb_sectors_per_track]
    inc dx                           ; dx = LBA % [bdb_sectors_per_track]
    mov cx, dx

    xor dx, dx ; No remainder
    div word [bdb_heads] ; ax = (LBA / [bdb_sectors_per_track]) / [bdb_heads] = LBA-CHS cylinder
                         ; && dx = (LBA / [bdb_sectors_per_track]) % [bdb_heads] = LBA-CHS head
    mov dh, dl  ;!         ; dh = LBA-CHS head
    mov ch, al  ;!       ; ch implemented
    shl ah, 6
	; mov dl, [ebr_drive_number]
    ; shl cx, 6            ; cx [8 - 15] = LBA-CHS cylinder
	or cl, ah ;!
    ;CX			CH | AX | CL
	; cylinder: 8b,	
    ; sector:		2b,  6b
	; save modified registers,
    mov dl, 0x00
	pop ax
	pop dx
	ret
	; push cl
	; push ch
	; push dh

; mov ah, 0x02        ; BIOS read sector function
; mov al, 1           ; Number of sectors to read
; mov ch, cylinder    ; Cylinder number
; mov cl, sector      ; Sector number (1-based!)
; mov dh, head        ; Head number
; mov dl, drive       ; Drive number (0x00 = floppy, 0x80 = HDD)
; mov bx, buffer      ; ES:BX points to memory buffer
; int 0x13            ; Call BIOS
; jc error_handler    ; If carry flag set, handle error

; Attempt reading from the Disk.
; ax: LBA Address
; cl: Number of Sectors to read(up to 128)
; dl: drive number(0)
; es:bx Memory address to store data at
disk_read:
	mov [cx_temp], cx

    ; Warn of reading attempt
    mov si, msg_disk_read
    call puts

    ; Convert to CHS
    ; call lbatochs
    mov di, 4
	jmp .retry
    jmp .done
; Retry disk reading
.retry:
    pusha
    ; mov ah, 0x02
    ; mov al, [maxsector_read]
    ; xor eax, eax
    mov ax, 0x0
    mov es, ax         ; Buffer segment
    mov ah, 0x02        ; Read sector
    mov al, 0x01        ; One sector
    mov ch, 0x00        ; Cylinder
    mov cl, 0x02        ; Sector (must be 1–63)
    mov dh, 0x00        ; Head
    mov dl, 0x00        ; Drive (floppy)
    mov bx, 0x7F00      ; Buffer offset

    ; stc	; set carry flag
    int 13h
    jnc .done	; If the BIOS fails it should have carry flag un-set

	; If carry flag still set then failed, disk should be reset and jump to beginning.
    ; Test for any possible fails.
    test di, di
    jnz .diskr_nd
    test ax, [bios_diskerrA]
    jz .disk_readerror
    jmp .diskr_nd
    test ax, [bios_diskerrB]
    jz .disk_readerror
    jmp .diskr_nd
    test ax, 0x0
    jz .done
    ; If not done at all
.diskr_nd:
    call .disk_reset
    popa

    dec di
    test di, di
    jnz .retry

; Disk reading, gave up
; - | AH Value | Meaning 
; | 01h    | Invalid function or parameter
; | 02h    | Address mark not found
; | 04h    | Sector not found
; | 10h    | CRC error
; | 20h    | Controller failure
; | 40h    | Seek failure
; | 80h    | Disk timeout

.done:
    mov si, disk_readsucc
    call puts
	pop cx
    ret

; Reset disk reading.
; three tries
; No arguments
.disk_reset:
    mov si, disk_resetmsg
    call puts
    ret
; Print error Message.
; No arguments
.disk_readerror:
    mov si, disk_errormsg
    call puts
    jmp waitkey_reboot

; Reset System.
waitkey_reboot:
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


msg_hello: db 'hello there! ', ENDL, 0
msg_bt2lod: db 'Jumping to Bootloader 2. ', ENDL, 0
msg_disk_read: db 'Reading from disk. ', ENDL, 0
disk_errormsg: db 'Error when reading Disk. ', ENDL, 0
disk_resetmsg: db 'Resetting Floppy disk. ', ENDL, 0
disk_readsucc: db 'Disk read success...', ENDL, 0
maxsector_read: db 1
bios_diskerrA: db 0x02
bios_diskerrB: db 0x01
cx_temp: db 0x0

times 510 - ($ - $$) db 0 ;Repeat so the Program can be 512 bytes large.
dw 0xAA55              ; The final 2 bytes will be the boot signature.