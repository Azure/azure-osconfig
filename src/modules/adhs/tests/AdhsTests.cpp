// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <version.h>
#include <Mmi.h>
#include <Adhs.h>
#include <CommonUtils.h>

using namespace std;

class AdhsTest : public ::testing::Test
{
    protected:
        const char* m_expectedMmiInfo = "{\"Name\": \"Adhs\","
            "\"Description\": \"Provides functionality to observe and configure Azure Device Health Services (ADHS)\","
            "\"Manufacturer\": \"Microsoft\","
            "\"VersionMajor\": 1,"
            "\"VersionMinor\": 0,"
            "\"VersionInfo\": \"Copper\","
            "\"Components\": [\"Adhs\"],"
            "\"Lifetime\": 2,"
            "\"UserAccount\": 0}";

        const char* m_adhsComponentName = "Adhs";
        const char* g_reportedOptInObjectName = "optIn";
        const char* g_desiredOptInObjectName = "desiredOptIn";

        const char* m_adhsConfigFile = "test-config.toml";

        const char* m_clientName = "Test";

        int m_normalMaxPayloadSizeBytes = 1024;
        int m_truncatedMaxPayloadSizeBytes = 1;

        void SetUp()
        {
            AdhsInitialize(m_adhsConfigFile);
        }

        void TearDown()
        {
            AdhsShutdown();
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

TEST_F(AdhsTest, MmiOpen)
{
    MMI_HANDLE handle = nullptr;
    EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    AdhsMmiClose(handle);
}

TEST_F(AdhsTest, MmiGetInfo)
{
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_EQ(MMI_OK, AdhsMmiGetInfo(m_clientName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);

    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_STREQ(m_expectedMmiInfo, payloadString);
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
}

TEST_F(AdhsTest, MmiGetValidConfigFile1)
{
    const char* testData = "Permission = \"Required\"";
    EXPECT_TRUE(SavePayloadToFile(m_adhsConfigFile, testData, strlen(testData), nullptr));

    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    EXPECT_EQ(MMI_OK, AdhsMmiGet(handle, m_adhsComponentName, g_reportedOptInObjectName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);
    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
    EXPECT_STREQ(payloadString, "1");
    FREE_MEMORY(payloadString);
    AdhsMmiFree(payload);

    AdhsMmiClose(handle);
    EXPECT_EQ(0, remove(m_adhsConfigFile));
}

TEST_F(AdhsTest, MmiGetValidConfigFile2)
{
    const char* testData = "# Comment\nNumber = 0\n  Permission=\'Required\'\nArray = [1, 2, 3]";
    EXPECT_TRUE(SavePayloadToFile(m_adhsConfigFile, testData, strlen(testData), nullptr));

    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    EXPECT_EQ(MMI_OK, AdhsMmiGet(handle, m_adhsComponentName, g_reportedOptInObjectName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);
    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
    EXPECT_STREQ(payloadString, "1");
    FREE_MEMORY(payloadString);
    AdhsMmiFree(payload);

    AdhsMmiClose(handle);
    EXPECT_EQ(0, remove(m_adhsConfigFile));
}

TEST_F(AdhsTest, MmiGetEmptyConfigFile)
{
    const char* testData = "# Empty";
    EXPECT_TRUE(SavePayloadToFile(m_adhsConfigFile, testData, strlen(testData), nullptr));

    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    EXPECT_EQ(MMI_OK, AdhsMmiGet(handle, m_adhsComponentName, g_reportedOptInObjectName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);
    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
    EXPECT_STREQ(payloadString, "0");
    FREE_MEMORY(payloadString);
    AdhsMmiFree(payload);

    AdhsMmiClose(handle);
    EXPECT_EQ(0, remove(m_adhsConfigFile));
}

TEST_F(AdhsTest, MmiGetTruncatedPayload)
{
    const char* testData = "Permission = \"Required\"";
    EXPECT_TRUE(SavePayloadToFile(m_adhsConfigFile, testData, strlen(testData), nullptr));

    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_truncatedMaxPayloadSizeBytes));

    EXPECT_EQ(MMI_OK, AdhsMmiGet(handle, m_adhsComponentName, g_reportedOptInObjectName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);
    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
    EXPECT_EQ(m_truncatedMaxPayloadSizeBytes, payloadSizeBytes);
    FREE_MEMORY(payloadString);
    AdhsMmiFree(payload);

    AdhsMmiClose(handle);
    EXPECT_EQ(0, remove(m_adhsConfigFile));
}

TEST_F(AdhsTest, MmiGetInvalidComponent)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    EXPECT_EQ(EINVAL, AdhsMmiGet(handle, "Test123", g_reportedOptInObjectName, &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);

    AdhsMmiClose(handle);
}

TEST_F(AdhsTest, MmiGetInvalidObject)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    EXPECT_EQ(EINVAL, AdhsMmiGet(handle, m_adhsComponentName, "Test123", &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);

    AdhsMmiClose(handle);
}

TEST_F(AdhsTest, MmiGetOutsideSession)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_EQ(EINVAL, AdhsMmiGet(handle, m_adhsComponentName, g_reportedOptInObjectName, &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);

    EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    AdhsMmiClose(handle);

    EXPECT_EQ(EINVAL, AdhsMmiGet(handle, m_adhsComponentName, g_reportedOptInObjectName, &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);
}

TEST_F(AdhsTest, MmiSet)
{
    const char* expectedFileContent = "Permission = \"Optional\"\n";
    const int expectedFileContentSizeBytes = strlen(expectedFileContent);
    char *actualFileContent = nullptr;

    MMI_HANDLE handle = NULL;
    const char *payload = "2";
    const int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    EXPECT_EQ(MMI_OK, AdhsMmiSet(handle, m_adhsComponentName, g_desiredOptInObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));
    EXPECT_STREQ(expectedFileContent, actualFileContent = LoadStringFromFile(m_adhsConfigFile, false, nullptr));
    EXPECT_EQ(expectedFileContentSizeBytes, strlen(actualFileContent));
    FREE_MEMORY(actualFileContent);
    EXPECT_EQ(0, remove(m_adhsConfigFile));

    AdhsMmiClose(handle);
}

TEST_F(AdhsTest, MmiSetInvalidComponent)
{
    MMI_HANDLE handle = NULL;
    const char *payload = "2";
    const int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    EXPECT_EQ(EINVAL, AdhsMmiSet(handle, "Test123", g_desiredOptInObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));

    AdhsMmiClose(handle);
}

TEST_F(AdhsTest, MmiSetInvalidObject)
{
    MMI_HANDLE handle = NULL;
    const char *payload = "2";
    const int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    EXPECT_EQ(EINVAL, AdhsMmiSet(handle, m_adhsComponentName, "Test123", (MMI_JSON_STRING)payload, payloadSizeBytes));

    AdhsMmiClose(handle);
}

TEST_F(AdhsTest, MmiSetInvalidDesiredOptIn1)
{
    MMI_HANDLE handle = NULL;
    const char *payload = "-1";
    const int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    EXPECT_EQ(EINVAL, AdhsMmiSet(handle, m_adhsComponentName, g_desiredOptInObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));

    AdhsMmiClose(handle);
}

TEST_F(AdhsTest, MmiSetInvalidDesiredOptIn2)
{
    MMI_HANDLE handle = NULL;
    const char *payload = "3";
    const int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    EXPECT_EQ(EINVAL, AdhsMmiSet(handle, m_adhsComponentName, g_desiredOptInObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));

    AdhsMmiClose(handle);
}

TEST_F(AdhsTest, MmiSetOutsideSession)
{
    MMI_HANDLE handle = NULL;
    const char *payload = "0";
    const int payloadSizeBytes = strlen(payload);

    EXPECT_EQ(EINVAL, AdhsMmiSet(handle, m_adhsComponentName, g_desiredOptInObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));

    EXPECT_NE(nullptr, handle = AdhsMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    AdhsMmiClose(handle);

    EXPECT_EQ(EINVAL, AdhsMmiSet(handle, m_adhsComponentName, g_desiredOptInObjectName, (MMI_JSON_STRING)payload, payloadSizeBytes));
}
