ENTRY(start)
OUTPUT_FORMAT("binary")
phys = 0x803B;                  /* BIOS (or MBR) puts us at 0x7C00 */

SECTIONS{
    . = 0x0
    .IDT    		: {*(.IDT)}
    . = 0x7C00;
	.DRIVEHEADER 	: {*(.DRIVEHEADER)}
	. = phys
    .entry  		: {*(.entry)}

    .text   		: {*(.text)}

    .data			: {*(.data)}

    .FILES 			: {*(.FILES)}
}
