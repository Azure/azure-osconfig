// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <vector>

#include <Mmi.h>
#include <Sample.h>

namespace OSConfig::Platform::Tests
{
    class CppSampleTests : public testing::Test
    {
    protected:
        void SetUp() override
        {
            session = new Sample(0);
        }

        void TearDown() override
        {
            delete session;
        }

        static Sample* session;

        static const char* componentName;

        static const char* desiredStringObjectName;
        static const char* reportedStringObjectName;
        static const char* desiredIntegerObjectName;
        static const char* reportedIntegerObjectName;
        static const char* desiredBooleanObjectName;
        static const char* reportedBooleanObjectName;
        static const char* desiredObjectName;
        static const char* reportedObjectName;
        static const char* desiredArrayObjectName;
        static const char* reportedArrayObjectName;
    };

    Sample* CppSampleTests::session;

    const char* CppSampleTests::componentName = "SampleComponent";

    const char* CppSampleTests::desiredStringObjectName = "desiredStringObject";
    const char* CppSampleTests::reportedStringObjectName = "reportedStringObject";
    const char* CppSampleTests::desiredIntegerObjectName = "desiredIntegerObject";
    const char* CppSampleTests::reportedIntegerObjectName = "reportedIntegerObject";
    const char* CppSampleTests::desiredBooleanObjectName = "desiredBooleanObject";
    const char* CppSampleTests::reportedBooleanObjectName = "reportedBooleanObject";
    const char* CppSampleTests::desiredObjectName = "desiredObject";
    const char* CppSampleTests::reportedObjectName = "reportedObject";
    const char* CppSampleTests::desiredArrayObjectName = "desiredArrayObject";
    const char* CppSampleTests::reportedArrayObjectName = "reportedArrayObject";

    TEST_F(CppSampleTests, GetSetStringObject)
    {
        char jsonPayload[] = "\"C++ Sample Module\"";
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(MMI_OK, session->Set(componentName, desiredStringObjectName, jsonPayload, strlen(jsonPayload)));
        ASSERT_EQ(MMI_OK, session->Get(componentName, reportedStringObjectName, &payload, &payloadSizeBytes));

        std::string payloadString(payload, payloadSizeBytes);
        ASSERT_STREQ(jsonPayload, payloadString.c_str());
    }

    TEST_F(CppSampleTests, GetSetIntegerObject)
    {
        char jsonPayload[] = "12345";
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(MMI_OK, session->Set(componentName, desiredIntegerObjectName, jsonPayload, strlen(jsonPayload)));
        ASSERT_EQ(MMI_OK, session->Get(componentName, reportedIntegerObjectName, &payload, &payloadSizeBytes));

        std::string payloadString(payload, payloadSizeBytes);
        ASSERT_STREQ(jsonPayload, payloadString.c_str());
    }

    TEST_F(CppSampleTests, GetSetBooleanObject)
    {
        char jsonPayload[] = "true";
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(MMI_OK, session->Set(componentName, desiredBooleanObjectName, jsonPayload, strlen(jsonPayload)));
        ASSERT_EQ(MMI_OK, session->Get(componentName, reportedBooleanObjectName, &payload, &payloadSizeBytes));

        std::string payloadString(payload, payloadSizeBytes);
        ASSERT_STREQ(jsonPayload, payloadString.c_str());
    }

    TEST_F(CppSampleTests, GetSetObject)
    {
        char jsonPayload[] = "{"
            "\"stringSetting\":\"C++ Sample Module\","
            "\"booleanSetting\":true,"
            "\"integerSetting\":12345,"
            "\"integerEnumerationSetting\":0,"
            "\"stringEnumerationSetting\":\"value1\","
            "\"stringsArraySetting\":[\"C++ Sample Module 1\",\"C++ Sample Module 2\"],"
            "\"integerArraySetting\":[1,2,3,4,5],"
            "\"stringMapSetting\":{"
                "\"key1\":\"C++ Sample Module 1\","
                "\"key2\":\"C++ Sample Module 2\""
            "},"
            "\"integerMapSetting\":{"
                "\"key1\":1,"
                "\"key2\":2"
            "}}";

        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(MMI_OK, session->Set(componentName, desiredObjectName, jsonPayload, strlen(jsonPayload)));
        ASSERT_EQ(MMI_OK, session->Get(componentName, reportedObjectName, &payload, &payloadSizeBytes));

        std::string payloadString(payload, payloadSizeBytes);
        ASSERT_STREQ(jsonPayload, payloadString.c_str());
    }

    TEST_F(CppSampleTests, GetSetObjectMapNullValues)
    {
        char jsonPayload[] = "{"
            "\"stringSetting\":\"C++ Sample Module\","
            "\"booleanSetting\":true,"
            "\"integerSetting\":12345,"
            "\"integerEnumerationSetting\":0,"
            "\"stringEnumerationSetting\":\"value1\","
            "\"stringsArraySetting\":[\"C++ Sample Module 1\",\"C++ Sample Module 2\"],"
            "\"integerArraySetting\":[1,2,3,4,5],"
            "\"stringMapSetting\":{"
                "\"key1\":\"C++ Sample Module 1\","
                "\"key2\":\"C++ Sample Module 2\""
            "},"
            "\"integerMapSetting\":{"
                "\"key1\":1,"
                "\"key2\":2"
            "}}";

        char jsonPayloadWithNullMapValues[] = "{"
            "\"stringSetting\":\"C++ Sample Module\","
            "\"booleanSetting\":true,"
            "\"integerSetting\":12345,"
            "\"integerEnumerationSetting\":0,"
            "\"stringEnumerationSetting\":\"value1\","
            "\"stringsArraySetting\":[\"C++ Sample Module 1\",\"C++ Sample Module 2\"],"
            "\"integerArraySetting\":[1,2,3,4,5],"
            "\"stringMapSetting\":{"
                "\"key1\":\"C++ Sample Module 1\","
                "\"key2\":null"
            "},"
            "\"integerMapSetting\":{"
                "\"key1\":1,"
                "\"key2\":null"
            "}}";

        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(MMI_OK, session->Set(componentName, desiredObjectName, jsonPayload, strlen(jsonPayload)));
        ASSERT_EQ(MMI_OK, session->Get(componentName, reportedObjectName, &payload, &payloadSizeBytes));

        std::string payloadString(payload, payloadSizeBytes);
        ASSERT_STREQ(jsonPayload, payloadString.c_str());

        ASSERT_EQ(MMI_OK, session->Set(componentName, desiredObjectName, jsonPayloadWithNullMapValues, strlen(jsonPayloadWithNullMapValues)));
        ASSERT_EQ(MMI_OK, session->Get(componentName, reportedObjectName, &payload, &payloadSizeBytes));

        std::string payloadStringWithNullMapValues(payload, payloadSizeBytes);
        ASSERT_STREQ(jsonPayloadWithNullMapValues, payloadStringWithNullMapValues.c_str());
    }

    TEST_F(CppSampleTests, GetSetArrayObject)
    {
        char jsonPayload[] = "["
            "{"
                "\"stringSetting\":\"C++ Sample Module\","
                "\"booleanSetting\":true,"
                "\"integerSetting\":12345,"
                "\"integerEnumerationSetting\":0,"
                "\"stringEnumerationSetting\":\"value1\","
                "\"stringsArraySetting\":[\"C++ Sample Module 1\",\"C++ Sample Module 2\"],"
                "\"integerArraySetting\":[1,2,3,4,5],"
                "\"stringMapSetting\":{"
                    "\"key1\":\"C++ Sample Module 1\","
                    "\"key2\":\"C++ Sample Module 2\""
                "},"
                "\"integerMapSetting\":{"
                    "\"key1\":1,"
                    "\"key2\":2"
                "}"
            "}]";

        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(MMI_OK, session->Set(componentName, desiredArrayObjectName, jsonPayload, strlen(jsonPayload)));
        ASSERT_EQ(MMI_OK, session->Get(componentName, reportedArrayObjectName, &payload, &payloadSizeBytes));

        std::string payloadString(payload, payloadSizeBytes);
        ASSERT_STREQ(jsonPayload, payloadString.c_str());
    }

    TEST_F(CppSampleTests, InvalidComponentObjectName)
    {
        std::string invalidName = "invalid";
        char jsonPayload[] = "\"C++ Sample Module\"";
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(EINVAL, session->Set(invalidName.c_str(), desiredStringObjectName, jsonPayload, strlen(jsonPayload)));
        ASSERT_EQ(EINVAL, session->Set(componentName, invalidName.c_str(), jsonPayload, strlen(jsonPayload)));

        ASSERT_EQ(EINVAL, session->Get(invalidName.c_str(), reportedStringObjectName, &payload, &payloadSizeBytes));
        ASSERT_EQ(EINVAL, session->Get(componentName, invalidName.c_str(), &payload, &payloadSizeBytes));
    }

    TEST_F(CppSampleTests, SetInvalidPayloadString)
    {
        char validPayload[] = "\"C++ Sample Module\"";
        char invalidPayload[] = "C++ Sample Module";

        ASSERT_EQ(EINVAL, session->Set(componentName, desiredStringObjectName, validPayload, strlen(validPayload) - 1));
        ASSERT_EQ(EINVAL, session->Set(componentName, desiredStringObjectName, invalidPayload, strlen(invalidPayload)));
    }

    TEST_F(CppSampleTests, InvalidSet)
    {
        char payload[] = "invalid payload";

        // Set with invalid arguments
        ASSERT_EQ(EINVAL, session->Set("invalid component", desiredStringObjectName, payload, sizeof(payload)));
        ASSERT_EQ(EINVAL, session->Set(componentName, "invalid component", payload, sizeof(payload)));
        ASSERT_EQ(EINVAL, session->Set(componentName, desiredStringObjectName, payload, sizeof(payload)));
        ASSERT_EQ(EINVAL, session->Set(componentName, desiredStringObjectName, payload, -1));
    }

    TEST_F(CppSampleTests, InvalidGet)
    {
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        // Get with invalid arguments
        ASSERT_EQ(EINVAL, session->Get("invalid component", reportedStringObjectName, &payload, &payloadSizeBytes));
        ASSERT_EQ(EINVAL, session->Get(componentName, "invalid object", &payload, &payloadSizeBytes));
    }
}
