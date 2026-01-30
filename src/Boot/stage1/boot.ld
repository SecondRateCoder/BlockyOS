ENTRY(start)
OUTPUT_FORMAT("binary")
phys = 0x7C00;

SECTIONS{
	. = phys
	
	.fsjmp		: {*(.fsjmp)}

	.BOOTDRIVE	: {*(.BOOTDRIVE)}

	.text		: {*(.text)}

	.data		: {*(.data)}

	.bss		: {*(.bss)}
}