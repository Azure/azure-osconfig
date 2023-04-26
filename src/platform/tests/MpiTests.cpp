// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <PlatformCommon.h>
#include <TestModules.h>
#include <ModulesManager.h>

namespace Tests
{
    class MpiTests : public ::testing::Test
    {
    protected:
        MPI_HANDLE m_handle;

        static const char m_defaultClient[];
        static const char m_defaultComponent[];
        static const char m_defaultObject[];
        static char m_defaultPayload[];
        static const int m_defaultPayloadSize;

        void SetUp() override;
        void TearDown() override;
    };

    const char MpiTests::m_defaultClient[] = "Default_MpiTest_Client";
    const char MpiTests::m_defaultComponent[] = "Default_MpiTest_Component";
    const char MpiTests::m_defaultObject[] = "Default_MpiTest_Object";
    char MpiTests::m_defaultPayload[] = "\"Default_MpiTest_Payload\"";
    const int MpiTests::m_defaultPayloadSize = ARRAY_SIZE(MpiTests::m_defaultPayload) - 1;

    void MpiTests::SetUp()
    {
        AreModulesLoadedAndLoadIfNot(TEST_MODULES_DIR, TEST_CONFIG_JSON_MULTIPLE_REPORTED);
        this->m_handle = MpiOpen(m_defaultClient, 0);
        ASSERT_NE(nullptr, this->m_handle);
    }

    void MpiTests::TearDown()
    {
        MpiClose(this->m_handle);
        UnloadModules();
    }

    TEST_F(MpiTests, MpiOpen)
    {
        MPI_HANDLE handle1 = MpiOpen(m_defaultClient, 0);
        MPI_HANDLE handle2 = MpiOpen(m_defaultClient, 0);

        EXPECT_NE(nullptr, handle1);
        EXPECT_NE(nullptr, handle2);
        EXPECT_NE(handle1, handle2);

        MpiClose(handle1);
        MpiClose(handle2);
    }

    TEST_F(MpiTests, MpiOpenInvalidClientName)
    {
        ASSERT_EQ(nullptr, MpiOpen(nullptr, 0));
    }

    TEST_F(MpiTests, MpiSetInvalidClientSession)
    {
        ASSERT_EQ(EINVAL, MpiSet(nullptr, m_defaultComponent, m_defaultObject, m_defaultPayload, m_defaultPayloadSize));
    }

    TEST_F(MpiTests, MpiSetInvalidComponentName)
    {
        ASSERT_EQ(EINVAL, MpiSet(m_handle, nullptr, m_defaultObject, m_defaultPayload, m_defaultPayloadSize));
    }

    TEST_F(MpiTests, MpiSetInvalidObjectName)
    {
        ASSERT_EQ(EINVAL, MpiSet(m_handle, m_defaultComponent, nullptr, m_defaultPayload, m_defaultPayloadSize));
    }

    TEST_F(MpiTests, MpiSetInvalidPayload)
    {
        ASSERT_EQ(EINVAL, MpiSet(m_handle, m_defaultComponent, m_defaultObject, nullptr, 1));
    }

    TEST_F(MpiTests, MpiGetInvalidClientSession)
    {
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(EINVAL, MpiGet(nullptr, m_defaultComponent, m_defaultObject, &payload, &payloadSizeBytes));
        ASSERT_EQ(0, payloadSizeBytes);
    }

    TEST_F(MpiTests, MpiGetInvalidComponentName)
    {
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(EINVAL, MpiGet(m_handle, nullptr, m_defaultObject, &payload, &payloadSizeBytes));
        ASSERT_EQ(nullptr, payload);
        ASSERT_EQ(0, payloadSizeBytes);
    }

    TEST_F(MpiTests, MpiGetInvalidObjectName)
    {
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(EINVAL, MpiGet(m_handle, m_defaultComponent, nullptr, &payload, &payloadSizeBytes));
        ASSERT_EQ(0, payloadSizeBytes);
    }

    TEST_F(MpiTests, MpiGetInvalidPayload)
    {
        int payloadSizeBytes = 0;

        ASSERT_EQ(EINVAL, MpiGet(m_handle, m_defaultComponent, m_defaultObject, nullptr, &payloadSizeBytes));
        ASSERT_EQ(0, payloadSizeBytes);
    }

    TEST_F(MpiTests, MpiGetInvalidPayloadSizeBytes)
    {
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(EINVAL, MpiGet(m_handle, m_defaultComponent, m_defaultObject, &payload, nullptr));
        ASSERT_EQ(nullptr, payload);
    }

    TEST_F(MpiTests, MpiSetDesiredInvalidHandle)
    {
        char payload[] = R"""({
            "component": {
                "object": "value"
            }
        })""";
        int payloadSizeBytes = 0;

        ASSERT_EQ(EINVAL, MpiSetDesired(nullptr, payload, payloadSizeBytes));
    }

    TEST_F(MpiTests, MpiGetReportedInvalidHandle)
    {
        MPI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        ASSERT_EQ(EINVAL, MpiGetReported(nullptr, &payload, &payloadSizeBytes));
        ASSERT_EQ(nullptr, payload);
        ASSERT_EQ(0, payloadSizeBytes);
    }
}