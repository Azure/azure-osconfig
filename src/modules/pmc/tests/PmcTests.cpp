// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>

#include <Mmi.h>
#include <PmcBase.h>

class PmcTests : public PmcBase
{
public:
    PmcTests(unsigned int maxPayloadSizeBytes);
    ~PmcTests() = default;

};

PmcTests::PmcTests(unsigned int maxPayloadSizeBytes)
    : PmcBase(maxPayloadSizeBytes)
{
}

namespace OSConfig::Platform::Tests
{
    constexpr const unsigned int g_maxPayloadSizeBytes = 4000;
    constexpr const char* componentName = "";
    constexpr const char* desiredObjectName = "";
    constexpr const char* reportedObjectName = "";
    static char validJsonPayload[] = "";

    TEST(PmcTests, ValidSetGet)
    {
        int status;
        PmcTests testModule(g_maxPayloadSizeBytes);
        status = testModule.Set(componentName, desiredObjectName, validJsonPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, MMI_OK);

        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;
        status = testModule.Get(componentName, reportedObjectName, &payload, &payloadSizeBytes);
        EXPECT_EQ(status, MMI_OK);
    }
}