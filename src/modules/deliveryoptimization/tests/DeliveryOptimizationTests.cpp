// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <version.h>
#include <Mmi.h>
#include <DeliveryOptimization.h>
#include <CommonUtils.h>

using namespace std;

class DeliveryOptimizationTest : public ::testing::Test
{
    protected:
        const char* m_expectedMmiInfo = "{\"Name\": \"DeliveryOptimization\","
            "\"Description\": \"Provides functionality to observe and configure Delivery Optimization\","
            "\"Manufacturer\": \"Microsoft\","
            "\"VersionMajor\": 1,"
            "\"VersionMinor\": 0,"
            "\"VersionInfo\": \"Copper\","
            "\"Components\": [\"DeliveryOptimization\"],"
            "\"Lifetime\": 2,"
            "\"UserAccount\": 0}";
        
        const char* m_clientName = "Test";
        int m_normalMaxPayloadSizeBytes = 1024;

        void SetUp()
        {
            DeliveryOptimizationInitialize("test-config.json");
        }

        void TearDown()
        {
            DeliveryOptimizationShutdown();
        }
};

char* CopyPayloadToString(const char* payload, int payloadSizeBytes)
{
    char* output = nullptr;
    
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);
    EXPECT_NE(nullptr, output = (char*)malloc(payloadSizeBytes + 1));

    if (nullptr != output)
    {
        memcpy(output, payload, payloadSizeBytes);
        output[payloadSizeBytes] = 0;
    }

    return output;
}

TEST_F(DeliveryOptimizationTest, MmiOpen)
{
    MMI_HANDLE handle = nullptr;
    EXPECT_NE(nullptr, handle = DeliveryOptimizationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    DeliveryOptimizationMmiClose(handle);
}

TEST_F(DeliveryOptimizationTest, MmiGetInfo)
{
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_EQ(MMI_OK, DeliveryOptimizationMmiGetInfo(m_clientName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);

    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_STREQ(m_expectedMmiInfo, payloadString);
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);

    FREE_MEMORY(payloadString);
    DeliveryOptimizationMmiFree(payload);
}

// TODO: ...
