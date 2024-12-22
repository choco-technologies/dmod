SECTIONS
{
    /* Sekcja nagłówka ELF */
    . = 0x08048000;

    /* Sekcja tekstowa (kod programu) */
    .text : {
        *(.text)
    }

    INCLUDE common.ld

    /* Sekcja danych tylko do odczytu */
    .rodata : {
        *(.rodata)
    }

    /* Sekcja danych inicjalizowanych */
    .data : {
        *(.data)
    }

    /* Sekcja danych niezainicjalizowanych */
    .bss : {
        *(.bss)
        *(COMMON)
    }
}
