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
        static const char* objectName;
    };

    Sample* CppSampleTests::session;
    const char* CppSampleTests::componentName = "SampleComponent";
    const char* CppSampleTests::objectName = "SampleObject";

    TEST_F(CppSampleTests, ValidGetSet)
    {
        char jsonPayload[] = "\"C++ Sample Module\"";
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(MMI_OK, session->Set(componentName, objectName, jsonPayload, strlen(jsonPayload)));
        ASSERT_EQ(MMI_OK, session->Get(componentName, objectName, &payload, &payloadSizeBytes));

        std::string payloadString(payload, payloadSizeBytes);
        ASSERT_STREQ(jsonPayload, payloadString.c_str());
    }

    TEST_F(CppSampleTests, InvalidComponentObjectName)
    {
        std::string invalidName = "invalid";
        char jsonPayload[] = "\"C++ Sample Module\"";
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(EINVAL, session->Set(invalidName.c_str(), objectName, jsonPayload, strlen(jsonPayload)));
        ASSERT_EQ(EINVAL, session->Set(componentName, invalidName.c_str(), jsonPayload, strlen(jsonPayload)));

        ASSERT_EQ(EINVAL, session->Get(invalidName.c_str(), objectName, &payload, &payloadSizeBytes));
        ASSERT_EQ(EINVAL, session->Get(componentName, invalidName.c_str(), &payload, &payloadSizeBytes));
    }

    TEST_F(CppSampleTests, SetInvalidPayloadString)
    {
        char validPayload[] = "\"C++ Sample Module\"";
        char invalidPayload[] = "C++ Sample Module";

        ASSERT_EQ(EINVAL, session->Set(componentName, objectName, validPayload, strlen(validPayload) - 1));
        ASSERT_EQ(EINVAL, session->Set(componentName, objectName, invalidPayload, strlen(invalidPayload)));
    }

    TEST_F(CppSampleTests, InvalidSet)
    {
        char payload[] = "invalid payload";

        // Set with invalid arguments
        ASSERT_EQ(EINVAL, session->Set("invalid component", objectName, payload, sizeof(payload)));
        ASSERT_EQ(EINVAL, session->Set(componentName, "invalid component", payload, sizeof(payload)));
        ASSERT_EQ(EINVAL, session->Set(componentName, objectName, payload, sizeof(payload)));
        ASSERT_EQ(EINVAL, session->Set(componentName, objectName, payload, -1));
    }

    TEST_F(CppSampleTests, InvalidGet)
    {
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        // Get with invalid arguments
        ASSERT_EQ(EINVAL, session->Get("invalid component", objectName, &payload, &payloadSizeBytes));
        ASSERT_EQ(EINVAL, session->Get(componentName, "invalid object", &payload, &payloadSizeBytes));
    }
}