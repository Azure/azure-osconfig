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
            "\"Description\": \"Provides functionality to observe and configure Delivery Optimization (DO)\","
            "\"Manufacturer\": \"Microsoft\","
            "\"VersionMajor\": 1,"
            "\"VersionMinor\": 0,"
            "\"VersionInfo\": \"Copper\","
            "\"Components\": [\"DeliveryOptimization\"],"
            "\"Lifetime\": 2,"
            "\"UserAccount\": 0}";

        const char* m_deliveryOptimizationComponentName = "DeliveryOptimization";
        const char* m_reportedCacheHostObjectName = "cacheHost";
        const char* m_reportedCacheHostSourceObjectName = "cacheHostSource";
        const char* m_reportedCacheHostFallbackObjectName = "cacheHostFallback";
        const char* m_reportedPercentageDownloadThrottleObjectName = "percentageDownloadThrottle";
        const char* m_desiredDeliveryOptimizationPoliciesObjectName = "desiredDeliveryOptimizationPolicies";

        const char* m_deliveryOptimizationConfigFile = "test-config.json";

        const char* m_clientName = "Test";

        int m_normalMaxPayloadSizeBytes = 1024;
        int m_truncatedMaxPayloadSizeBytes = 1;

        void SetUp()
        {
            DeliveryOptimizationInitialize(m_deliveryOptimizationConfigFile);
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
}

TEST_F(DeliveryOptimizationTest, MmiGetValidConfigFile)
{
    const char* testData = "{\"DOCacheHost\":\"10.0.0.0:80,host.com:8080\",\"DOCacheHostSource\":1,\"DOCacheHostFallback\":2,\"DOPercentageDownloadThrottle\":3}";
    EXPECT_TRUE(SavePayloadToFile(m_deliveryOptimizationConfigFile, testData, strlen(testData), nullptr));

    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = DeliveryOptimizationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    EXPECT_EQ(MMI_OK, DeliveryOptimizationMmiGet(handle, m_deliveryOptimizationComponentName, m_reportedCacheHostObjectName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);
    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
    EXPECT_STREQ(payloadString, "\"10.0.0.0:80,host.com:8080\"");
    FREE_MEMORY(payloadString);
    DeliveryOptimizationMmiFree(payload);

    payload = nullptr;
    payloadSizeBytes = 0;

    EXPECT_EQ(MMI_OK, DeliveryOptimizationMmiGet(handle, m_deliveryOptimizationComponentName, m_reportedCacheHostSourceObjectName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);
    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
    EXPECT_STREQ(payloadString, "1");
    FREE_MEMORY(payloadString);
    DeliveryOptimizationMmiFree(payload);

    payload = nullptr;
    payloadSizeBytes = 0;

    EXPECT_EQ(MMI_OK, DeliveryOptimizationMmiGet(handle, m_deliveryOptimizationComponentName, m_reportedCacheHostFallbackObjectName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);
    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
    EXPECT_STREQ(payloadString, "2");
    FREE_MEMORY(payloadString);
    DeliveryOptimizationMmiFree(payload);

    payload = nullptr;
    payloadSizeBytes = 0;

    EXPECT_EQ(MMI_OK, DeliveryOptimizationMmiGet(handle, m_deliveryOptimizationComponentName, m_reportedPercentageDownloadThrottleObjectName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);
    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
    EXPECT_STREQ(payloadString, "3");
    FREE_MEMORY(payloadString);
    DeliveryOptimizationMmiFree(payload);

    DeliveryOptimizationMmiClose(handle);
    EXPECT_EQ(0, remove(m_deliveryOptimizationConfigFile));
}

TEST_F(DeliveryOptimizationTest, MmiGetEmptyConfigFile)
{
    const char* testData = "{}";
    EXPECT_TRUE(SavePayloadToFile(m_deliveryOptimizationConfigFile, testData, strlen(testData), nullptr));

    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = DeliveryOptimizationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    EXPECT_EQ(MMI_OK, DeliveryOptimizationMmiGet(handle, m_deliveryOptimizationComponentName, m_reportedCacheHostObjectName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);
    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
    EXPECT_STREQ(payloadString, "\"\"");
    FREE_MEMORY(payloadString);
    DeliveryOptimizationMmiFree(payload);

    payload = nullptr;
    payloadSizeBytes = 0;

    EXPECT_EQ(MMI_OK, DeliveryOptimizationMmiGet(handle, m_deliveryOptimizationComponentName, m_reportedCacheHostSourceObjectName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);
    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
    EXPECT_STREQ(payloadString, "0");
    FREE_MEMORY(payloadString);
    DeliveryOptimizationMmiFree(payload);

    payload = nullptr;
    payloadSizeBytes = 0;

    EXPECT_EQ(MMI_OK, DeliveryOptimizationMmiGet(handle, m_deliveryOptimizationComponentName, m_reportedCacheHostFallbackObjectName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);
    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
    EXPECT_STREQ(payloadString, "0");
    FREE_MEMORY(payloadString);
    DeliveryOptimizationMmiFree(payload);

    payload = nullptr;
    payloadSizeBytes = 0;

    EXPECT_EQ(MMI_OK, DeliveryOptimizationMmiGet(handle, m_deliveryOptimizationComponentName, m_reportedPercentageDownloadThrottleObjectName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);
    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
    EXPECT_STREQ(payloadString, "0");
    FREE_MEMORY(payloadString);
    DeliveryOptimizationMmiFree(payload);

    DeliveryOptimizationMmiClose(handle);
    EXPECT_EQ(0, remove(m_deliveryOptimizationConfigFile));
}

TEST_F(DeliveryOptimizationTest, MmiGetTruncatedPayload)
{
    const char* testData = "{\"DOCacheHost\":\"10.0.0.0:80,host.com:8080\",\"DOCacheHostSource\":1,\"DOCacheHostFallback\":2,\"DOPercentageDownloadThrottle\":3}";
    EXPECT_TRUE(SavePayloadToFile(m_deliveryOptimizationConfigFile, testData, strlen(testData), nullptr));

    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = DeliveryOptimizationMmiOpen(m_clientName, m_truncatedMaxPayloadSizeBytes));

    EXPECT_EQ(MMI_OK, DeliveryOptimizationMmiGet(handle, m_deliveryOptimizationComponentName, m_reportedCacheHostObjectName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);
    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
    EXPECT_EQ(m_truncatedMaxPayloadSizeBytes, payloadSizeBytes);
    FREE_MEMORY(payloadString);
    DeliveryOptimizationMmiFree(payload);

    payload = nullptr;
    payloadSizeBytes = 0;

    EXPECT_EQ(MMI_OK, DeliveryOptimizationMmiGet(handle, m_deliveryOptimizationComponentName, m_reportedCacheHostSourceObjectName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);
    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
    EXPECT_EQ(m_truncatedMaxPayloadSizeBytes, payloadSizeBytes);
    FREE_MEMORY(payloadString);
    DeliveryOptimizationMmiFree(payload);

    payload = nullptr;
    payloadSizeBytes = 0;

    EXPECT_EQ(MMI_OK, DeliveryOptimizationMmiGet(handle, m_deliveryOptimizationComponentName, m_reportedCacheHostFallbackObjectName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);
    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
    EXPECT_EQ(m_truncatedMaxPayloadSizeBytes, payloadSizeBytes);
    FREE_MEMORY(payloadString);
    DeliveryOptimizationMmiFree(payload);

    payload = nullptr;
    payloadSizeBytes = 0;

    EXPECT_EQ(MMI_OK, DeliveryOptimizationMmiGet(handle, m_deliveryOptimizationComponentName, m_reportedPercentageDownloadThrottleObjectName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);
    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
    EXPECT_EQ(m_truncatedMaxPayloadSizeBytes, payloadSizeBytes);
    FREE_MEMORY(payloadString);
    DeliveryOptimizationMmiFree(payload);

    DeliveryOptimizationMmiClose(handle);
    EXPECT_EQ(0, remove(m_deliveryOptimizationConfigFile));
}

TEST_F(DeliveryOptimizationTest, MmiGetInvalidComponent)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = DeliveryOptimizationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    EXPECT_EQ(EINVAL, DeliveryOptimizationMmiGet(handle, "Test123", m_reportedCacheHostObjectName, &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);

    DeliveryOptimizationMmiClose(handle);
}

TEST_F(DeliveryOptimizationTest, MmiGetInvalidObject)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = DeliveryOptimizationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    EXPECT_EQ(EINVAL, DeliveryOptimizationMmiGet(handle, m_deliveryOptimizationComponentName, "Test123", &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);

    DeliveryOptimizationMmiClose(handle);
}

TEST_F(DeliveryOptimizationTest, MmiGetOutsideSession)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_EQ(EINVAL, DeliveryOptimizationMmiGet(handle, m_deliveryOptimizationComponentName, m_reportedCacheHostObjectName, &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);

    EXPECT_NE(nullptr, handle = DeliveryOptimizationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    DeliveryOptimizationMmiClose(handle);

    EXPECT_EQ(EINVAL, DeliveryOptimizationMmiGet(handle, m_deliveryOptimizationComponentName, m_reportedCacheHostObjectName, &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);
}

TEST_F(DeliveryOptimizationTest, MmiSetAllSettings)
{
    const char* expectedFileContent =
    "{\n"
    "    \"DOCacheHost\": \"10.0.0.0:80,host.com:8080\",\n"
    "    \"DOCacheHostSource\": 1,\n"
    "    \"DOCacheHostFallback\": 2,\n"
    "    \"DOPercentageDownloadThrottle\": 3\n"
    "}";
    const int expectedFileContentSizeBytes = strlen(expectedFileContent);
    char *actualFileContent = nullptr;

    MMI_HANDLE handle = NULL;
    const char *payload = "{\"cacheHost\":\"10.0.0.0:80,host.com:8080\",\"cacheHostSource\":1,\"cacheHostFallback\":2,\"percentageDownloadThrottle\":3}";
    const int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = DeliveryOptimizationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    EXPECT_EQ(MMI_OK, DeliveryOptimizationMmiSet(handle, m_deliveryOptimizationComponentName, m_desiredDeliveryOptimizationPoliciesObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));
    EXPECT_STREQ(expectedFileContent, actualFileContent = LoadStringFromFile(m_deliveryOptimizationConfigFile, false, nullptr));
    EXPECT_EQ(expectedFileContentSizeBytes, strlen(actualFileContent));
    FREE_MEMORY(actualFileContent);
    EXPECT_EQ(0, remove(m_deliveryOptimizationConfigFile));

    DeliveryOptimizationMmiClose(handle);
}

TEST_F(DeliveryOptimizationTest, MmiSetOneSetting)
{
    const char* expectedFileContent =
    "{\n"
    "    \"DOCacheHost\": \"10.0.0.0:80,host.com:8080\"\n"
    "}";
    const int expectedFileContentSizeBytes = strlen(expectedFileContent);
    char *actualFileContent = nullptr;

    MMI_HANDLE handle = NULL;
    const char *payload = "{\"cacheHost\":\"10.0.0.0:80,host.com:8080\"}";
    const int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = DeliveryOptimizationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    EXPECT_EQ(MMI_OK, DeliveryOptimizationMmiSet(handle, m_deliveryOptimizationComponentName, m_desiredDeliveryOptimizationPoliciesObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));
    EXPECT_STREQ(expectedFileContent, actualFileContent = LoadStringFromFile(m_deliveryOptimizationConfigFile, false, nullptr));
    EXPECT_EQ(expectedFileContentSizeBytes, strlen(actualFileContent));
    FREE_MEMORY(actualFileContent);
    EXPECT_EQ(0, remove(m_deliveryOptimizationConfigFile));

    DeliveryOptimizationMmiClose(handle);
}

TEST_F(DeliveryOptimizationTest, MmiSetInvalidComponent)
{
    MMI_HANDLE handle = NULL;
    const char *payload = "{\"cacheHost\":\"10.0.0.0:80,host.com:8080\"}";
    const int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = DeliveryOptimizationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    EXPECT_EQ(EINVAL, DeliveryOptimizationMmiSet(handle, "Test123", m_desiredDeliveryOptimizationPoliciesObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));

    DeliveryOptimizationMmiClose(handle);
}

TEST_F(DeliveryOptimizationTest, MmiSetInvalidObject)
{
    MMI_HANDLE handle = NULL;
    const char *payload = "{\"cacheHost\":\"10.0.0.0:80,host.com:8080\"}";
    const int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = DeliveryOptimizationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    EXPECT_EQ(EINVAL, DeliveryOptimizationMmiSet(handle, m_deliveryOptimizationComponentName, "Test123", (MMI_JSON_STRING)payload, payloadSizeBytes));

    DeliveryOptimizationMmiClose(handle);
}

TEST_F(DeliveryOptimizationTest, MmiSetInvalidSetting)
{
    const char* expectedFileContent = "{}";
    const int expectedFileContentSizeBytes = strlen(expectedFileContent);
    char *actualFileContent = nullptr;

    MMI_HANDLE handle = NULL;
    const char *payload = "{\"testSetting\":\"testValue\"}";
    const int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = DeliveryOptimizationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    EXPECT_EQ(MMI_OK, DeliveryOptimizationMmiSet(handle, m_deliveryOptimizationComponentName, m_desiredDeliveryOptimizationPoliciesObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));
    EXPECT_STREQ(expectedFileContent, actualFileContent = LoadStringFromFile(m_deliveryOptimizationConfigFile, false, nullptr));
    EXPECT_EQ(expectedFileContentSizeBytes, strlen(actualFileContent));
    FREE_MEMORY(actualFileContent);
    EXPECT_EQ(0, remove(m_deliveryOptimizationConfigFile));

    DeliveryOptimizationMmiClose(handle);
}

TEST_F(DeliveryOptimizationTest, MmiSetEmptyObject)
{
    const char* expectedFileContent = "{}";
    const int expectedFileContentSizeBytes = strlen(expectedFileContent);
    char *actualFileContent = nullptr;

    MMI_HANDLE handle = NULL;
    const char *payload = "{}";
    const int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = DeliveryOptimizationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    EXPECT_EQ(MMI_OK, DeliveryOptimizationMmiSet(handle, m_deliveryOptimizationComponentName, m_desiredDeliveryOptimizationPoliciesObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));
    EXPECT_STREQ(expectedFileContent, actualFileContent = LoadStringFromFile(m_deliveryOptimizationConfigFile, false, nullptr));
    EXPECT_EQ(expectedFileContentSizeBytes, strlen(actualFileContent));
    FREE_MEMORY(actualFileContent);
    EXPECT_EQ(0, remove(m_deliveryOptimizationConfigFile));

    DeliveryOptimizationMmiClose(handle);
}

TEST_F(DeliveryOptimizationTest, MmiSetInvalidCacheHostSource)
{
    MMI_HANDLE handle = NULL;
    const char *payload = "{\"cacheHostSource\":-1}";
    const int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = DeliveryOptimizationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    EXPECT_EQ(EINVAL, DeliveryOptimizationMmiSet(handle, m_deliveryOptimizationComponentName, m_desiredDeliveryOptimizationPoliciesObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));

    DeliveryOptimizationMmiClose(handle);
}

TEST_F(DeliveryOptimizationTest, MmiSetInvalidPercentageDownloadThrottle)
{
    MMI_HANDLE handle = NULL;
    const char *payload = "{\"percentageDownloadThrottle\":-1}";
    const int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = DeliveryOptimizationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    EXPECT_EQ(EINVAL, DeliveryOptimizationMmiSet(handle, m_deliveryOptimizationComponentName, m_desiredDeliveryOptimizationPoliciesObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));

    DeliveryOptimizationMmiClose(handle);
}

TEST_F(DeliveryOptimizationTest, MmiSetOutsideSession)
{
    MMI_HANDLE handle = NULL;
    const char *payload = "{\"cacheHostSource\":0}";
    const int payloadSizeBytes = strlen(payload);

    EXPECT_EQ(EINVAL, DeliveryOptimizationMmiSet(handle, m_deliveryOptimizationComponentName, m_desiredDeliveryOptimizationPoliciesObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));

    EXPECT_NE(nullptr, handle = DeliveryOptimizationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    DeliveryOptimizationMmiClose(handle);

    EXPECT_EQ(EINVAL, DeliveryOptimizationMmiSet(handle, m_deliveryOptimizationComponentName, m_desiredDeliveryOptimizationPoliciesObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));
}
