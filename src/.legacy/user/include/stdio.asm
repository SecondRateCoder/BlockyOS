

global putc
;   void putc(char)
putc:
    push bp
    mov bp, sp
    push ax
    mov ax, [bp + 4]
    int 1h
    leave
    pop ax

global puts
;   void puts(char *)
puts:
    
    