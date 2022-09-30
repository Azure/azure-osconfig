// // Copyright (c) Microsoft Corporation. All rights reserved.
// // Licensed under the MIT License.

// #include <gtest/gtest.h>
// #include <version.h>
// #include <Mmi.h>
// #include <Adhs.h>
// #include <CommonUtils.h>

// using namespace std;

// class AdhsTest : public ::testing::Test
// {
//     protected:
//         const char* m_expectedMmiInfo = "{\"Name\": \"Adhs\","
//             "\"Description\": \"Provides functionality to observe and configure ADHS\","
//             "\"Manufacturer\": \"Microsoft\","
//             "\"VersionMajor\": 1,"
//             "\"VersionMinor\": 0,"
//             "\"VersionInfo\": \"Copper\","
//             "\"Components\": [\"Adhs\"],"
//             "\"Lifetime\": 2,"
//             "\"UserAccount\": 0}";

//         const char* m_adhsComponentName = "Adhs";
//         const char* m_reportedCacheHostObjectName = "cacheHost";
//         const char* m_reportedCacheHostSourceObjectName = "cacheHostSource";
//         const char* m_reportedCacheHostFallbackObjectName = "cacheHostFallback";
//         const char* m_reportedPercentageDownloadThrottleObjectName = "percentageDownloadThrottle";
//         const char* m_desiredAdhsPoliciesObjectName = "desiredAdhsPolicies";

//         const char* m_adhsConfigFile = "test-config.json";
        
//         const char* m_clientName = "Test";

//         int m_normalMaxPayloadSizeBytes = 1024;
//         int m_truncatedMaxPayloadSizeBytes = 1;

//         void SetUp()
//         {
//             AdhsInitialize(m_adhsConfigFile);
//         }

//         void TearDown()
//         {
//             AdhsShutdown();
//         }
// };

// char* CopyPayloadToString(const char* payload, int payloadSizeBytes)
// {
//     char* output = nullptr;
    
//     EXPECT_NE(nullptr, payload);
//     EXPECT_NE(0, payloadSizeBytes);
//     EXPECT_NE(nullptr, output = (char*)malloc(payloadSizeBytes + 1));

//     if (nullptr != output)
//     {
//         memcpy(output, payload, payloadSizeBytes);
//         output[payloadSizeBytes] = 0;
//     }

//     return output;
// }

// TEST_F(AdhsTest, MmiOpen)
// {
//     MMI_HANDLE handle = nullptr;
//     EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
//     AdhsMmiClose(handle);
// }

// TEST_F(AdhsTest, MmiGetInfo)
// {
//     char* payload = nullptr;
//     char* payloadString = nullptr;
//     int payloadSizeBytes = 0;

//     EXPECT_EQ(MMI_OK, AdhsMmiGetInfo(m_clientName, &payload, &payloadSizeBytes));
//     EXPECT_NE(nullptr, payload);
//     EXPECT_NE(0, payloadSizeBytes);

//     EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
//     EXPECT_STREQ(m_expectedMmiInfo, payloadString);
//     EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
// }

// TEST_F(AdhsTest, MmiGetConfigFile)
// {
//     const char* testData = "{\"DOCacheHost\":\"10.0.0.0:80,host.com:8080\",\"DOCacheHostSource\":1,\"DOCacheHostFallback\":2,\"DOPercentageDownloadThrottle\":3}";
//     EXPECT_TRUE(SavePayloadToFile(m_adhsConfigFile, testData, strlen(testData), nullptr));

//     MMI_HANDLE handle = NULL;
//     char* payload = nullptr;
//     char* payloadString = nullptr;
//     int payloadSizeBytes = 0;

//     EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

//     EXPECT_EQ(MMI_OK, AdhsMmiGet(handle, m_adhsComponentName, m_reportedCacheHostObjectName, &payload, &payloadSizeBytes));
//     EXPECT_NE(nullptr, payload);
//     EXPECT_NE(0, payloadSizeBytes);
//     EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
//     EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
//     EXPECT_STREQ(payloadString, "\"10.0.0.0:80,host.com:8080\"");
//     FREE_MEMORY(payloadString);
//     AdhsMmiFree(payload);

//     payload = nullptr;
//     payloadSizeBytes = 0;

//     EXPECT_EQ(MMI_OK, AdhsMmiGet(handle, m_adhsComponentName, m_reportedCacheHostSourceObjectName, &payload, &payloadSizeBytes));
//     EXPECT_NE(nullptr, payload);
//     EXPECT_NE(0, payloadSizeBytes);
//     EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
//     EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
//     EXPECT_STREQ(payloadString, "1");
//     FREE_MEMORY(payloadString);
//     AdhsMmiFree(payload);

//     payload = nullptr;
//     payloadSizeBytes = 0;

//     EXPECT_EQ(MMI_OK, AdhsMmiGet(handle, m_adhsComponentName, m_reportedCacheHostFallbackObjectName, &payload, &payloadSizeBytes));
//     EXPECT_NE(nullptr, payload);
//     EXPECT_NE(0, payloadSizeBytes);
//     EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
//     EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
//     EXPECT_STREQ(payloadString, "2");
//     FREE_MEMORY(payloadString);
//     AdhsMmiFree(payload);

//     payload = nullptr;
//     payloadSizeBytes = 0;

//     EXPECT_EQ(MMI_OK, AdhsMmiGet(handle, m_adhsComponentName, m_reportedPercentageDownloadThrottleObjectName, &payload, &payloadSizeBytes));
//     EXPECT_NE(nullptr, payload);
//     EXPECT_NE(0, payloadSizeBytes);
//     EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
//     EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
//     EXPECT_STREQ(payloadString, "3");
//     FREE_MEMORY(payloadString);
//     AdhsMmiFree(payload);

//     AdhsMmiClose(handle);
//     EXPECT_EQ(0, remove(m_adhsConfigFile));
// }

// TEST_F(AdhsTest, MmiGetEmptyConfigFile)
// {
//     const char* testData = "{}";
//     EXPECT_TRUE(SavePayloadToFile(m_adhsConfigFile, testData, strlen(testData), nullptr));

//     MMI_HANDLE handle = NULL;
//     char* payload = nullptr;
//     char* payloadString = nullptr;
//     int payloadSizeBytes = 0;

//     EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

//     EXPECT_EQ(MMI_OK, AdhsMmiGet(handle, m_adhsComponentName, m_reportedCacheHostObjectName, &payload, &payloadSizeBytes));
//     EXPECT_NE(nullptr, payload);
//     EXPECT_NE(0, payloadSizeBytes);
//     EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
//     EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
//     EXPECT_STREQ(payloadString, "\"\"");
//     FREE_MEMORY(payloadString);
//     AdhsMmiFree(payload);

//     payload = nullptr;
//     payloadSizeBytes = 0;

//     EXPECT_EQ(MMI_OK, AdhsMmiGet(handle, m_adhsComponentName, m_reportedCacheHostSourceObjectName, &payload, &payloadSizeBytes));
//     EXPECT_NE(nullptr, payload);
//     EXPECT_NE(0, payloadSizeBytes);
//     EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
//     EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
//     EXPECT_STREQ(payloadString, "0");
//     FREE_MEMORY(payloadString);
//     AdhsMmiFree(payload);

//     payload = nullptr;
//     payloadSizeBytes = 0;

//     EXPECT_EQ(MMI_OK, AdhsMmiGet(handle, m_adhsComponentName, m_reportedCacheHostFallbackObjectName, &payload, &payloadSizeBytes));
//     EXPECT_NE(nullptr, payload);
//     EXPECT_NE(0, payloadSizeBytes);
//     EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
//     EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
//     EXPECT_STREQ(payloadString, "0");
//     FREE_MEMORY(payloadString);
//     AdhsMmiFree(payload);

//     payload = nullptr;
//     payloadSizeBytes = 0;

//     EXPECT_EQ(MMI_OK, AdhsMmiGet(handle, m_adhsComponentName, m_reportedPercentageDownloadThrottleObjectName, &payload, &payloadSizeBytes));
//     EXPECT_NE(nullptr, payload);
//     EXPECT_NE(0, payloadSizeBytes);
//     EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
//     EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
//     EXPECT_STREQ(payloadString, "0");
//     FREE_MEMORY(payloadString);
//     AdhsMmiFree(payload);

//     AdhsMmiClose(handle);
//     EXPECT_EQ(0, remove(m_adhsConfigFile));
// }

// TEST_F(AdhsTest, MmiGetTruncatedPayload)
// {
//     const char* testData = "{\"DOCacheHost\":\"10.0.0.0:80,host.com:8080\",\"DOCacheHostSource\":1,\"DOCacheHostFallback\":2,\"DOPercentageDownloadThrottle\":3}";
//     EXPECT_TRUE(SavePayloadToFile(m_adhsConfigFile, testData, strlen(testData), nullptr));

//     MMI_HANDLE handle = NULL;
//     char* payload = nullptr;
//     char* payloadString = nullptr;
//     int payloadSizeBytes = 0;

//     EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_truncatedMaxPayloadSizeBytes));

//     EXPECT_EQ(MMI_OK, AdhsMmiGet(handle, m_adhsComponentName, m_reportedCacheHostObjectName, &payload, &payloadSizeBytes));
//     EXPECT_NE(nullptr, payload);
//     EXPECT_NE(0, payloadSizeBytes);
//     EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
//     EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
//     EXPECT_EQ(m_truncatedMaxPayloadSizeBytes, payloadSizeBytes);
//     FREE_MEMORY(payloadString);
//     AdhsMmiFree(payload);

//     payload = nullptr;
//     payloadSizeBytes = 0;

//     EXPECT_EQ(MMI_OK, AdhsMmiGet(handle, m_adhsComponentName, m_reportedCacheHostSourceObjectName, &payload, &payloadSizeBytes));
//     EXPECT_NE(nullptr, payload);
//     EXPECT_NE(0, payloadSizeBytes);
//     EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
//     EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
//     EXPECT_EQ(m_truncatedMaxPayloadSizeBytes, payloadSizeBytes);
//     FREE_MEMORY(payloadString);
//     AdhsMmiFree(payload);

//     payload = nullptr;
//     payloadSizeBytes = 0;

//     EXPECT_EQ(MMI_OK, AdhsMmiGet(handle, m_adhsComponentName, m_reportedCacheHostFallbackObjectName, &payload, &payloadSizeBytes));
//     EXPECT_NE(nullptr, payload);
//     EXPECT_NE(0, payloadSizeBytes);
//     EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
//     EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
//     EXPECT_EQ(m_truncatedMaxPayloadSizeBytes, payloadSizeBytes);
//     FREE_MEMORY(payloadString);
//     AdhsMmiFree(payload);

//     payload = nullptr;
//     payloadSizeBytes = 0;

//     EXPECT_EQ(MMI_OK, AdhsMmiGet(handle, m_adhsComponentName, m_reportedPercentageDownloadThrottleObjectName, &payload, &payloadSizeBytes));
//     EXPECT_NE(nullptr, payload);
//     EXPECT_NE(0, payloadSizeBytes);
//     EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
//     EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
//     EXPECT_EQ(m_truncatedMaxPayloadSizeBytes, payloadSizeBytes);
//     FREE_MEMORY(payloadString);
//     AdhsMmiFree(payload);

//     AdhsMmiClose(handle);
//     EXPECT_EQ(0, remove(m_adhsConfigFile));
// }

// TEST_F(AdhsTest, MmiGetInvalidComponent)
// {
//     MMI_HANDLE handle = NULL;
//     char* payload = nullptr;
//     int payloadSizeBytes = 0;

//     EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

//     EXPECT_EQ(EINVAL, AdhsMmiGet(handle, "Test123", m_reportedCacheHostObjectName, &payload, &payloadSizeBytes));
//     EXPECT_EQ(nullptr, payload);
//     EXPECT_EQ(0, payloadSizeBytes);
    
//     AdhsMmiClose(handle);
// }

// TEST_F(AdhsTest, MmiGetInvalidObject)
// {
//     MMI_HANDLE handle = NULL;
//     char* payload = nullptr;
//     int payloadSizeBytes = 0;

//     EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

//     EXPECT_EQ(EINVAL, AdhsMmiGet(handle, m_adhsComponentName, "Test123", &payload, &payloadSizeBytes));
//     EXPECT_EQ(nullptr, payload);
//     EXPECT_EQ(0, payloadSizeBytes);
    
//     AdhsMmiClose(handle);
// }

// TEST_F(AdhsTest, MmiGetOutsideSession)
// {
//     MMI_HANDLE handle = NULL;
//     char* payload = nullptr;
//     int payloadSizeBytes = 0;

//     EXPECT_EQ(EINVAL, AdhsMmiGet(handle, m_adhsComponentName, m_reportedCacheHostObjectName, &payload, &payloadSizeBytes));
//     EXPECT_EQ(nullptr, payload);
//     EXPECT_EQ(0, payloadSizeBytes);

//     EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
//     AdhsMmiClose(handle);

//     EXPECT_EQ(EINVAL, AdhsMmiGet(handle, m_adhsComponentName, m_reportedCacheHostObjectName, &payload, &payloadSizeBytes));
//     EXPECT_EQ(nullptr, payload);
//     EXPECT_EQ(0, payloadSizeBytes);
// }

// TEST_F(AdhsTest, MmiSetAllSettings)
// {
//     const char* expectedFileContent = 
//     "{\n"
//     "    \"DOCacheHost\": \"10.0.0.0:80,host.com:8080\",\n"
//     "    \"DOCacheHostSource\": 1,\n"
//     "    \"DOCacheHostFallback\": 2,\n"
//     "    \"DOPercentageDownloadThrottle\": 3\n"
//     "}";
//     const int expectedFileContentSizeBytes = strlen(expectedFileContent);
//     char *actualFileContent = nullptr;

//     MMI_HANDLE handle = NULL;
//     const char *payload = "{\"cacheHost\":\"10.0.0.0:80,host.com:8080\",\"cacheHostSource\":1,\"cacheHostFallback\":2,\"percentageDownloadThrottle\":3}";
//     const int payloadSizeBytes = strlen(payload);

//     EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
//     EXPECT_EQ(MMI_OK, AdhsMmiSet(handle, m_adhsComponentName, m_desiredAdhsPoliciesObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));
//     EXPECT_STREQ(expectedFileContent, actualFileContent = LoadStringFromFile(m_adhsConfigFile, false, nullptr));
//     EXPECT_EQ(expectedFileContentSizeBytes, strlen(actualFileContent));
//     FREE_MEMORY(actualFileContent);
//     EXPECT_EQ(0, remove(m_adhsConfigFile));

//     AdhsMmiClose(handle);
// }

// TEST_F(AdhsTest, MmiSetOneSetting)
// {
//     const char* expectedFileContent = 
//     "{\n"
//     "    \"DOCacheHost\": \"10.0.0.0:80,host.com:8080\"\n"
//     "}";
//     const int expectedFileContentSizeBytes = strlen(expectedFileContent);
//     char *actualFileContent = nullptr;

//     MMI_HANDLE handle = NULL;
//     const char *payload = "{\"cacheHost\":\"10.0.0.0:80,host.com:8080\"}";
//     const int payloadSizeBytes = strlen(payload);

//     EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
//     EXPECT_EQ(MMI_OK, AdhsMmiSet(handle, m_adhsComponentName, m_desiredAdhsPoliciesObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));
//     EXPECT_STREQ(expectedFileContent, actualFileContent = LoadStringFromFile(m_adhsConfigFile, false, nullptr));
//     EXPECT_EQ(expectedFileContentSizeBytes, strlen(actualFileContent));
//     FREE_MEMORY(actualFileContent);
//     EXPECT_EQ(0, remove(m_adhsConfigFile));

//     AdhsMmiClose(handle);
// }

// TEST_F(AdhsTest, MmiSetInvalidComponent)
// {
//     MMI_HANDLE handle = NULL;
//     const char *payload = "{\"cacheHost\":\"10.0.0.0:80,host.com:8080\"}";
//     const int payloadSizeBytes = strlen(payload);

//     EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
//     EXPECT_EQ(EINVAL, AdhsMmiSet(handle, "Test123", m_desiredAdhsPoliciesObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));

//     AdhsMmiClose(handle);
// }

// TEST_F(AdhsTest, MmiSetInvalidObject)
// {
//     MMI_HANDLE handle = NULL;
//     const char *payload = "{\"cacheHost\":\"10.0.0.0:80,host.com:8080\"}";
//     const int payloadSizeBytes = strlen(payload);

//     EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
//     EXPECT_EQ(EINVAL, AdhsMmiSet(handle, m_adhsComponentName, "Test123", (MMI_JSON_STRING)payload, payloadSizeBytes));

//     AdhsMmiClose(handle);
// }

// TEST_F(AdhsTest, MmiSetInvalidSetting)
// {
//     const char* expectedFileContent = "{}";
//     const int expectedFileContentSizeBytes = strlen(expectedFileContent);
//     char *actualFileContent = nullptr;

//     MMI_HANDLE handle = NULL;
//     const char *payload = "{\"testSetting\":\"testValue\"}";
//     const int payloadSizeBytes = strlen(payload);

//     EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
//     EXPECT_EQ(MMI_OK, AdhsMmiSet(handle, m_adhsComponentName, m_desiredAdhsPoliciesObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));
//     EXPECT_STREQ(expectedFileContent, actualFileContent = LoadStringFromFile(m_adhsConfigFile, false, nullptr));
//     EXPECT_EQ(expectedFileContentSizeBytes, strlen(actualFileContent));
//     FREE_MEMORY(actualFileContent);
//     EXPECT_EQ(0, remove(m_adhsConfigFile));

//     AdhsMmiClose(handle);
// }

// TEST_F(AdhsTest, MmiSetEmptyObject)
// {
//     const char* expectedFileContent = "{}";
//     const int expectedFileContentSizeBytes = strlen(expectedFileContent);
//     char *actualFileContent = nullptr;

//     MMI_HANDLE handle = NULL;
//     const char *payload = "{}";
//     const int payloadSizeBytes = strlen(payload);

//     EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
//     EXPECT_EQ(MMI_OK, AdhsMmiSet(handle, m_adhsComponentName, m_desiredAdhsPoliciesObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));
//     EXPECT_STREQ(expectedFileContent, actualFileContent = LoadStringFromFile(m_adhsConfigFile, false, nullptr));
//     EXPECT_EQ(expectedFileContentSizeBytes, strlen(actualFileContent));
//     FREE_MEMORY(actualFileContent);
//     EXPECT_EQ(0, remove(m_adhsConfigFile));

//     AdhsMmiClose(handle);
// }

// TEST_F(AdhsTest, MmiSetInvalidCacheHostSource)
// {
//     MMI_HANDLE handle = NULL;
//     const char *payload = "{\"cacheHostSource\":-1}";
//     const int payloadSizeBytes = strlen(payload);

//     EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
//     EXPECT_EQ(EINVAL, AdhsMmiSet(handle, m_adhsComponentName, m_desiredAdhsPoliciesObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));

//     AdhsMmiClose(handle);
// }

// TEST_F(AdhsTest, MmiSetInvalidPercentageDownloadThrottle)
// {
//     MMI_HANDLE handle = NULL;
//     const char *payload = "{\"percentageDownloadThrottle\":-1}";
//     const int payloadSizeBytes = strlen(payload);

//     EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
//     EXPECT_EQ(EINVAL, AdhsMmiSet(handle, m_adhsComponentName, m_desiredAdhsPoliciesObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));

//     AdhsMmiClose(handle);
// }

// TEST_F(AdhsTest, MmiSetOutsideSession)
// {
//     MMI_HANDLE handle = NULL;
//     const char *payload = "{\"cacheHostSource\":0}";
//     const int payloadSizeBytes = strlen(payload);

//     EXPECT_EQ(EINVAL, AdhsMmiSet(handle, m_adhsComponentName, m_desiredAdhsPoliciesObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));

//     EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
//     AdhsMmiClose(handle);

//     EXPECT_EQ(EINVAL, AdhsMmiSet(handle, m_adhsComponentName, m_desiredAdhsPoliciesObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));
// }