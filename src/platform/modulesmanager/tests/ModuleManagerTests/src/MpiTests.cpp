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

    class MpiPayloadValidation : public MpiTests, public ::testing::WithParamInterface<std::pair<std::string, std::string>> {};

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

    INSTANTIATE_TEST_SUITE_P(MpiTests, MpiPayloadValidation, Values(
        std::pair<std::string, std::string>{"string", "\"string\""},
        std::pair<std::string, std::string>{"integer", "123"},
        std::pair<std::string, std::string>{"boolean", "true"},
        std::pair<std::string, std::string>{"integerArray", "[1, 2, 3]"},
        std::pair<std::string, std::string>{"stringArray", "[\"a\", \"b\", \"c\"]"},
        std::pair<std::string, std::string>{"integerMap", "{\"key1\": 1, \"key2\": 2}"},
        std::pair<std::string, std::string>{"stringMap", "{\"key1\": \"a\", \"key2\": \"b\"}"},
        std::pair<std::string, std::string>{"object", "{\"string\": \"value\",\"integer\": 1,\"boolean\": true,\"integerEnum\": 1,\"integerArray\": [1, 2, 3],\"stringArray\": [\"a\", \"b\", \"c\"],\"integerMap\": { \"key1\": 1, \"key2\": 2 },\"stringMap\": { \"key1\": \"a\", \"key2\": \"b\" }}"},
        std::pair<std::string, std::string>{"objectArray", "[{\"string\": \"value\",\"integer\": 1,\"boolean\": true,\"integerEnum\": 1,\"integerArray\": [1, 2, 3],\"stringArray\": [\"a\", \"b\", \"c\"],\"integerMap\": { \"key1\": 1, \"key2\": 2 },\"stringMap\": { \"key1\": \"a\", \"key2\": \"b\" }}, {\"string\": \"value\",\"integer\": 1,\"boolean\": true,\"integerEnum\": 1,\"integerArray\": [1, 2, 3],\"stringArray\": [\"a\", \"b\", \"c\"],\"integerMap\": { \"key1\": 1, \"key2\": 2 },\"stringMap\": { \"key1\": \"a\", \"key2\": \"b\" }}]"}));


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

    TEST_P(MpiPayloadValidation, ObjectTypes)
    {
        MPI_HANDLE handle = nullptr;
        ModulesManager* modulesManager = new ModulesManager(defaultClient, 0);
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;
        const char* objectName = std::get<0>(GetParam()).c_str();
        const char* validPayload = std::get<1>(GetParam()).c_str();

        modulesManager->LoadModules(g_moduleDir, g_configJsonNoneReported);
        handle = reinterpret_cast<MPI_HANDLE>(modulesManager);

        ASSERT_EQ(MPI_OK, MpiSet(handle, g_testModuleComponent1, objectName, (MPI_JSON_STRING)validPayload, strlen(validPayload)));
        ASSERT_EQ(MPI_OK, MpiGet(handle, g_testModuleComponent1, objectName, &payload, &payloadSizeBytes));

        std::string jsonPayload(payload, payloadSizeBytes);
        ASSERT_TRUE(JSON_EQ(validPayload, jsonPayload));
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