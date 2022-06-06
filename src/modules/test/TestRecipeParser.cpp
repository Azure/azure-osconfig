#include "Common.h"

static const std::string g_componentName = "ComponentName";
static const std::string g_objectName = "ObjectName";
static const std::string g_desired = "Desired";
static const std::string g_payload = "Payload";
static const std::string g_payloadSizeBytes = "PayloadSizeBytes";
static const std::string g_expectedResult = "ExpectedResult";
static const std::string g_waitSeconds = "WaitSeconds";

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