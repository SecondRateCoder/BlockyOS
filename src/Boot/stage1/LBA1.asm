bits 16

PartitionTableHeader:
Signature:					db 'EFI PART'
GPTRevision:				dd 00h:00h:00h:00h
HeaderSize:					dd 00
GPTHeaderChecksum:			dd 00
							dd 0
LocalLBA:					dq 1
AlternateLBA:				dq 0
FirstUsable:				dq 0
LastUsable:					dq 0
DiskGUID:					dq 0, 0
GUIDPartitionEntryArray:	dq 0
GUIDPartitionNumber:		dd 0
GUIDPartitionChecksum:		dd 00

times (512 - ($ - $$)) db 0
