#define DMOD_PRIVATE
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <pthread.h>
#include "private/dmod_ctx.h"
#include "private/dmod_vars.h"
#include "dmod.h"
#include "dmod_system.h"

// ===============================================================
//                  Mock implementations
// ===============================================================

void* Dmod_AlignedMalloc(size_t Size, size_t Alignment)
{
    size_t pagesize = sysconf(_SC_PAGESIZE);
    void* mem = aligned_alloc(pagesize, Size);
    if (mprotect(mem, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) 
    {
        DMOD_LOG_ERROR("Cannot set memory protection. Pagesize: %zu\n", pagesize);
        free(mem);
        return NULL;
    }
    return mem;
}

void Dmod_Free(void *ptr)
{
    free(ptr);
}

void* Dmod_Malloc(size_t Size)        
{
    return malloc(Size);
}

void* Dmod_Mutex_New( bool Recursive )
{
    pthread_mutex_t* Mutex = (pthread_mutex_t*)malloc( sizeof( pthread_mutex_t ) );
    if( Mutex == NULL )
    {
        return NULL;
    }

    pthread_mutexattr_t Attr;
    if( pthread_mutexattr_init( &Attr ) != 0 )
    {
        free(Mutex);
        return NULL;
    }

    if( Recursive )
    {
        if( pthread_mutexattr_settype( &Attr, PTHREAD_MUTEX_RECURSIVE ) != 0 )
        {
            pthread_mutexattr_destroy( &Attr );
            free(Mutex);
            return NULL;
        }
    }

    if( pthread_mutex_init( Mutex, &Attr ) != 0 )
    {
        pthread_mutexattr_destroy( &Attr );
        free(Mutex);
        return NULL;
    }

    pthread_mutexattr_destroy( &Attr );
    return Mutex;
}

void Dmod_Mutex_Delete( void* Mutex )
{
    if( Mutex != NULL )
    {
        pthread_mutex_destroy( (pthread_mutex_t*)Mutex );
        free(Mutex);
    }
}

int Dmod_Mutex_Lock( void* Mutex )
{
    if( Mutex != NULL )
    {
        return pthread_mutex_lock( (pthread_mutex_t*)Mutex );
    }
    return 0;
}

int Dmod_Mutex_Unlock( void* Mutex )
{
    if( Mutex != NULL )
    {
        return pthread_mutex_unlock( (pthread_mutex_t*)Mutex );
    }
    return 0;
}

void Dmod_EnterCritical()
{
}

void Dmod_ExitCritical()
{
}

// ===============================================================
//                  Test fixture
// ===============================================================

class DmodGetNextModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear the contexts array
        for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
        {
            Dmod_Contexts[i] = NULL;
        }
    }

    void TearDown() override {
        // Clean up all contexts
        for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
        {
            if( Dmod_Contexts[i] != NULL )
            {
                Dmod_Context_Delete( Dmod_Contexts[i] );
                Dmod_Contexts[i] = NULL;
            }
        }
    }

    // Helper to create a mock module context
    Dmod_Context_t* CreateMockContext(const char* name, const char* version, 
                                       bool enabled, bool running)
    {
        // Create a minimal valid module header
        size_t headerSize = sizeof(Dmod_ModuleHeader_t);
        Dmod_Context_t* ctx = Dmod_Context_New(NULL, headerSize);
        if( ctx == NULL )
        {
            return NULL;
        }

        // Initialize header
        ctx->Header = (Dmod_ModuleHeader_t*)ctx->Data;
        ctx->Header->Signature = DMOD_HEADER_SIGNATURE;
        strncpy(ctx->Header->Name, name, DMOD_MAX_MODULE_NAME_LENGTH - 1);
        ctx->Header->Name[DMOD_MAX_MODULE_NAME_LENGTH - 1] = '\0';
        strncpy(ctx->Header->Version, version, DMOD_MAX_VERSION_LENGTH - 1);
        ctx->Header->Version[DMOD_MAX_VERSION_LENGTH - 1] = '\0';
        
        ctx->Enabled = enabled;
        ctx->Running = running;
        
        return ctx;
    }
};

// ===============================================================
//                  Tests
// ===============================================================

TEST_F(DmodGetNextModuleTest, GetNextModuleNoModules)
{
    const Dmod_ModuleInfo_t* info = Dmod_GetNextModule(NULL);
    EXPECT_EQ(info, nullptr);
}

TEST_F(DmodGetNextModuleTest, GetNextModuleSingleModule)
{
    Dmod_Context_t* ctx = CreateMockContext("test_module", "1.0", true, false);
    ASSERT_NE(ctx, nullptr);
    Dmod_Context_Add(ctx);

    const Dmod_ModuleInfo_t* info = Dmod_GetNextModule(NULL);
    ASSERT_NE(info, nullptr);
    EXPECT_STREQ(info->ModuleName, "test_module");
    EXPECT_STREQ(info->Version, "1.0");
    EXPECT_EQ(info->State, Dmod_ModuleState_Enabled);

    // Next call should return NULL
    const Dmod_ModuleInfo_t* next = Dmod_GetNextModule(info);
    EXPECT_EQ(next, nullptr);
}

TEST_F(DmodGetNextModuleTest, GetNextModuleMultipleModules)
{
    Dmod_Context_t* ctx1 = CreateMockContext("module_one", "1.0", true, false);
    Dmod_Context_t* ctx2 = CreateMockContext("module_two", "2.0", false, false);
    Dmod_Context_t* ctx3 = CreateMockContext("module_three", "3.0", true, true);
    
    ASSERT_NE(ctx1, nullptr);
    ASSERT_NE(ctx2, nullptr);
    ASSERT_NE(ctx3, nullptr);
    
    Dmod_Context_Add(ctx1);
    Dmod_Context_Add(ctx2);
    Dmod_Context_Add(ctx3);

    const Dmod_ModuleInfo_t* info = Dmod_GetNextModule(NULL);
    ASSERT_NE(info, nullptr);
    EXPECT_STREQ(info->ModuleName, "module_one");
    EXPECT_EQ(info->State, Dmod_ModuleState_Enabled);

    info = Dmod_GetNextModule(info);
    ASSERT_NE(info, nullptr);
    EXPECT_STREQ(info->ModuleName, "module_two");
    EXPECT_EQ(info->State, Dmod_ModuleState_Loaded);

    info = Dmod_GetNextModule(info);
    ASSERT_NE(info, nullptr);
    EXPECT_STREQ(info->ModuleName, "module_three");
    EXPECT_EQ(info->State, Dmod_ModuleState_Running);

    // Next call should return NULL
    info = Dmod_GetNextModule(info);
    EXPECT_EQ(info, nullptr);
}

TEST_F(DmodGetNextModuleTest, GetNextModuleStates)
{
    // Test all three states
    Dmod_Context_t* ctxLoaded = CreateMockContext("loaded", "1.0", false, false);
    Dmod_Context_t* ctxEnabled = CreateMockContext("enabled", "1.0", true, false);
    Dmod_Context_t* ctxRunning = CreateMockContext("running", "1.0", true, true);
    
    ASSERT_NE(ctxLoaded, nullptr);
    ASSERT_NE(ctxEnabled, nullptr);
    ASSERT_NE(ctxRunning, nullptr);
    
    Dmod_Context_Add(ctxLoaded);
    Dmod_Context_Add(ctxEnabled);
    Dmod_Context_Add(ctxRunning);

    const Dmod_ModuleInfo_t* info = Dmod_GetNextModule(NULL);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->State, Dmod_ModuleState_Loaded);

    info = Dmod_GetNextModule(info);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->State, Dmod_ModuleState_Enabled);

    info = Dmod_GetNextModule(info);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->State, Dmod_ModuleState_Running);
}

TEST_F(DmodGetNextModuleTest, GetNextModuleWithGaps)
{
    // Add modules with gaps in the array
    Dmod_Context_t* ctx1 = CreateMockContext("module1", "1.0", true, false);
    Dmod_Context_t* ctx2 = CreateMockContext("module2", "2.0", true, false);
    
    ASSERT_NE(ctx1, nullptr);
    ASSERT_NE(ctx2, nullptr);
    
    Dmod_Contexts[0] = ctx1;
    Dmod_Contexts[5] = ctx2;  // Leave gaps

    const Dmod_ModuleInfo_t* info = Dmod_GetNextModule(NULL);
    ASSERT_NE(info, nullptr);
    EXPECT_STREQ(info->ModuleName, "module1");

    info = Dmod_GetNextModule(info);
    ASSERT_NE(info, nullptr);
    EXPECT_STREQ(info->ModuleName, "module2");

    info = Dmod_GetNextModule(info);
    EXPECT_EQ(info, nullptr);
}
