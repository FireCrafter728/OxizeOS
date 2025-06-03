ENTRY(_start)

PHDRS {
    text PT_LOAD FLAGS(5);
    data PT_LOAD FLAGS(6);
}

SECTIONS {
    . = 0x0;

    .entry : {
        KEEP(*(.entry))
    } :text

    .text : {
        *(.text .text.*)
        __init_array_start = .;
        KEEP(*(.init_array))
        __init_array_end = .;
    } :text

    .data : {
        *(.data .data.*)
    } :data

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data
}