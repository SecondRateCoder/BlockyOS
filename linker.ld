ENTRY(_start)
OUTPUT_FORMAT("binary")
STARTUP(crt0.o)
INPUT()
OUTPUT(BlockyOS.bin)

MEMORY{
  ROM (rx) : ORIGIN = 0x00000000, LENGTH = 256K
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 1M
  RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 10M
}

SECTIONS{
    .text :{
        TEXT_START = .;
        *(.text)
        TEXT_END = .;
    } > FLASH
    .data :{
        DATA_START =.;
        *(.data)
        DATA_END =.;
    } > RAM

    .bss :{
        BSS_START =.;
        *(.data)
        BSS_END =.;
    } > RAM
	. =ALIGN(4);
/* Define symbols to represent the start, end, and size of RAM */
_ram_start = . + 10M;
_ram_size = LENGTH(RAM) - _ram_start;
_heap_start = .;

/* Example: Define the heap start after the .bss section, and calculate heap size */
.heap (NOLOAD) :{
	. = ORIGIN(RAM) + LENGTH(RAM) - _stack_size; /* Adjust for stack if stack grows downwards */
} > RAM
_heap_size = . - _heap_start;


/* Check for memory overflow (common in embedded systems) */
ASSERT( _ram_end >= ADDR(.bss) + SIZEOF(.bss), "RAM overflow: .bss section is too large for RAM!" );
ASSERT( LENGTH(RAM) >= (_heap_end - _ram_start) + _stack_size_defined_elsewhere, "RAM overflow: Heap/Stack clash!" );

}

/DISCARD/ :
{
    *(.eh_frame)
    *(.fini)
    *(.interp)
    *(.note.ABI-tag)
    *(.note.gnu.build-id)
    *(.stab)
    *(.stabstr)
    *(.comment)
    *(.debug*)
    *(.gnu.attributes)
}