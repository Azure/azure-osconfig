// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <version.h>
#include <Mmi.h>
#include <DeviceInfo.h>
#include <CommonUtils.h>

using namespace std;

class DeviceInfoTest : public ::testing::Test
{
    protected:
        const char* m_expectedMmiInfo = "{\"Name\": \"DeviceInfo\","
            "\"Description\": \"Provides functionality to observe device information\","
            "\"Manufacturer\": \"Microsoft\","
            "\"VersionMajor\": 3,"
            "\"VersionMinor\": 0,"
            "\"VersionInfo\": \"Copper\","
            "\"Components\": [\"DeviceInfo\"],"
            "\"Lifetime\": 2,"
            "\"UserAccount\": 0}";

        const char* m_osInfoModuleName = "DeviceInfo module";
        const char* m_osInfoComponentName = "DeviceInfo";
        const char* m_osNameObject = "osName";
        const char* m_osVersionObject = "osVersion";
        const char* m_cpuTypeObject = "cpuType";
        const char* m_cpuVendorIdObject = "cpuVendorId";
        const char* m_cpuModelObject = "cpuModel";
        const char* m_totalMemoryObject = "totalMemory";
        const char* m_freeMemoryObject = "freeMemory";
        const char* m_kernelNameObject = "kernelName";
        const char* m_kernelReleaseObject = "kernelRelease";
        const char* m_kernelVersionObject = "kernelVersion";
        const char* m_productVendorObject  = "productVendor";
        const char* m_productNameObject = "productName";
        const char* m_productVersionObject = "productVersion";
        const char* m_systemCapabilitiesObject = "systemCapabilities";
        const char* m_systemConfigurationObject = "systemConfiguration";
        const char* m_osConfigVersionObject = "osConfigVersion";

        const char* m_clientName = "Test";

        int m_normalMaxPayloadSizeBytes = 1024;
        int m_truncatedMaxPayloadSizeBytes = 1;

        void SetUp()
        {
            DeviceInfoInitialize();
        }

        void TearDown()
        {
            DeviceInfoShutdown();
        }
};

TEST_F(DeviceInfoTest, MmiOpen)
{
    MMI_HANDLE handle = nullptr;
    EXPECT_NE(nullptr, handle = DeviceInfoMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    DeviceInfoMmiClose(handle);
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

TEST_F(DeviceInfoTest, MmiGetInfo)
{
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_EQ(MMI_OK, DeviceInfoMmiGetInfo(m_clientName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);

    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_STREQ(m_expectedMmiInfo, payloadString);
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);

    FREE_MEMORY(payloadString);
    DeviceInfoMmiFree(payload);
}

TEST_F(DeviceInfoTest, MmiSet)
{
    MMI_HANDLE handle = nullptr;
    const char* payload = "\"Test\":\"test\"";
    int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = DeviceInfoMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    EXPECT_EQ(EPERM, DeviceInfoMmiSet(handle, m_osInfoComponentName, m_osVersionObject, (MMI_JSON_STRING)payload, payloadSizeBytes));

    DeviceInfoMmiClose(handle);
}

#define STRING_QUOTE "\""
#define OSCONFIG_VERSION_PAYLOAD STRING_QUOTE OSCONFIG_VERSION STRING_QUOTE

TEST_F(DeviceInfoTest, MmiGetRequiredObjects)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    const char* mimRequiredObjects[] = {
        m_osNameObject,
        m_osVersionObject,
        m_cpuTypeObject,
        m_cpuVendorIdObject,
        m_cpuModelObject,
        m_totalMemoryObject,
        m_freeMemoryObject,
        m_kernelNameObject,
        m_kernelReleaseObject,
        m_kernelVersionObject,
        m_osConfigVersionObject
    };

    int mimRequiredObjectsNumber = ARRAY_SIZE(mimRequiredObjects);

    EXPECT_NE(nullptr, handle = DeviceInfoMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    for (int i = 0; i < mimRequiredObjectsNumber; i++)
    {
        EXPECT_EQ(MMI_OK, DeviceInfoMmiGet(handle, m_osInfoComponentName, mimRequiredObjects[i], &payload, &payloadSizeBytes));
        EXPECT_NE(nullptr, payload);
        EXPECT_NE(0, payloadSizeBytes);
        EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
        EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
        if (0 == strcmp(mimRequiredObjects[i], m_osConfigVersionObject))
        {
            EXPECT_STREQ(payloadString, OSCONFIG_VERSION_PAYLOAD);
        }
        FREE_MEMORY(payloadString);
        DeviceInfoMmiFree(payload);
    }

    DeviceInfoMmiClose(handle);
}

TEST_F(DeviceInfoTest, MmiGetTruncatedPayload)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    const char* mimRequiredObjects[] = {
        m_osNameObject,
        m_osVersionObject,
        m_cpuTypeObject,
        m_cpuVendorIdObject,
        m_cpuModelObject,
        m_totalMemoryObject,
        m_freeMemoryObject,
        m_kernelNameObject,
        m_kernelReleaseObject,
        m_kernelVersionObject,
        m_osConfigVersionObject
    };

    int mimRequiredObjectsNumber = ARRAY_SIZE(mimRequiredObjects);

    EXPECT_NE(nullptr, handle = DeviceInfoMmiOpen(m_clientName, m_truncatedMaxPayloadSizeBytes));

    for (int i = 0; i < mimRequiredObjectsNumber; i++)
    {
        EXPECT_EQ(MMI_OK, DeviceInfoMmiGet(handle, m_osInfoComponentName, mimRequiredObjects[i], &payload, &payloadSizeBytes));
        EXPECT_NE(nullptr, payload);
        EXPECT_NE(0, payloadSizeBytes);
        EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
        EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
        EXPECT_EQ(m_truncatedMaxPayloadSizeBytes, payloadSizeBytes);
        FREE_MEMORY(payloadString);
        DeviceInfoMmiFree(payload);
    }

    DeviceInfoMmiClose(handle);
}

TEST_F(DeviceInfoTest, MmiGetOptionalObjects)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    const char* mimOptionalObjects[] = {
        m_productNameObject,
        m_productVendorObject,
        m_productVersionObject,
        m_systemCapabilitiesObject,
        m_systemConfigurationObject
    };

    int mimOptionalObjectsNumber = ARRAY_SIZE(mimOptionalObjects);

    EXPECT_NE(nullptr, handle = DeviceInfoMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    for (int i = 0; i < mimOptionalObjectsNumber; i++)
    {
        EXPECT_EQ(MMI_OK, DeviceInfoMmiGet(handle, m_osInfoComponentName, mimOptionalObjects[i], &payload, &payloadSizeBytes));
        if ((nullptr == payload) || (0 == payloadSizeBytes))
        {
            EXPECT_EQ(nullptr, payload);
            EXPECT_EQ(0, payloadSizeBytes);
        }
        else
        {
            EXPECT_NE(0, payloadSizeBytes);
            EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
            EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
            FREE_MEMORY(payloadString);
            DeviceInfoMmiFree(payload);
        }
    }

    DeviceInfoMmiClose(handle);
}

TEST_F(DeviceInfoTest, MmiGetInvalidComponent)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = DeviceInfoMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    EXPECT_EQ(EINVAL, DeviceInfoMmiGet(handle, "Test123", m_osNameObject, &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);

    DeviceInfoMmiClose(handle);
}

TEST_F(DeviceInfoTest, MmiGetInvalidObject)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = DeviceInfoMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    EXPECT_EQ(EINVAL, DeviceInfoMmiGet(handle, m_osInfoComponentName, "Test123", &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);

    DeviceInfoMmiClose(handle);
}

TEST_F(DeviceInfoTest, MmiGetOutsideSession)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_EQ(EINVAL, DeviceInfoMmiGet(handle, m_osInfoComponentName, m_osNameObject, &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);

    EXPECT_NE(nullptr, handle = DeviceInfoMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    DeviceInfoMmiClose(handle);

    EXPECT_EQ(EINVAL, DeviceInfoMmiGet(handle, m_osInfoComponentName, m_osNameObject, &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);
}
