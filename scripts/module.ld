PHDRS
{
  header       PT_LOAD;
  text         PT_LOAD;
  data         PT_LOAD;
  bss          PT_LOAD;
  got          PT_LOAD;
  footer       PT_LOAD;
}

SECTIONS
{
    . = 0x0000;
    __bin_start = .;

    .header : 
    {
        __header_start = .;
        *(.header)
        __header_end = .;
    } : header

    INCLUDE dmod-common.ld

    . = ALIGN(8);

    .text : 
    {
        __text_start = .;
        *(.text .text.* .gnu.linkonce.t.*);
        
        . = ALIGN(4);
        KEEP(*(.init));
        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP(*(.preinit_array));
        . = ALIGN(4);
        __preinit_array_end = .;
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)));
        . = ALIGN(4);
        KEEP(*(.init_array));
        . = ALIGN(4);
        __init_array_end = .;
        KEEP(*(.fini));
        . = ALIGN(4);
        __fini_array_start = .;
        KEEP(*(.fini_array));
        . = ALIGN(4);
        KEEP(*(SORT(.fini_array.*)));
        . = ALIGN(4);
        __text_end = .;
    } : text

    .data : 
    {
        __data_start = .;
        *(.data .data.* .gnu.linkonce.d.*)
        __data_end = .;
    } : data

    .bss : 
    {
        __bss_start = .;
        *(.bss .bss.* .gnu.linkonce.b.*)
        __bss_end = .;
    } : bss

    .got : 
    {
        __got_start = .;
        *(.got .got.*)
        __got_end = .;
    } : got

    __bin_end = .;
    __bin_size = __bin_end - __bin_start;

    .footer : 
    {
        PROVIDE(__footer_start = .);
        LONG(__header_start)
        LONG(SIZEOF(.header))
        LONG(__inputs_start)
        LONG(SIZEOF(.inputs))
        LONG(__outputs_start)
        LONG(SIZEOF(.outputs))
        LONG(__text_start)
        LONG(SIZEOF(.text))
        LONG(__data_start)
        LONG(SIZEOF(.data))
        LONG(__bss_start)
        LONG(SIZEOF(.bss))
        LONG(__got_start)
        LONG(SIZEOF(.got))
        LONG(__bin_start)
        LONG(__bin_size)
    } : footer

    . = ALIGN(8);
    .unwanted_sections : { *(.unwanted) }
}