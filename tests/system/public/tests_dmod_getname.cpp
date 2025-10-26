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
//                  Test fixture
// ===============================================================

/**
 * @brief Allocate aligned memory
 * 
 * @param Size Size of memory to allocate
 * @param Alignment Alignment of memory
 * 
 * @return Pointer to allocated memory
 * 
 * @note Optional - set to NULL if not supported
 */
void* Dmod_AlignedMalloc(size_t Size, size_t Alignment)
{
    size_t pagesize = Alignment;
    pagesize = sysconf(_SC_PAGESIZE);
    void* mem = aligned_alloc(pagesize, Size);
    if (mprotect(mem, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) 
    {
        DMOD_LOG_ERROR("Cannot set memory protection. Pagesize: %d\n", pagesize);
        free(mem);
        return NULL;
    }
    return mem;
}

/**
 * @brief Free memory
 * 
 * @param ptr Pointer to memory to free
 */
void Dmod_Free(void *ptr)
{
    free(ptr);
}

/**
 * @brief Allocate memory
 * 
 * @param Size Size of memory to allocate
 * 
 * @return Pointer to allocated memory
 */
void* Dmod_Malloc(size_t Size)        
{
    return malloc(Size);
}

/**
 * @brief Create new mutex
 * 
 * @param Recursive Recursive mutex
 * 
 * @return Pointer to new mutex
 */
void* Dmod_Mutex_New( bool Recursive )
{
    pthread_mutex_t* Mutex = (pthread_mutex_t*)malloc( sizeof( pthread_mutex_t ) );
    if( Mutex == NULL )
    {
        DMOD_LOG_ERROR("Cannot create new mutex - cannot allocate memory\n");
        return NULL;
    }

    pthread_mutexattr_t Attr;
    if( pthread_mutexattr_init( &Attr ) != 0 )
    {
        DMOD_LOG_ERROR("Cannot create new mutex - cannot initialize mutex attribute\n");
        Dmod_Free( Mutex );
        return NULL;
    }

    if( Recursive )
    {
        if( pthread_mutexattr_settype( &Attr, PTHREAD_MUTEX_RECURSIVE_NP ) != 0 )
        {
            DMOD_LOG_ERROR("Cannot create new mutex - cannot set mutex attribute\n");
            Dmod_Free( Mutex );
            return NULL;
        }
    }

    if( pthread_mutex_init( Mutex, &Attr ) != 0 )
    {
        DMOD_LOG_ERROR("Cannot create new mutex - cannot initialize mutex\n");
        Dmod_Free( Mutex );
        return NULL;
    }

    return Mutex;
}

/**
 * @brief loads DMF test file for testing
 */
static bool LoadDmfTestFile( void** outData, size_t* outSize )
{
#ifdef DMOD_TEST_DMF_FILE
    Dmod_Printf("Loading DMF test file: %s\n", DMOD_TEST_DMF_FILE);
    FILE* file = fopen(DMOD_TEST_DMF_FILE, "rb");
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
#else
    return false;
#endif
}

class DmodGetNameTest : public ::testing::Test
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
//                  Tests for Dmod_GetName
// ===============================================================

/**
 * @brief Test for Dmod_GetName
 * 
 * The test checks if the function returns the module name for a valid context.
 */
TEST_F(DmodGetNameTest, GetNameValidContext)
{
#ifdef DMOD_TEST_DMF_FILE
    size_t fileSize = 0;
    void* data = nullptr;
    if (!LoadDmfTestFile(&data, &fileSize))
    {
        GTEST_SKIP() << "DMF test file not available";
    }

    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);

    // Set the module header
    ASSERT_NE(context, nullptr);
    context->Header = reinterpret_cast<Dmod_ModuleHeader_t*>(data);
    const char* moduleName = Dmod_GetName(context);
    EXPECT_NE(moduleName, nullptr);
    EXPECT_STREQ(moduleName, "example_app");

    Dmod_Context_Delete(context);
#else
    GTEST_SKIP() << "DMF test file not compiled in";
#endif
}

/**
 * @brief Test for Dmod_GetName
 * 
 * The test checks if the function returns "Invalid" for an invalid context.
 */
TEST_F(DmodGetNameTest, GetNameInvalidContext)
{
    Dmod_Context_t* context = nullptr;
    ASSERT_STREQ(Dmod_GetName(context), "Invalid");
}

/**
 * @brief Test for Dmod_GetName
 * 
 * The test checks if the function returns "Unknown" for a context without header.
 */
TEST_F(DmodGetNameTest, GetNameNoHeader)
{
    size_t fileSize = 1024;
    void* data = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_STREQ(Dmod_GetName(context), "Unknown");
    Dmod_Context_Delete(context);
}
