ENTRY(start)
OUTPUT_FORMAT("binary")
boot = (0x00000D00);

SECTIONS{
	. = boot;
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
        *(._IDT)
        *(._GDT)
        __DATAEND = .;
    }

    __kernel_end = .;
    __kernel_sectors = ((__kernel_end - __kernel_start) + 511) / 512;
}
