#include "Common.h"

std::string g_commandLogging = "CommandLogging";
std::string g_configFile = "/etc/osconfig/osconfig.json";
std::string g_defaultPath = "testplate.json";
std::string g_fullLogging = "FullLogging";

static std::string str_tolower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), 
                   [](unsigned char c){ return std::tolower(c); }
                  );
    return s;
}

std::map<std::string, std::pair<std::shared_ptr<ManagementModule>, std::shared_ptr<MmiSession>>> g_moduleSessionMap;

void RegisterRecipesWithGTest(TestRecipes &testRecipes)
{
    for (TestRecipe recipe : *testRecipes)
    {
        ASSERT_STRNE(recipe.m_metadata.m_modulePath.c_str(), "") << "No module path defined!";
        
        std::shared_ptr<ManagementModule> module;
        std::shared_ptr<MmiSession> session;
        auto search = g_moduleSessionMap.find(recipe.m_metadata.m_modulePath);
        if (search != g_moduleSessionMap.end())
        {
            // Module already registered, use existing session
            module = search->second.first;
            session = search->second.second;
        }
        else
        {
            module = std::make_shared<ManagementModule>(recipe.m_metadata.m_modulePath);
            session = std::make_shared<MmiSession>(module, g_defaultClient);
            g_moduleSessionMap[recipe.m_metadata.m_modulePath] = std::make_pair(module, session);

            // Load and open session only before first test
            // ASSERT_EQ(0, module->Load()) << "Failed to load module!";
            // ASSERT_EQ(0, session->Open()) << "Failed to open session!";
        }

        // See gtest.h for details on this test registration
        // https://github.com/google/googletest/blob/v1.10.x/googletest/include/gtest/gtest.h#L2438
        std::string testName(recipe.m_componentName + "." + recipe.m_objectName);
        testing::RegisterTest(
            "ModulesTest", testName.c_str(), nullptr, nullptr, __FILE__, __LINE__,
            [recipe, module, session]()->RecipeFixture *
            {
                return new RecipeInvoker(recipe, module, session);
            });
    }
}

TestRecipes LoadValuesFromConfiguration(std::stringstream& ss, std::string moduleName = "")
{
    TestRecipes testRecipes = std::make_shared<std::vector<TestRecipe>>();
    JSON_Value *root_value;
    
    char buf[PATH_MAX] = {0};
    char *res = realpath("/proc/self/exe", buf);
    if (res == nullptr)
    {
        TestLogError("Could not find the executable path");
        return testRecipes;
    }
    std::string fullPath(buf);
    fullPath = fullPath.substr(0, fullPath.find_last_of("/") + 1) + g_defaultPath;
    if (!std::filesystem::exists(fullPath.c_str()))
    {
        TestLogError("Could not find test configuration: %s\n 'jq' may be missing, install application and rebuild.", fullPath.c_str());
        return testRecipes;
    }

    std::cout << "Using test recipes: " << fullPath.c_str() << std::endl;
    root_value = json_parse_file_with_comments(fullPath.c_str());

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
        if ((moduleName.empty()) || (str_tolower(moduleName) == str_tolower(recipeMetadata.m_moduleName)))
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

    int status = 0;
    if (ss.str().length() > 0)
    {
        status = RUN_ALL_TESTS();

        std::cout << "[==========]" << std::endl;
        std::cout << "Test Summary: " << std::endl
                  << ss.str();
        std::cout << "[==========]" << std::endl;
    }
    else
    {
        std::cerr << "No tests found." << std::endl;
        status = 1;
    }

    // Unload modules, close sessions
    for (auto &moduleSessionPair : g_moduleSessionMap)
    {
        moduleSessionPair.second.second->Close();
        moduleSessionPair.second.first->Unload();
    }

    return status;
}