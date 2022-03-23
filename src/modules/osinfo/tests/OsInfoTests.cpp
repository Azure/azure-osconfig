// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <Mmi.h>
#include <OsInfo.h>
#include <CommonUtils.h>

using namespace std;

class OsInfoTest : public ::testing::Test
{
    protected:
        const char* m_expectedMmiInfo = "{\"Name\": \"OsInfo\","
            "\"Description\": \"Provides functionality to observe OS and device information\","
            "\"Manufacturer\": \"Microsoft\","
            "\"VersionMajor\": 1,"
            "\"VersionMinor\": 0,"
            "\"VersionInfo\": \"Copper\","
            "\"Components\": [\"OsInfo\"],"
            "\"Lifetime\": 2,"
            "\"UserAccount\": 0}";

        const char* m_osInfoModuleName = "OsInfo module";
        const char* m_osInfoComponentName = "OsInfo";
        const char* m_osNameObject = "Name";
        const char* m_osVersionObject = "Version";
        const char* m_cpuTypeObject = "CpuType";
        const char* m_kernelNameObject = "KernelName";
        const char* m_kernelReleaseObject = "KernelRelease";
        const char* m_kernelVersionObject = "KernelVersion";
        const char* m_productVendorObject = "Manufacturer";
        const char* m_productNameObject = "Model";

        const char* m_clientName = "Test";

        int m_normalMaxPayloadSizeBytes = 1024;
        int m_truncatedMaxPayloadSizeBytes = 1;

        void SetUp()
        {
            OsInfoInitialize();
        }

        void TearDown()
        {
            OsInfoShutdown();
        }
};

TEST_F(OsInfoTest, MmiOpen)
{
    MMI_HANDLE handle = nullptr;
    EXPECT_NE(nullptr, handle = OsInfoMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    OsInfoMmiClose(handle);
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

TEST_F(OsInfoTest, MmiGetInfo)
{
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_EQ(MMI_OK, OsInfoMmiGetInfo(m_clientName, &payload, &payloadSizeBytes));
    EXPECT_NE(nullptr, payload);
    EXPECT_NE(0, payloadSizeBytes);

    EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
    EXPECT_STREQ(m_expectedMmiInfo, payloadString);
    EXPECT_EQ(strlen(payloadString), payloadSizeBytes);

    FREE_MEMORY(payloadString);
    OsInfoMmiFree(payload);
}

TEST_F(OsInfoTest, MmiSet)
{
    MMI_HANDLE handle = nullptr;
    const char* payload = "\"Test\":\"test\"";
    int payloadSizeBytes = strlen(payload);

    EXPECT_NE(nullptr, handle = OsInfoMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    
    EXPECT_EQ(EPERM, OsInfoMmiSet(handle, m_osInfoComponentName, m_osVersionObject, (MMI_JSON_STRING)payload, payloadSizeBytes));
    
    OsInfoMmiClose(handle);
}

TEST_F(OsInfoTest, MmiGetRequiredObjects)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    const char* mimRequiredObjects[] = {
        m_osNameObject,
        m_osVersionObject,
        m_cpuTypeObject,
        m_kernelNameObject,
        m_kernelReleaseObject,
        m_kernelVersionObject,
        m_productNameObject,
        m_productVendorObject
    };

    int mimRequiredObjectsNumber = ARRAY_SIZE(mimRequiredObjects);

    EXPECT_NE(nullptr, handle = OsInfoMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    for (int i = 0; i < mimRequiredObjectsNumber; i++)
    {
        EXPECT_EQ(MMI_OK, OsInfoMmiGet(handle, m_osInfoComponentName, mimRequiredObjects[i], &payload, &payloadSizeBytes));
        EXPECT_NE(nullptr, payload);
        EXPECT_NE(0, payloadSizeBytes);
        EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
        EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
        FREE_MEMORY(payloadString);
        OsInfoMmiFree(payload);
    }
    
    OsInfoMmiClose(handle);
}

TEST_F(OsInfoTest, MmiGetTruncatedPayload)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    const char* mimRequiredObjects[] = {
        m_osNameObject,
        m_osVersionObject,
        m_cpuTypeObject,
        m_kernelNameObject,
        m_kernelReleaseObject,
        m_kernelVersionObject,
        m_productNameObject,
        m_productVendorObject
    };

    int mimRequiredObjectsNumber = ARRAY_SIZE(mimRequiredObjects);

    EXPECT_NE(nullptr, handle = OsInfoMmiOpen(m_clientName, m_truncatedMaxPayloadSizeBytes));

    for (int i = 0; i < mimRequiredObjectsNumber; i++)
    {
        EXPECT_EQ(MMI_OK, OsInfoMmiGet(handle, m_osInfoComponentName, mimRequiredObjects[i], &payload, &payloadSizeBytes));
        EXPECT_NE(nullptr, payload);
        EXPECT_NE(0, payloadSizeBytes);
        EXPECT_NE(nullptr, payloadString = CopyPayloadToString(payload, payloadSizeBytes));
        EXPECT_EQ(strlen(payloadString), payloadSizeBytes);
        EXPECT_EQ(m_truncatedMaxPayloadSizeBytes, payloadSizeBytes);
        FREE_MEMORY(payloadString);
        OsInfoMmiFree(payload);
    }

    OsInfoMmiClose(handle);
}

TEST_F(OsInfoTest, MmiGetOptionalObjects)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    char* payloadString = nullptr;
    int payloadSizeBytes = 0;

    const char* mimOptionalObjects[] = {
        m_productNameObject,
        m_productVendorObject
    };

    int mimOptionalObjectsNumber = ARRAY_SIZE(mimOptionalObjects);

    EXPECT_NE(nullptr, handle = OsInfoMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    for (int i = 0; i < mimOptionalObjectsNumber; i++)
    {
        EXPECT_EQ(MMI_OK, OsInfoMmiGet(handle, m_osInfoComponentName, mimOptionalObjects[i], &payload, &payloadSizeBytes));
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
            OsInfoMmiFree(payload);
        }
    }

    OsInfoMmiClose(handle);
}

TEST_F(OsInfoTest, MmiGetInvalidComponent)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = OsInfoMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    EXPECT_EQ(EINVAL, OsInfoMmiGet(handle, "Test123", m_osNameObject, &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);
    
    OsInfoMmiClose(handle);
}

TEST_F(OsInfoTest, MmiGetInvalidObject)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_NE(nullptr, handle = OsInfoMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));

    EXPECT_EQ(EINVAL, OsInfoMmiGet(handle, m_osInfoComponentName, "Test123", &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);
    
    OsInfoMmiClose(handle);
}

TEST_F(OsInfoTest, MmiGetOutsideSession)
{
    MMI_HANDLE handle = NULL;
    char* payload = nullptr;
    int payloadSizeBytes = 0;

    EXPECT_EQ(EINVAL, OsInfoMmiGet(handle, m_osInfoComponentName, m_osNameObject, &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);

    EXPECT_NE(nullptr, handle = OsInfoMmiOpen(m_clientName, m_normalMaxPayloadSizeBytes));
    OsInfoMmiClose(handle);

    EXPECT_EQ(EINVAL, OsInfoMmiGet(handle, m_osInfoComponentName, m_osNameObject, &payload, &payloadSizeBytes));
    EXPECT_EQ(nullptr, payload);
    EXPECT_EQ(0, payloadSizeBytes);
}