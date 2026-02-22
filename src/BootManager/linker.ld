ENTRY(efi_main)

SECTIONS
{
	. = 0x1000;
	.text : { *(.text) __init_array_start = .; KEEP(*(.init_array*)) __init_array_end = .; }
	.data : { *(.data) }
	.rodata : { *(.rodata) }
	.bss : { *(.bss) *(COMMON) }

	/DISCARD/ : {
		*(.eh_frame*)
		*(.eh_frame_hdr*)
		*(.gcc_except_table*)
		*(.comment*)
  		*(.note*)
  		*(.note.*)
		*(.stack_chk*)
	}
}