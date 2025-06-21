ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)
STARTUP(crt0.o)
INPUT()
OUTPUT(BlockyOS.bin)

MEMORY
{
  ROM (rx) : ORIGIN = 0x00000000, LENGTH = 256K
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 1M
  RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 10M
}

SECTIONS
{
    .text : ALIGN()
    .text :
    {
        TEXT_START = .;
        *(.text)
        TEXT_END = .;
    } > FLASH

    .data : ALIGN()
    .data :
    {
        DATA_START =.;
        *(.data)
        DATA_END =.;
    } > RAM

    .bss : ALIGN()
    .bss :
    {
        BSS_START =.;
        *(.data)
        BSS_END =.;
    } > RAM

/* Define symbols to represent the start, end, and size of RAM */
_ram_start = .;
_ram_end = . + LENGTH(RAM);
_ram_size = LENGTH(RAM) - .;

  /* Example: Define the heap start after the .bss section, and calculate heap size */
.heap (NOLOAD) :
{
        . = ALIGN(4);
        _heap_start = .;
        . = ORIGIN(RAM) + LENGTH(RAM) - _stack_size; /* Adjust for stack if stack grows downwards */
        _heap_end = .;
} > RAM

_heap_size = _heap_end - _heap_start;

/* Check for memory overflow (common in embedded systems) */
ASSERT( _ram_end >= ADDR(.bss) + SIZEOF(.bss), "RAM overflow: .bss section is too large for RAM!" );
ASSERT( LENGTH(RAM) >= (_heap_end - _ram_start) + _stack_size_defined_elsewhere, "RAM overflow: Heap/Stack clash!" );

}