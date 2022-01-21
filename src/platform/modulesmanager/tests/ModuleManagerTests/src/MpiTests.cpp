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
        MPI_HANDLE handle;

        static const char defaultClient[];
        static const char defaultComponent[];
        static const char defaultObject[];
        static char defaultPayload[];
        static const int defaultPayloadSize;

        void SetUp() override;
        void TearDown() override;
    };

    const char MpiTests::defaultClient[] = "Default_MpiTest_Client";
    const char MpiTests::defaultComponent[] = "Default_MpiTest_Component";
    const char MpiTests::defaultObject[] = "Default_MpiTest_Object";
    char MpiTests::defaultPayload[] = "\"Default_MpiTest_Payload\"";
    const int MpiTests::defaultPayloadSize = ARRAY_SIZE(MpiTests::defaultPayload) - 1;

    void MpiTests::SetUp()
    {
        ModulesManager* modulesManager = new ModulesManager(defaultClient);
        ASSERT_NE(nullptr, modulesManager);

        this->handle = reinterpret_cast<MPI_HANDLE>(modulesManager);
    }

    void MpiTests::TearDown()
    {
        ModulesManager* modulesManager = reinterpret_cast<ModulesManager*>(this->handle);
        ASSERT_NE(nullptr, modulesManager);
        delete modulesManager;
    }

    TEST_F(MpiTests, MpiOpen)
    {
        MPI_HANDLE handle = MpiOpen(defaultClient, 0);
        ASSERT_NE(nullptr, handle);

        ModulesManager* modulesManager = reinterpret_cast<ModulesManager*>(handle);
        ASSERT_STREQ(defaultClient, modulesManager->GetClientName().c_str());

        MpiClose(handle);
    }

    TEST_F(MpiTests, MpiOpen_InvalidClientName)
    {
        ASSERT_EQ(nullptr, MpiOpen(nullptr, 0));
    }

    TEST_F(MpiTests, MpiSet_InvalidClientSession)
    {
        ASSERT_EQ(EINVAL, MpiSet(nullptr, defaultComponent, defaultObject, defaultPayload, defaultPayloadSize));
    }

    TEST_F(MpiTests, MpiSet_InvalidComponentName)
    {
        ASSERT_EQ(EINVAL, MpiSet(handle, nullptr, defaultObject, defaultPayload, defaultPayloadSize));
    }

    TEST_F(MpiTests, MpiSet_InvalidObjectName)
    {
        ASSERT_EQ(EINVAL, MpiSet(handle, defaultComponent, nullptr, defaultPayload, defaultPayloadSize));
    }

    TEST_F(MpiTests, MpiSet_InvalidPayload)
    {
        ASSERT_EQ(EINVAL, MpiSet(handle, defaultComponent, defaultObject, nullptr, 1));
    }

    TEST_F(MpiTests, PayloadValidation)
    {
        MPI_HANDLE handle = nullptr;
        ModulesManager* modulesManager = new ModulesManager(defaultClient, 0);
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

        modulesManager->LoadModules(g_moduleDir, g_configJsonNoneReported);
        handle = reinterpret_cast<MPI_HANDLE>(modulesManager);

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

        ASSERT_EQ(EINVAL, MpiGet(nullptr, defaultComponent, defaultObject, &payload, &payloadSizeBytes));
        ASSERT_EQ(0, payloadSizeBytes);
    }

    TEST_F(MpiTests, MpiGet_InvalidComponentName)
    {
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(EINVAL, MpiGet(handle, nullptr, defaultObject, &payload, &payloadSizeBytes));
        ASSERT_EQ(nullptr, payload);
        ASSERT_EQ(0, payloadSizeBytes);
    }

    TEST_F(MpiTests, MpiGet_InvalidObjectName)
    {
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(EINVAL, MpiGet(handle, defaultComponent, nullptr, &payload, &payloadSizeBytes));
        ASSERT_EQ(0, payloadSizeBytes);
    }

    TEST_F(MpiTests, MpiGet_InvalidPayload)
    {
        int payloadSizeBytes = 0;

        ASSERT_EQ(EINVAL, MpiGet(handle, defaultComponent, defaultObject, nullptr, &payloadSizeBytes));
        ASSERT_EQ(0, payloadSizeBytes);
    }

    TEST_F(MpiTests, MpiGet_InvalidPayloadSizeBytes)
    {
        MMI_JSON_STRING payload = nullptr;

        ASSERT_EQ(EINVAL, MpiGet(handle, defaultComponent, defaultObject, &payload, nullptr));
        ASSERT_EQ(nullptr, payload);
    }

    TEST_F(MpiTests, MpiSetDesired_InvalidClientName)
    {
        char payload[] = R"""({
            "component": {
                "object": "value"
            }
        })""";
        int payloadSizeBytes = 0;

        ASSERT_EQ(EINVAL, MpiSetDesired(nullptr, payload, payloadSizeBytes));
    }

    TEST_F(MpiTests, MpiGetReported_InvalidClientName)
    {
        MPI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        ASSERT_EQ(EINVAL, MpiGetReported(nullptr, 0, &payload, &payloadSizeBytes));
        ASSERT_EQ(nullptr, payload);
        ASSERT_EQ(0, payloadSizeBytes);
    }
}