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
        static const char* desiredObjectName;
        static const char* reportedObjectName;
        static char validJsonPayload[];
        static const char *invalidJsonPayload;        
    };

    AptInstall* AptInstallTests::session;
    const char* AptInstallTests::componentName = "AptInstall";
    const char* AptInstallTests::desiredObjectName = "DesiredPackages";
    const char* AptInstallTests::reportedObjectName = "State";
    char AptInstallTests::validJsonPayload[] = "{\"StateId\":\"my-id-1\",\"Packages\":[\"cowsay\",\"sl\"]}";


    TEST_F(AptInstallTests, ValidGetSet)
    {
        char reportedJsonPayload[] = "{\"StateId\":\"my-id-1\"}";

        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(MMI_OK, session->Set(componentName, desiredObjectName, validJsonPayload, strlen(validJsonPayload)));
        ASSERT_EQ(MMI_OK, session->Get(componentName, reportedObjectName, &payload, &payloadSizeBytes));

        std::string payloadString(payload, payloadSizeBytes);
        ASSERT_STREQ(reportedJsonPayload, payloadString.c_str());
    }

    TEST_F(AptInstallTests, InvalidComponentObjectName)
    {
        std::string invalidName = "invalid";

        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(EINVAL, session->Set(invalidName.c_str(), desiredObjectName, validJsonPayload, strlen(validJsonPayload)));
        ASSERT_EQ(EINVAL, session->Set(componentName, invalidName.c_str(), validJsonPayload, strlen(validJsonPayload)));

        ASSERT_EQ(EINVAL, session->Get(invalidName.c_str(), desiredObjectName, &payload, &payloadSizeBytes));
        ASSERT_EQ(EINVAL, session->Get(componentName, invalidName.c_str(), &payload, &payloadSizeBytes));
    }

    TEST_F(AptInstallTests, SetInvalidPayloadString)
    {
        char invalidPayload[] = "C++ AptInstall Module";

        ASSERT_EQ(EINVAL, session->Set(componentName, desiredObjectName, validJsonPayload, strlen(validJsonPayload) - 1)); //test invalid length
        ASSERT_EQ(EINVAL, session->Set(componentName, desiredObjectName, invalidPayload, strlen(invalidPayload))); //test invalid payload
    }
}