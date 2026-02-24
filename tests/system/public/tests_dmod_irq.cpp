#define DMOD_PRIVATE
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "private/dmod_ctx.h"
#include "private/dmod_vars.h"
#include "dmod.h"
#include "dmod_system.h"

// ===============================================================
//                  Memory helpers (one definition per test binary)
// ===============================================================
#ifndef DMOD_IRQ_TEST_HELPERS_DEFINED
#define DMOD_IRQ_TEST_HELPERS_DEFINED

void* Dmod_AlignedMalloc(size_t Size, size_t Alignment)
{
    size_t pagesize = sysconf(_SC_PAGESIZE);
    void* mem = aligned_alloc(pagesize, Size);
    if (mprotect(mem, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
    {
        free(mem);
        return NULL;
    }
    return mem;
}

void Dmod_Free(void* ptr)
{
    free(ptr);
}

void* Dmod_Malloc(size_t Size)
{
    return malloc(Size);
}

void* Dmod_Mutex_New(bool Recursive)
{
    pthread_mutex_t* Mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (Mutex == NULL) { return NULL; }
    pthread_mutexattr_t Attr;
    pthread_mutexattr_init(&Attr);
    if (Recursive) { pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE_NP); }
    if (pthread_mutex_init(Mutex, &Attr) != 0) { free(Mutex); return NULL; }
    return Mutex;
}

void Dmod_Mutex_Delete(void* Mutex)
{
    if (Mutex != NULL)
    {
        pthread_mutex_destroy((pthread_mutex_t*)Mutex);
        free(Mutex);
    }
}

void Dmod_FreeModule(const char* ModuleName)
{
    (void)ModuleName;
}

#endif // DMOD_IRQ_TEST_HELPERS_DEFINED

// ===============================================================
//                  Test fixture
// ===============================================================

class DmodIrqTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        memset(Dmod_Contexts, 0, sizeof(Dmod_Contexts));
    }

    void TearDown() override
    {
        memset(Dmod_Contexts, 0, sizeof(Dmod_Contexts));
    }
};

// ===============================================================
//                  Tests for Dmod_Irq
// ===============================================================

/**
 * @brief Test Dmod_Irq with NULL context returns -EINVAL
 */
TEST_F(DmodIrqTest, IrqNullContextReturnsError)
{
    EXPECT_EQ(Dmod_Irq(NULL, 0), -EINVAL);
}

/**
 * @brief Test Dmod_Irq with invalid context returns -EINVAL
 */
TEST_F(DmodIrqTest, IrqInvalidContextReturnsError)
{
    Dmod_Context_t invalidCtx;
    invalidCtx.Signature = 0; // invalid signature
    EXPECT_EQ(Dmod_Irq(&invalidCtx, 0), -EINVAL);
}

/**
 * @brief Test Dmod_Irq with valid context but no inputs returns -EINVAL
 */
TEST_F(DmodIrqTest, IrqValidContextNoInputsReturnsError)
{
    size_t fileSize = 1024;
    void* data = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);
    ASSERT_NE(data, nullptr);
    Dmod_Context_t* ctx = Dmod_Context_New(data, fileSize);
    ASSERT_NE(ctx, nullptr);

    // Inputs section is NULL by default (not set up)
    ctx->Inputs.InputSection = NULL;
    ctx->Inputs.SectionSize = 0;

    EXPECT_EQ(Dmod_Irq(ctx, 0), -EINVAL);

    Dmod_Context_Delete(ctx);
}

/**
 * @brief Test Dmod_Irq with valid context and matching IRQ handler calls the handler
 */
TEST_F(DmodIrqTest, IrqCallsMatchingHandler)
{
    static bool handlerCalled = false;

    // Build a fake input section with one IRQ entry
    static void (*irqHandler)(void) = []() { handlerCalled = true; };

    // Build the IRQ signature for IRQ number 5
    char signature[DMOD_IRQ_SIGNATURE_BUFFER_SIZE];
    Dmod_SnPrintf(signature, sizeof(signature), DMOD_IRQ_SIGNATURE_PREFIX "%d", 5);

    static char sigBuf[DMOD_IRQ_SIGNATURE_BUFFER_SIZE];
    memcpy(sigBuf, signature, sizeof(signature));

    static Dmod_InputsSection_t inputSection;
    inputSection.Entries[0].Function  = (void*)irqHandler;
    inputSection.Entries[0].Signature = sigBuf;
    inputSection.Entries[1].Function  = NULL;
    inputSection.Entries[1].Signature = NULL;

    size_t fileSize = 1024;
    void* data = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);
    ASSERT_NE(data, nullptr);
    Dmod_Context_t* ctx = Dmod_Context_New(data, fileSize);
    ASSERT_NE(ctx, nullptr);

    ctx->Inputs.InputSection = &inputSection;
    ctx->Inputs.SectionSize  = sizeof(Dmod_ApiRegistration_t); // one entry
    ctx->Inputs.ApiType      = Dmod_ApiType_Input;
    ctx->Inputs.Crossplatform = false;

    handlerCalled = false;
    EXPECT_EQ(Dmod_Irq(ctx, 5), 0);
    EXPECT_TRUE(handlerCalled);

    ctx->Inputs.InputSection = NULL;
    ctx->Inputs.SectionSize  = 0;
    Dmod_Context_Delete(ctx);
}

/**
 * @brief Test Dmod_Irq does NOT call handler for a different IRQ number
 */
TEST_F(DmodIrqTest, IrqDoesNotCallHandlerForDifferentNumber)
{
    static bool handlerCalled = false;
    static void (*irqHandler)(void) = []() { handlerCalled = true; };

    char signature[DMOD_IRQ_SIGNATURE_BUFFER_SIZE];
    Dmod_SnPrintf(signature, sizeof(signature), DMOD_IRQ_SIGNATURE_PREFIX "%d", 3);

    static char sigBuf[DMOD_IRQ_SIGNATURE_BUFFER_SIZE];
    memcpy(sigBuf, signature, sizeof(signature));

    static Dmod_InputsSection_t inputSection;
    inputSection.Entries[0].Function  = (void*)irqHandler;
    inputSection.Entries[0].Signature = sigBuf;
    inputSection.Entries[1].Function  = NULL;
    inputSection.Entries[1].Signature = NULL;

    size_t fileSize = 1024;
    void* data = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);
    ASSERT_NE(data, nullptr);
    Dmod_Context_t* ctx = Dmod_Context_New(data, fileSize);
    ASSERT_NE(ctx, nullptr);

    ctx->Inputs.InputSection = &inputSection;
    ctx->Inputs.SectionSize  = sizeof(Dmod_ApiRegistration_t);
    ctx->Inputs.ApiType      = Dmod_ApiType_Input;
    ctx->Inputs.Crossplatform = false;

    handlerCalled = false;
    // IRQ 7 != 3, so handler should NOT be called
    EXPECT_EQ(Dmod_Irq(ctx, 7), 0);
    EXPECT_FALSE(handlerCalled);

    ctx->Inputs.InputSection = NULL;
    ctx->Inputs.SectionSize  = 0;
    Dmod_Context_Delete(ctx);
}

// ===============================================================
//                  Tests for Dmod_IrqAll
// ===============================================================

/**
 * @brief Test Dmod_IrqAll with no loaded modules does nothing
 */
TEST_F(DmodIrqTest, IrqAllNoModulesDoesNotCrash)
{
    // All Dmod_Contexts are NULL - should simply return without crash
    EXPECT_NO_FATAL_FAILURE(Dmod_IrqAll(0));
}

/**
 * @brief Test Dmod_IrqAll calls handlers in all loaded modules
 */
TEST_F(DmodIrqTest, IrqAllCallsHandlersInAllModules)
{
    static int callCount = 0;
    static void (*irqHandler)(void) = []() { callCount++; };

    char signature[DMOD_IRQ_SIGNATURE_BUFFER_SIZE];
    Dmod_SnPrintf(signature, sizeof(signature), DMOD_IRQ_SIGNATURE_PREFIX "%d", 2);

    static char sigBuf[DMOD_IRQ_SIGNATURE_BUFFER_SIZE];
    memcpy(sigBuf, signature, sizeof(signature));

    static Dmod_InputsSection_t inputSection;
    inputSection.Entries[0].Function  = (void*)irqHandler;
    inputSection.Entries[0].Signature = sigBuf;
    inputSection.Entries[1].Function  = NULL;
    inputSection.Entries[1].Signature = NULL;

    size_t fileSize = 1024;

    void* data1 = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);
    ASSERT_NE(data1, nullptr);
    Dmod_Context_t* ctx1 = Dmod_Context_New(data1, fileSize);
    ASSERT_NE(ctx1, nullptr);
    ctx1->Inputs.InputSection = &inputSection;
    ctx1->Inputs.SectionSize  = sizeof(Dmod_ApiRegistration_t);
    ctx1->Inputs.ApiType      = Dmod_ApiType_Input;
    ctx1->Inputs.Crossplatform = false;

    void* data2 = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);
    ASSERT_NE(data2, nullptr);
    Dmod_Context_t* ctx2 = Dmod_Context_New(data2, fileSize);
    ASSERT_NE(ctx2, nullptr);
    ctx2->Inputs.InputSection = &inputSection;
    ctx2->Inputs.SectionSize  = sizeof(Dmod_ApiRegistration_t);
    ctx2->Inputs.ApiType      = Dmod_ApiType_Input;
    ctx2->Inputs.Crossplatform = false;

    // Register both contexts
    Dmod_Contexts[0] = ctx1;
    Dmod_Contexts[1] = ctx2;

    callCount = 0;
    Dmod_IrqAll(2);
    EXPECT_EQ(callCount, 2);

    // Cleanup
    ctx1->Inputs.InputSection = NULL;
    ctx1->Inputs.SectionSize  = 0;
    ctx2->Inputs.InputSection = NULL;
    ctx2->Inputs.SectionSize  = 0;
    Dmod_Contexts[0] = NULL;
    Dmod_Contexts[1] = NULL;
    Dmod_Context_Delete(ctx1);
    Dmod_Context_Delete(ctx2);
}
