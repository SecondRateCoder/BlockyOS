bits 16
MBR:
Partition0:
BootIndicator:					db 80h
StartingCHS:
	StartingHead:				db 00h
	StartingSector_Cylinder:	db 00h
	StartingCylinder:			db 00h
SystemID:						db 0xEE
EndingCHS:
	EndingHead:					db FFh
	EndingSector_Cylinder:		db FFh
	EndingCylinder:				db FFh
StartingLBA:					dd 01h
EndingLBA:						dd 2879

Partition1:
BootIndicator:					db 0x00
StartingCHS:
	StartingHead:				db 00h
	StartingSector_Cylinder:	db 00h
	StartingCylinder:			db 00h
SystemID:						db 0xEE
EndingCHS:
	EndingHead:					db FFh
	EndingSector_Cylinder:		db FFh
	EndingCylinder:				db FFh
StartingLBA:					dd 01h
EndingLBA:						dd 2879

Partition2:
BootIndicator:					db 0x00
StartingCHS:
	StartingHead:				db 00h
	StartingSector_Cylinder:	db 00h
	StartingCylinder:			db 00h
SystemID:						db 0xEE
EndingCHS:
	EndingHead:					db FFh
	EndingSector_Cylinder:		db FFh
	EndingCylinder:				db FFh
StartingLBA:					dd 01h
EndingLBA:						dd 2879

Partition3:
BootIndicator:					db 0x00
StartingCHS:
	StartingHead:				db 00h
	StartingSector_Cylinder:	db 00h
	StartingCylinder:			db 00h
SystemID:						db 0xEE
EndingCHS:
	EndingHead:					db FFh
	EndingSector_Cylinder:		db FFh
	EndingCylinder:				db FFh
StartingLBA:					dd 01h
EndingLBA:						dd 2879
