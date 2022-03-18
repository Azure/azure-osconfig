// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include <Mmi.h>
#include <PackageManagerConfigurationBase.h>

class PackageManagerConfigurationTests : public PackageManagerConfigurationBase
{
public:
    PackageManagerConfigurationTests(unsigned int maxPayloadSizeBytes);
    ~PackageManagerConfigurationTests() = default;

};

PackageManagerConfigurationTests::PackageManagerConfigurationTests(unsigned int maxPayloadSizeBytes)
    : PackageManagerConfigurationBase(maxPayloadSizeBytes)
{
}

namespace OSConfig::Platform::Tests
{
    constexpr const unsigned int g_maxPayloadSizeBytes = 4000;
    constexpr const char* componentName = "";
    constexpr const char* desiredObjectName = "";
    constexpr const char* reportedObjectName = "";
    static char validJsonPayload[] = "";

    TEST(PackageManagerConfigurationTests, ValidSetGet)
    {
        int status;
        PackageManagerConfigurationTests testModule(g_maxPayloadSizeBytes);
        status = testModule.Set(componentName, desiredObjectName, validJsonPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, MMI_OK);

        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;
        status = testModule.Get(componentName, reportedObjectName, &payload, &payloadSizeBytes);
        EXPECT_EQ(status, MMI_OK);
    }
}