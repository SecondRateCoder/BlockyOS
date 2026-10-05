SECTIONS {
    . = ALIGN(4096);
    
    .text : {
        CODEBASE = .;

        KEEP(*(.text))
        KEEP(*(.text.*))
        KEEP(*(.gnu.linkonce.t.*))
        *(.init)
        *(.fini)
        *(.plt)
        *(.plt.*)

        . = ALIGN(16);
        CODELIMIT = .;
    }

    . = ALIGN(4096);
    .data : {
        DATABASE = .;

        *(.data)
        *(.data.*)
        *(.sdata)
        *(.sdata.*)
        *(.rodata)
        *(.rodata.*)
        *(.rdata)
        *(.rdata.*)
        *(.bss)
        *(.bss.*)
        *(.sbss)
        *(.sbss.*)
        *(COMMON)
        *(.got)
        *(.got.*)
        *(.init_array*)
        *(.fini_array*)
        *(.ctors*)
        *(.dtors*)

        . = ALIGN(16);
        DATALIMIT = .;
    }

    /DISCARD/ : {
		*(.idata)
		*(.idata*)
        *(.pdata)
        *(.xdata)
        *(.reloc)
		*(.rela)
		*(.rela.*)
        *(.iplt)
        *(.iplt.*)
    }
}