// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <rapidjson/document.h>

#include <CommonTests.h>
#include <CommonUtils.h>
#include <ManagementModule.h>
#include <MockModulesManager.h>
#include <ModulesManagerTests.h>
#include <Mpi.h>

using testing::_;
using testing::ElementsAre;
using testing::DoAll;
using testing::InSequence;
using testing::Return;
using testing::SetArgPointee;
using testing::StrEq;
using ::testing::StrictMock;

namespace Tests
{
    class ModuleManagerTests : public ::testing::Test
    {
    protected:
        std::shared_ptr<StrictMock<MockModulesManager>> moduleManager;
        std::shared_ptr<StrictMock<MockManagementModule>> defaultModule;

        static const char defaultClient[];
        static const char defaultComponent[];
        static const char defaultObject[];
        static char defaultPayload[];
        static const int defaultPayloadSize;

        void SetUp() override;
        void TearDown() override;
    };

    const char ModuleManagerTests::defaultClient[] = "Default_ModuleManagerTest_Client";
    const char ModuleManagerTests::defaultComponent[] = "Default_ModuleManagerTest_Component";
    const char ModuleManagerTests::defaultObject[] = "Default_ModuleManagerTest_Object";
    char ModuleManagerTests::defaultPayload[] = "Default_ModuleManagerTest_Payload";
    const int ModuleManagerTests::defaultPayloadSize = ARRAY_SIZE(ModuleManagerTests::defaultPayload) - 1;

    void ModuleManagerTests::SetUp()
    {
        moduleManager.reset(new StrictMock<MockModulesManager>(defaultClient, 0));
        ASSERT_NE(nullptr, moduleManager);

        this->defaultModule = moduleManager->CreateModule(defaultComponent);
    }

    void ModuleManagerTests::TearDown()
    {
        moduleManager.reset();
        defaultModule.reset();
    }

    TEST_F(ModuleManagerTests, MpiSet)
    {
        EXPECT_CALL(*defaultModule, CallMmiSet(defaultComponent, defaultObject, defaultPayload, defaultPayloadSize)).Times(1).WillOnce(Return(MMI_OK));
        ASSERT_EQ(MPI_OK, moduleManager->MpiSet(defaultComponent, defaultObject, defaultPayload, defaultPayloadSize));
    }

    TEST_F(ModuleManagerTests, MpiSet_InvalidComponentName)
    {
        ASSERT_EQ(EINVAL, moduleManager->MpiSet(nullptr, defaultObject, defaultPayload, defaultPayloadSize));
    }

    TEST_F(ModuleManagerTests, MpiSet_InvalidObjectName)
    {
        ASSERT_EQ(EINVAL, moduleManager->MpiSet(defaultComponent, nullptr, defaultPayload, defaultPayloadSize));
    }

    TEST_F(ModuleManagerTests, MpiSet_InvalidPayload)
    {
        ASSERT_EQ(EINVAL, moduleManager->MpiSet(defaultComponent, defaultObject, nullptr, 0));
    }

    TEST_F(ModuleManagerTests, MpiGet)
    {
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;
        char expected[] = "\"expected\"";

        EXPECT_CALL(*defaultModule, CallMmiGet(defaultComponent, defaultObject, _, _)).Times(1).WillOnce(DoAll(SetArgPointee<2>(expected), SetArgPointee<3>(strlen(expected)), Return(MMI_OK)));

        EXPECT_EQ(MPI_OK, moduleManager->MpiGet(defaultComponent, defaultObject, &payload, &payloadSizeBytes));
        EXPECT_STREQ(expected, payload);
        EXPECT_EQ(strlen(expected), payloadSizeBytes);
    }

    TEST_F(ModuleManagerTests, MpiGet_InvalidComponentName)
    {
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        EXPECT_EQ(EINVAL, moduleManager->MpiGet(nullptr, defaultObject, &payload, &payloadSizeBytes));
        EXPECT_EQ(nullptr, payload);
        EXPECT_EQ(0, payloadSizeBytes);
    }

    TEST_F(ModuleManagerTests, MpiGet_InvalidObjectName)
    {
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        EXPECT_EQ(EINVAL, moduleManager->MpiGet(defaultComponent, nullptr, &payload, &payloadSizeBytes));
        EXPECT_EQ(nullptr, payload);
        EXPECT_EQ(0, payloadSizeBytes);
    }

    TEST_F(ModuleManagerTests, MpiGet_InvalidPayload)
    {
        int payloadSizeBytes = 0;

        EXPECT_EQ(EINVAL, moduleManager->MpiGet(defaultComponent, defaultObject, nullptr, &payloadSizeBytes));
        EXPECT_EQ(0, payloadSizeBytes);
    }

    TEST_F(ModuleManagerTests, MpiGet_InvalidPayloadSize)
    {
        MMI_JSON_STRING payload = nullptr;

        EXPECT_EQ(EINVAL, moduleManager->MpiGet(defaultComponent, defaultObject, &payload, nullptr));
        EXPECT_EQ(nullptr, payload);
    }

    TEST_F(ModuleManagerTests, MpiSetDesired)
    {
        const char componentName[] = "component";
        const char objectName[] = "object";
        char value[] = "\"value\"";
        char payload[] = R""""(
            {
                "component": {
                    "object": "value"
                }
            })"""";

        std::shared_ptr<MockManagementModule> mockModule = moduleManager->CreateModule(componentName);

        EXPECT_CALL(*mockModule, CallMmiSet(StrEq(componentName), StrEq(objectName), StrEq(value), strlen(value))).Times(1).WillOnce(Return(MMI_OK));
        ASSERT_EQ(MPI_OK, moduleManager->MpiSetDesired(payload, strlen(payload)));
    }

    TEST_F(ModuleManagerTests, MpiSetDesired_MultipleComponents)
    {
        const char componentName_1[] = "component_1";
        const char componentName_2[] = "component_2";
        const char objectName_1[] = "object_1";
        const char objectName_2[] = "object_2";
        char value_1[] = "\"value_1\"";
        char value_2[] = "\"value_2\"";
        char payload[] = R""""(
            {
                "component_1": {
                    "object_1": "value_1"
                },
                "component_2": {
                    "object_2": "value_2"
                }
            })"""";

        std::shared_ptr<MockManagementModule> mockModule_1 = moduleManager->CreateModule(componentName_1);
        std::shared_ptr<MockManagementModule> mockModule_2 = moduleManager->CreateModule(componentName_2);

        EXPECT_CALL(*mockModule_1, CallMmiSet(StrEq(componentName_1), StrEq(objectName_1), StrEq(value_1), strlen(value_1))).Times(1).WillOnce(Return(MMI_OK));
        EXPECT_CALL(*mockModule_2, CallMmiSet(StrEq(componentName_2), StrEq(objectName_2), StrEq(value_2), strlen(value_2))).Times(1).WillOnce(Return(MMI_OK));

        ASSERT_EQ(MPI_OK, moduleManager->MpiSetDesired(payload, strlen(payload)));
    }

    TEST_F(ModuleManagerTests, MpiSetDesired_MultipleObjects)
    {
        const char componentName[] = "component";
        const char objectName_1[] = "object_1";
        const char objectName_2[] = "object_2";
        char value_1[] = "\"value_1\"";
        char value_2[] = "\"value_2\"";
        char payload[] = R""""(
            {
                "component": {
                    "object_1": "value_1",
                    "object_2": "value_2"
                }
            })"""";

        std::shared_ptr<MockManagementModule> mockModule = moduleManager->CreateModule(componentName);

        EXPECT_CALL(*mockModule, CallMmiSet(StrEq(componentName), StrEq(objectName_1), StrEq(value_1), strlen(value_1))).Times(1).WillOnce(Return(MMI_OK));
        EXPECT_CALL(*mockModule, CallMmiSet(StrEq(componentName), StrEq(objectName_2), StrEq(value_2), strlen(value_2))).Times(1).WillOnce(Return(MMI_OK));

        ASSERT_EQ(MPI_OK, moduleManager->MpiSetDesired(payload, strlen(payload)));
    }

    TEST_F(ModuleManagerTests, MpiSetDesired_InvalidJsonPayload)
    {
        const char componentName[] = "component";
        char invalid[] = "invalid";
        std::shared_ptr<MockManagementModule> mockModule = moduleManager->CreateModule(componentName);

        ASSERT_EQ(EINVAL, moduleManager->MpiSetDesired(invalid, strlen(invalid)));
    }

    TEST_F(ModuleManagerTests, MpiSetDesired_InvalidJsonSchema)
    {
        const char componentName[] = "component";
        char invalid[] = R""""([
            {
                "component": {
                    "object": "value"
                }
            }])"""";
        std::shared_ptr<MockManagementModule> mockModule = moduleManager->CreateModule(componentName);

        ASSERT_EQ(EINVAL, moduleManager->MpiSetDesired(invalid, strlen(invalid)));
    }

    TEST_F(ModuleManagerTests, MpiGetReported)
    {
        const char componentName[] = "component";
        const char objectName[] = "object";
        char value[] = "\"value\"";
        char expected[] = R""""(
            {
                "component": {
                    "object": "value"
                }
            })"""";

        MPI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        std::shared_ptr<MockManagementModule> mockModule = moduleManager->CreateModule(componentName);
        mockModule->AddReportedObject(componentName, objectName);

        EXPECT_CALL(*mockModule, CallMmiGet(StrEq(componentName), StrEq(objectName), _, _)).Times(1).WillOnce(DoAll(SetArgPointee<2>(value), SetArgPointee<3>(strlen(value)), Return(MMI_OK)));

        EXPECT_EQ(MPI_OK, moduleManager->MpiGetReported(&payload, &payloadSizeBytes));
        EXPECT_TRUE(JSON_EQ(expected, payload));
    }

    TEST_F(ModuleManagerTests, MpiGetReported_MultipleComponents)
    {
        const char componentName_1[] = "component_1";
        const char componentName_2[] = "component_2";
        const char objectName_1[] = "object_1";
        const char objectName_2[] = "object_2";
        char value_1[] = "\"value_1\"";
        char value_2[] = "\"value_2\"";
        char expected[] = R""""(
            {
                "component_1": {
                    "object_1": "value_1"
                },
                "component_2": {
                    "object_2": "value_2"
                }
            })"""";

        MPI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        std::shared_ptr<MockManagementModule> mockModule_1 = moduleManager->CreateModule(componentName_1);
        std::shared_ptr<MockManagementModule> mockModule_2 = moduleManager->CreateModule(componentName_2);
        mockModule_1->AddReportedObject(componentName_1, objectName_1);
        mockModule_2->AddReportedObject(componentName_2, objectName_2);

        EXPECT_CALL(*mockModule_1, CallMmiGet(StrEq(componentName_1), StrEq(objectName_1), _, _)).Times(1).WillOnce(DoAll(SetArgPointee<2>(value_1), SetArgPointee<3>(strlen(value_1)), Return(MMI_OK)));
        EXPECT_CALL(*mockModule_2, CallMmiGet(StrEq(componentName_2), StrEq(objectName_2), _, _)).Times(1).WillOnce(DoAll(SetArgPointee<2>(value_2), SetArgPointee<3>(strlen(value_2)), Return(MMI_OK)));

        EXPECT_EQ(MPI_OK, moduleManager->MpiGetReported(&payload, &payloadSizeBytes));
        EXPECT_TRUE(JSON_EQ(expected, payload));
    }

    TEST_F(ModuleManagerTests, MpiGetReported_MultipleObjects)
    {
        const char componentName[] = "component";
        const char objectName_1[] = "object_1";
        const char objectName_2[] = "object_2";
        char value_1[] = "\"value_1\"";
        char value_2[] = "\"value_2\"";
        char expected[] = R""""(
            {
                "component": {
                    "object_1": "value_1",
                    "object_2": "value_2"
                }
            })"""";

        MPI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        std::shared_ptr<MockManagementModule> mockModule = moduleManager->CreateModule(componentName);
        mockModule->AddReportedObject(componentName, objectName_1);
        mockModule->AddReportedObject(componentName, objectName_2);

        EXPECT_CALL(*mockModule, CallMmiGet(StrEq(componentName), StrEq(objectName_1), _, _)).Times(1).WillOnce(DoAll(SetArgPointee<2>(value_1), SetArgPointee<3>(strlen(value_1)), Return(MMI_OK)));
        EXPECT_CALL(*mockModule, CallMmiGet(StrEq(componentName), StrEq(objectName_2), _, _)).Times(1).WillOnce(DoAll(SetArgPointee<2>(value_2), SetArgPointee<3>(strlen(value_2)), Return(MMI_OK)));

        EXPECT_EQ(MPI_OK, moduleManager->MpiGetReported(&payload, &payloadSizeBytes));
        EXPECT_TRUE(JSON_EQ(expected, payload));
    }

    TEST_F(ModuleManagerTests, MpiGetReported_InvalidPayload)
    {
        int payloadSizeBytes = 0;

        ASSERT_EQ(EINVAL, moduleManager->MpiGetReported(nullptr, &payloadSizeBytes));
        ASSERT_EQ(0, payloadSizeBytes);
    }

    TEST_F(ModuleManagerTests, MpiGetReported_InvalidPayloadSizeBytes)
    {
        MPI_JSON_STRING payload = nullptr;

        ASSERT_EQ(EINVAL, moduleManager->MpiGetReported(&payload, nullptr));
        ASSERT_EQ(nullptr, payload);
    }

    TEST_F(ModuleManagerTests, LoadModules)
    {
        ASSERT_EQ(MPI_OK, moduleManager->LoadModules(g_moduleDir, g_configJsonNoneReported));
    }

    TEST_F(ModuleManagerTests, LoadModules_SingleReported)
    {
        ASSERT_EQ(MPI_OK, moduleManager->LoadModules(g_moduleDir, g_configJsonSingleReported));
    }

    TEST_F(ModuleManagerTests, LoadModules_MultipleReported)
    {
        MPI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        ASSERT_EQ(MPI_OK, moduleManager->LoadModules(g_moduleDir, g_configJsonMultipleReported));
        EXPECT_EQ(MPI_OK, moduleManager->MpiSetDesired((MPI_JSON_STRING)g_localPayload, strlen(g_localPayload)));
        EXPECT_EQ(MPI_OK, moduleManager->MpiGetReported(&payload, &payloadSizeBytes));
        EXPECT_TRUE(JSON_EQ(payload, g_localPayload));
    }

    TEST_F(ModuleManagerTests, LoadModules_InvalidDirectory)
    {
        ASSERT_EQ(ENOENT, moduleManager->LoadModules("/invalid/path", g_configJsonNoneReported));
    }

    TEST_F(ModuleManagerTests, LoadModules_InvalidConfigPath)
    {
        ASSERT_EQ(ENOENT, moduleManager->LoadModules(g_moduleDir, "/invalid/path/config.json"));
    }

    TEST_F(ModuleManagerTests, LoadModules_InvalidConfig)
    {
        ASSERT_EQ(EINVAL, moduleManager->LoadModules(g_moduleDir, g_configJsonInvalid));
    }

    TEST_F(ModuleManagerTests, LoadModules_LatestModuleVersion)
    {
        ASSERT_EQ(MPI_OK, moduleManager->LoadModules(g_moduleDir, g_configJsonNoneReported));

        std::shared_ptr<ManagementModule> module = moduleManager->GetModule(g_testModuleComponent1);
        EXPECT_STREQ("Valid Test Module V2", module->GetName().c_str());
        EXPECT_STREQ("2.0.0.0", module->GetVersion().ToString().c_str());
    }

    TEST_F(ModuleManagerTests, ModuleCleanup_MpiSet)
    {
        moduleManager->SetDefaultCleanupTimespan(1);

        EXPECT_CALL(*defaultModule, CallMmiSet(defaultComponent, defaultObject, defaultPayload, defaultPayloadSize)).Times(1).WillOnce(Return(MMI_OK));
        ASSERT_EQ(MMI_OK, moduleManager->MpiSet(defaultComponent, defaultObject, defaultPayload, defaultPayloadSize));
        ASSERT_EQ(1, moduleManager->GetModulesToUnload().size());

        moduleManager->DoWork();
        ASSERT_EQ(1, moduleManager->GetModulesToUnload().size());
        EXPECT_CALL(*defaultModule, UnloadModule()).Times(1);
        std::this_thread::sleep_for(std::chrono::seconds(2));

        moduleManager->DoWork();
        ASSERT_EQ(0, moduleManager->GetModulesToUnload().size());
    }

    TEST_F(ModuleManagerTests, ModuleCleanup_MpiGet)
    {
        MPI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;
        moduleManager->SetDefaultCleanupTimespan(1);

        EXPECT_CALL(*defaultModule, CallMmiGet(defaultComponent, defaultObject, _, _)).Times(1).WillOnce(DoAll(SetArgPointee<2>(defaultPayload), SetArgPointee<3>(defaultPayloadSize), Return(MMI_OK)));
        ASSERT_EQ(MMI_OK, moduleManager->MpiGet(defaultComponent, defaultObject, &payload, &payloadSizeBytes));
        ASSERT_EQ(1, moduleManager->GetModulesToUnload().size());
        EXPECT_STREQ(defaultPayload, payload);
        EXPECT_EQ(defaultPayloadSize, payloadSizeBytes);

        moduleManager->DoWork();
        ASSERT_EQ(1, moduleManager->GetModulesToUnload().size());
        EXPECT_CALL(*defaultModule, UnloadModule()).Times(1);
        std::this_thread::sleep_for(std::chrono::seconds(2));

        moduleManager->DoWork();
        ASSERT_EQ(0, moduleManager->GetModulesToUnload().size());
    }

} // namespace Tests