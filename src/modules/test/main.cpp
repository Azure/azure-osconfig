#include "Common.h"

std::string g_defaultPath = "testplate.json";

void RegisterRecipesWithGTest(TestRecipes &testRecipes)
{
    for (TestRecipe recipe : *testRecipes)
    {
        // See gtest.h for details on this test registration
        // https://github.com/google/googletest/blob/v1.10.x/googletest/include/gtest/gtest.h#L2438
        std::string testName(recipe.m_componentName + "-" + recipe.m_objectName);
        testing::RegisterTest(
            "TestRecipes", testName.c_str(), nullptr, testName.c_str(), __FILE__, __LINE__,
            [recipe]()->RecipeFixture *
            {
                return new RecipeInvoker(recipe);
            });
    }
}

TestRecipes LoadValuesFromConfiguration()
{
    TestRecipes testRecipes = std::make_shared<std::vector<TestRecipe>>();
    JSON_Value *root_value;
    std::cout << "Using test recipes: " << g_defaultPath << std::endl;
    root_value = json_parse_file_with_comments(g_defaultPath.c_str());

    if (json_value_get_type(root_value) != JSONArray)
    {
        json_value_free(root_value);
        return testRecipes;
    }

    JSON_Array *jsonTestRecipesMetadata = json_value_get_array(root_value);
    JSON_Object *jsonTestRecipeMetadata = nullptr;
    for (size_t i = 0; i < json_array_get_count(jsonTestRecipesMetadata); i++)
    {
        jsonTestRecipeMetadata = json_array_get_object(jsonTestRecipesMetadata, i);
        TestRecipeMetadata recipeMetadata = {
            json_object_get_string(jsonTestRecipeMetadata, g_modulePath.c_str()),
            json_object_get_string(jsonTestRecipeMetadata, g_mimPath.c_str()),
            json_object_get_string(jsonTestRecipeMetadata, g_testRecipesPath.c_str()),
        };

        TestRecipes recipes = TestRecipeParser::ParseTestRecipe(recipeMetadata.m_testRecipesPath);
        for (auto &recipe : *recipes)
        {
            recipe.m_metadata = recipeMetadata;
            recipe.m_mimObjects = MimParser::ParseMim(recipeMetadata.m_mimPath);
        }

        testRecipes->insert(testRecipes->end(), recipes->begin(), recipes->end());
    }

    json_value_free(root_value);
    return testRecipes;
}

TestRecipes LoadValuesFromCLI(char* modulePath, char* mimPath, char* testRecipesPath)
{
    TestRecipes testRecipes = std::make_shared<std::vector<TestRecipe>>();

    TestRecipeMetadata recipeMetadata = {
        modulePath,
        mimPath,
        testRecipesPath,
    };

    TestRecipes recipes = TestRecipeParser::ParseTestRecipe(recipeMetadata.m_testRecipesPath);
    for (auto &recipe : *recipes)
    {
        recipe.m_metadata = recipeMetadata;
        recipe.m_mimObjects = MimParser::ParseMim(recipeMetadata.m_mimPath);
    }

    testRecipes->insert(testRecipes->end(), recipes->begin(), recipes->end());

    return testRecipes;
}

static void PrintUsage()
{
    std::cout << "Usage: modulestest [options] [ testplate.json | <module.so> <moduleMim.json> <testRecipes.json>]" << std::endl
              << "modulestest is a module tester for OSConfig." << std::endl << std::endl
              << "Options: " << std::endl
              << "  -h, --help: Print help message" << std::endl;
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    TestRecipes testRecipes;
    
    if (argc == 1)
    {
        PrintUsage();
    }

    if (argc == 2)
    {
        // Test definitions present - perform more extensive tests using Mim and test recipes
        if (strcmp(".json", std::filesystem::path(argv[1]).extension().c_str()) == 0)
        {
            testRecipes = LoadValuesFromConfiguration();
        }
        else
        {
            // Perform basic module tests - MMI only
            std::cout << "Performing basic module tests on '" << argv[1] << "'" << std::endl;
            auto module = std::make_shared<ManagementModule>(argv[1]);

            // See gtest.h for details on this test registration
            // https://github.com/google/googletest/blob/v1.10.x/googletest/include/gtest/gtest.h#L2438
            testing::RegisterTest(
                "TestRecipes", "Basic-Module-Test", nullptr, "Basic-Module-Test", __FILE__, __LINE__,
                    [module]()->RecipeFixture*
                    { 
                        return new BasicModuleTester(module); }
                    );
        }
    }
    else if (argc > 3)
    {
        // Use given module path, mim path, and test recipe path
        if (std::filesystem::exists(argv[1]) && std::filesystem::exists(argv[2]) && std::filesystem::exists(argv[3]))
        {
            std::cout << "Module : " << argv[1] << std::endl;
            std::cout << "Mmi    : " << argv[2] << std::endl;
            std::cout << "Recipe : " << argv[3] << std::endl;

            testRecipes = LoadValuesFromCLI(argv[1], argv[2], argv[3]);
        }
        else
        {
            PrintUsage();
            return 1;
        }
    }
    else
    {
        testRecipes = LoadValuesFromConfiguration();
    }
    if (testRecipes)
    {
        RegisterRecipesWithGTest(testRecipes);
    }
    return RUN_ALL_TESTS();
}