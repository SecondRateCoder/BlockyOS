;
; FDC Commands
%define CMD_READ  0x46   ; Read Data, MFM, MT=0, SK=1
%define CMD_WRITE 0x45   ; Write Data, MFM, MT=0, SK=1

%define FDC_DOR   0x3F2  ; Digital Output Register
%define FDC_MSR   0x3F4  ; Main Status Register (read)
%define FDC_DSR   0x3F4  ; Data Rate Select (write)
%define FDC_DATA  0x3F5  ; Data Register
%define FDC_DIR   0x3F7  ; Digital Input Register (read)
%define FDC_CCR   0x3F7  ; Configuration Control Register (write)

global __x86DISKREAD
;   void _x86DISKREAD(void *)
__x86DISKREAD:
    ret

global __x86DISKWRITE
;   void _x86DISKWRITE(void *)
__x86DISKWRITE:
    ret

global __x86DISKUPDATE
;   bool _x86DISKUPDATE()
__x86DISKUPDATE:
    ret