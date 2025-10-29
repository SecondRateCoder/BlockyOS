org 0x7E00
bits 16

bt1_main: dw 0
bt1_write: dw 0
bt1_dm_read: dw 0
bt1_lbatochs: dw 0
bt1_diskread: dw 0
bt1_frestart: dw 0
bt1_hlt: dw 0

%define ENDL 0x0A, 0x00
global start

start:
	mov si, msg_bt2
	mov bx, [bt1_write]
	call [es:bx]

msg_bt2: db 'This is Boot2', ENDL, 0