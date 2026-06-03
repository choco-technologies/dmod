#include <errno.h>
#include <gtest/gtest.h>
#include "dmod.h"

TEST(DmodSemaphoreTest, NewWaitPostDelete)
{
    void* semaphore = Dmod_Semaphore_New(1, 2);
    ASSERT_NE(semaphore, nullptr);

    EXPECT_EQ(Dmod_Semaphore_Wait(semaphore), 0);
    EXPECT_EQ(Dmod_Semaphore_Post(semaphore), 0);
    EXPECT_EQ(Dmod_Semaphore_Wait(semaphore), 0);

    Dmod_Semaphore_Delete(semaphore);
}

TEST(DmodSemaphoreTest, HandlesMaxCount)
{
    void* semaphore = Dmod_Semaphore_New(1, 1);
    ASSERT_NE(semaphore, nullptr);

    EXPECT_EQ(Dmod_Semaphore_Post(semaphore), -EOVERFLOW);
    EXPECT_EQ(Dmod_Semaphore_Wait(semaphore), 0);
    EXPECT_EQ(Dmod_Semaphore_Post(semaphore), 0);

    Dmod_Semaphore_Delete(semaphore);
}

TEST(DmodSemaphoreTest, RejectsInvalidInitialOrMaxCount)
{
    EXPECT_EQ(Dmod_Semaphore_New(1, 0), nullptr);
    EXPECT_EQ(Dmod_Semaphore_New(2, 1), nullptr);
}

TEST(DmodSemaphoreTest, HandlesNullSemaphore)
{
    EXPECT_EQ(Dmod_Semaphore_Wait(nullptr), -EINVAL);
    EXPECT_EQ(Dmod_Semaphore_Post(nullptr), -EINVAL);
    EXPECT_NO_FATAL_FAILURE(Dmod_Semaphore_Delete(nullptr));
}
