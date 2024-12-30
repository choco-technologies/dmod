SECTIONS
{
    . = 0x08048000;

    .text : {
        *(.text)
    }

    /* Include the common linker script for DMOD */
    INCLUDE dmod-common.ld
    
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
