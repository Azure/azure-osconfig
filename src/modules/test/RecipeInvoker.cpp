#include "Common.h"

void RecipeInvoker::TestBody()
{
    MMI_JSON_STRING payload = nullptr;
    int payloadSize = 0;

    auto session = m_recipe.m_metadata.m_recipeModuleSessionLoader->GetSession(m_recipe.m_componentName);

    if (m_recipe.m_desired)
    {
        // If no payloadSizeBytes defined, use the size of the payload
        m_recipe.m_payloadSizeBytes = (0 == m_recipe.m_payloadSizeBytes) ? std::strlen(m_recipe.m_payload.c_str()) : m_recipe.m_payloadSizeBytes;
        EXPECT_EQ(m_recipe.m_expectedResult, session->Set(m_recipe.m_componentName.c_str(), m_recipe.m_objectName.c_str(), (MMI_JSON_STRING)m_recipe.m_payload.c_str(), m_recipe.m_payloadSizeBytes)) << "Failed JSON payload: " << m_recipe.m_payload;
    }
    else
    {
        ASSERT_EQ(m_recipe.m_expectedResult, session->Get(m_recipe.m_componentName.c_str(), m_recipe.m_objectName.c_str(), &payload, &payloadSize));

        if (0 == m_recipe.m_expectedResult)
        {
            std::string payloadString(payload, payloadSize);
            JSON_Value* root_value = json_parse_string(payloadString.c_str());
            ASSERT_NE(nullptr, root_value) << "Invalid JSON payload: " << payloadString;
            JSON_Object *jsonObject = json_value_get_object(root_value);

            EXPECT_NE(0, m_recipe.m_mimObjects->size()) << "Invalid MIM JSON!";

            // Only validate MimObjects from the calling module recipe
            if (m_recipe.m_mimObjects->end() != m_recipe.m_mimObjects->find(m_recipe.m_objectName))
            {
                EXPECT_NE(0, m_recipe.m_mimObjects->at(m_recipe.m_componentName)->size()) << "No MimObjects for " << m_recipe.m_componentName << "!";
                auto map = m_recipe.m_mimObjects->at(m_recipe.m_componentName)->at(m_recipe.m_objectName);

                // Validate settings + supported values
                TestLogInfo("Validating settings and supported values for '%s'", m_recipe.m_objectName.c_str());
                if ((map.m_type.compare("array") == 0) ||
                    (map.m_type.compare("map") == 0))
                {
                    EXPECT_EQ(JSONArray, json_value_get_type(root_value)) << "Expecting '" << m_recipe.m_objectName << "' to contain an array" << std::endl << "JSON: " << payloadString;
                }
                else
                {
                    for (auto setting : *map.m_settings)
                    {
                        if (setting.second.type.compare("string") == 0)
                        {
                            EXPECT_NE(nullptr, json_object_get_string(jsonObject, setting.second.name.c_str())) << "Expecting '" << m_recipe.m_objectName << "' to contain string setting '" << setting.second.name << "'" << std::endl << "JSON: " << payloadString;

                            std::string value = json_object_get_string(jsonObject, setting.second.name.c_str());
                            if (setting.second.allowedValues->size() && std::find(setting.second.allowedValues->begin(), setting.second.allowedValues->end(), value) == setting.second.allowedValues->end())
                            {
                                FAIL() << "Field '" << setting.second.name << "' contains unsupported value '" << value << "'" << std::endl << "JSON: " << payloadString;
                            }
                        }
                        else if (setting.second.type.compare("integer") == 0)
                        {
                            JSON_Value *value = json_object_get_value(jsonObject, setting.second.name.c_str());
                            EXPECT_EQ(JSONNumber, json_value_get_type(value)) << "Expecting '" << m_recipe.m_objectName << "' to contain integer setting '" << setting.second.name << "'" << std::endl << "JSON: " << payloadString;
                        }
                        else if (setting.second.type.compare("boolean") == 0)
                        {
                            JSON_Value *value = json_object_get_value(jsonObject, setting.second.name.c_str());
                            EXPECT_EQ(JSONBoolean, json_value_get_type(value)) << "Expecting '" << m_recipe.m_objectName << "' to contain boolean setting '" << setting.second.name << "'" << std::endl << "JSON: " << payloadString;
                        }
                    }
                }

                json_value_free(root_value);
            }

            // Validate payload size
            if (m_recipe.m_payloadSizeBytes)
            {
                EXPECT_EQ(m_recipe.m_payloadSizeBytes, payloadSize) << "Non matching recipe payload size" << std::endl;
            }

            if (m_recipe.m_payload.size())
            {
                JSON_Value *recipe_payload = json_parse_string(m_recipe.m_payload.c_str());
                JSON_Value *returned_payload = json_parse_string(payloadString.c_str());

                EXPECT_NE(nullptr, recipe_payload) << "Failed to parse recipe payload" << std::endl << "JSON: " << m_recipe.m_payload.c_str() << std::endl;
                EXPECT_NE(nullptr, returned_payload) << "Failed to parse returned payload" << std::endl << "JSON: " << payloadString.c_str() << std::endl;
                EXPECT_EQ(json_value_get_type(returned_payload), json_value_get_type(recipe_payload)) << "Non matching payload types. Recipe payload: " << json_value_get_type(recipe_payload) << ", returned payload: " << json_value_get_type(returned_payload) << std::endl;
                EXPECT_EQ(json_value_equals(recipe_payload, returned_payload), 1) << "Non matching recipe payload" << std::endl << "Recipe   payload: " << m_recipe.m_payload << std::endl << "Returned payload: " << payloadString.c_str() << std::endl;

                json_value_free(recipe_payload);
                json_value_free(returned_payload);
            }
        }
    }

    if (m_recipe.m_waitSeconds)
    {
        std::cout << "Waiting for " << m_recipe.m_waitSeconds << " seconds" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(m_recipe.m_waitSeconds));
    }
}

void BasicModuleTester::TestBody()
{
    EXPECT_EQ(0, m_module->Load()) << "Failed to load module!";
    m_module->Unload();
}