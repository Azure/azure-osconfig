// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <vector>

#include <Mmi.h>
#include <AptInstall.h>

namespace OSConfig::Platform::Tests
{
    class AptInstallTests : public testing::Test
    {
    protected:
        void SetUp() override
        {
            session = new AptInstall(0);
        }

        void TearDown() override
        {
            delete session;
        }

        static AptInstall* session;

        static const char* componentName;
        static const char* objectName;
    };

    AptInstall* AptInstallTests::session;
    const char* AptInstallTests::componentName = "AptInstallComponent";
    const char* AptInstallTests::objectName = "AptInstallObject";

    TEST_F(AptInstallTests, ValidGetSet)
    {
        char jsonPayload[] = "\"C++ AptInstall Module\"";
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(MMI_OK, session->Set(componentName, objectName, jsonPayload, strlen(jsonPayload)));
        ASSERT_EQ(MMI_OK, session->Get(componentName, objectName, &payload, &payloadSizeBytes));

        std::string payloadString(payload, payloadSizeBytes);
        ASSERT_STREQ(jsonPayload, payloadString.c_str());
    }

    TEST_F(AptInstallTests, InvalidComponentObjectName)
    {
        std::string invalidName = "invalid";
        char jsonPayload[] = "\"C++ AptInstall Module\"";
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(EINVAL, session->Set(invalidName.c_str(), objectName, jsonPayload, strlen(jsonPayload)));
        ASSERT_EQ(EINVAL, session->Set(componentName, invalidName.c_str(), jsonPayload, strlen(jsonPayload)));

        ASSERT_EQ(EINVAL, session->Get(invalidName.c_str(), objectName, &payload, &payloadSizeBytes));
        ASSERT_EQ(EINVAL, session->Get(componentName, invalidName.c_str(), &payload, &payloadSizeBytes));
    }

    TEST_F(AptInstallTests, SetInvalidPayloadString)
    {
        char validPayload[] = "\"C++ AptInstall Module\"";
        char invalidPayload[] = "C++ AptInstall Module";

        ASSERT_EQ(EINVAL, session->Set(componentName, objectName, validPayload, strlen(validPayload) - 1));
        ASSERT_EQ(EINVAL, session->Set(componentName, objectName, invalidPayload, strlen(invalidPayload)));
    }
}