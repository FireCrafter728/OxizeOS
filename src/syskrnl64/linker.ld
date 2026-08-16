# SPDX-License-Identifier: GPL-3.0-or-later
# OxizeOS build system

# Output settings
OUTPUT_FORMAT("pei-x86-64")
OUTPUT_ARCH(i386:x86-64)

# Source settings
ENTRY(_start)

# Sections
SECTIONS
{
	. = 0x1000;

	.entry : { 
		*(.entry*)						# Assembly entry function section
		__init_array_start = .;			# Global constructor array start
		KEEP(*(.init_array*))			# Specify to the linker to not discard the global constructor array
		__init_array_end = .;			# Global constructor array end
	}

	.code : {
		*(.code*)						 # Assembly source code
	}

	.text : {
		*(.text*)						# C/C++ source code
	}

	.rdata : {
		*(.rdata*)						# Read-only data, PE name
		*(.rodata*)						# Read-only data, ELF name
	}

	.data : {
		*(.data*)						# Initialized non-constant data
	}

	.bss : {
		*(.bss*)						# Uninitialized non-constant data
		*(COMMON)						# Makes sure that the linker puts all common values into the BSS section
	}

	.rsrc : {
		*(.rsrc*)						# Various PE resources specified by a resource script
	}

	.reloc : {
		*(.reloc*)						# Entries for relocating the executable
	}

	# Mark sections that are not needed to be discarded
	
	/DISCARD/ : {
		*(.pdata*)
		*(.xdata*)
		*(.eh_frame*)
		*(.eh_frame_hdr*)

		*(.gcc_except_table*)

		*(.comment*)

		*(.note*)
		*(.gnu*)
	}
}