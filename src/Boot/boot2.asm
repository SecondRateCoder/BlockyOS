org 0x7E00
bits 16

jmp short start
%define ENDL 0x00, 0x0A
boot2msg: db 'This is Boot2', ENDL, 0

global start

start:
    mov si, msg_hello
    call puts
    hlt

puts:
    push ax
    push si
.loop:
    lodsb
    or al, al
    jz .done

    mov ah, 0x0E
    int 0x10

    jmp .loop
.done:
    pop si
    pop ax

    ret

msg_hello: db 'hello there!', ENDL, 0