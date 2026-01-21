ENTRY(start)
OUTPUT_FORMAT("binary")
phys = 0x803B;                  /* BIOS (or MBR) puts us at 0x7C00 */

SECTIONS{
    . = phys;
    /*Entry Point*/
    .entry    : {
        *(.entry)
      }
    .text    : {
        *(.text)
    }

    /* Modifiable data */
    .data    : {
        *(.data)
    }

    /* Read-only data */
    .rodata    : {
        *(.rodata)
    }
}
