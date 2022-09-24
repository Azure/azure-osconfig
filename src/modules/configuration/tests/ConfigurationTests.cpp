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
        const char* m_expectedMmiInfo = "{\"Name\": \"OsConfigConfiguration\","
            "\"Description\": \"Provides functionality to manage OSConfig configuration on device\","
            "\"Manufacturer\": \"Microsoft\","
            "\"VersionMajor\": 1,"
            "\"VersionMinor\": 0,"
            "\"VersionInfo\": \"Nickel\","
            "\"Components\": [\"OsConfigConfiguration\"],"
            "\"Lifetime\": 2,"
            "\"UserAccount\": 0}";

        const char* m_configurationModuleName = "OSConfig Configuration module";
        const char* m_configurationComponentName = "OsConfigConfiguration";

        const char* m_modelVersionObject = "modelVersion";
        const char* m_refreshIntervalObject = "refreshInterval";
        const char* m_localManagementEnabledObject = "localManagementEnabled";
        const char* m_fullLoggingEnabledObject = "fullLoggingEnabled";
        const char* m_commandLoggingEnabledObject = "commandLoggingEnabled";
        const char* m_iotHubProtocolObject = "iotHubProtocol";
        //TBD: add desired object    
        //const char* m_desiredConfigurationObject = "desiredConfiguration";

        const char* m_clientName = "Test";

        int m_normalMaxPayloadSizeBytes = 1024;
        int m_truncatedMaxPayloadSizeBytes = 1;

        void SetUp()
        {
            ConfigurationInitialize();
        }

        void TearDown()
        {
            ConfigurationShutdown();
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

TEST_F(ConfigurationTest, MmiSet)
{
    MMI_HANDLE handle = nullptr;
    const char* payload = "\"Test\":\"test\"";
    int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = ConfigurationMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    
    EXPECT_EQ(EPERM, ConfigurationMmiSet(handle, m_configurationComponentName, m_modelVersionObject, (MMI_JSON_STRING)payload, payloadSizeBytes));
    
    ConfigurationMmiClose(handle);
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