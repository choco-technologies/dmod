#include <gtest/gtest.h>
#include <dmod.h>
extern "C"
{
void DMOD_WEAK_SYMBOL Dmod_TestApi1() {}
void* __dmod_inputs_start DMOD_SECTION(".dmod.inputs.test")  = 0;
volatile Dmod_ApiRegistration_t __dmod_inputs[] DMOD_SECTION(".dmod.inputs.test") DMOD_USED = {
    { .Function = (void*)Dmod_TestApi1, .Signature = DMOD_MAKE_SIGNATURE(Dmod, 1.0, TestApi1) },
};
void* __dmod_inputs_end DMOD_SECTION(".dmod.inputs.test")= 0;
void* __dmod_inputs_size DMOD_SECTION(".dmod.inputs.test")= 0;
void* __dmod_outputs_start DMOD_SECTION(".dmod.outputs.test")= 0;
void* __dmod_outputs_end DMOD_SECTION(".dmod.outputs.test")= 0;
void* __dmod_outputs_size DMOD_SECTION(".dmod.outputs.test")= 0;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}