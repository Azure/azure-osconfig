// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <Common.h>

namespace Tests
{
    TEST(TestRecipeParserTests, LoadInvalidRecipe)
    {
        TestRecipes testRecipes = TestRecipeParser::ParseTestRecipe("");
        ASSERT_EQ(testRecipes->size(), 0);
    }

    TEST(TestRecipeParserTests, LoadRecipes)
    {
        TestRecipes testRecipes = TestRecipeParser::ParseTestRecipe("./recipes/test.json");
        ASSERT_EQ(testRecipes->size(), 2);
    }

    TEST(TestRecipeParserTests, AllValuesPresent)
    {
        TestRecipes testRecipes = TestRecipeParser::ParseTestRecipe("./recipes/test.json");
        ASSERT_EQ(testRecipes->size(), 2);

        EXPECT_STRCASEEQ("", testRecipes->at(1).m_payload.c_str());
        EXPECT_EQ(0, testRecipes->at(1).m_payloadSizeBytes);
    }
}