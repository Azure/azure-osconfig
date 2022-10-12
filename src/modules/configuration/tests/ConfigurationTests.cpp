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
            "\"VersionMinor\": 0,"
            "\"VersionInfo\": \"Zinc\","
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
        const char* m_iotHubProtocolObject = "iotHubProtocol";
        
        const char* g_desiredRefreshIntervalObject = "desiredRefreshInterval";
        const char* g_desiredLocalManagementEnabledObject = "desiredLocalManagementEnabled";
        const char* g_desiredFullLoggingEnabledObject = "desiredFullLoggingEnabled";
        const char* g_desiredCommandLoggingEnabledObject = "desiredCommandLoggingEnabled";
        const char* g_desiredIotHubProtocolObject = "desiredIotHubProtocol";

        const char* m_testConfiguration = 
            "{"
                "\"CommandLogging\": 0,"
                "\"FullLogging\" : 0,"
                "\"LocalManagement\" : 0,"
                "\"ModelVersion\" : 15,"
                "\"IotHubProtocol\" : 2,"
                "\"ReportingIntervalSeconds\": 30"
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
        m_iotHubProtocolObject
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
        m_iotHubProtocolObject
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
    const char* refreshInterval;
    const char* localManagementEnabled;
    const char* fullLoggingEnabled;
    const char* commandLoggingEnabled;
    const char* iotHubProtocol;
    int expectedResult;
};

TEST_F(ConfigurationTest, MmiSet)
{
    MMI_HANDLE handle = nullptr;

    ConfigurationCombination testCombinations[] = 
    {
        { "5", "true", "true", "false", "auto",  MMI_OK },
        { "3", "false", "false", "true", "mqtt", MMI_OK },
        { "15", "false", "false", "true", "mqtt", MMI_OK },
        { "30", "false", "false", "false", "mqttWebSocket", MMI_OK },
        { "30", "bogus", "false", "false", "mqttWebSocket", EINVAL },
        { "30", "false", "bogus", "false", "mqttWebSocket", EINVAL },
        { "30", "false", "false", "bogus", "mqttWebSocket", EINVAL },
        { "30", "false", "false", "false", "bogus", EINVAL }
    };
    
    int numTestCombinations = ARRAY_SIZE(testCombinations);
    
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = ConfigurationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    for (int i = 0; i < numTestCombinations; i++)
    {
        EXPECT_EQ(testCombinations[i].expectedResult, ConfigurationMmiSet(handle, m_configurationComponentName, g_desiredRefreshIntervalObject, 
            (MMI_JSON_STRING)testCombinations[i].refreshInterval, strlen(testCombinations[i].refreshInterval)));

        if (MMI_OK == testCombinations[i].expectedResult)
        {
            EXPECT_EQ(MMI_OK, ConfigurationMmiGet(handle, m_configurationComponentName, m_refreshIntervalObject, &payload, &payloadSizeBytes));
            EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
            EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
            EXPECT_STREQ(payloadString, testCombinations[i].refreshInterval);
            FREE_MEMORY(payload);
            FREE_MEMORY(payloadString);
            payloadSizeBytes = 0;
        }

        EXPECT_EQ(testCombinations[i].expectedResult, ConfigurationMmiSet(handle, m_configurationComponentName, g_desiredLocalManagementEnabledObject, 
            (MMI_JSON_STRING)testCombinations[i].desiredLocalManagementEnabled, strlen(testCombinations[i].desiredLocalManagementEnabled)));

        if (MMI_OK == testCombinations[i].expectedResult)
        {
            EXPECT_EQ(MMI_OK, ConfigurationMmiGet(handle, m_configurationComponentName, m_localManagementEnabledObject, &payload, &payloadSizeBytes));
            EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
            EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
            EXPECT_STREQ(payloadString, testCombinations[i].localManagementEnabled);
            FREE_MEMORY(payload);
            FREE_MEMORY(payloadString);
            payloadSizeBytes = 0;
        }
        
        EXPECT_EQ(testCombinations[i].expectedResult, ConfigurationMmiSet(handle, m_configurationComponentName, g_desiredFullLoggingEnabledObject, 
            (MMI_JSON_STRING)testCombinations[i].desiredFullLoggingEnabled, strlen(testCombinations[i].rdesiredFullLoggingEnabled)));

        if (MMI_OK == testCombinations[i].expectedResult)
        {
            EXPECT_EQ(MMI_OK, ConfigurationMmiGet(handle, m_configurationComponentName, m_fullLoggingEnabledObject, &payload, &payloadSizeBytes));
            EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
            EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
            EXPECT_STREQ(payloadString, testCombinations[i].fullLoggingEnabled);
            FREE_MEMORY(payload);
            payloadSizeBytes = 0;
        }
        
        EXPECT_EQ(testCombinations[i].expectedResult, ConfigurationMmiSet(handle, m_configurationComponentName, g_desiredCommandLoggingEnabledObject, 
            (MMI_JSON_STRING)testCombinations[i].desiredCommandLoggingEnabled, strlen(testCombinations[i].desiredCommandLoggingEnabled)));

        if (MMI_OK == testCombinations[i].expectedResult)
        {
            EXPECT_EQ(MMI_OK, ConfigurationMmiGet(handle, m_configurationComponentName, m_commandLoggingEnabledObject, &payload, &payloadSizeBytes));
            EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
            EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
            EXPECT_STREQ(payloadString, testCombinations[i].commandLoggingEnabled);
            FREE_MEMORY(payload);
            FREE_MEMORY(payloadString);
            payloadSizeBytes = 0;
        }
        
        EXPECT_EQ(testCombinations[i].expectedResult, ConfigurationMmiSet(handle, m_configurationComponentName, g_desiredIotHubProtocolObject, 
            (MMI_JSON_STRING)testCombinations[i].desiredIotHubProtocol, strlen(testCombinations[i].desiredIotHubProtocol)));

        if (MMI_OK == testCombinations[i].expectedResult)
        {
            EXPECT_EQ(MMI_OK, ConfigurationMmiGet(handle, m_configurationComponentName, m_iotHubProtocolObject, &payload, &payloadSizeBytes));
            EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
            EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
            EXPECT_STREQ(payloadString, testCombinations[i].iotHubProtocol);
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
    const char* payload = "{\"refreshInterval\":15,\"localManagementEnabled\":false,\"fullLoggingEnabled\":false,\"commandLoggingEnabled\":true,\"iotHubProtocol\":1}";
    int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = ConfigurationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    EXPECT_EQ(EINVAL, ConfigurationMmiSet(handle, "Test123", m_modelVersionObject, (MMI_JSON_STRING)payload, payloadSizeBytes));
    ConfigurationMmiClose(handle);
}

TEST_F(ConfigurationTest, MmiSetInvalidObject)
{
    MMI_HANDLE handle = NULL;
    const char* payload = "{\"refreshInterval\":15,\"localManagementEnabled\":false,\"fullLoggingEnabled\":false,\"commandLoggingEnabled\":true,\"iotHubProtocol\":1}";
    int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = ConfigurationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    EXPECT_EQ(EINVAL, ConfigurationMmiSet(handle, m_configurationComponentName, "Test123", (MMI_JSON_STRING)payload, payloadSizeBytes));
    ConfigurationMmiClose(handle);
}

TEST_F(ConfigurationTest, MmiSetOutsideSession)
{
    MMI_HANDLE handle = NULL;
    const char* payload = "{\"refreshInterval\":15,\"localManagementEnabled\":false,\"fullLoggingEnabled\":false,\"commandLoggingEnabled\":true,\"iotHubProtocol\":1}";
    int payloadSizeBytes = strlen(payload);

    EXPECT_EQ(EINVAL, ConfigurationMmiSet(handle, m_configurationComponentName, m_desiredConfigurationObject, (MMI_JSON_STRING)payload, payloadSizeBytes));

    EXPECT_NE(nullptr, handle = ConfigurationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    ConfigurationMmiClose(handle);
    EXPECT_EQ(EINVAL, ConfigurationMmiSet(handle, m_configurationComponentName, m_desiredConfigurationObject, (MMI_JSON_STRING)payload, payloadSizeBytes));
}