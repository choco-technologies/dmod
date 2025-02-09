#define DMOD_PRIVATE
#include <stdlib.h>
#include <gtest/gtest.h>
#include "private/dmod_ctx.h"
#include "private/dmod_ldr.h"
#include "private/dmod_vars.h"
#include "dmod.h"


/**
 * @brief loads DMF test file for testing
 */
static bool LoadDmfTestFile( void** outData, size_t* outSize, bool AppFile = true )
{
    const char* fileName = AppFile ? DMOD_TEST_DMF_FILE : DMOD_TEST_DMF_LIB_FILE;
    Dmod_Printf("Loading DMF test file: %s\n", fileName);
    
    FILE* file = fopen(fileName, "rb");
    if( file == NULL )
    {
        return false;
    }

    fseek(file, 0, SEEK_END);
    *outSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    *outData = Dmod_AlignedMalloc(*outSize, DMOD_STACK_ALIGNMENT);
    if( *outData == NULL )
    {
        fclose(file);
        return false;
    }

    fread(*outData, 1, *outSize, file);
    fclose(file);

    return true;
}

class DmodLdrTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        memset(Dmod_Contexts, 0, sizeof(Dmod_Contexts));
    }

    void TearDown() override
    {
    }
};

// ===============================================================
//                  Tests for Dmod_Ldr_LoadHeader
// ===============================================================

/**
 * @brief Test for Dmod_Ldr_LoadHeader
 * 
 * The test checks if the function loads the header correctly.
 */
TEST_F(DmodLdrTest, LoadHeader)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    Dmod_ModuleHeader_t* header = (Dmod_ModuleHeader_t*)data;
    header->Signature = DMOD_HEADER_SIGNATURE;
    header->DmodVersion = DMOD_VERSION;
    strcpy(header->Arch, DMOD_ARCH);
    strcpy(header->CpuName, DMOD_CPU_NAME);
    strcpy(header->Name, "TestModule");

    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadHeader
 * 
 * The test checks if the function fails to load the header with an invalid context.
 */
TEST_F(DmodLdrTest, LoadHeaderInvalidContext)
{
    Dmod_Context_t* context = nullptr;
    ASSERT_FALSE(Dmod_Ldr_LoadHeader(context));
}

/**
 * @brief Test for Dmod_Ldr_LoadHeader
 * 
 * The test checks if the function fails to load the header with an invalid signature.
 */
TEST_F(DmodLdrTest, LoadHeaderInvalidSignature)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    Dmod_ModuleHeader_t* header = (Dmod_ModuleHeader_t*)data;
    header->Signature = 0;
    header->DmodVersion = DMOD_VERSION;
    strcpy(header->Arch, DMOD_ARCH);
    strcpy(header->CpuName, DMOD_CPU_NAME);
    strcpy(header->Name, "TestModule");

    ASSERT_FALSE(Dmod_Ldr_LoadHeader(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadHeader
 * 
 * The test checks if the function fails to load the header with an incompatible version.
 */
TEST_F(DmodLdrTest, LoadHeaderIncompatibleVersion)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    Dmod_ModuleHeader_t* header = (Dmod_ModuleHeader_t*)data;
    header->Signature = DMOD_HEADER_SIGNATURE;
    header->DmodVersion = 0xFFFF0000;
    strcpy(header->Arch, DMOD_ARCH);
    strcpy(header->CpuName, DMOD_CPU_NAME);
    strcpy(header->Name, "TestModule");

    ASSERT_FALSE(Dmod_Ldr_LoadHeader(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadHeader
 * 
 * The test checks if the function fails to load the header with an invalid architecture.
 */
TEST_F(DmodLdrTest, LoadHeaderInvalidArchitecture)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    Dmod_ModuleHeader_t* header = (Dmod_ModuleHeader_t*)data;
    header->Signature = DMOD_HEADER_SIGNATURE;
    header->DmodVersion = DMOD_VERSION;
    strcpy(header->Arch, "InvalidArch");
    strcpy(header->CpuName, DMOD_CPU_NAME);
    strcpy(header->Name, "TestModule");

    ASSERT_FALSE(Dmod_Ldr_LoadHeader(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadHeader
 * 
 * The test checks if the function fails to load the header with an invalid target cpu.
 */
TEST_F(DmodLdrTest, LoadHeaderInvalidTargetCpu)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    Dmod_ModuleHeader_t* header = (Dmod_ModuleHeader_t*)data;
    header->Signature = DMOD_HEADER_SIGNATURE;
    header->DmodVersion = DMOD_VERSION;
    strcpy(header->Arch, DMOD_ARCH);
    strcpy(header->CpuName, "InvalidCpu");
    strcpy(header->Name, "TestModule");

    ASSERT_FALSE(Dmod_Ldr_LoadHeader(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadHeader
 * 
 * The test checks if the function fails to load the header with a too long name.
 */
TEST_F(DmodLdrTest, LoadHeaderTooLongName)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    Dmod_ModuleHeader_t* header = (Dmod_ModuleHeader_t*)data;
    header->Signature = DMOD_HEADER_SIGNATURE;
    header->DmodVersion = DMOD_VERSION;
    strcpy(header->Arch, DMOD_ARCH);
    strcpy(header->CpuName, DMOD_CPU_NAME);
    strcpy(header->Name, "ThisIsAVeryLongModuleNameThatShouldFail");

    ASSERT_FALSE(Dmod_Ldr_LoadHeader(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadHeader
 * 
 * The test checks if the function fails to load the header when the module type is library and the function is already loaded.
 */
TEST_F(DmodLdrTest, LoadHeaderModuleAlreadyLoaded)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize, false));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    Dmod_Context_t* context2 = Dmod_LoadFile(DMOD_TEST_DMF_LIB_FILE);
    EXPECT_TRUE( Dmod_Context_Add(context2) );
    ASSERT_FALSE(Dmod_Ldr_LoadHeader(context));

    Dmod_Context_Delete(context);
    Dmod_Context_Delete(context2);
}

/**
 * @brief Test for Dmod_Ldr_LoadHeader
 * 
 * The test checks if the function does not fail to load the header when the module type is application and the module is already loaded.
 */
TEST_F(DmodLdrTest, LoadHeaderModuleAlreadyLoadedApplication)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    Dmod_Context_t* context2 = Dmod_LoadFile(DMOD_TEST_DMF_FILE);
    EXPECT_TRUE( Dmod_Context_Add(context2) );
    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));

    Dmod_Context_Delete(context);
    Dmod_Context_Delete(context2);
}

/**
 * @brief Test for Dmod_Ldr_LoadHeader
 * 
 * The test checks if the function fails to load the header with missing init or deinit function.
 */
TEST_F(DmodLdrTest, LoadHeaderMissingInitDeinitFunction)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    Dmod_ModuleHeader_t* header = (Dmod_ModuleHeader_t*)data;
    header->Signature = DMOD_HEADER_SIGNATURE;
    header->DmodVersion = DMOD_VERSION;
    strcpy(header->Arch, DMOD_ARCH);
    strcpy(header->CpuName, DMOD_CPU_NAME);
    strcpy(header->Name, "TestModule");
    header->ModuleType = Dmod_ModuleType_Library;
    header->Init = NULL;
    header->Deinit = NULL;

    ASSERT_FALSE(Dmod_Ldr_LoadHeader(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadHeader
 * 
 * The test checks if the function fails to load the header with an invalid module type.
 */
TEST_F(DmodLdrTest, LoadHeaderInvalidModuleType)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    Dmod_ModuleHeader_t* header = (Dmod_ModuleHeader_t*)data;
    header->Signature = DMOD_HEADER_SIGNATURE;
    header->DmodVersion = DMOD_VERSION;
    strcpy(header->Arch, DMOD_ARCH);
    strcpy(header->CpuName, DMOD_CPU_NAME);
    strcpy(header->Name, "TestModule");
    header->ModuleType = 0xFF;

    ASSERT_FALSE(Dmod_Ldr_LoadHeader(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadHeader
 * 
 * The test checks if the function fails to load the header when module type is application and main function is missing.
 */
TEST_F(DmodLdrTest, LoadHeaderMissingMainFunction)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    Dmod_ModuleHeader_t* header = (Dmod_ModuleHeader_t*)data;
    header->Signature = DMOD_HEADER_SIGNATURE;
    header->DmodVersion = DMOD_VERSION;
    strcpy(header->Arch, DMOD_ARCH);
    strcpy(header->CpuName, DMOD_CPU_NAME);
    strcpy(header->Name, "TestModule");
    header->ModuleType = Dmod_ModuleType_Application;
    header->Main = NULL;

    ASSERT_FALSE(Dmod_Ldr_LoadHeader(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadHeader
 * 
 * The test checks if the function loads the header correctly with a valid license.
 */
TEST_F(DmodLdrTest, LoadHeaderValidLicense)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    Dmod_ModuleHeader_t* header = (Dmod_ModuleHeader_t*)data;
    header->Signature = DMOD_HEADER_SIGNATURE;
    header->DmodVersion = DMOD_VERSION;
    strcpy(header->Arch, DMOD_ARCH);
    strcpy(header->CpuName, DMOD_CPU_NAME);
    strcpy(header->Name, "TestModule");
    uint8_t* license = &(((uint8_t*)data)[500]);
    char* licenseText = &(((char*)data)[600]);
    header->License = (Dmod_License_t*)license;
    header->License->Text = (char*)600;
    strcpy(licenseText, "License");
    
    // Convert pointers into offsets
    header->License       = (Dmod_License_t*)500;

    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadHeader
 * 
 * The test checks if the function fails to load the header with an invalid license.
 */
TEST_F(DmodLdrTest, LoadHeaderInvalidLicense)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    Dmod_ModuleHeader_t* header = (Dmod_ModuleHeader_t*)data;
    
    // Invalidate license
    header->License = (Dmod_License_t*)(fileSize + 1);

    ASSERT_FALSE(Dmod_Ldr_LoadHeader(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadHeader
 * 
 * The test checks if the function fails to load the header with invalid license text pointer.
 */
TEST_F(DmodLdrTest, LoadHeaderInvalidLicenseTextPointer)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    Dmod_ModuleHeader_t* header = (Dmod_ModuleHeader_t*)data;
    
    uint8_t* license = &(((uint8_t*)data)[500]);
    header->License = (Dmod_License_t*)license;
    header->License->Text = (char*)(fileSize + 1);
    // Invalidate license text pointer
    header->License = (Dmod_License_t*)500;

    ASSERT_FALSE(Dmod_Ldr_LoadHeader(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadHeader
 * 
 * The test checks if the function fails with invalid footer pointer. 
 */
TEST_F(DmodLdrTest, LoadHeaderInvalidFooterPointer)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    Dmod_ModuleHeader_t* header = (Dmod_ModuleHeader_t*)data;
    header->Signature = DMOD_HEADER_SIGNATURE;
    header->DmodVersion = DMOD_VERSION;
    strcpy(header->Arch, DMOD_ARCH);
    strcpy(header->CpuName, DMOD_CPU_NAME);
    strcpy(header->Name, "TestModule");

    header->Footer = (void*)(fileSize + 1);

    ASSERT_FALSE(Dmod_Ldr_LoadHeader(context));

    Dmod_Context_Delete(context);
}

// ===============================================================
//                  Tests for Dmod_Ldr_LoadFooter
// ===============================================================
/**
 * @brief Test for Dmod_Ldr_LoadFooter
 * 
 * The test checks if the function loads the footer correctly.
 */
TEST_F(DmodLdrTest, LoadFooter)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));
    ASSERT_TRUE(Dmod_Ldr_LoadFooter(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadFooter
 * 
 * The test checks if the function fails to load the footer with an invalid context.
 */
TEST_F(DmodLdrTest, LoadFooterInvalidContext)
{
    Dmod_Context_t* context = nullptr;
    ASSERT_FALSE(Dmod_Ldr_LoadFooter(context));
}

/**
 * @brief Test for Dmod_Ldr_LoadFooter
 * 
 * The test checks if the function fails to load the footer without a valid header.
 */
TEST_F(DmodLdrTest, LoadFooterNoHeader)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    ASSERT_FALSE(Dmod_Ldr_LoadFooter(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadFooter
 * 
 * The test checks if the function fails to load the footer with an invalid header section.
 */
TEST_F(DmodLdrTest, LoadFooterInvalidHeaderSection)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    Dmod_ModuleHeader_t* header = (Dmod_ModuleHeader_t*)data;
    header->Signature = DMOD_HEADER_SIGNATURE;
    header->DmodVersion = DMOD_VERSION;
    strcpy(header->Arch, DMOD_ARCH);
    strcpy(header->CpuName, DMOD_CPU_NAME);
    strcpy(header->Name, "TestModule");

    uint64_t footerOffset = (uint64_t)header->Footer;
    uint8_t* footerPtr = &(((uint8_t*)data)[footerOffset]);
    Dmod_ModuleFooter_t* footer = (Dmod_ModuleFooter_t*) footerPtr;

    footer->Header.SectionStart = 0;
    footer->Header.SectionSize = 0;

    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));
    ASSERT_FALSE(Dmod_Ldr_LoadFooter(context));

    Dmod_Context_Delete(context);
}

// ===============================================================
//                  Tests for Dmod_Ldr_LoadInput
// ===============================================================
/**
 * @brief Test for Dmod_Ldr_LoadInput
 * 
 * The test checks if the function loads the input correctly.
 */
TEST_F(DmodLdrTest, LoadInput)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));
    ASSERT_TRUE(Dmod_Ldr_LoadFooter(context));
    ASSERT_TRUE(Dmod_Ldr_LoadInput(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadInput
 * 
 * The test checks if the function fails to load the input with an invalid context.
 */
TEST_F(DmodLdrTest, LoadInputInvalidContext)
{
    Dmod_Context_t* context = nullptr;
    ASSERT_FALSE(Dmod_Ldr_LoadInput(context));
}

/**
 * @brief Test for Dmod_Ldr_LoadInput
 * 
 * The test checks if the function fails to load the input without a valid footer.
 */
TEST_F(DmodLdrTest, LoadInputNoFooter)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    ASSERT_FALSE(Dmod_Ldr_LoadInput(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadInput
 * 
 * The test checks if the function fails to load the input with an empty input section.
 */
TEST_F(DmodLdrTest, LoadInputEmptyInputSection)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));
    ASSERT_TRUE(Dmod_Ldr_LoadFooter(context));

    Dmod_ModuleFooter_t* footer = context->Footer;
    Dmod_ModuleSection_t* input = &footer->Inputs;
    input->SectionStart = 0;
    input->SectionSize = 0;

    ASSERT_TRUE(Dmod_Ldr_LoadInput(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadInput
 * 
 * The test checks if the function fails to load the input with empty input entries and without any functions to call.
 */
TEST_F(DmodLdrTest, LoadInputEmptyInputEntriesNoFunctionsToCall)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));
    ASSERT_TRUE(Dmod_Ldr_LoadFooter(context));

    Dmod_ModuleFooter_t* footer = context->Footer;
    Dmod_ModuleSection_t* input = &footer->Inputs;
    input->SectionStart = 0;
    input->SectionSize = 0;

    Dmod_ModuleHeader_t* header = context->Header;
    header->Init = NULL;
    header->Main = NULL;
    header->Deinit = NULL;

    ASSERT_FALSE(Dmod_Ldr_LoadInput(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadInput
 * 
 * The test checks if the function fails to load the input with an invalid input section.
 */
TEST_F(DmodLdrTest, LoadInputInvalidInputSection)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));
    ASSERT_TRUE(Dmod_Ldr_LoadFooter(context));

    Dmod_ModuleFooter_t* footer = context->Footer;
    Dmod_ModuleSection_t* input = &footer->Inputs;
    input->SectionStart = 1;
    input->SectionSize = context->Size;

    ASSERT_FALSE(Dmod_Ldr_LoadInput(context));

    input->SectionSize = 1;

    ASSERT_FALSE(Dmod_Ldr_LoadInput(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadInput
 * 
 * The test checks if the function fails to load the input with entry function pointer set to NULL.
 */
TEST_F(DmodLdrTest, LoadInputEmptyEntryFunctionPointer)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));
    ASSERT_TRUE(Dmod_Ldr_LoadFooter(context));

    Dmod_ModuleFooter_t* footer = context->Footer;
    Dmod_ModuleSection_t* input = &footer->Inputs;
    input->SectionStart = 500;
    input->SectionSize = sizeof(Dmod_InputsSection_t) * 1;

    uint8_t* dataBuffer = (uint8_t*)context->Data;
    Dmod_InputsSection_t* inputSection = (Dmod_InputsSection_t*)(&dataBuffer[input->SectionStart]);
    inputSection->Entries[0].Function = NULL;

    ASSERT_FALSE(Dmod_Ldr_LoadInput(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadInput
 * 
 * The test checks if the function fails to load the input with an invalid input entry signature.
 */
TEST_F(DmodLdrTest, LoadInputInvalidEntrySignature)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));
    ASSERT_TRUE(Dmod_Ldr_LoadFooter(context));

    Dmod_ModuleFooter_t* footer = context->Footer;
    Dmod_ModuleSection_t* input = &footer->Inputs;
    input->SectionStart = 500;
    input->SectionSize = sizeof(Dmod_InputsSection_t) * 1;

    uint8_t* dataBuffer = (uint8_t*)context->Data;
    Dmod_InputsSection_t* inputSection = (Dmod_InputsSection_t*)(&dataBuffer[input->SectionStart]);
    inputSection->Entries[0].Function = (void*)100;
    inputSection->Entries[0].Signature = (char*)200;

    ASSERT_FALSE(Dmod_Ldr_LoadInput(context));

    Dmod_Context_Delete(context);
} 

// ===============================================================
//                  Tests for Dmod_Ldr_LoadOutput
// ===============================================================
/**
 * @brief Test for Dmod_Ldr_LoadOutput
 * 
 * The test checks if the function loads the output correctly.
 */
TEST_F(DmodLdrTest, LoadOutput)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));
    ASSERT_TRUE(Dmod_Ldr_LoadFooter(context));
    ASSERT_TRUE(Dmod_Ldr_LoadOutput(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadOutput
 * 
 * The test checks if the function fails to load the output with an invalid context.
 */
TEST_F(DmodLdrTest, LoadOutputInvalidContext)
{
    Dmod_Context_t* context = nullptr;
    ASSERT_FALSE(Dmod_Ldr_LoadOutput(context));
}

/**
 * @brief Test for Dmod_Ldr_LoadOutput
 * 
 * The test checks if the function fails to load the output without a valid footer.
 */
TEST_F(DmodLdrTest, LoadOutputNoFooter)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    ASSERT_FALSE(Dmod_Ldr_LoadOutput(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadOutput
 * 
 * The test checks if the function fails to load the output with an invalid output section.
 */
TEST_F(DmodLdrTest, LoadOutputInvalidOutputSection)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));
    ASSERT_TRUE(Dmod_Ldr_LoadFooter(context));

    Dmod_ModuleFooter_t* footer = context->Footer;
    Dmod_ModuleSection_t* output = &footer->Outputs;
    output->SectionStart = 1;
    output->SectionSize = context->Size;

    ASSERT_FALSE(Dmod_Ldr_LoadOutput(context));

    output->SectionSize = 1;

    ASSERT_FALSE(Dmod_Ldr_LoadOutput(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadOutput
 * 
 * The test checks if the function fails to load the output with an invalid output section out of bounds.
 */
TEST_F(DmodLdrTest, LoadOutputInvalidOutputSectionOutOfBounds)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));
    ASSERT_TRUE(Dmod_Ldr_LoadFooter(context));

    Dmod_ModuleFooter_t* footer = context->Footer;
    Dmod_ModuleSection_t* output = &footer->Outputs;
    output->SectionStart = 500;
    output->SectionSize = context->Size;

    ASSERT_FALSE(Dmod_Ldr_LoadOutput(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadOutput
 * 
 * The test checks if the function fails to load the output with empty section size and start.
 */
TEST_F(DmodLdrTest, LoadOutputEmptySectionSizeStart)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));
    ASSERT_TRUE(Dmod_Ldr_LoadFooter(context));

    Dmod_ModuleFooter_t* footer = context->Footer;
    Dmod_ModuleSection_t* output = &footer->Outputs;
    output->SectionStart = 0;
    output->SectionSize = 0;

    ASSERT_TRUE(Dmod_Ldr_LoadOutput(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadOutput
 * 
 * The test checks if the function fails to load the output with invalid entry function pointer.
 */
TEST_F(DmodLdrTest, LoadOutputInvalidEntryFunctionPointer)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));
    ASSERT_TRUE(Dmod_Ldr_LoadFooter(context));

    Dmod_ModuleFooter_t* footer = context->Footer;
    Dmod_ModuleSection_t* output = &footer->Outputs;
    output->SectionStart = 500;
    output->SectionSize = sizeof(Dmod_OutputsSection_t) * 2;

    uint8_t* dataBuffer = (uint8_t*)context->Data;
    Dmod_OutputsSection_t* outputSection = (Dmod_OutputsSection_t*)(&dataBuffer[output->SectionStart]);
    outputSection->Entries[0] = NULL;
    outputSection->Entries[1] = (void*)(fileSize + 1);

    ASSERT_FALSE(Dmod_Ldr_LoadOutput(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadOutput
 * 
 * The test checks if the function fails to load the output with an invalid output entry signature.
 */
TEST_F(DmodLdrTest, LoadOutputInvalidEntrySignature)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);

    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));
    ASSERT_TRUE(Dmod_Ldr_LoadFooter(context));

    Dmod_ModuleFooter_t* footer = context->Footer;
    Dmod_ModuleSection_t* output = &footer->Outputs;
    output->SectionStart = 500;
    output->SectionSize = sizeof(Dmod_OutputsSection_t) * 1;

    uint8_t* dataBuffer = (uint8_t*)context->Data;
    Dmod_OutputsSection_t* outputSection = (Dmod_OutputsSection_t*)(&dataBuffer[output->SectionStart]);
    outputSection->Entries[0] = (void*)100;

    ASSERT_FALSE(Dmod_Ldr_LoadOutput(context));

    Dmod_Context_Delete(context);
}

// ===============================================================
//                  Tests for Dmod_Ldr_LoadGot
// ===============================================================

/**
 * @brief Test for Dmod_Ldr_LoadGot
 * 
 * The test checks if the function loads the GOT correctly.
 */
TEST_F(DmodLdrTest, LoadGot)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);
    
    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));
    ASSERT_TRUE(Dmod_Ldr_LoadFooter(context));
    ASSERT_TRUE(Dmod_Ldr_LoadGot(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadGot
 * 
 * The test checks if the function fails to load the GOT with an invalid context.
 */
TEST_F(DmodLdrTest, LoadGotInvalidContext)
{
    Dmod_Context_t* context = nullptr;
    ASSERT_FALSE(Dmod_Ldr_LoadGot(context));
}

/**
 * @brief Test for Dmod_Ldr_LoadGot
 * 
 * The test checks if the function fails to load the GOT without a valid footer.
 */
TEST_F(DmodLdrTest, LoadGotNoFooter)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);
    
    ASSERT_FALSE(Dmod_Ldr_LoadGot(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadGot
 * 
 * The test checks if the function fails to load the GOT with an invalid GOT section.
 */
TEST_F(DmodLdrTest, LoadGotInvalidGotSection)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);
    
    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));
    ASSERT_TRUE(Dmod_Ldr_LoadFooter(context));

    Dmod_ModuleFooter_t* footer = context->Footer;
    Dmod_ModuleSection_t* got = &footer->Got;
    got->SectionStart = 1;
    got->SectionSize = context->Size;

    ASSERT_FALSE(Dmod_Ldr_LoadGot(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadGot
 * 
 * The test checks if the function fails to load the GOT without a valid GOT section.
 */
TEST_F(DmodLdrTest, LoadGotEmptyGotSectionSizeStart)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);
    
    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));
    ASSERT_TRUE(Dmod_Ldr_LoadFooter(context));

    Dmod_ModuleFooter_t* footer = context->Footer;
    Dmod_ModuleSection_t* got = &footer->Got;
    got->SectionStart = 0;
    got->SectionSize = 0;

    ASSERT_TRUE(Dmod_Ldr_LoadGot(context));

    Dmod_Context_Delete(context);
}

//================================================================
//                  Tests for Dmod_Ldr_LoadBss
//================================================================

/**
 * @brief Test for Dmod_Ldr_LoadBss
 * 
 * The test checks if the function loads the BSS correctly.
 */
TEST_F(DmodLdrTest, LoadBss)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);
    
    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));
    ASSERT_TRUE(Dmod_Ldr_LoadFooter(context));

    // Add bss section
    Dmod_ModuleFooter_t* footer = context->Footer;
    Dmod_ModuleSection_t* bss = &footer->Bss;
    bss->SectionStart = 300;
    bss->SectionSize = 100;

    ASSERT_TRUE(Dmod_Ldr_LoadBss(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadBss
 * 
 * The test checks if the function fails to load the BSS with an invalid context.
 */
TEST_F(DmodLdrTest, LoadBssInvalidContext)
{
    Dmod_Context_t* context = nullptr;
    ASSERT_FALSE(Dmod_Ldr_LoadBss(context));
}

/**
 * @brief Test for Dmod_Ldr_LoadBss
 * 
 * The test checks if the function fails to load the BSS without a valid footer.
 */
TEST_F(DmodLdrTest, LoadBssNoFooter)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize, false));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);
    
    ASSERT_FALSE(Dmod_Ldr_LoadBss(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_LoadBss
 * 
 * The test checks if the function fails to load the BSS with an invalid BSS section.
 */
TEST_F(DmodLdrTest, LoadBssInvalidBssSection)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize, false));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);
    
    ASSERT_TRUE(Dmod_Ldr_LoadHeader(context));
    ASSERT_TRUE(Dmod_Ldr_LoadFooter(context));

    // Add bss section
    Dmod_ModuleFooter_t* footer = context->Footer;
    Dmod_ModuleSection_t* bss = &footer->Bss;
    bss->SectionStart = 1;
    bss->SectionSize = context->Size;

    ASSERT_FALSE(Dmod_Ldr_LoadBss(context));

    Dmod_Context_Delete(context);
}

//================================================================
//                  Tests for Dmod_Ldr_Load
//================================================================

/**
 * @brief Test for Dmod_Ldr_Load
 * 
 * The test checks if the function loads the module correctly.
 */
TEST_F(DmodLdrTest, Load)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize, false));
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);
    
    ASSERT_TRUE(Dmod_Ldr_Load(context));

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Ldr_Load
 * 
 * The test checks if the function fails to load the module with an invalid context.
 */
TEST_F(DmodLdrTest, LoadInvalidContext)
{
    Dmod_Context_t* context = nullptr;
    ASSERT_FALSE(Dmod_Ldr_Load(context));
}