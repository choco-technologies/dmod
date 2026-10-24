/**
 * MIT License
 * 
 * Copyright (c) 2025 patryk.kubiak90@gmail.com
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * @brief Tests for Dmod_ModuleExists function
 * @date 24 Oct 2026
 * 
 * @file tests_dmod_module_exists.cpp
 * @version 0.1
 */
#define DMOD_PRIVATE
#include <gtest/gtest.h>
#include "dmod.h"

/**
 * @brief Test for Dmod_ModuleExists with NULL module name
 */
TEST(DmodModuleExistsTest, NullModuleName)
{
    bool result = Dmod_ModuleExists(NULL);
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_ModuleExists with empty module name
 */
TEST(DmodModuleExistsTest, EmptyModuleName)
{
    bool result = Dmod_ModuleExists("");
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_ModuleExists with non-existent module
 */
TEST(DmodModuleExistsTest, NonExistentModule)
{
    bool result = Dmod_ModuleExists("definitely_nonexistent_module_12345");
    ASSERT_FALSE(result);
}

/**
 * @brief Test for Dmod_ModuleExists with existing module
 * 
 * This test verifies the function doesn't crash with a valid module name.
 * The result depends on whether dmsystem is available in the search paths.
 */
TEST(DmodModuleExistsTest, ExistingModule)
{
    char outPath[256];
    bool foundViaFind = Dmod_FindModuleFile("dmsystem", NULL, outPath, sizeof(outPath));
    
    bool result = Dmod_ModuleExists("dmsystem");
    
    // Dmod_ModuleExists should return the same result as Dmod_FindModuleFile
    ASSERT_EQ(result, foundViaFind);
}
