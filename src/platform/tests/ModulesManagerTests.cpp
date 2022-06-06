// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <rapidjson/document.h>

#include <PlatformCommon.h>
#include <ManagementModule.h>
#include <ModulesManager.h>
#include <MockManagementModule.h>
#include <MockModulesManager.h>
#include <CommonTests.h>
#include <CommonUtils.h>
#include <ModulesManagerTests.h>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrEq;
using ::testing::StrictMock;

namespace Tests
{
    class ModuleManagerTests : public ::testing::Test
    {
    protected:
        std::shared_ptr<MockModulesManager> m_mockModuleManager;
        std::shared_ptr<StrictMock<MockManagementModule>> m_mockModule;
        std::shared_ptr<MpiSession> m_mpiSession;

        static const char m_defaultClient[];
        static const char m_defaultComponent[];
        static const char m_defaultObject[];
        static char m_defaultPayload[];
        static const int m_defaultPayloadSize;

        void SetUp() override;
        void TearDown() override;
    };

    const char ModuleManagerTests::m_defaultClient[] = "Default_ModuleManagerTest_Client";
    const char ModuleManagerTests::m_defaultComponent[] = "Default_ModuleManagerTest_Component";
    const char ModuleManagerTests::m_defaultObject[] = "Default_ModuleManagerTest_Object";
    char ModuleManagerTests::m_defaultPayload[] = "\"Default_ModuleManagerTest_Payload\"";
    const int ModuleManagerTests::m_defaultPayloadSize = ARRAY_SIZE(ModuleManagerTests::m_defaultPayload) - 1;

    void ModuleManagerTests::SetUp()
    {
        // Create a ModulesManager
        this->m_mockModuleManager = std::make_shared<MockModulesManager>();

        // Create a mock ManagementModule and "Load" it into the ModulesManager
        this->m_mockModule = std::make_shared<StrictMock<MockManagementModule>>("Default_Module_Name", std::vector<std::string>({m_defaultComponent}));
        this->m_mockModuleManager->Load(this->m_mockModule);

        // "Open" an MpiSession using the ModulesManager with the mock module loaded
        this->m_mpiSession = std::make_shared<MpiSession>(*m_mockModuleManager, m_defaultClient);
        ASSERT_EQ(0, this->m_mpiSession->Open());
    }

    void ModuleManagerTests::TearDown()
    {
        this->m_mpiSession.reset();
        this->m_mockModule.reset();
        this->m_mockModuleManager.reset();
    }

    TEST_F(ModuleManagerTests, MpiSet)
    {
        EXPECT_CALL(*m_mockModule, CallMmiSet(_, m_defaultComponent, m_defaultObject, m_defaultPayload, m_defaultPayloadSize)).Times(1).WillOnce(Return(MMI_OK));
        ASSERT_EQ(MPI_OK, m_mpiSession->Set(m_defaultComponent, m_defaultObject, m_defaultPayload, m_defaultPayloadSize));
    }

    TEST_F(ModuleManagerTests, MpiSet_InvalidComponentName)
    {
        ASSERT_EQ(EINVAL, m_mpiSession->Set(nullptr, m_defaultObject, m_defaultPayload, m_defaultPayloadSize));
    }

    TEST_F(ModuleManagerTests, MpiSet_InvalidObjectName)
    {
        ASSERT_EQ(EINVAL, m_mpiSession->Set(m_defaultComponent, nullptr, m_defaultPayload, m_defaultPayloadSize));
    }

    TEST_F(ModuleManagerTests, MpiSet_InvalidPayload)
    {
        ASSERT_EQ(EINVAL, m_mpiSession->Set(m_defaultComponent, m_defaultObject, nullptr, 0));
    }

    TEST_F(ModuleManagerTests, MpiGet)
    {
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;
        char expected[] = "\"expected\"";

        EXPECT_CALL(*m_mockModule, CallMmiGet(_, m_defaultComponent, m_defaultObject, _, _)).Times(1).WillOnce(DoAll(SetArgPointee<3>(expected), SetArgPointee<4>(strlen(expected)), Return(MMI_OK)));

        EXPECT_EQ(MPI_OK, m_mpiSession->Get(m_defaultComponent, m_defaultObject, &payload, &payloadSizeBytes));
        EXPECT_STREQ(expected, payload);
        EXPECT_EQ(strlen(expected), payloadSizeBytes);
    }

    TEST_F(ModuleManagerTests, MpiGet_InvalidComponentName)
    {
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        EXPECT_EQ(EINVAL, m_mpiSession->Get(nullptr, m_defaultObject, &payload, &payloadSizeBytes));
        EXPECT_EQ(nullptr, payload);
        EXPECT_EQ(0, payloadSizeBytes);
    }

    TEST_F(ModuleManagerTests, MpiGet_InvalidObjectName)
    {
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        EXPECT_EQ(EINVAL, m_mpiSession->Get(m_defaultComponent, nullptr, &payload, &payloadSizeBytes));
        EXPECT_EQ(nullptr, payload);
        EXPECT_EQ(0, payloadSizeBytes);
    }

    TEST_F(ModuleManagerTests, MpiGet_InvalidPayload)
    {
        int payloadSizeBytes = 0;

        EXPECT_EQ(EINVAL, m_mpiSession->Get(m_defaultComponent, m_defaultObject, nullptr, &payloadSizeBytes));
        EXPECT_EQ(0, payloadSizeBytes);
    }

    TEST_F(ModuleManagerTests, MpiGet_InvalidPayloadSize)
    {
        MMI_JSON_STRING payload = nullptr;

        EXPECT_EQ(EINVAL, m_mpiSession->Get(m_defaultComponent, m_defaultObject, &payload, nullptr));
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

        std::shared_ptr<MockManagementModule> mockModule = std::make_shared<MockManagementModule>("mockModule", std::vector<std::string>({componentName}));
        m_mockModuleManager->Load(mockModule);

        std::shared_ptr<MpiSession> mpiSession = std::make_shared<MpiSession>(*m_mockModuleManager, m_defaultClient);
        EXPECT_EQ(0, mpiSession->Open());

        EXPECT_CALL(*mockModule, CallMmiSet(_, StrEq(componentName), StrEq(objectName), StrEq(value), strlen(value))).Times(1).WillOnce(Return(MMI_OK));
        ASSERT_EQ(MPI_OK, mpiSession->SetDesired(payload, strlen(payload)));
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

        std::shared_ptr<MockManagementModule> mockModule_1 = std::make_shared<MockManagementModule>("mockModule_1", std::vector<std::string>({componentName_1}));
        std::shared_ptr<MockManagementModule> mockModule_2 = std::make_shared<MockManagementModule>("mockModule_2", std::vector<std::string>({componentName_2}));

        m_mockModuleManager->Load(mockModule_1);
        m_mockModuleManager->Load(mockModule_2);

        std::shared_ptr<MpiSession> mpiSession = std::make_shared<MpiSession>(*m_mockModuleManager, m_defaultClient);
        EXPECT_EQ(0, mpiSession->Open());

        EXPECT_CALL(*mockModule_1, CallMmiSet(_, StrEq(componentName_1), StrEq(objectName_1), StrEq(value_1), strlen(value_1))).Times(1).WillOnce(Return(MMI_OK));
        EXPECT_CALL(*mockModule_2, CallMmiSet(_, StrEq(componentName_2), StrEq(objectName_2), StrEq(value_2), strlen(value_2))).Times(1).WillOnce(Return(MMI_OK));

        ASSERT_EQ(MPI_OK, mpiSession->SetDesired(payload, strlen(payload)));
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

        std::shared_ptr<MockManagementModule> mockModule = std::make_shared<MockManagementModule>("mockModule", std::vector<std::string>({componentName}));
        m_mockModuleManager->Load(mockModule);

        std::shared_ptr<MpiSession> mpiSession = std::make_shared<MpiSession>(*m_mockModuleManager, m_defaultClient);
        EXPECT_EQ(0, mpiSession->Open());

        EXPECT_CALL(*mockModule, CallMmiSet(_, StrEq(componentName), StrEq(objectName_1), StrEq(value_1), strlen(value_1))).Times(1).WillOnce(Return(MMI_OK));
        EXPECT_CALL(*mockModule, CallMmiSet(_, StrEq(componentName), StrEq(objectName_2), StrEq(value_2), strlen(value_2))).Times(1).WillOnce(Return(MMI_OK));

        EXPECT_EQ(MPI_OK, mpiSession->SetDesired(payload, strlen(payload)));
    }

    TEST_F(ModuleManagerTests, MpiSetDesired_InvalidJsonPayload)
    {
        char invalid[] = "invalid";
        EXPECT_EQ(EINVAL, m_mpiSession->SetDesired(invalid, strlen(invalid)));
    }

    TEST_F(ModuleManagerTests, MpiSetDesired_InvalidJsonSchema)
    {
        char invalid[] = R""""([
            {
                "component": {
                    "object": "value"
                }
            }])"""";

        EXPECT_EQ(EINVAL, m_mpiSession->SetDesired(invalid, strlen(invalid)));
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

        std::shared_ptr<MockManagementModule> mockModule = std::make_shared<MockManagementModule>("mockModule", std::vector<std::string>({componentName}));

        m_mockModuleManager->Load(mockModule);
        m_mockModuleManager->AddReportedObject(componentName, objectName);

        std::shared_ptr<MpiSession> mpiSession = std::make_shared<MpiSession>(*m_mockModuleManager, m_defaultClient);
        EXPECT_EQ(0, mpiSession->Open());

        EXPECT_CALL(*mockModule, CallMmiGet(_, StrEq(componentName), StrEq(objectName), _, _)).Times(1).WillOnce(DoAll(SetArgPointee<3>(value), SetArgPointee<4>(strlen(value)), Return(MMI_OK)));
        EXPECT_EQ(MPI_OK, mpiSession->GetReported(&payload, &payloadSizeBytes));

        std::string actual(payload, payloadSizeBytes);
        EXPECT_TRUE(JSON_EQ(expected, actual));
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

        std::shared_ptr<MockManagementModule> mockModule_1 = std::make_shared<MockManagementModule>("mockModule_1", std::vector<std::string>({componentName_1}));
        std::shared_ptr<MockManagementModule> mockModule_2 = std::make_shared<MockManagementModule>("mockModule_2", std::vector<std::string>({componentName_2}));

        m_mockModuleManager->Load(mockModule_1);
        m_mockModuleManager->Load(mockModule_2);
        m_mockModuleManager->AddReportedObject(componentName_1, objectName_1);
        m_mockModuleManager->AddReportedObject(componentName_2, objectName_2);

        EXPECT_CALL(*mockModule_1, CallMmiGet(_, StrEq(componentName_1), StrEq(objectName_1), _, _)).Times(1).WillOnce(DoAll(SetArgPointee<3>(value_1), SetArgPointee<4>(strlen(value_1)), Return(MMI_OK)));
        EXPECT_CALL(*mockModule_2, CallMmiGet(_, StrEq(componentName_2), StrEq(objectName_2), _, _)).Times(1).WillOnce(DoAll(SetArgPointee<3>(value_2), SetArgPointee<4>(strlen(value_2)), Return(MMI_OK)));

        std::shared_ptr<MpiSession> mpiSession = std::make_shared<MpiSession>(*m_mockModuleManager, m_defaultClient);
        EXPECT_EQ(0, mpiSession->Open());
        EXPECT_EQ(MPI_OK, mpiSession->GetReported(&payload, &payloadSizeBytes));

        std::string actual(payload, payloadSizeBytes);
        EXPECT_TRUE(JSON_EQ(expected, actual));
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

        std::shared_ptr<MockManagementModule> mockModule = std::make_shared<MockManagementModule>("mockModule", std::vector<std::string>({ componentName }));

        m_mockModuleManager->Load(mockModule);
        m_mockModuleManager->AddReportedObject(componentName, objectName_1);
        m_mockModuleManager->AddReportedObject(componentName, objectName_2);

        EXPECT_CALL(*mockModule, CallMmiGet(_, StrEq(componentName), StrEq(objectName_1), _, _)).Times(1).WillOnce(DoAll(SetArgPointee<3>(value_1), SetArgPointee<4>(strlen(value_1)), Return(MMI_OK)));
        EXPECT_CALL(*mockModule, CallMmiGet(_, StrEq(componentName), StrEq(objectName_2), _, _)).Times(1).WillOnce(DoAll(SetArgPointee<3>(value_2), SetArgPointee<4>(strlen(value_2)), Return(MMI_OK)));

        std::shared_ptr<MpiSession> mpiSession = std::make_shared<MpiSession>(*m_mockModuleManager, m_defaultClient);
        EXPECT_EQ(0, mpiSession->Open());
        EXPECT_EQ(MPI_OK, mpiSession->GetReported(&payload, &payloadSizeBytes));

        std::string actual(payload, payloadSizeBytes);
        EXPECT_TRUE(JSON_EQ(expected, actual));
    }

    TEST_F(ModuleManagerTests, MpiGetReported_InvalidPayload)
    {
        int payloadSizeBytes = 0;

        ASSERT_EQ(EINVAL, m_mpiSession->GetReported(nullptr, &payloadSizeBytes));
        ASSERT_EQ(0, payloadSizeBytes);
    }

    TEST_F(ModuleManagerTests, MpiGetReported_InvalidPayloadSizeBytes)
    {
        MPI_JSON_STRING payload = nullptr;

        ASSERT_EQ(EINVAL, m_mpiSession->GetReported(&payload, nullptr));
        ASSERT_EQ(nullptr, payload);
    }

    TEST_F(ModuleManagerTests, LoadModules)
    {
        MPI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;
        const char emptyPayload[] = "{}";

        ASSERT_EQ(MPI_OK, m_mockModuleManager->LoadModules(g_moduleDir, g_configJsonNoneReported));

        std::shared_ptr<MpiSession> mpiSession = std::make_shared<MpiSession>(*m_mockModuleManager, m_defaultClient);
        EXPECT_EQ(0, mpiSession->Open());

        EXPECT_EQ(MPI_OK, mpiSession->SetDesired((MPI_JSON_STRING)emptyPayload, strlen(emptyPayload)));
        EXPECT_EQ(MPI_OK, mpiSession->GetReported(&payload, &payloadSizeBytes));

        std::string actual(payload, payloadSizeBytes);
        EXPECT_TRUE(JSON_EQ(emptyPayload, actual));
    }

    TEST_F(ModuleManagerTests, LoadModules_SingleReported)
    {
        MPI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        ASSERT_EQ(MPI_OK, m_mockModuleManager->LoadModules(g_moduleDir, g_configJsonSingleReported));

        std::shared_ptr<MpiSession> mpiSession = std::make_shared<MpiSession>(*m_mockModuleManager, m_defaultClient);
        EXPECT_EQ(0, mpiSession->Open());

        EXPECT_EQ(MPI_OK, mpiSession->SetDesired((MPI_JSON_STRING)g_singleObjectPayload, strlen(g_singleObjectPayload)));
        EXPECT_EQ(MPI_OK, mpiSession->GetReported(&payload, &payloadSizeBytes));

        std::string actual(payload, payloadSizeBytes);
        EXPECT_TRUE(JSON_EQ(g_singleObjectPayload, actual));
    }

    TEST_F(ModuleManagerTests, LoadModules_MultipleReported)
    {
        MPI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        ASSERT_EQ(MPI_OK, m_mockModuleManager->LoadModules(g_moduleDir, g_configJsonMultipleReported));

        std::shared_ptr<MpiSession> mpiSession = std::make_shared<MpiSession>(*m_mockModuleManager, m_defaultClient);
        EXPECT_EQ(0, mpiSession->Open());

        EXPECT_EQ(MPI_OK, mpiSession->SetDesired((MPI_JSON_STRING)g_multipleObjectsPayload, strlen(g_multipleObjectsPayload)));
        EXPECT_EQ(MPI_OK, mpiSession->GetReported(&payload, &payloadSizeBytes));

        std::string actual(payload, payloadSizeBytes);
        EXPECT_TRUE(JSON_EQ(g_multipleObjectsPayload, actual));
    }

    TEST_F(ModuleManagerTests, LoadModules_InvalidDirectory)
    {
        ASSERT_EQ(ENOENT, m_mockModuleManager->LoadModules("/invalid/path", g_configJsonNoneReported));
    }

    TEST_F(ModuleManagerTests, LoadModules_InvalidConfigPath)
    {
        ASSERT_EQ(ENOENT, m_mockModuleManager->LoadModules(g_moduleDir, "/invalid/path/config.json"));
    }

    TEST_F(ModuleManagerTests, LoadModules_InvalidConfig)
    {
        ASSERT_EQ(EINVAL, m_mockModuleManager->LoadModules(g_moduleDir, g_configJsonInvalid));
    }
} // namespace Tests