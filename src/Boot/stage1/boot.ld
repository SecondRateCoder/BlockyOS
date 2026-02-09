ENTRY(start)
OUTPUT_FORMAT("binary")
phys = 0x7C00;

SECTIONS{
	. = phys;

	.text		: {
        *(.fsjmp)
        *(.BOOTDRIVE)
        *(.text)
    }

	.data		: {
        *(.data)
    }
}