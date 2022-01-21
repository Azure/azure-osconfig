// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <CommonTests.h>
#include <ManagementModule.h>
#include <MockManagementModule.h>
#include <ModulesManagerTests.h>
#include <Mpi.h>

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Values;

namespace Tests
{
    class ManagementModuleTests : public ::testing::Test
    {
    public:
        std::shared_ptr<MockManagementModule> module;

        static const char defaultClient[];
        static const char defaultComponent[];
        static const char defaultObject[];

        static const std::vector<std::tuple<const char*, const char *>> objectPayloads;

        void SetUp() override;
        void TearDown() override;
    };

    const char ManagementModuleTests::defaultClient[] = "Default_ManagementModuleTest_Client";
    const char ManagementModuleTests::defaultComponent[] = "Default_ManagementModuleTest_Component";
    const char ManagementModuleTests::defaultObject[] = "Default_ManagementModuleTest_Object";

    void ManagementModuleTests::SetUp()
    {
        module = std::make_shared<MockManagementModule>(defaultClient);
    }

    void ManagementModuleTests::TearDown()
    {
        module.reset();
    }

    TEST_F(ManagementModuleTests, LoadModule)
    {
        ManagementModule module(defaultClient, g_validModulePathV1);

        module.LoadModule();

        ASSERT_TRUE(module.IsValid());
        ASSERT_TRUE(module.IsLoaded());

        // Check info parsed from module GetInfo() call
        EXPECT_STREQ("Valid Test Module V1", module.GetName().c_str());
        EXPECT_STREQ("1.0.0.0", module.GetVersion().ToString().c_str());
        EXPECT_EQ(ManagementModule::Lifetime::Short, module.GetLifetime());
        EXPECT_THAT(module.GetSupportedComponents(), ElementsAre(g_testModuleComponent1, g_testModuleComponent2));

        module.UnloadModule();

        ASSERT_TRUE(module.IsValid());
        ASSERT_FALSE(module.IsLoaded());
    }

    TEST_F(ManagementModuleTests, LoadModule_InvalidPath)
    {
        const std::string invalidPath = g_moduleDir;
        ManagementModule invalidModule(defaultClient, (invalidPath + "/blah.so"));

        ASSERT_FALSE(invalidModule.IsValid());
        ASSERT_FALSE(invalidModule.IsLoaded());
        EXPECT_THAT(invalidModule.GetSupportedComponents(), IsEmpty());
    }

    TEST_F(ManagementModuleTests, LoadModule_InvalidMmi)
    {
        ManagementModule invalidModule(defaultClient, g_invalidModulePath);

        ASSERT_FALSE(invalidModule.IsValid());
        ASSERT_FALSE(invalidModule.IsLoaded());
        EXPECT_THAT(invalidModule.GetSupportedComponents(), IsEmpty());
    }

    TEST_F(ManagementModuleTests, LoadModule_InvalidModuleInfo)
    {
        ManagementModule invalidModule(defaultClient, g_invalidGetInfoModulePath);

        ASSERT_FALSE(invalidModule.IsValid());
        ASSERT_FALSE(invalidModule.IsLoaded());
        EXPECT_THAT(invalidModule.GetSupportedComponents(), IsEmpty());
    }

    TEST_F(ManagementModuleTests, ReportedObjects)
    {
        ManagementModule module(defaultClient, g_validModulePathV1);

        const char object_1[] = "object_1";
        const char object_2[] = "object_2";
        const char object_3[] = "object_3";

        module.AddReportedObject(g_testModuleComponent1, object_1);
        EXPECT_THAT(module.GetReportedObjects(g_testModuleComponent1), ElementsAre(object_1));
        EXPECT_THAT(module.GetReportedObjects(g_testModuleComponent2), IsEmpty());

        module.AddReportedObject(g_testModuleComponent1, object_2);
        EXPECT_THAT(module.GetReportedObjects(g_testModuleComponent1), ElementsAre(object_1, object_2));
        EXPECT_THAT(module.GetReportedObjects(g_testModuleComponent2), IsEmpty());

        module.AddReportedObject(g_testModuleComponent2, object_3);
        EXPECT_THAT(module.GetReportedObjects(g_testModuleComponent1), ElementsAre(object_1, object_2));
        EXPECT_THAT(module.GetReportedObjects(g_testModuleComponent2), ElementsAre(object_3));
    }

    TEST_F(ManagementModuleTests, CallMmiSet)
    {
        MockManagementModule module(defaultClient);
        char componentName[] = "component_name";
        char objectName[] = "object_name";
        char payload[] = "\"payload\"";
        int payloadSize = strlen(payload);

        // Provide a mock implmenation of an MmiSet() function pointer to the module
        module.MmiSet(
            [](MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes) -> int
            {
                (void)clientSession;
                if ((0 == strcmp(componentName, "component_name")) && (0 == strcmp(objectName, "object_name")) && (0 == strcmp(payload, "\"payload\"")) && (payloadSizeBytes == strlen("\"payload\"")))
                {
                    return 0;
                }
                return -1;
            });

        // Delegate mock call to parent class and call mmiSet function pointer
        EXPECT_CALL(module, CallMmiSet(componentName, objectName, (MMI_JSON_STRING)payload, payloadSize)).Times(1).WillOnce(
            [&module](const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes) -> int
            {
                return module.ManagementModule::CallMmiSet(componentName, objectName, payload, payloadSizeBytes);
            });

        EXPECT_CALL(module, LoadModule()).Times(1);
        ASSERT_EQ(0, module.CallMmiSet(componentName, objectName, (MMI_JSON_STRING)payload, payloadSize));
    }

    TEST_F(ManagementModuleTests, CallMmiGet)
    {
        MockManagementModule module(defaultClient);
        char componentName[] = "component_name";
        char objectName[] = "object_name";
        char expectedPayload[] = "\"payload\"";
        MMI_JSON_STRING payload = nullptr;
        int payloadSize = 0;

        // Provide a mock implmenation of an MmiGet() function pointer to the module that returns a payload and payloadSize
        module.MmiGet(
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
                        std::fill(*payload, *payload + size, 0);
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

        // Delegate mock call to parent class and call mmiGet function pointer
        EXPECT_CALL(module, CallMmiGet(componentName, objectName, _, _)).Times(1).WillOnce(
            [&module](const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes) -> int
            {
                return module.ManagementModule::CallMmiGet(componentName, objectName, payload, payloadSizeBytes);
            });

        EXPECT_CALL(module, LoadModule()).Times(1);
        ASSERT_EQ(0, module.CallMmiGet(componentName, objectName, &payload, &payloadSize));
        ASSERT_STREQ(expectedPayload, payload);
        ASSERT_EQ(strlen(expectedPayload), payloadSize);
    }

    TEST_F(ManagementModuleTests, PayloadValidation)
    {
        MockManagementModule module(defaultClient);
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

        module.MmiSet(
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
            // Delegate mock call to parent class and call mmiSet function pointer
            EXPECT_CALL(module, CallMmiSet(defaultComponent, objectName, (MMI_JSON_STRING)payload, strlen(payload))).Times(1).WillOnce(
                [&module](const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes) -> int
                {
                    return module.ManagementModule::CallMmiSet(componentName, objectName, payload, payloadSizeBytes);
                });

            EXPECT_CALL(module, LoadModule()).Times(1);
            ASSERT_EQ(MMI_OK, module.CallMmiSet(defaultComponent, objectName, (MMI_JSON_STRING)payload, strlen(payload)));
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