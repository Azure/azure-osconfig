// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <rapidjson/document.h>

#include <CommonUtils.h>
#include <CommonTests.h>
#include <ManagementModule.h>
#include <ModulesManager.h>
#include <ModulesManagerTests.h>
#include <Mpi.h>

const std::string clientName;

const std::string libInvalidModuleSo = "libinvalidmodule.so";
const std::string libGoodModuleSo = "libgoodmodule.so";
const std::string libInvalidSchemaModuleSo = "libinvalidschemamodule.so";
const std::string libMultiComponentModuleSo = "libmulticomponentmodule.so";

const std::string componentNameLongRunningModule = "LongRunningModule";
const std::string componentName1 = "TestComponent1";
const std::string componentName2 = "TestComponent2";

using testing::AtLeast;
using testing::ElementsAre;

namespace Tests
{
    class ModuleManagerTests : public ::testing::Test
    {
    protected:
        std::unique_ptr<ModulesManager> mm;
        ModuleManagerTests()
        {
            mm.reset(new ModulesManager("ModuleManagerTests"));
        }

        virtual void SetUp()
        {
            ASSERT_EQ(0, mm->LoadModules(MODULE_TEST_PATH));
        }

        virtual void TearDown()
        {
            mm.reset();
        }
    };

    TEST_F(ModuleManagerTests, LoadSomeInvalidDirectory)
    {
        ASSERT_EQ(ENOENT, mm->LoadModules("/some/bad/path/blahblah"));
    }

    TEST_F(ModuleManagerTests, MpiGetDispatch)
    {
        int szPayload;
        MMI_JSON_STRING payload;
        ASSERT_EQ(MMI_OK, mm->MpiGet(componentName1.c_str(), "", &payload, &szPayload));
        ASSERT_TRUE(szPayload > 0);
        std::string payloadStr(payload, szPayload);

        constexpr const char expectedStr[] = R""""( { "returnValue": "TestComponent1-MultiComponentModule" } )"""";
        ASSERT_TRUE(JSON_EQ(expectedStr, payloadStr.c_str()));
        delete[] payload;
    }

    TEST_F(ModuleManagerTests, MpiGetDispatchOverrideComponent)
    {
        int szPayload;
        MMI_JSON_STRING payload;
        ASSERT_EQ(MMI_OK, mm->MpiGet(componentName2.c_str(), "", &payload, &szPayload));
        ASSERT_TRUE(szPayload > 0);
        std::string payloadStr(payload, szPayload);

        constexpr const char expectedStr[] = R""""( {"returnValue": "TestComponent2-MultiComponentTheLargestVersionModule"} )"""";
        ASSERT_TRUE(JSON_EQ(expectedStr, payloadStr.c_str()));
        delete[] payload;
    }

    TEST_F(ModuleManagerTests, MpiSetDispatch)
    {
        constexpr const char payload[] = R""""( {"testParameter": "testValue"} )"""";
        ASSERT_EQ(MMI_OK, mm->MpiSet(componentName1.c_str(), "", payload, ARRAY_SIZE(payload)));
    }

    TEST_F(ModuleManagerTests, ModuleCleanup)
    {
        constexpr const char payload[] = R""""( {"testParameter": "testValue"} )"""";
        mm->SetDefaultCleanupTimespan(5);
        ASSERT_EQ(MMI_OK, mm->MpiSet(componentName1.c_str(), "", payload, ARRAY_SIZE(payload)));
        ASSERT_EQ(1, mm->modulesToUnload.size());
        mm->DoWork();
        ASSERT_EQ(1, mm->modulesToUnload.size());
        std::this_thread::sleep_for(std::chrono::seconds(10));
        mm->DoWork();
        ASSERT_EQ(0, mm->modulesToUnload.size());
    }

    TEST(ManagementModuleTests, LoadInvalidModule)
    {
        std::string module_path = MODULE_TEST_PATH;
        module_path = module_path.append("/").append(libInvalidModuleSo);
        ASSERT_EQ(false, ManagementModule::IsExportingMmi(module_path));
    }

    TEST(ManagementModuleTests, LoadNormalModule)
    {
        std::string module_path = MODULE_TEST_PATH;
        module_path = module_path.append("/").append(libGoodModuleSo);
        ASSERT_EQ(true, ManagementModule::IsExportingMmi(module_path));
    }

    TEST(ManagementModuleTests, LoadInvalidShemaModule)
    {
        std::string module_path = MODULE_TEST_PATH;
        module_path = module_path.append("/").append(libInvalidSchemaModuleSo);
        ASSERT_EQ(true, ManagementModule::IsExportingMmi(module_path));
        ManagementModule mm(clientName, module_path);
        ASSERT_EQ(false, mm.IsValid());
    }

    TEST(ManagementModuleTests, CreateModule)
    {
        std::string module_path = MODULE_TEST_PATH;
        module_path = module_path.append("/").append(libGoodModuleSo);
        ManagementModule mm(clientName, module_path);
        ASSERT_EQ(true, mm.IsValid());
        EXPECT_THAT(mm.GetSupportedComponents(), ElementsAre("NormalModule"));
    }

    TEST(ManagementModuleTests, CreateModule_Multiple_Components)
    {
        std::string module_path = MODULE_TEST_PATH;
        module_path = module_path.append("/").append(libMultiComponentModuleSo);
        ManagementModule mm(clientName, module_path);
        ASSERT_EQ(true, mm.IsValid());
        EXPECT_THAT(mm.GetSupportedComponents(), ElementsAre("TestComponent1", "TestComponent2"));
    }

    TEST(ManagementModuleTests, VersionTests)
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

    TEST(ManagementModuleTests, VersionStringTests)
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

} // namespace Tests