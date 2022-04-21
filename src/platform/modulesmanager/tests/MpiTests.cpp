// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <CommonTests.h>
#include <CommonUtils.h>
#include <ManagementModule.h>
#include <ModulesManager.h>
#include <ModulesManagerTests.h>
#include <Mpi.h>

using ::testing::Combine;
using ::testing::Values;

namespace Tests
{
    class MpiTests : public ::testing::Test
    {
    protected:
        ModulesManager m_modulesManager;
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
        MpiSession* session = new (std::nothrow) MpiSession(m_modulesManager, m_defaultClient);
        ASSERT_NE(nullptr, session);
        this->m_handle = reinterpret_cast<MPI_HANDLE>(session);
    }

    void MpiTests::TearDown()
    {
        MpiSession* session = reinterpret_cast<MpiSession*>(this->m_handle);
        ASSERT_NE(nullptr, session);
        delete session;
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

    TEST_F(MpiTests, MpiOpen_InvalidClientName)
    {
        ASSERT_EQ(nullptr, MpiOpen(nullptr, 0));
    }

    TEST_F(MpiTests, MpiSet_InvalidClientSession)
    {
        ASSERT_EQ(EINVAL, MpiSet(nullptr, m_defaultComponent, m_defaultObject, m_defaultPayload, m_defaultPayloadSize));
    }

    TEST_F(MpiTests, MpiSet_InvalidComponentName)
    {
        ASSERT_EQ(EINVAL, MpiSet(m_handle, nullptr, m_defaultObject, m_defaultPayload, m_defaultPayloadSize));
    }

    TEST_F(MpiTests, MpiSet_InvalidObjectName)
    {
        ASSERT_EQ(EINVAL, MpiSet(m_handle, m_defaultComponent, nullptr, m_defaultPayload, m_defaultPayloadSize));
    }

    TEST_F(MpiTests, MpiSet_InvalidPayload)
    {
        ASSERT_EQ(EINVAL, MpiSet(m_handle, m_defaultComponent, m_defaultObject, nullptr, 1));
    }

    TEST_F(MpiTests, PayloadValidation)
    {
        std::vector<std::pair<std::string, std::string>> objects = {
            {g_string, g_stringPayload},
            {g_integer, g_integerPayload},
            {g_boolean, g_booleanPayload},
            {g_integerArray, g_integerArrayPayload},
            {g_stringArray, g_stringArrayPayload},
            {g_integerMap, g_integerMapPayload},
            {g_stringMap, g_stringMapPayload},
            {g_object, g_objectPayload},
            {g_objectArray, g_objectArrayPayload}
        };

        ModulesManager modulesManager;
        EXPECT_EQ(MPI_OK, modulesManager.LoadModules(g_moduleDir, g_configJsonNoneReported));

        MpiSession mpiSession(modulesManager, m_defaultClient);
        EXPECT_EQ(MPI_OK, mpiSession.Open());
        MPI_HANDLE handle = reinterpret_cast<MPI_HANDLE>(&mpiSession);

        for (auto object : objects)
        {
            MMI_JSON_STRING payload = nullptr;
            int payloadSizeBytes = 0;
            const char* objectName = object.first.c_str();
            const char* validPayload = object.second.c_str();

            EXPECT_EQ(MPI_OK, MpiSet(handle, g_testModuleComponent1, objectName, (MPI_JSON_STRING)validPayload, strlen(validPayload)));
            EXPECT_EQ(MPI_OK, MpiGet(handle, g_testModuleComponent1, objectName, &payload, &payloadSizeBytes));

            std::string jsonPayload(payload, payloadSizeBytes);
            EXPECT_TRUE(JSON_EQ(validPayload, jsonPayload));
        }
    }

    TEST_F(MpiTests, MpiGet_InvalidClientSession)
    {
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(EINVAL, MpiGet(nullptr, m_defaultComponent, m_defaultObject, &payload, &payloadSizeBytes));
        ASSERT_EQ(0, payloadSizeBytes);
    }

    TEST_F(MpiTests, MpiGet_InvalidComponentName)
    {
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(EINVAL, MpiGet(m_handle, nullptr, m_defaultObject, &payload, &payloadSizeBytes));
        ASSERT_EQ(nullptr, payload);
        ASSERT_EQ(0, payloadSizeBytes);
    }

    TEST_F(MpiTests, MpiGet_InvalidObjectName)
    {
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(EINVAL, MpiGet(m_handle, m_defaultComponent, nullptr, &payload, &payloadSizeBytes));
        ASSERT_EQ(0, payloadSizeBytes);
    }

    TEST_F(MpiTests, MpiGet_InvalidPayload)
    {
        int payloadSizeBytes = 0;

        ASSERT_EQ(EINVAL, MpiGet(m_handle, m_defaultComponent, m_defaultObject, nullptr, &payloadSizeBytes));
        ASSERT_EQ(0, payloadSizeBytes);
    }

    TEST_F(MpiTests, MpiGet_InvalidPayloadSizeBytes)
    {
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(EINVAL, MpiGet(m_handle, m_defaultComponent, m_defaultObject, &payload, nullptr));
        ASSERT_EQ(nullptr, payload);
    }

    TEST_F(MpiTests, MpiSetDesired_InvalidHandle)
    {
        char payload[] = R"""({
            "component": {
                "object": "value"
            }
        })""";
        int payloadSizeBytes = 0;

        ASSERT_EQ(EINVAL, MpiSetDesired(nullptr, payload, payloadSizeBytes));
    }

    TEST_F(MpiTests, MpiGetReported_InvalidHandle)
    {
        MPI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        ASSERT_EQ(EINVAL, MpiGetReported(nullptr, &payload, &payloadSizeBytes));
        ASSERT_EQ(nullptr, payload);
        ASSERT_EQ(0, payloadSizeBytes);
    }
}