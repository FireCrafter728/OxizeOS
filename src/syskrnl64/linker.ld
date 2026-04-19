ENTRY(_start)
OUTPUT_FORMAT("elf64-x86-64")

PHDRS {
	code PT_LOAD FLAGS(5);
	data PT_LOAD FLAGS(6);
	rodata PT_LOAD FLAGS(4);
}

SECTIONS
{
	. = 0x0;
	.text : ALIGN(0x1000) { *(.text) } :code
	.data : ALIGN(0x1000) { *(.data) } :data
	.rodata : ALIGN(0x1000) { *(.rodata) } :rodata
	.init_array : ALIGN(0x1000) { __init_array_start = .; KEEP(*(.init_array)) __init_array_end = .; } :rodata 
	.bss : ALIGN(0x1000) { *(.bss) *(COMMON) } :data
}