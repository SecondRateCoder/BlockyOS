OUTPUT_FORMAT(pe-x86-64)
ENTRY(kboot_km)

SECTIONS {

    /* 3. The Executable Code Block */
    .text : ALIGN(4096) {
        CODEBASE = .;
        KEEP(*(.text))
        KEEP(*(.text.*))
        *(.init)
        *(.fini)
        . = ALIGN(16);
        CODELIMIT = .;
    }

    /* 4. Page-align and define the Global Data Block */
    .data : ALIGN(0x1000) {
        DATABASE = .;
        *(.IDT*)
        *(.IDT.*)
        *(.data*)
        *(.rodata*)
        *(.rdata*)
        *(.bss*)
        *(COMMON)
        . = ALIGN(16);
        DATALIMIT = .;
    }

    /* 5. Separate .idata allows the Data Directory map to compute cleanly */
    .idata : ALIGN(0x1000) {
		/* KEEP (SORT(*)(.idata$2))
		KEEP (SORT(*)(.idata$3))
		KEEP (SORT(*)(.idata$4))
		KEEP (SORT(*)(.idata$5))
		KEEP (SORT(*)(.idata$6))
		KEEP (SORT(*)(.idata$7)) */
		KEEP (SORT(*)(.idata$2))
		KEEP (SORT(*)(.idata$3))
		/* These zeroes mark the end of the import list.  */
		LONG (0); LONG (0); LONG (0); LONG (0); LONG (0);
		. = ALIGN(8);
		KEEP (SORT(*)(.idata$4))
		__IAT_start__ = .;
		SORT(*)(.idata$5)
		__IAT_end__ = .;
		KEEP (SORT(*)(.idata$6))
		KEEP (SORT(*)(.idata$7))
    }

    /DISCARD/ : {
        *(.pdata)
        *(.xdata)
        *(.reloc)
        *(.rela)
        *(.rela.*)
        *(.iplt)
        *(.iplt.*)
    }
}
