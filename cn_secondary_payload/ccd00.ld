OUTPUT_ARCH(arm)

MEMORY
{
	RAMRO (rx) : ORIGIN = 0x00108000, LENGTH = 0x0000D000
	DERPO (rw) : ORIGIN = 0x14380000, LENGTH = 0x00080000
}

SECTIONS
{
	.text : ALIGN(0x1000) {
		build/crt0.o(.init)
		*(.text)
	} > RAMRO
	
	.bss : ALIGN(4)
	{
		__bss_start__ = .;
		*(.bss)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;
	} > DERPO
}

