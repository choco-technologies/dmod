#include <gtest/gtest.h>

extern "C"
{
void* __dmod_inputs_start = 0;
void* __dmod_inputs_end = 0;
void* __dmod_inputs_size = 0;
void* __dmod_outputs_start = 0;
void* __dmod_outputs_end = 0;
void* __dmod_outputs_size = 0;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}