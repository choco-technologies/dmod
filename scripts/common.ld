.inputs :
{
    __inputs_start = .;
    PROVIDE(__dmod_inputs_start = .);
    *(.inputs)
    __inputs_end = .;
    __inputs_size = __inputs_end - __inputs_start;
    PROVIDE(__dmod_inputs_size = __inputs_size);
} 

.outputs :
{
    __outputs_start = .;
    PROVIDE(__dmod_outputs_start = .);
    *(.outputs)
    __outputs_end = .;
    __outputs_size = __outputs_end - __outputs_start;
    PROVIDE(__dmod_outputs_size = __outputs_size);
} 
