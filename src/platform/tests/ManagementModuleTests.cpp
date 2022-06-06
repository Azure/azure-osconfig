// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <CommonTests.h>
#include <PlatformCommon.h>
#include <ManagementModule.h>
#include <MockManagementModule.h>
#include <ModulesManagerTests.h>
#include <Mpi.h>
namespace Tests
{
    class ManagementModuleTests : public ::testing::Test
    {
    public:
        std::shared_ptr<MockManagementModule> m_mockModule;
        std::shared_ptr<MmiSession> m_mmiSession;

        static const char m_defaultClient[];
        static const char m_defaultComponent[];
        static const char m_defaultObject[];

        static const std::vector<std::tuple<const char*, const char *>> objectPayloads;

        void SetUp() override;
        void TearDown() override;
    };

    const char ManagementModuleTests::m_defaultClient[] = "Default_ManagementModuleTest_Client";
    const char ManagementModuleTests::m_defaultComponent[] = "Default_ManagementModuleTest_Component";
    const char ManagementModuleTests::m_defaultObject[] = "Default_ManagementModuleTest_Object";

    void ManagementModuleTests::SetUp()
    {
        m_mockModule = std::make_shared<MockManagementModule>();

        m_mmiSession = std::make_shared<MmiSession>(m_mockModule, m_defaultClient);
        ASSERT_EQ(0, m_mmiSession->Open());
    }

    void ManagementModuleTests::TearDown()
    {
        m_mmiSession->Close();
        m_mmiSession.reset();
        m_mockModule.reset();
    }

    TEST_F(ManagementModuleTests, LoadModule)
    {
        ManagementModule module(g_validModulePathV1);
        EXPECT_EQ(0, module.Load());

        ManagementModule::Info info = module.GetInfo();
        EXPECT_STREQ("Valid Test Module", info.name.c_str());
        EXPECT_STREQ("1.0.0.0", info.version.ToString().c_str());
        EXPECT_EQ(ManagementModule::Lifetime::Short, info.lifetime);
    }

    TEST_F(ManagementModuleTests, LoadModule_InvalidPath)
    {
        const std::string invalidPath = g_moduleDir;
        ManagementModule invalidModule(invalidPath + "/blah.so");
        EXPECT_EQ(EINVAL, invalidModule.Load());
    }

    TEST_F(ManagementModuleTests, LoadModule_InvalidMmi)
    {
        ManagementModule invalidModule(g_invalidModulePath);
        EXPECT_EQ(EINVAL, invalidModule.Load());
    }

    TEST_F(ManagementModuleTests, LoadModule_InvalidModuleInfo)
    {
        ManagementModule invalidModule(g_invalidGetInfoModulePath);
        EXPECT_EQ(EINVAL, invalidModule.Load());
    }

    TEST_F(ManagementModuleTests, CallMmiSet)
    {
        char componentName[] = "component_name";
        char objectName[] = "object_name";
        char payload[] = "\"payload\"";
        int payloadSize = strlen(payload);

        // Provide a mock implmenation of CallMmiSet()
        ON_CALL(*m_mockModule, CallMmiSet).WillByDefault(
            [](MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes) -> int
            {
                (void)clientSession;
                if ((0 == strcmp(componentName, "component_name")) && (0 == strcmp(objectName, "object_name")) && (0 == strcmp(payload, "\"payload\"")) && (payloadSizeBytes == strlen("\"payload\"")))
                {
                    return 0;
                }
                return -1;
            });

        ASSERT_EQ(MMI_OK, m_mmiSession->Set(componentName, objectName, (MMI_JSON_STRING)payload, payloadSize));
    }

    TEST_F(ManagementModuleTests, CallMmiGet)
    {
        char componentName[] = "component_name";
        char objectName[] = "object_name";
        char expectedPayload[] = "\"payload\"";
        MMI_JSON_STRING payload = nullptr;
        int payloadSize = 0;

        // Provide a mock implmenation of CallMmiGet()
        ON_CALL(*m_mockModule, CallMmiGet).WillByDefault(
           [](MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes) -> int
            {
                (void)clientSession;
                int status = 0;

                if ((0 == strcmp(componentName, "component_name")) && (0 == strcmp(objectName, "object_name")))
                {
                    const char* buffer = "\"payload\"";
                    int size = strlen(buffer);

                    *payload = nullptr;
                    *payloadSizeBytes = 0;

                    *payload = new (std::nothrow) char[size];

                    if (nullptr == *payload)
                    {
                        status = ENOMEM;
                    }
                    else
                    {
                        std::fill(*payload, *payload + size + 1, 0);
                        std::memcpy(*payload, buffer, size);
                        *payloadSizeBytes = size;
                    }
                }
                else
                {
                    status = -1;
                }

                return status;
            });

        EXPECT_EQ(MMI_OK, m_mmiSession->Get(componentName, objectName, &payload, &payloadSize));
        EXPECT_STREQ(expectedPayload, payload);
        EXPECT_EQ(strlen(expectedPayload), payloadSize);
    }

    TEST_F(ManagementModuleTests, PayloadValidation)
    {
        MockManagementModule mockModule;
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

        mockModule.MmiSet(
            [](MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes) -> int
            {
                // MmiSet
                (void)(clientSession);
                (void)(componentName);
                (void)(objectName);
                (void)(payload);
                (void)(payloadSizeBytes);
                return MMI_OK;
            });

        for (auto object : objects)
        {
            const char* objectName = object.first.c_str();
            const char* payload = object.second.c_str();
            EXPECT_EQ(MMI_OK, m_mmiSession->Set(m_defaultComponent, objectName, (MMI_JSON_STRING)payload, strlen(payload)));
        }

    }

    TEST(ManagementModuleVersionTests, Version)
    {
        ManagementModule::Version v1 = {1,0,0,0};
        ManagementModule::Version v1a = {1,0};
        ManagementModule::Version v2 = {2,0,0,0};
        ManagementModule::Version v2b = {2,1};
        ManagementModule::Version v01 = {0,1,0,0};
        ManagementModule::Version v02 = {0,2,0,0};
        ManagementModule::Version v101 = {1,0,1,0};
        ManagementModule::Version v001a = {0,0,1};
        ManagementModule::Version v002 = {0,0,2,0};
        ManagementModule::Version v002b = {0,0,2};
        ManagementModule::Version v0001 = {0,0,0,1};
        ManagementModule::Version v0002 = {0,0,0,2};

        ASSERT_LT(v1, v2);
        ASSERT_LT(v1a, v2b);
        ASSERT_LT(v1, v101);
        ASSERT_LT(v01, v1);
        ASSERT_LT(v01, v02);
        ASSERT_LT(v02, v2);
        ASSERT_LT(v0001, v0002);
        ASSERT_LT(v0001, v1);
        ASSERT_LT(v001a, v002b);

        ASSERT_TRUE(v1 < v2);
        ASSERT_TRUE(v01 < v02);
        ASSERT_TRUE(v0002 < v02);
        ASSERT_TRUE(v002 < v02);

        ASSERT_FALSE(v1 < v02);
        ASSERT_FALSE(v2 < v1);
        ASSERT_FALSE(v2 < v002);
        ASSERT_FALSE(v2b < v1a);
        ASSERT_FALSE(v002b < v001a);
    }

    TEST(ManagementModuleVersionTests, VersionString)
    {
        ManagementModule::Version v = {1,2,3,4};
        ManagementModule::Version v1 = {0};
        ManagementModule::Version v2 = {0,0,1};
        ManagementModule::Version v3 = {0,0,0,1};

        ASSERT_STREQ("1.2.3.4", v.ToString().c_str());
        ASSERT_STREQ("0.0.0.0", v1.ToString().c_str());
        ASSERT_STREQ("0.0.1.0", v2.ToString().c_str());
        ASSERT_STREQ("0.0.0.1", v3.ToString().c_str());
    }
}