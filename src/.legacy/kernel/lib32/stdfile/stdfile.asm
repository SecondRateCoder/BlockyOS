bits 32

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