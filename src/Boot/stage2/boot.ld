ENTRY(start)
OUTPUT_FORMAT("binary")
boot = 0x8000;
start = 0x8000 + 0x0200;

SECTIONS{
    . = 0x0;
    ._IDT    		: {*(._IDT)}
    . = boot;
	._DRIVEHEADER 	: {*(._DRIVEHEADER)}
	. = start;
    __kernel_start = .;
    .text   		: {
        __TRUECODEADDR = .;
        *(.entry)
        __CODEADDR = .;
        *(.PROGHEADER)
        *(.text)
        __CODEEND = .;
    }
    .data			: {
        __DATAADDR = .;
        *(.data)
        *(._FILES)
        *(.PROGRAMBUFFER)
        *(._GDT)
        __DATAEND = .;
    }

    __kernel_end = .;
}
