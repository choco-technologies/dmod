. = ALIGN(16);

.inputs :
{
    __inputs_start = .;
    PROVIDE(__dmod_inputs_start = .);
    KEEP(*(.dmod.inputs))
    __inputs_end = .;
    PROVIDE(__dmod_inputs_end = .);
    __inputs_size = __inputs_end - __inputs_start;
    PROVIDE(__dmod_inputs_size = __inputs_end - __inputs_start);
} > ram AT > rom

. = ALIGN(16);

.outputs :
{
    __outputs_start = .;
    PROVIDE(__dmod_outputs_start = .);
    KEEP(*(.dmod.outputs))
    __outputs_end = .;
    PROVIDE(__dmod_outputs_end = .);
    __outputs_size = __outputs_end - __outputs_start;
    PROVIDE(__dmod_outputs_size = __outputs_end - __outputs_start);
} > ram AT > rom
