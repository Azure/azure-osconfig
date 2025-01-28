// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <version.h>
#include <Mmi.h>
#include <CommonUtils.h>
#include <Configuration.h>

using namespace std;

class ConfigurationTest : public ::testing::Test
{
    protected:
        const char* m_expectedMmiInfo = "{\"Name\": \"Configuration\","
            "\"Description\": \"Provides functionality to manage OSConfig configuration on device\","
            "\"Manufacturer\": \"Microsoft\","
            "\"VersionMajor\": 1,"
            "\"VersionMinor\": 4,"
            "\"VersionInfo\": \"Dilithium\","
            "\"Components\": [\"Configuration\"],"
            "\"Lifetime\": 2,"
            "\"UserAccount\": 0}";

        const char* m_configurationModuleName = "OSConfig Configuration module";
        const char* m_configurationComponentName = "Configuration";

        const char* m_modelVersionObject = "modelVersion";
        const char* m_refreshIntervalObject = "refreshInterval";
        const char* m_localManagementEnabledObject = "localManagementEnabled";
        const char* m_fullLoggingEnabledObject = "fullLoggingEnabled";
        const char* m_commandLoggingEnabledObject = "commandLoggingEnabled";
        const char* m_iotHubManagementEnabledObject = "iotHubManagementEnabled";
        const char* m_iotHubProtocolObject = "iotHubProtocol";
        const char* m_gitManagementEnabledObject = "gitManagementEnabled";
        const char* m_gitBranchObject = "gitBranch";

        const char* m_desiredRefreshIntervalObject = "desiredRefreshInterval";
        const char* m_desiredLocalManagementEnabledObject = "desiredLocalManagementEnabled";
        const char* m_desiredFullLoggingEnabledObject = "desiredFullLoggingEnabled";
        const char* m_desiredCommandLoggingEnabledObject = "desiredCommandLoggingEnabled";
        const char* m_desiredIotHubManagementEnabledObject = "desiredIotHubManagementEnabled";
        const char* m_desiredIotHubProtocolObject = "desiredIotHubProtocol";
        const char* m_desiredGitManagementEnabledObject = "desiredGitManagementEnabled";
        const char* m_desiredGitBranchObject = "desiredGitBranch";

        const char* m_testConfiguration =
            "{"
                "\"CommandLogging\": 0,"
                "\"FullLogging\" : 0,"
                "\"LocalManagement\" : 0,"
                "\"ModelVersion\" : 15,"
                "\"IotHubManagement\" : 0,"
                "\"IotHubProtocol\" : 2,"
                "\"ReportingIntervalSeconds\": 30,"
                "\"GitManagement\" : 1,"
                "\"GitBranch\" : \"Test/Foo\""
            "}";

        const char* m_testConfigurationFile = "~testConfiguration.json";

        const char* m_clientName = "Test";

        int m_normalMaxPayloadSizeBytes = 1024;
        int m_truncatedMaxPayloadSizeBytes = 1;

        void SetUp()
        {
            EXPECT_TRUE(SavePayloadToFile(m_testConfigurationFile, m_testConfiguration, strlen(m_testConfiguration), nullptr));
            ConfigurationInitialize(m_testConfigurationFile);
        }

        void TearDown()
        {
            ConfigurationShutdown();
            EXPECT_EQ(0, remove(m_testConfigurationFile));
        }
};

TEST_F(ConfigurationTest, MmiOpen)
{
    MMI_HANDLE handle = nullptr;
    EXPECT_NE(nullptr, handle = ConfigurationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    ConfigurationMmiClose(handle);
}

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

TEST_F(ConfigurationTest, MmiGetInfo)
{
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_EQ(MMI_OK, ConfigurationMmiGetInfo(m_clientName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);

    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_STREQ(m_expectedMmiInfo, payloadString);
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);

    FREE_MEMORY(payloadString);
    ConfigurationMmiFree(payload);
}

TEST_F(ConfigurationTest, MmiGet)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    const char* mimRequiredObjects[] = {
        m_modelVersionObject,
        m_refreshIntervalObject,
        m_localManagementEnabledObject,
        m_fullLoggingEnabledObject,
        m_commandLoggingEnabledObject,
        m_iotHubManagementEnabledObject,
        m_iotHubProtocolObject,
        m_gitManagementEnabledObject,
        m_gitBranchObject
    };

    int mimRequiredObjectsNumber = ARRAY_SIZE(mimRequiredObjects);

    EXPECT_NE(nullptr, handle = ConfigurationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    for (int i = 0; i < mimRequiredObjectsNumber; i++)
    {
        EXPECT_EQ(MMI_OK, ConfigurationMmiGet(handle, m_configurationComponentName, mimRequiredObjects[i], &payload, &payloadSizeBytes));
        EXPECT_NE(nullptr, payload);
        EXPECT_NE(0, payloadSizeBytes);
        EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
        EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
        FREE_MEMORY(payloadString);
        ConfigurationMmiFree(payload);
    }

    ConfigurationMmiClose(handle);
}

TEST_F(ConfigurationTest, MmiGetTruncatedPayload)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    const char* mimRequiredObjects[] = {
        m_modelVersionObject,
        m_refreshIntervalObject,
        m_localManagementEnabledObject,
        m_fullLoggingEnabledObject,
        m_commandLoggingEnabledObject,
        m_iotHubManagementEnabledObject,
        m_iotHubProtocolObject,
        m_gitManagementEnabledObject,
        m_gitBranchObject
    };

    int mimRequiredObjectsNumber = ARRAY_SIZE(mimRequiredObjects);

    EXPECT_NE(nullptr, handle = ConfigurationMmiOpen(m_clientName, m_truncatedMaxPayloadSizeBytes));

    for (int i = 0; i < mimRequiredObjectsNumber; i++)
    {
        EXPECT_EQ(MMI_OK, ConfigurationMmiGet(handle, m_configurationComponentName, mimRequiredObjects[i], &payload, &payloadSizeBytes));
        EXPECT_NE(nullptr, payload);
        EXPECT_NE(0, payloadSizeBytes);
        EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
        EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
        EXPECT_EQ(m_truncatedMaxPayloadSizeBytes, payloadSizeBytes);
        FREE_MEMORY(payloadString);
        ConfigurationMmiFree(payload);
    }

    ConfigurationMmiClose(handle);
}

TEST_F(ConfigurationTest, MmiGetInvalidComponent)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = ConfigurationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    EXPECT_EQ(EINVAL, ConfigurationMmiGet(handle, "Test123", m_modelVersionObject, &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);

    ConfigurationMmiClose(handle);
}

TEST_F(ConfigurationTest, MmiGetInvalidObject)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = ConfigurationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    EXPECT_EQ(EINVAL, ConfigurationMmiGet(handle, m_configurationComponentName, "Test123", &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);

    ConfigurationMmiClose(handle);
}

TEST_F(ConfigurationTest, MmiGetOutsideSession)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_EQ(EINVAL, ConfigurationMmiGet(handle, m_configurationComponentName, m_modelVersionObject, &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);

    EXPECT_NE(nullptr, handle = ConfigurationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    ConfigurationMmiClose(handle);

    EXPECT_EQ(EINVAL, ConfigurationMmiGet(handle, m_configurationComponentName, m_modelVersionObject, &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);
}

struct ConfigurationCombination
{
    const char* desiredObject;
    const char* desiredValue;
    int expectedResult;
    const char* reportedObject;
    const char* reportedValue;
};

TEST_F(ConfigurationTest, MmiSet)
{
    MMI_HANDLE handle = nullptr;

    ConfigurationCombination testCombinations[] =
    {
        { m_desiredRefreshIntervalObject, "5", 0, m_refreshIntervalObject, "5" },
        { m_desiredRefreshIntervalObject, "30", 0, m_refreshIntervalObject, "30" },
        { m_desiredLocalManagementEnabledObject, "true", 0, m_localManagementEnabledObject, "true" },
        { m_desiredLocalManagementEnabledObject, "false", 0, m_localManagementEnabledObject, "false" },
        { m_desiredLocalManagementEnabledObject, "notImplemented", 22, m_localManagementEnabledObject, "false" },
        { m_desiredFullLoggingEnabledObject, "true", 0, m_fullLoggingEnabledObject, "true" },
        { m_desiredFullLoggingEnabledObject, "false", 0, m_fullLoggingEnabledObject, "false" },
        { m_desiredFullLoggingEnabledObject, "notImplemented", 22, m_fullLoggingEnabledObject, "false" },
        { m_desiredCommandLoggingEnabledObject, "true", 0, m_commandLoggingEnabledObject, "true" },
        { m_desiredCommandLoggingEnabledObject, "false", 0, m_commandLoggingEnabledObject, "false" },
        { m_desiredCommandLoggingEnabledObject, "notImplemented", 22, m_commandLoggingEnabledObject, "false" },
        { m_desiredIotHubManagementEnabledObject, "true", 0, m_iotHubManagementEnabledObject, "true" },
        { m_desiredIotHubManagementEnabledObject, "false", 0, m_iotHubManagementEnabledObject, "false" },
        { m_desiredIotHubManagementEnabledObject, "notImplemented", 22, m_iotHubManagementEnabledObject, "false" },
        { m_desiredIotHubProtocolObject, "\"auto\"", 0, m_iotHubProtocolObject, "\"auto\"" },
        { m_desiredIotHubProtocolObject, "\"mqtt\"", 0, m_iotHubProtocolObject, "\"mqtt\"" },
        { m_desiredIotHubProtocolObject, "\"mqttWebSocket\"", 0, m_iotHubProtocolObject, "\"mqttWebSocket\"" },
        { m_desiredIotHubProtocolObject, "notImplemented", 22, m_iotHubProtocolObject, "\"mqttWebSocket\"" },
        { m_desiredGitManagementEnabledObject, "true", 0, m_gitManagementEnabledObject, "true" },
        { m_desiredGitManagementEnabledObject, "false", 0, m_gitManagementEnabledObject, "false" },
        { m_desiredGitBranchObject, "\"Test\\/Foo\"", 0, m_gitBranchObject, "\"Test\\/Foo\"" }
    };

    int numTestCombinations = ARRAY_SIZE(testCombinations);

    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = ConfigurationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    for (int i = 0; i < numTestCombinations; i++)
    {
        EXPECT_EQ(testCombinations[i].expectedResult, ConfigurationMmiSet(handle, m_configurationComponentName,
            testCombinations[i].desiredObject, (MMI_JSON_STRING)testCombinations[i].desiredValue, strlen(testCombinations[i].desiredValue)));

        if (MMI_OK == testCombinations[i].expectedResult)
        {
            EXPECT_EQ(MMI_OK, ConfigurationMmiGet(handle, m_configurationComponentName, testCombinations[i].reportedObject, &payload, &payloadSizeBytes));
            EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
            EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
            EXPECT_STREQ(payloadString, testCombinations[i].reportedValue);
            FREE_MEMORY(payload);
            FREE_MEMORY(payloadString);
            payloadSizeBytes = 0;
        }
    }

    ConfigurationMmiClose(handle);
}

TEST_F(ConfigurationTest, MmiSetInvalidComponent)
{
    MMI_HANDLE handle = NULL;
    const char* payload = "1";
    int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = ConfigurationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    EXPECT_EQ(EINVAL, ConfigurationMmiSet(handle, "Test123", m_modelVersionObject, (MMI_JSON_STRING)payload, payloadSizeBytes));
    ConfigurationMmiClose(handle);
}

TEST_F(ConfigurationTest, MmiSetInvalidObject)
{
    MMI_HANDLE handle = NULL;
    const char* payload = "\"false\"";
    int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = ConfigurationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    EXPECT_EQ(EINVAL, ConfigurationMmiSet(handle, m_configurationComponentName, "Test123", (MMI_JSON_STRING)payload, payloadSizeBytes));
    ConfigurationMmiClose(handle);
}

TEST_F(ConfigurationTest, MmiSetOutsideSession)
{
    MMI_HANDLE handle = NULL;
    const char* payload = "30";
    int payloadSizeBytes = strlen(payload);

    EXPECT_EQ(EINVAL, ConfigurationMmiSet(handle, m_configurationComponentName, m_desiredRefreshIntervalObject, (MMI_JSON_STRING)payload, payloadSizeBytes));

    EXPECT_NE(nullptr, handle = ConfigurationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    ConfigurationMmiClose(handle);
    EXPECT_EQ(EINVAL, ConfigurationMmiSet(handle, m_configurationComponentName, m_desiredRefreshIntervalObject, (MMI_JSON_STRING)payload, payloadSizeBytes));
}
