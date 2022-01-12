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

const std::string componentNameLongRunningModule = "LongRunningModule";
const std::string componentName1 = "TestComponent1";
const std::string componentName2 = "TestComponent2";
const std::string invalidComponent= "InvalidComponent";
const std::string payloadValidationComponent = "PayloadValidationComponent";
const std::string objectName = "TestObject";

 // Uses MPI directly
namespace Tests
{
    class MpiTests : public ::testing::Test
    {
    protected:
        MPI_HANDLE h;
        virtual void SetUp(std::string testCaseName, unsigned int maxPayloadsize)
        {
            const char* clientName = testCaseName.c_str();
            h = MpiOpen(clientName, maxPayloadsize);
            ASSERT_TRUE(0 != h);

            // By default MpiOpen uses /usr/lib/osconfig to load modules. Unload default modules and load test modules.
            static_cast<ModulesManager*>(h)->UnloadAllModules();
            static_cast<ModulesManager*>(h)->LoadModules(MODULE_TEST_PATH, OSCONFIG_JSON_SINGLE_REPORTED);
        }

        virtual void SetUp(std::string testCaseName)
        {
            SetUp(testCaseName, 0);
        }

        virtual void TearDown()
        {
            MpiClose(h);
        }
    };

    TEST_F(MpiTests, MpiOpenRepeat)
    {
        SetUp("MpiTests.MpiOpenRepeat");
        ASSERT_EQ(h, MpiOpen("MpiTests.MpiOpenRepeat", 123));
        ASSERT_EQ(h, MpiOpen("MpiTests.MpiOpenRepeat", 4567));
    }

    TEST_F(MpiTests, MpiSet)
    {
        SetUp("MpiTests.MpiSet");
        char payload[] = R""""( {"testParameter": "testValue"} )"""";
        ASSERT_EQ(MMI_OK, MpiSet(h, componentName1.c_str(), "", payload, ARRAY_SIZE(payload)));
        ASSERT_EQ(MMI_OK, MpiSet(h, componentName2.c_str(), "", payload, ARRAY_SIZE(payload)));
    }

    TEST_F(MpiTests, MpiGet)
    {
        SetUp("MpiTests.MpiGet");
        int szPayload;
        MMI_JSON_STRING payload;
        ASSERT_EQ(MMI_OK, MpiGet(h, componentName2.c_str(), "", &payload, &szPayload));
        ASSERT_TRUE(szPayload > 0);
        constexpr const char expectedStr[] = R""""( {"returnValue": "TestComponent2-MultiComponentTheLargestVersionModule"} )"""";
        std::string payloadStr(payload, szPayload);
        ASSERT_TRUE(Tests::JSON_EQ(expectedStr, payloadStr.c_str()));
    }

    TEST_F(MpiTests, MpiGetInvalidSession)
    {
        SetUp("MpiTests.MpiGetInvalidSession");
        int szPayload = 0;
        MMI_JSON_STRING payload;
        ASSERT_EQ(EINVAL, MpiGet(nullptr, componentName2.c_str(), "", &payload, &szPayload));
        ASSERT_EQ(0, szPayload);
    }

    TEST_F(MpiTests, MpiSetInvalidSession)
    {
        SetUp("MpiTests.MpiSetInvalidSession");
        ASSERT_EQ(EINVAL, MpiSet(nullptr, componentName1.c_str(), "", nullptr, 0));
    }

    TEST_F(MpiTests, MpiSetPayloadSizeExeceedLimit)
    {
        char largePayload[] = R""""( {"testParameter": "testValue", "testKey": "value"} )"""";
        SetUp("MpiTests.MpiSetPayloadSizeExeceedLimit", ARRAY_SIZE(largePayload) - 2);
        ASSERT_EQ(ENOMEM, MpiSet(h, payloadValidationComponent.c_str(), objectName.c_str(), largePayload, ARRAY_SIZE(largePayload) - 1));
    }

    TEST_F(MpiTests, MpiSetPayloadValidation)
    {
        char expectedPayload[] = R""""( {"testParameter": "testValue"} )"""";
        SetUp("MpiTests.MpiSetPayloadValidation");
        int szPayload;
        MMI_JSON_STRING payload;
        ASSERT_EQ(MPI_OK, MpiSet(h, payloadValidationComponent.c_str(), objectName.c_str(), expectedPayload, ARRAY_SIZE(expectedPayload) - 1));
        ASSERT_EQ(MPI_OK, MpiGet(h, payloadValidationComponent.c_str(), objectName.c_str(), &payload, &szPayload));
        ASSERT_TRUE(szPayload > 0);
        std::string payloadStr(payload, szPayload);
        ASSERT_TRUE(Tests::JSON_EQ(expectedPayload, payloadStr.c_str()));
    }

    TEST_F(MpiTests, MpiSetDesired)
    {
        std::string clientName = "MpiTests.MpiSetDesired";
        char payload[] = R""""(
            [
                {
                    "TestComponent1": {
                        "testParameter": "testValue"
                    }
                }
            ])"""";

        SetUp(clientName);
        ASSERT_EQ(MPI_OK, MpiSetDesired(clientName.c_str(), payload, ARRAY_SIZE(payload)));
    }

    TEST_F(MpiTests, MpiGetReported)
    {
        int payloadSize;
        MMI_JSON_STRING payload;
        std::string clientName = "MpiTests.MpiGetReported";
        constexpr const char expected[] = R"""(
            {
                "TestComponent1": {
                    "TestObject1": {
                        "returnValue": "TestComponent1-MultiComponentModule"
                    }
                }
            })""";

        SetUp(clientName);
        ASSERT_EQ(MPI_OK, MpiGetReported(clientName.c_str(), 0, &payload, &payloadSize));
        ASSERT_TRUE(payloadSize > 0);

        std::string payloadStr(payload, payloadSize);
        ASSERT_TRUE(Tests::JSON_EQ(expected, payloadStr.c_str()));
    }
}