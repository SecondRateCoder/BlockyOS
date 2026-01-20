

global putc
;   void putc(char)
putc:
    push ax
    mov ax, [bp + 4]
    int 1h
    pop ax

global puts
;   void puts(char *)
puts:
    
    