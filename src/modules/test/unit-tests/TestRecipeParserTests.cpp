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

    TEST(TestRecipeParserTests, TestNaming)
    {
        TestRecipe recipe = {
            "ComponentName",
            "ObjectName",
            true,
            "Payload",
            0,
            0,
            0,
            { "TestModule", "TestModulePath", "TestMimPath", "TestRecipesPath", nullptr },
            {}
        };
        TestRecipe recipeNoComponentNoObject = {
            "",
            "",
            true,
            "Payload",
            0,
            0,
            0,
            { "TestModule", "TestModulePath", "TestMimPath", "TestRecipesPath", nullptr },
            {}
        };
        TestRecipe recipeNoComponent = {
            "",
            "ObjectName",
            true,
            "Payload",
            0,
            0,
            0,
            { "TestModule", "TestModulePath", "TestMimPath", "TestRecipesPath", nullptr },
            {}
        };

        EXPECT_STRCASEEQ("TestModule.ComponentName.ObjectName", TestRecipeParser::GetTestName(recipe).c_str());
        EXPECT_STRCASEEQ("TestModule.<null>.<null>", TestRecipeParser::GetTestName(recipeNoComponentNoObject).c_str());
        EXPECT_STRCASEEQ("TestModule.<null>.ObjectName", TestRecipeParser::GetTestName(recipeNoComponent).c_str());
    }

    TEST(TestRecipeParserTests, RequiredFieldsMissing)
    {
        std::stringstream newBuffer;
        std::streambuf *originalBuffer = std::cerr.rdbuf();
        // Redirect cerr to our buffer
        std::cerr.rdbuf(newBuffer.rdbuf());

        const std::string expectedStr = 
            "Test recipe './recipes/testInvalid.json' [1] missing required field: ObjectName\n"
            "Test recipe './recipes/testInvalid.json' [1] missing required field: Desired\n"
            "Test recipe './recipes/testInvalid.json' [1] missing required field: ExpectedResult\n";

        TestRecipes testRecipes = TestRecipeParser::ParseTestRecipe("./recipes/testInvalid.json");
        ASSERT_EQ(testRecipes->size(), 1);
        EXPECT_STRCASEEQ(expectedStr.c_str(), newBuffer.str().c_str());

        // Redirect cerr back to original stream
        std::cerr.rdbuf(originalBuffer);
    }

    TEST(TestRecipeParserTests, GetStringWithToken)
    {
        const std::string input = "Hello ${echo World} ${echo !}";
        EXPECT_STRCASEEQ("Hello World!", TestRecipeParser::GetStringWithToken(input).c_str());
    }
}