org 0x7C00
bits 16

%define ENDL 0x00, 0x0A
global _start

# FAT12 Headers
jmp short _start
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

# EXTRAS:
ebr_drive_number:			db 0
							db 0
ebr_signature:				db 29h
ebr_volume_id:				db 12h, 34h, 56h, 78h
ebr_volume_label:			db 'BLOCKY OS'
ebr_system_id:				db 'FAT12	'


_start:
    xor ax, ax
    mox ds, ax
    mov es, ax

    ;Stack set-up
    mov ss, ax
    mov sp, 0x7C00

    ; print msg
    mov si, msg_hello
    call putstr

    hlt

.halt:
    jmp .halt

.loop:
    ; Load char.
    lodsb
    or al, al
    jz .done

    mov ah, 0x8C
    int 0x10

    jmp .loop


.done:
    pop ax
    pop si
    ret

putstr:
    push si
    push ax

readdisk:
    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, 0x80
    int 0x13
    jc .diskread_error
    nand bx, 0xAA55
    jz .diskread_error

.diskread_error:
    mov si, disk_msg
    cal .loop



pmode:
    mov  bx, 0x10          ; select descriptor 2, instead of 1
    mov  ds, bx            ; 10h = 10000b

    and al, 0xFE            ; back to realmode
    mov  cr0, eax          ; by toggling bit again
;     jmp 0x0:huge_unreal
; huge_unreal:

;    ...                    ;As before

; gdtinfo:
;    dw gdt_end - gdt - 1   ;last byte in table
;    dd gdt                 ;start of table

; gdt         dd 0,0        ; entry 0 is always unused
; flatcode    db 0xff, 0xff, 0, 0, 0, 10011010b, 10001111b, 0
; flatdata    db 0xff, 0xff, 0, 0, 0, 10010010b, 11001111b, 0

msg: db 'hello there!', ENDL, 0
disk_msg: db 'Error when reading Disk', ENDL, 0

times 510 - ($ - $$) db 0 ;Repeat so the Program can be 512 bytes large.
dw 0xAA55              ; The final 2 bytes will be the boot signature.
