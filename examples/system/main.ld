SECTIONS
{
    . = 0x08048000;

    .text : {
        *(.text)
    }

    /* Include the common linker script for DMOD */
    INCLUDE common.ld

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
        *(COMMON)
    }
}
