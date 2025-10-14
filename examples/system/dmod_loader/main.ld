ENTRY(_start)

SECTIONS
{
    /* Section for program headers (PHDR) */
    . = 0x400000;

    /* Code section */
    .text : {
        *(.text)         /* Main code section */
        *(.text.*)       /* Additional code sections */
        *(.init)         /* Initialization section */
        *(.fini)         /* Finalization section */
    }

    /* Read-only data section */
    .rodata : {
        *(.rodata)       /* Read-only data */
        *(.rodata.*)
    }

    INCLUDE dmod-common.ld

    /* Initialized data section */
    .data : {
        *(.data)
        *(.data.*)
    }

    /* Uninitialized data section */
    .bss : {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }

    /* Sections needed for C++ */
    .init_array : {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(.init_array))
        PROVIDE_HIDDEN(__init_array_end = .);
    }

    .fini_array : {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN(__fini_array_end = .);
    }
}
