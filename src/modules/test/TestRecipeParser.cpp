#include "Common.h"

static const std::string g_componentName = "ComponentName";
static const std::string g_objectName = "ObjectName";
static const std::string g_desired = "Desired";
static const std::string g_payload = "Payload";
static const std::string g_payloadSizeBytes = "PayloadSizeBytes";
static const std::string g_expectedResult = "ExpectedResult";
static const std::string g_waitSeconds = "WaitSeconds";
static const std::string g_nullValue = "<null>";

static const std::vector<std::string> g_requiredProperties = {
    g_componentName,
    g_objectName,
    g_desired,
    g_expectedResult
};

std::string TestRecipeParser::GetStringWithToken(const std::string& str)
{
    std::string result = str;
    
    // std::regex words_regex("\\$\\{.*\\}");  // Matches ${...} tokens
    // auto words_begin = std::sregex_iterator(str.begin(), str.end(), words_regex);
    // auto words_end = std::sregex_iterator();

    // // Replace all tokens with their values - add to map
    // for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
    //     std::smatch match = *i;                                   
    //     result = match.str().substr(7, match.str().length() - 2);  // Remove ${echo } from token
    //     std::cout << result << '\n';
    // }

    std::size_t found = std::string::npos;
    std::size_t foundEnd = 0;
    while (std::string::npos != (found = str.find("${", foundEnd)))
    {
        // get the token
        foundEnd = str.find("}", found);
        if (std::string::npos != foundEnd)
        {
            std::string token = str.substr(found + 7, foundEnd - found - 7);    // Remove ${echo } from token == 7 chars
            std::cout << token.c_str() << '\n';

            // TODO: get the value

            // TODO: Add to map
        }
    }

    //

    return result;
}

TestRecipes TestRecipeParser::ParseTestRecipe(std::string path)
{
    JSON_Value *root_value;
    root_value = json_parse_file_with_comments(path.c_str());

    TestRecipes testRecipes = std::make_shared<std::vector<TestRecipe>>();

    if (json_value_get_type(root_value) != JSONArray)
    {
        json_value_free(root_value);
        return testRecipes;
    }

    JSON_Array *jsonTestRecipes = json_value_get_array(root_value);
    JSON_Object *jsonTestRecipe = nullptr;
    for (size_t i = 0; i < json_array_get_count(jsonTestRecipes); i++)
    {
        jsonTestRecipe = json_array_get_object(jsonTestRecipes, i);

        // Validate recipe contains all required fields
        bool recipeValid = true;
        for (auto &requiredProperty : g_requiredProperties)
        {
            if (!json_object_has_value(jsonTestRecipe, requiredProperty.c_str()))
            {
                std::cerr << "Test recipe '" << path.c_str() << "' [" << i << "] missing required field: " << requiredProperty << std::endl;
                recipeValid = false;
            }
        }
        if (!recipeValid)
        {
            json_value_free(root_value);
            return testRecipes;
        }

        TestRecipe recipe = {
            json_object_get_string(jsonTestRecipe, g_componentName.c_str()),
            json_object_get_string(jsonTestRecipe, g_objectName.c_str()),
            !!json_object_get_boolean(jsonTestRecipe, g_desired.c_str()),
            json_object_get_string(jsonTestRecipe, g_payload.c_str()) == nullptr ? "" : json_object_get_string(jsonTestRecipe, g_payload.c_str()),
            static_cast<size_t>(json_object_get_number(jsonTestRecipe, g_payloadSizeBytes.c_str())),
            static_cast<int>(json_object_get_number(jsonTestRecipe, g_expectedResult.c_str())),
            static_cast<int>(json_object_get_number(jsonTestRecipe, g_waitSeconds.c_str())),
            {},
            {}
        };

        testRecipes->push_back(recipe);
    }

    json_value_free(root_value);
    return testRecipes;
}

std::string TestRecipeParser::GetTestName(TestRecipe &recipe)
{
    std::string componentName = recipe.m_componentName.empty() ? g_nullValue : recipe.m_componentName;
    std::string objectName = recipe.m_objectName.empty() ? g_nullValue : recipe.m_objectName;
    
    return recipe.m_metadata.m_moduleName + "." + componentName + "." + objectName;
}