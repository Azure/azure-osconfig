#include "Common.h"

void RecipeInvoker::TestBody()
{
    ASSERT_STRNE(m_recipe.m_metadata.m_modulePath.c_str(), "") << "No module path defined!";
    auto module = std::make_shared<ManagementModule>(m_recipe.m_metadata.m_modulePath);
    MmiSession session(module, g_defaultClient);
    ASSERT_EQ(0, module->Load()) << "Failed to load module!";
    ASSERT_EQ(0, session.Open()) << "Failed to open session!";

    MMI_JSON_STRING payload = nullptr;
    int payloadSize = 0;

    if (m_recipe.m_desired)
    {
        EXPECT_EQ(m_recipe.m_expectedResult, session.Set(m_recipe.m_componentName.c_str(), m_recipe.m_objectName.c_str(), (MMI_JSON_STRING)m_recipe.m_payload.c_str(), m_recipe.m_payloadSizeBytes));
    }
    else
    {
        ASSERT_EQ(m_recipe.m_expectedResult, session.Get(m_recipe.m_componentName.c_str(), m_recipe.m_objectName.c_str(), &payload, &payloadSize));

        if (0 == m_recipe.m_expectedResult)
        {
            JSON_Value *root_value = json_parse_string(payload);
            ASSERT_NE(nullptr, root_value);
            JSON_Object *jsonObject = json_value_get_object(root_value);

            EXPECT_NE(0, m_recipe.m_mimObjects->size()) << "Invalid MIM JSON!";
            EXPECT_NE(0, m_recipe.m_mimObjects->at(m_recipe.m_componentName)->size()) << "No MimObjects for " << m_recipe.m_componentName << "!";
            auto map = m_recipe.m_mimObjects->at(m_recipe.m_componentName)->at(m_recipe.m_objectName);

            std::string payloadStr(payload, payloadSize);

            // Validate settings + supported values
            TestLogInfo("Validating settings and supported values for '%s'", m_recipe.m_objectName.c_str());
            if ((map.m_type.compare("array") == 0) ||
                (map.m_type.compare("map") == 0))
            {
                EXPECT_EQ(JSONArray, json_value_get_type(root_value)) << "Expecting '" << m_recipe.m_objectName << "' to contain an array" << std::endl << "JSON: " << payloadStr;
            }
            else
            {
                for (auto setting : *map.m_settings)
                {
                    if (setting.second.type.compare("string") == 0)
                    {
                        EXPECT_NE(nullptr, json_object_get_string(jsonObject, setting.second.name.c_str())) << "Expecting '" << m_recipe.m_objectName << "' to contain string setting '" << setting.second.name << "'" << std::endl << "JSON: " << payloadStr;

                        std::string value = json_object_get_string(jsonObject, setting.second.name.c_str());
                        if (setting.second.allowedValues->size() && std::find(setting.second.allowedValues->begin(), setting.second.allowedValues->end(), value) == setting.second.allowedValues->end())
                        {
                            FAIL() << "Field '" << setting.second.name << "' contains unsupported value '" << value << "'" << std::endl << "JSON: " << payloadStr;
                        }
                    }
                    else if (setting.second.type.compare("integer") == 0)
                    {
                        JSON_Value *value = json_object_get_value(jsonObject, setting.second.name.c_str());
                        EXPECT_EQ(JSONNumber, json_value_get_type(value)) << "Expecting '" << m_recipe.m_objectName << "' to contain integer setting '" << setting.second.name << "'" << std::endl << "JSON: " << payloadStr;
                    }
                    else if (setting.second.type.compare("boolean") == 0)
                    {
                        JSON_Value *value = json_object_get_value(jsonObject, setting.second.name.c_str());
                        EXPECT_EQ(JSONBoolean, json_value_get_type(value)) << "Expecting '" << m_recipe.m_objectName << "' to contain boolean setting '" << setting.second.name << "'" << std::endl << "JSON: " << payloadStr;
                    }
                    else
                    {
                        FAIL() << "Unsupported type: " << setting.second.type << std::endl << "JSON: " << payloadStr;
                    }
                }
            }

            json_value_free(root_value);

            // Validate payload size
            if (m_recipe.m_payloadSizeBytes)
            {
                EXPECT_EQ(m_recipe.m_payloadSizeBytes, payloadSize) << "Non matching recipe payload size" << std::endl;
            }

            if (m_recipe.m_payload.size())
            {
                JSON_Value *recipe_payload = json_parse_string(m_recipe.m_payload.c_str());
                JSON_Value *returned_payload = json_parse_string(payload);

                std::string recipe_payload_str(payload, payloadSize);
                EXPECT_NE(nullptr, recipe_payload) << "Failed to parse recipe payload" << std::endl << "JSON: " << m_recipe.m_payload;
                EXPECT_TRUE(json_value_equals(recipe_payload, returned_payload)) << "Non matching recipe payload" << std::endl << "Recipe   payload: " << m_recipe.m_payload << std::endl << "Returned payload: " << recipe_payload_str << std::endl;

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

    session.Close();
}

void BasicModuleTester::TestBody()
{
    EXPECT_EQ(0, m_module->Load()) << "Failed to load module!";
    m_module->Unload();
}