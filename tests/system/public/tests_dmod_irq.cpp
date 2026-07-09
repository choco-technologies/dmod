#define DMOD_PRIVATE
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "private/dmod_ctx.h"
#include "private/dmod_irq.h"
#include "private/dmod_vars.h"
#include "dmod.h"
#include "dmod_system.h"

// ===============================================================
//                  Memory / RTOS helpers
// ===============================================================

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

void Dmod_Free(void* ptr)   { free(ptr); }
void* Dmod_Malloc(size_t Size) { return malloc(Size); }

// Dmod_Context_New() calls these directly (it has no ambient module identity of its own)
void* Dmod_MallocEx(size_t Size, const char* ModuleName) { (void)ModuleName; return Dmod_Malloc(Size); }
void* Dmod_AlignedMallocEx(size_t Size, size_t Alignment, const char* ModuleName) { (void)ModuleName; return Dmod_AlignedMalloc(Size, Alignment); }
void Dmod_FreeEx(void* ptr, bool Concatenate) { (void)Concatenate; Dmod_Free(ptr); }

void* Dmod_Mutex_New(bool Recursive)
{
    pthread_mutex_t* m = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (!m) return NULL;
    pthread_mutexattr_t a;
    pthread_mutexattr_init(&a);
    if (Recursive) pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE_NP);
    if (pthread_mutex_init(m, &a) != 0) { free(m); return NULL; }
    return m;
}

void Dmod_Mutex_Delete(void* Mutex)
{
    if (Mutex) { pthread_mutex_destroy((pthread_mutex_t*)Mutex); free(Mutex); }
}

void Dmod_FreeModule(const char* ModuleName) { (void)ModuleName; }

// ===============================================================
//                  Helpers
// ===============================================================

static Dmod_Context_t* MakeContext(size_t fileSize = 1024)
{
    void* data = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);
    if (!data) return nullptr;
    return Dmod_Context_New(data, fileSize, NULL);
}

static void AttachInputSection(Dmod_Context_t* ctx,
                                Dmod_InputsSection_t* section,
                                size_t numEntries)
{
    ctx->Inputs.InputSection  = section;
    ctx->Inputs.SectionSize   = numEntries * sizeof(Dmod_ApiRegistration_t);
    ctx->Inputs.ApiType       = Dmod_ApiType_Input;
    ctx->Inputs.Crossplatform = false;
}

static void DetachInputSection(Dmod_Context_t* ctx)
{
    ctx->Inputs.InputSection = NULL;
    ctx->Inputs.SectionSize  = 0;
}

// ===============================================================
//                  Test fixture
// ===============================================================

class DmodIrqTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        memset(Dmod_Contexts, 0, sizeof(Dmod_Contexts));
        Dmod_Irq_Deinit();
    }

    void TearDown() override
    {
        Dmod_Irq_Deinit();
        memset(Dmod_Contexts, 0, sizeof(Dmod_Contexts));
    }
};

// ===============================================================
//                  Tests for Dmod_Irq_Init / Dmod_Irq_Deinit
// ===============================================================

TEST_F(DmodIrqTest, IrqInitZeroDisablesTable)
{
    EXPECT_TRUE(Dmod_Irq_Init(0, 0));
    // IrqAll with disabled table should be a no-op
    EXPECT_NO_FATAL_FAILURE(Dmod_IrqAll(0));
}

TEST_F(DmodIrqTest, IrqInitAllocatesTable)
{
    EXPECT_TRUE(Dmod_Irq_Init(16, 4));
    EXPECT_NO_FATAL_FAILURE(Dmod_IrqAll(0));
    EXPECT_NO_FATAL_FAILURE(Dmod_IrqAll(15));
}

TEST_F(DmodIrqTest, IrqDeinitCanBeCalledTwice)
{
    EXPECT_TRUE(Dmod_Irq_Init(8, 2));
    Dmod_Irq_Deinit();
    EXPECT_NO_FATAL_FAILURE(Dmod_Irq_Deinit());
}

// ===============================================================
//                  Tests for Dmod_Irq_RegisterModule
// ===============================================================

TEST_F(DmodIrqTest, RegisterModuleAddsHandlerToTable)
{
    static int callCount = 0;
    static void (*handler)(void) = []() { callCount++; };

    char sigBuf[DMOD_IRQ_SIGNATURE_BUFFER_SIZE];
    Dmod_SnPrintf(sigBuf, sizeof(sigBuf), DMOD_IRQ_SIGNATURE_PREFIX "%d", 3);

    static Dmod_InputsSection_t section;
    section.Entries[0].Function  = (void*)handler;
    section.Entries[0].Signature = sigBuf;
    section.Entries[1].Function  = NULL;
    section.Entries[1].Signature = NULL;

    Dmod_Context_t* ctx = MakeContext();
    ASSERT_NE(ctx, nullptr);
    AttachInputSection(ctx, &section, 1);

    ASSERT_TRUE(Dmod_Irq_Init(8, 4));
    EXPECT_TRUE(Dmod_Irq_RegisterModule(ctx));

    callCount = 0;
    Dmod_IrqAll(3);
    EXPECT_EQ(callCount, 1);

    DetachInputSection(ctx);
    Dmod_Context_Delete(ctx);
}

TEST_F(DmodIrqTest, RegisterModuleIgnoresNonIrqEntries)
{
    static int callCount = 0;
    static void (*handler)(void) = []() { callCount++; };

    static Dmod_InputsSection_t section;
    section.Entries[0].Function  = (void*)handler;
    // Regular API signature, not an IRQ signature
    section.Entries[0].Signature = "\021DMOD\022SomeApi@MyModule:1.0";
    section.Entries[1].Function  = NULL;
    section.Entries[1].Signature = NULL;

    Dmod_Context_t* ctx = MakeContext();
    ASSERT_NE(ctx, nullptr);
    AttachInputSection(ctx, &section, 1);

    ASSERT_TRUE(Dmod_Irq_Init(8, 4));
    EXPECT_TRUE(Dmod_Irq_RegisterModule(ctx));

    callCount = 0;
    Dmod_IrqAll(0); // should not call the non-IRQ handler
    EXPECT_EQ(callCount, 0);

    DetachInputSection(ctx);
    Dmod_Context_Delete(ctx);
}

TEST_F(DmodIrqTest, RegisterModuleIrqNumberOutOfRange)
{
    static int callCount = 0;
    static void (*handler)(void) = []() { callCount++; };

    char sigBuf[DMOD_IRQ_SIGNATURE_BUFFER_SIZE];
    Dmod_SnPrintf(sigBuf, sizeof(sigBuf), DMOD_IRQ_SIGNATURE_PREFIX "%d", 99);

    static Dmod_InputsSection_t section;
    section.Entries[0].Function  = (void*)handler;
    section.Entries[0].Signature = sigBuf;
    section.Entries[1].Function  = NULL;
    section.Entries[1].Signature = NULL;

    Dmod_Context_t* ctx = MakeContext();
    ASSERT_NE(ctx, nullptr);
    AttachInputSection(ctx, &section, 1);

    ASSERT_TRUE(Dmod_Irq_Init(4, 2)); // table only has IRQs 0..3
    // IRQ 99 is out of range - should be silently skipped (returns true, not an error)
    EXPECT_TRUE(Dmod_Irq_RegisterModule(ctx));

    DetachInputSection(ctx);
    Dmod_Context_Delete(ctx);
}

TEST_F(DmodIrqTest, RegisterModuleReturnsFalseWhenNoFreeSlots)
{
    static void (*h1)(void) = [](){};
    static void (*h2)(void) = [](){};

    char sigBuf[DMOD_IRQ_SIGNATURE_BUFFER_SIZE];
    Dmod_SnPrintf(sigBuf, sizeof(sigBuf), DMOD_IRQ_SIGNATURE_PREFIX "%d", 0);

    // Table has only 1 slot per IRQ
    ASSERT_TRUE(Dmod_Irq_Init(4, 1));

    static Dmod_InputsSection_t sec1, sec2;
    sec1.Entries[0] = { (void*)h1, sigBuf };
    sec1.Entries[1] = { NULL, NULL };
    sec2.Entries[0] = { (void*)h2, sigBuf };
    sec2.Entries[1] = { NULL, NULL };

    Dmod_Context_t* ctx1 = MakeContext();
    Dmod_Context_t* ctx2 = MakeContext();
    ASSERT_NE(ctx1, nullptr);
    ASSERT_NE(ctx2, nullptr);
    AttachInputSection(ctx1, &sec1, 1);
    AttachInputSection(ctx2, &sec2, 1);

    EXPECT_TRUE(Dmod_Irq_RegisterModule(ctx1));   // first registration succeeds
    EXPECT_FALSE(Dmod_Irq_RegisterModule(ctx2));  // second fails - no free slot

    DetachInputSection(ctx1);
    DetachInputSection(ctx2);
    Dmod_Context_Delete(ctx1);
    Dmod_Context_Delete(ctx2);
}

// ===============================================================
//                  Tests for Dmod_Irq_UnregisterModule
// ===============================================================

TEST_F(DmodIrqTest, UnregisterModuleRemovesHandlerFromTable)
{
    static int callCount = 0;
    static void (*handler)(void) = []() { callCount++; };

    char sigBuf[DMOD_IRQ_SIGNATURE_BUFFER_SIZE];
    Dmod_SnPrintf(sigBuf, sizeof(sigBuf), DMOD_IRQ_SIGNATURE_PREFIX "%d", 1);

    static Dmod_InputsSection_t section;
    section.Entries[0].Function  = (void*)handler;
    section.Entries[0].Signature = sigBuf;
    section.Entries[1].Function  = NULL;
    section.Entries[1].Signature = NULL;

    Dmod_Context_t* ctx = MakeContext();
    ASSERT_NE(ctx, nullptr);
    AttachInputSection(ctx, &section, 1);

    ASSERT_TRUE(Dmod_Irq_Init(8, 4));
    EXPECT_TRUE(Dmod_Irq_RegisterModule(ctx));

    // Verify it fires
    callCount = 0;
    Dmod_IrqAll(1);
    EXPECT_EQ(callCount, 1);

    // Unregister and verify it no longer fires
    Dmod_Irq_UnregisterModule(ctx);
    callCount = 0;
    Dmod_IrqAll(1);
    EXPECT_EQ(callCount, 0);

    DetachInputSection(ctx);
    Dmod_Context_Delete(ctx);
}

// ===============================================================
//                  Tests for Dmod_IrqAll
// ===============================================================

TEST_F(DmodIrqTest, IrqAllDisabledTableIsNoOp)
{
    // Table not initialised - should not crash
    EXPECT_NO_FATAL_FAILURE(Dmod_IrqAll(0));
}

TEST_F(DmodIrqTest, IrqAllOutOfRangeIsNoOp)
{
    ASSERT_TRUE(Dmod_Irq_Init(4, 2));
    EXPECT_NO_FATAL_FAILURE(Dmod_IrqAll(-1));
    EXPECT_NO_FATAL_FAILURE(Dmod_IrqAll(100));
}

TEST_F(DmodIrqTest, IrqAllCallsMultipleHandlersFromMultipleModules)
{
    static int count = 0;
    static void (*h1)(void) = []() { count++; };
    static void (*h2)(void) = []() { count++; };

    char sigBuf[DMOD_IRQ_SIGNATURE_BUFFER_SIZE];
    Dmod_SnPrintf(sigBuf, sizeof(sigBuf), DMOD_IRQ_SIGNATURE_PREFIX "%d", 5);

    static Dmod_InputsSection_t sec1, sec2;
    sec1.Entries[0] = { (void*)h1, sigBuf };
    sec1.Entries[1] = { NULL, NULL };
    sec2.Entries[0] = { (void*)h2, sigBuf };
    sec2.Entries[1] = { NULL, NULL };

    Dmod_Context_t* ctx1 = MakeContext();
    Dmod_Context_t* ctx2 = MakeContext();
    ASSERT_NE(ctx1, nullptr);
    ASSERT_NE(ctx2, nullptr);
    AttachInputSection(ctx1, &sec1, 1);
    AttachInputSection(ctx2, &sec2, 1);

    ASSERT_TRUE(Dmod_Irq_Init(8, 4));
    EXPECT_TRUE(Dmod_Irq_RegisterModule(ctx1));
    EXPECT_TRUE(Dmod_Irq_RegisterModule(ctx2));

    count = 0;
    Dmod_IrqAll(5);
    EXPECT_EQ(count, 2);

    DetachInputSection(ctx1);
    DetachInputSection(ctx2);
    Dmod_Context_Delete(ctx1);
    Dmod_Context_Delete(ctx2);
}

// ===============================================================
//                  Tests for Dmod_Irq (per-context slow path)
// ===============================================================

TEST_F(DmodIrqTest, IrqNullContextReturnsError)
{
    EXPECT_EQ(Dmod_Irq(NULL, 0), -EINVAL);
}

TEST_F(DmodIrqTest, IrqInvalidContextReturnsError)
{
    Dmod_Context_t bad;
    bad.Signature = 0;
    EXPECT_EQ(Dmod_Irq(&bad, 0), -EINVAL);
}

TEST_F(DmodIrqTest, IrqCallsMatchingHandler)
{
    static bool called = false;
    static void (*handler)(void) = []() { called = true; };

    char sigBuf[DMOD_IRQ_SIGNATURE_BUFFER_SIZE];
    Dmod_SnPrintf(sigBuf, sizeof(sigBuf), DMOD_IRQ_SIGNATURE_PREFIX "%d", 7);

    static Dmod_InputsSection_t section;
    section.Entries[0] = { (void*)handler, sigBuf };
    section.Entries[1] = { NULL, NULL };

    Dmod_Context_t* ctx = MakeContext();
    ASSERT_NE(ctx, nullptr);
    AttachInputSection(ctx, &section, 1);

    called = false;
    EXPECT_EQ(Dmod_Irq(ctx, 7), 0);
    EXPECT_TRUE(called);

    DetachInputSection(ctx);
    Dmod_Context_Delete(ctx);
}

TEST_F(DmodIrqTest, IrqDoesNotCallHandlerForDifferentNumber)
{
    static bool called = false;
    static void (*handler)(void) = []() { called = true; };

    char sigBuf[DMOD_IRQ_SIGNATURE_BUFFER_SIZE];
    Dmod_SnPrintf(sigBuf, sizeof(sigBuf), DMOD_IRQ_SIGNATURE_PREFIX "%d", 2);

    static Dmod_InputsSection_t section;
    section.Entries[0] = { (void*)handler, sigBuf };
    section.Entries[1] = { NULL, NULL };

    Dmod_Context_t* ctx = MakeContext();
    ASSERT_NE(ctx, nullptr);
    AttachInputSection(ctx, &section, 1);

    called = false;
    EXPECT_EQ(Dmod_Irq(ctx, 9), 0);
    EXPECT_FALSE(called);

    DetachInputSection(ctx);
    Dmod_Context_Delete(ctx);
}
