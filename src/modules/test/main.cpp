#include "Common.h"

std::string g_commandLogging = "CommandLogging";
std::string g_configFile = "/etc/osconfig/osconfig.json";
std::string g_defaultPath = "testplate.json";
std::string g_fullLogging = "FullLogging";

void RegisterRecipesWithGTest(TestRecipes &testRecipes)
{
    for (TestRecipe recipe : *testRecipes)
    {
        // See gtest.h for details on this test registration
        // https://github.com/google/googletest/blob/v1.10.x/googletest/include/gtest/gtest.h#L2438
        std::string testName(recipe.m_componentName + "." + recipe.m_objectName);
        testing::RegisterTest(
            "TestRecipes", testName.c_str(), nullptr, nullptr, __FILE__, __LINE__,
            [recipe]()->RecipeFixture *
            {
                return new RecipeInvoker(recipe);
            });
    }
}

TestRecipes LoadValuesFromConfiguration(std::stringstream& ss, std::string moduleName = "")
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
            json_object_get_string(jsonTestRecipeMetadata, g_moduleName.c_str()),
            json_object_get_string(jsonTestRecipeMetadata, g_modulePath.c_str()),
            json_object_get_string(jsonTestRecipeMetadata, g_mimPath.c_str()),
            json_object_get_string(jsonTestRecipeMetadata, g_testRecipesPath.c_str()),
        };

        // Add recipe if no specific module specified or if the module name matches
        if ((moduleName.empty()) || (moduleName == recipeMetadata.m_moduleName))
        {
            std::cout << "Adding test recipes for " << recipeMetadata.m_moduleName << std::endl;
            ss << "Module : " << recipeMetadata.m_modulePath << std::endl;
            ss << "Mmi    : " << recipeMetadata.m_mimPath << std::endl;
            ss << "Recipe : " << recipeMetadata.m_testRecipesPath << std::endl;

            TestRecipes recipes = TestRecipeParser::ParseTestRecipe(recipeMetadata.m_testRecipesPath);
            for (auto &recipe : *recipes)
            {
                recipe.m_metadata = recipeMetadata;
                recipe.m_mimObjects = MimParser::ParseMim(recipeMetadata.m_mimPath);
            }

            testRecipes->insert(testRecipes->end(), recipes->begin(), recipes->end());
        }
    }

    json_value_free(root_value);
    return testRecipes;
}

TestRecipes LoadValuesFromCLI(char* modulePath, char* mimPath, char* testRecipesPath)
{
    TestRecipes testRecipes = std::make_shared<std::vector<TestRecipe>>();

    TestRecipeMetadata recipeMetadata = {
        "",
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
    std::cout << "Usage: modulestest [options] [ testplate.json | <Module Name> | <module.so> <moduleMim.json> <testRecipes.json>]" << std::endl
              << "modulestest is a module tester for OSConfig." << std::endl << std::endl
              << "Options: " << std::endl
              << "  -h, --help: Print help message" << std::endl;
}

static bool IsLoggingEnabledInJsonConfig(const char* jsonString, const char* loggingSetting)
{
    bool result = false;
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;

    if (NULL != jsonString)
    {
        if (NULL != (rootValue = json_parse_string(jsonString)))
        {
            if (NULL != (rootObject = json_value_get_object(rootValue)))
            {
                result = (0 == (int)json_object_get_number(rootObject, loggingSetting)) ? false : true;
            }
            json_value_free(rootValue);
        }
    }

    return result;
}

bool IsCommandLoggingEnabledInJsonConfig(const char* jsonString)
{
    return IsLoggingEnabledInJsonConfig(jsonString, g_commandLogging.c_str());
}

bool IsFullLoggingEnabledInJsonConfig(const char* jsonString)
{
    return IsLoggingEnabledInJsonConfig(jsonString, g_fullLogging.c_str());
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    TestRecipes testRecipes;

    std::stringstream ss;
    
    if (argc == 1)
    {
        PrintUsage();
    }

    char* jsonConfiguration = LoadStringFromFile(g_configFile.c_str(), false, NULL);
    if (NULL != jsonConfiguration)
    {
        SetCommandLogging(IsCommandLoggingEnabledInJsonConfig(jsonConfiguration));
        SetFullLogging(IsFullLoggingEnabledInJsonConfig(jsonConfiguration));
        FREE_MEMORY(jsonConfiguration);
    }

    if (argc == 2)
    {
        // Test definitions present - perform more extensive tests using Mim and test recipes
        if (std::filesystem::exists(argv[1]))
        {
            if (strcmp(".json", std::filesystem::path(argv[1]).extension().c_str()) == 0)
            {
                testRecipes = LoadValuesFromConfiguration(ss);
            }
            else
            {
                // Perform basic module tests - MMI only
                std::cout << "Performing basic module tests on '" << argv[1] << "'" << std::endl;
                auto module = std::make_shared<ManagementModule>(argv[1]);

                // See gtest.h for details on this test registration
                // https://github.com/google/googletest/blob/v1.10.x/googletest/include/gtest/gtest.h#L2438
                testing::RegisterTest(
                    "ModulesTest", "Basic-Module-Test", nullptr, nullptr, __FILE__, __LINE__,
                        [module]()->RecipeFixture*
                        {
                            return new BasicModuleTester(module); }
                        );
            }

        }
        else
        {
            // TODO: TGest specific modulename -- if found!
            // std::cout << "Testing Module ---> " << argv[1] << std::endl;
            testRecipes = LoadValuesFromConfiguration(ss, argv[1]);
        }
    }
    else if (argc > 3)
    {
        // Use given module path, mim path, and test recipe path
        if (std::filesystem::exists(argv[1]) && std::filesystem::exists(argv[2]) && std::filesystem::exists(argv[3]))
        {
            ss << "Module : " << argv[1] << std::endl;
            ss << "Mmi    : " << argv[2] << std::endl;
            ss << "Recipe : " << argv[3] << std::endl << std::endl;

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
        testRecipes = LoadValuesFromConfiguration(ss);
    }
    if (testRecipes)
    {
        RegisterRecipesWithGTest(testRecipes);
    }

    int status = RUN_ALL_TESTS();

    std::cout << "[==========]"   << std::endl;
    std::cout << "Test Summary: " << std::endl << ss.str();
    std::cout << "[==========]"   << std::endl;

    return status;
}