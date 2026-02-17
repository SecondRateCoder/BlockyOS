ENTRY(start)
OUTPUT_FORMAT("binary")
boot = (0x00000D00);

SECTIONS{
	. = boot;
    __kernel_start = boot;
    .all            :{
        *(.entry)
        __CODEADDR = .;
        *(PROGHEADER)
        *(.text)
        __CODEEND = .;
        __DATAADDR = .;
        *(.rodata)
        *(.data)
        *(PROGRAMBUFFER)
        *(SymbolIDT)
        *(SymbolGDT)
        . += 0x0800;
        *(.bss)
        __DATAEND = .;
    }
    __kernel_end = .;
    __kernel_sectors = ((__kernel_end - __CODEADDR) / 512) + (((__kernel_end - __CODEADDR) % 512) != 0? 1: 0);
    /DISCARD/ : {
        *(.eh_frame)
        *(.eh_frame*)
        *(.gcc_except_table)
        *(.gcc_except_table*)
        *(.note*)
        *(.comment)
        *(.debug*)
        *(.gnu*)
    }
}
