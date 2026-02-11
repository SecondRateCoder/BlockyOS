; Copied from: "https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-partition_information_gpt"

; There is no partition.
PARTITION_ENTRY_UNUSED_GUID						equ 0000h:0000h:0000h:0000h, 0000h:0000h:0000h:0000h
; The partition is an EFI system partition.
PARTITION_SYSTEM_GUID							equ c12ah:7328h:f81fh:11d2h, ba4bh:00a0h:c93eh:c93bh

; If this attribute is set, the partition is required by a computer to function properly.
GPT_ATTRIBUTE_PLATFORM_REQUIRED					equ 01h
GPT_ATTRIBUTE_OS_BOOT							equ 04h

; FAT32 Format
EFISystemPartition:
	GUIDPartitionType:		dq PARTITION_SYSTEM_GUID
	UniqueGUIDPartition:	dq 0, 0
	StartingLBA:			dq 0
	EndingLBA:				dq 0
	Attributes:				dq 0
	PartitionName:			dw 'EFI Table'
	times ((128 * 1) - ($ - $$)) db ' '

; FrAT Partition, contains the FrAT Drive.
Drive:
	GUIDPartitionType:		dq 0, 01h
	UniqueGUIDPartition:	dq 0, 0
	StartingLBA:			dq 0
	EndingLBA:				dq 0
	Attributes:				dq GPT_ATTRIBUTE_OS_BOOT
	PartitionName:			dw 'Boot'
	times ((128 * 3) - ($ - $$)) db ' '

times (512 - ($ - $$)) db 0
