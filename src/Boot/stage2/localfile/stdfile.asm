bits 32

struc driveHeader
    .BOOTINSTRUCTION        resb 3
    .OEM_ID				    resb 8
    .bytes_per_sector		resb 2
    .sectors_per_cluster 	resb 2
	.reserved_sectors		resb 1
	.fat_count				resb 1
    .dir_entries_count		resb 2
	.total_sectors			resb 2
    .media_descriptor_type	resb 1
    .sectors_per_fat		resb 2
	.sectors_per_track		resb 2
	.bdb_heads				resb 2
    .hidden_sectors			resb 4
	.large_sector_count		resb 4
    .drive_number			resb 1
	.signature				resb 1
    .volume_id				resb 4
    .volume_label           resb 11
    .sys_id                 resb 8
	.segment_clusters       resb 1
endstruc

extern _bootDrive
extern getDrive
%define DRIVEH_SIZE 64

global __x86DISKREAD
;   void x86DISKREAD(uint8_t *address)
__x86DISKREAD:
.interceptFloppyFinish:
    ret

global __x86DISKWRITE
;   void x86DISKWRITE(uint8_t *address)
__x86DISKWRITE:
.interceptFloppyFinish:
    ret

global __x86DISKUPDATE
;   bool x86DISKUPDATE(size_t new_addr, bool update)
__x86DISKUPDATE:
.interceptFloppyFinish:
ret

global LBA2CHS
;   uint32_t LBA2CHS(uint32_t LBA)
LBA2CHS:
    push ebp
    mov ebp, esp
    push ebx
    push ecx

    mov eax, [ebp + 8]
    xor edx, edx
    sub esp, DRIVEH_SIZE
    push esp
    call getDrive
    push ebp
    mov ebp, esp
    div word [ebp + driveHeader.sectors_per_track]

    inc edx
    mov ecx, edx
    xor edx, edx
    div word [ebp + driveHeader.bdb_heads]

;	al: Cylinder num ;
;	cl: Sector num ;
;	dl: Head number

;   eax: 16(Cylinder); 8(Head); (8)Sector
    shl eax, 16
    mov ah, dl
    mov al, cl

	leave
    pop ecx
    pop ebx
    leave
    ret