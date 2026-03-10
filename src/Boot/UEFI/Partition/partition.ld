ENTRY(main)
OUTPUT_FORMAT("binary")

SECTIONS{
    .data : {
        *(.UEFIPartitionTable*)
        *(.data*)
        *(.rodata*)
    }
    
    /DISCARD/ : {*(.*)}
}
