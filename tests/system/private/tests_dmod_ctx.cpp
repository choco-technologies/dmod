#define DMOD_PRIVATE
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "private/dmod_ctx.h"
#include "private/dmod_vars.h"
#include "dmod.h"


// ===============================================================
//                  Mocks
// ===============================================================

class MockMemory 
{
public:
    MOCK_METHOD(void*, Malloc, (size_t Size));
    MOCK_METHOD(void*, AlignedMalloc, (size_t Size, size_t Alignment));
    MOCK_METHOD(void, Free, (void* ptr));
};

class MockRtos 
{
public:
    MOCK_METHOD(void*, MutexNew, (bool Recursive));
};

MockMemory* mockMemory = nullptr;
MockRtos* mockRtos = nullptr;

/**
 * @brief Allocate memory
 * 
 * @param Size Size of memory to allocate
 * 
 * @return Pointer to allocated memory
 */
void* Dmod_Malloc(size_t Size)        
{
    if( mockMemory != nullptr )
    {
        return mockMemory->Malloc(Size);
    }
    return malloc(Size);
}

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
    if( mockMemory != nullptr )
    {
        return mockMemory->AlignedMalloc(Size, Alignment);
    }
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
    if( mockMemory != nullptr )
    {
        mockMemory->Free(ptr);
        return;
    }
    free(ptr);
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
    if( mockRtos != nullptr )
    {
        return mockRtos->MutexNew(Recursive);
    }
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


// ===============================================================
//                  Tests for Dmod_Context_New
// ===============================================================

/**
 * @brief loads DMF test file for testing
 */
static bool LoadDmfTestFile( void** outData, size_t* outSize )
{
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
}

class DmodContextTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        memset(Dmod_Contexts, 0, sizeof(Dmod_Contexts));
    }

    void TearDown() override
    {
        DisableMockMemory();
        DisableMockRtos();
    }

    void EnableMockMemory()
    {
        mockMemory = new MockMemory();
    }

    void EnableMockRtos()
    {
        mockRtos = new MockRtos();
    }

    void DisableMockMemory()
    {
        if(mockMemory != nullptr)
        {
            testing::Mock::VerifyAndClearExpectations(&mockMemory);
            delete mockMemory;
            mockMemory = nullptr;
        }
    }

    void DisableMockRtos()
    {
        if(mockRtos != nullptr)
        {
            testing::Mock::VerifyAndClearExpectations(&mockRtos);
            delete mockRtos;
            mockRtos = nullptr;
        }
    }
};

/**
 * @brief Test for Dmod_Context_New
 * 
 * The test checks if the function creates a new context with the given data, 
 * assuming that the data is not NULL.
 */
TEST_F(DmodContextTest, NewWithData) 
{
    size_t fileSize = 1024;
    void* data = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_NE(context, nullptr);
    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Context_New
 * 
 * The test checks if the function creates a new context with the given data, 
 * assuming that the data is NULL.
 */
TEST_F(DmodContextTest, NewWithoutData)
{
    size_t fileSize = 1024;
    Dmod_Context_t* context = Dmod_Context_New(NULL, fileSize);
    ASSERT_NE(context, nullptr);
    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Context_New
 * 
 * The test checks if the function fails to create a new context with the given data, 
 * assuming that the data is NULL and the file size is 0.
 */
TEST_F(DmodContextTest, NewWithoutDataAndZeroFileSize)
{
    Dmod_Context_t* context = Dmod_Context_New(NULL, 0);
    ASSERT_EQ(context, nullptr);
}

/**
 * @brief Test for Dmod_Context_New
 * 
 * The test checks if the function fails if the Dmod_Malloc function fails.
 */
TEST_F(DmodContextTest, NewMallocFail)
{
    size_t fileSize = 1024;
    void* data = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);

    EnableMockMemory();
    EXPECT_CALL(*mockMemory, Malloc(testing::_)).WillOnce(testing::Return(nullptr));

    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_EQ(context, nullptr);
}

/**
 * @brief Test for Dmod_Context_New
 * 
 * The test checks if the function fails if the Dmod_AlignedMalloc function fails.
 */
TEST_F(DmodContextTest, NewAlignedMallocFail)
{
    size_t fileSize = 1024;
    void* data = nullptr;
    void* expectedContext = malloc(fileSize);

    EnableMockMemory();
    EnableMockRtos();
    EXPECT_CALL(*mockMemory, Malloc(testing::_)).WillOnce(testing::Return(expectedContext));
    EXPECT_CALL(*mockMemory, Free(testing::_)).Times(1);
    EXPECT_CALL(*mockMemory, AlignedMalloc(testing::_, testing::_)).WillOnce(testing::Return(nullptr));
    EXPECT_CALL(*mockRtos, MutexNew(testing::_)).WillOnce(testing::Return(nullptr));

    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_EQ(context, nullptr);

    free(expectedContext);
}

/**
 * @brief Test for Dmod_Context_New
 * 
 * The test checks if the function fails if the Dmod_Mutex_New function fails.
 */
TEST_F(DmodContextTest, NewMutexFail)
{
    size_t fileSize = 1024;
    void* data = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);

    EnableMockRtos();
    EXPECT_CALL(*mockRtos, MutexNew(testing::_)).WillOnce(testing::Return(nullptr));

    Dmod_Context_New(data, fileSize);

    // The test is successful if the function does not crash or hang
}

// ===============================================================
//                  Tests for Dmod_Context_IsValid
// ===============================================================
/**
 * @brief Test for Dmod_Context_IsValid
 * 
 * The test checks if the function returns true for a valid context.
 */
TEST_F(DmodContextTest, IsValidTrue)
{
    size_t fileSize = 1024;
    void* data = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_TRUE(Dmod_Context_IsValid(context));
    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Context_IsValid
 * 
 * The test checks if the function returns false for an invalid context.
 */
TEST_F(DmodContextTest, IsValidFalse)
{
    Dmod_Context_t* context = nullptr;
    ASSERT_FALSE(Dmod_Context_IsValid(context));
}

/**
 * @brief Test for Dmod_Context_IsValid
 * 
 * The test checks if the function returns false for a context with an invalid signature.
 */
TEST_F(DmodContextTest, IsValidInvalidSignature)
{
    size_t fileSize = 1024;
    void* data = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    context->Signature = 0;
    ASSERT_FALSE(Dmod_Context_IsValid(context));
    Dmod_Context_Delete(context);
}

// ===============================================================
//                  Tests for Dmod_Context_Delete
// ===============================================================
/**
 * @brief Test for Dmod_Context_Delete
 * 
 * The test checks if the function deletes a valid context.
 */
TEST_F(DmodContextTest, DeleteValidContext)
{
    size_t fileSize = 1024;
    void* data = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Context_Delete
 * 
 * The test checks if the function deletes an invalid context.
 */
TEST_F(DmodContextTest, DeleteInvalidContext)
{
    Dmod_Context_t* context = nullptr;
    Dmod_Context_Delete(context);
}

// ===============================================================
//                  Tests for Dmod_Context_GetModuleName
// ===============================================================
/**
 * @brief Test for Dmod_Context_GetModuleName
 * 
 * The test checks if the function returns the module name for a valid context.
 */
TEST_F(DmodContextTest, GetModuleNameValidContext)
{
    size_t fileSize = 1024;
    void* data = Dmod_AlignedMalloc(fileSize, DMOD_STACK_ALIGNMENT);
    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);
    ASSERT_STREQ(Dmod_Context_GetModuleName(context), "Unknown");
    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Context_GetModuleName
 * 
 * The test checks if the function returns "Invalid" for an invalid context.
 */
TEST_F(DmodContextTest, GetModuleNameInvalidContext)
{
    Dmod_Context_t* context = nullptr;
    ASSERT_STREQ(Dmod_Context_GetModuleName(context), "Invalid");
}

// ===============================================================
//                  Tests for Dmod_Context_Add
// ===============================================================
/**
 * @brief Test for Dmod_Context_Add
 * 
 * The test checks if the function adds a valid context.
 */
TEST_F(DmodContextTest, AddValidContext)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));

    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);

    // Get the module name
    ASSERT_NE(context, nullptr);
    context->Header = reinterpret_cast<Dmod_ModuleHeader_t*>(data);
    const char* moduleName = Dmod_Context_GetModuleName(context);
    EXPECT_NE(moduleName, nullptr);
    EXPECT_STREQ(moduleName, "example_app");

    ASSERT_TRUE(Dmod_Context_Add(context));

    // Check if the context is added to the list
    Dmod_Context_t* contextFromList = Dmod_Context_Get(moduleName);
    EXPECT_EQ(context, contextFromList);

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Context_Add
 * 
 * The test checks if the function fails to add an invalid context.
 */
TEST_F(DmodContextTest, AddInvalidContext)
{
    Dmod_Context_t* context = nullptr;
    ASSERT_FALSE(Dmod_Context_Add(context));
}

/**
 * @brief Test for Dmod_Context_Add
 * 
 * The test checks if the function fails to add a context when there is no space left.
 */
TEST_F(DmodContextTest, AddNoSpaceLeft)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));

    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);

    // Get the module name
    ASSERT_NE(context, nullptr);
    context->Header = reinterpret_cast<Dmod_ModuleHeader_t*>(data);
    const char* moduleName = Dmod_Context_GetModuleName(context);
    EXPECT_NE(moduleName, nullptr);
    EXPECT_STREQ(moduleName, "example_app");

    for(size_t i = 0; i < DMOD_MAX_MODULES; i++)
    {
        Dmod_Contexts[i] = context;
    }

    ASSERT_FALSE(Dmod_Context_Add(context));

    Dmod_Context_Delete(context);
}

// ===============================================================
//                  Tests for Dmod_Context_Remove
// ===============================================================
/**
 * @brief Test for Dmod_Context_Remove
 * 
 * The test checks if the function removes a valid context.
 */
TEST_F(DmodContextTest, RemoveValidContext)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));

    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);

    // Get the module name
    ASSERT_NE(context, nullptr);
    context->Header = reinterpret_cast<Dmod_ModuleHeader_t*>(data);
    const char* moduleName = Dmod_Context_GetModuleName(context);
    EXPECT_NE(moduleName, nullptr);
    EXPECT_STREQ(moduleName, "example_app");

    ASSERT_TRUE(Dmod_Context_Add(context));

    // Check if the context is added to the list
    Dmod_Context_t* contextFromList = Dmod_Context_Get(moduleName);
    EXPECT_EQ(context, contextFromList);

    ASSERT_TRUE(Dmod_Context_Remove(context));

    // Check if the context is removed from the list
    contextFromList = Dmod_Context_Get(moduleName);
    EXPECT_EQ(contextFromList, nullptr);

    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Context_Remove
 * 
 * The test checks if the function removes an invalid context.
 */
TEST_F(DmodContextTest, RemoveInvalidContext)
{
    Dmod_Context_t* context = nullptr;
    ASSERT_FALSE(Dmod_Context_Remove(context));
}

/**
 * @brief Test for Dmod_Context_Remove
 * 
 * The test checks if the function removes a context that is not in the list.
 */
TEST_F(DmodContextTest, RemoveNotInList)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));

    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);

    // Get the module name
    ASSERT_NE(context, nullptr);
    context->Header = reinterpret_cast<Dmod_ModuleHeader_t*>(data);
    const char* moduleName = Dmod_Context_GetModuleName(context);
    EXPECT_NE(moduleName, nullptr);
    EXPECT_STREQ(moduleName, "example_app");

    ASSERT_TRUE(Dmod_Context_Remove(context)); // expect true, because the context is not in the list

    Dmod_Context_Delete(context);
}

// ===============================================================
//                  Tests for Dmod_Context_Get
// ===============================================================
/**
 * @brief Test for Dmod_Context_Get
 * 
 * The test checks if the function returns a valid context.
 */
TEST_F(DmodContextTest, GetValidContext)
{
    size_t fileSize = 0;
    void* data = nullptr;
    EXPECT_TRUE(LoadDmfTestFile(&data, &fileSize));

    Dmod_Context_t* context = Dmod_Context_New(data, fileSize);

    // Set the module name
    ASSERT_NE(context, nullptr);
    context->Header = reinterpret_cast<Dmod_ModuleHeader_t*>(data);
    const char* moduleName = Dmod_Context_GetModuleName(context);
    EXPECT_NE(moduleName, nullptr);
    EXPECT_STREQ(moduleName, "example_app");

    // Add the context to the list
    EXPECT_TRUE(Dmod_Context_Add(context));

    ASSERT_EQ(Dmod_Context_Get(moduleName), context);
    EXPECT_TRUE(Dmod_Context_Remove(context));
    Dmod_Context_Delete(context);
}

/**
 * @brief Test for Dmod_Context_Get
 * 
 * The test checks if the function returns NULL for an invalid context.
 */
TEST_F(DmodContextTest, GetInvalidContext)
{
    Dmod_Context_t* context = nullptr;
    ASSERT_EQ(Dmod_Context_Get("Unknown"), context);
}

/**
 * @brief Test for Dmod_Context_Get
 * 
 * The test checks if the function returns NULL for NULL module name.
 */
TEST_F(DmodContextTest, GetNullModuleName)
{
    Dmod_Context_t* context = nullptr;
    ASSERT_EQ(Dmod_Context_Get(nullptr), context);
}
