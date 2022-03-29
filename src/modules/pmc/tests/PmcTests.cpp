// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <fstream>
#include <gtest/gtest.h>
#include <vector>

#include <Mmi.h>
#include <PmcBase.h>

class PmcTestImpl : public PmcBase
{
public:
    PmcTestImpl(unsigned int maxPayloadSizeBytes);
    int RunCommand(const char* command, std::string* textResult, unsigned int timeoutSeconds) override;
    void SetTextResult(const std::map<std::string, std::string> &textResults);

private:
    std::map<std::string, std::string> m_textResults;
};

PmcTestImpl::PmcTestImpl(unsigned int maxPayloadSizeBytes)
: PmcBase(maxPayloadSizeBytes)
{
}

void PmcTestImpl::SetTextResult(const std::map<std::string, std::string> &textResults)
{
    m_textResults = textResults;
}

int PmcTestImpl::RunCommand(const char* command, std::string* textResult, unsigned int timeoutSeconds)
{
    UNUSED(timeoutSeconds);

    std::map<std::string, std::string>::const_iterator it = m_textResults.find(command);
    if (it != m_textResults.end())
    {
        if (it->second == "COMMAND_ERROR")
        {
            return 100;
        }
        else if (it->second == "TIMEOUT_ERROR")
        {
            return ETIME;
        }

        if (textResult)
        {
            *textResult = it->second;
        }
        return MMI_OK;
    }
    return ENOSYS;
}

namespace OSConfig::Platform::Tests
{
    class PmcTests : public testing::Test
    {
    protected:
        void SetUp() override
        {
            testModule = new PmcTestImpl(g_maxPayloadSizeBytes);
        }

        void TearDown() override
        {
            delete testModule;
        }

        static PmcTestImpl* testModule;
        static constexpr const unsigned int g_maxPayloadSizeBytes = 4000;
        static constexpr const char* componentName = "PackageManagerConfiguration";
        static constexpr const char* desiredObjectName = "DesiredState";
        static constexpr const char* reportedObjectName = "State";
        static char validJsonPayload[];
    };

    PmcTestImpl* PmcTests::testModule;
    char PmcTests::validJsonPayload[] = "{\"Packages\":[\"cowsay=3.03+dfsg2-7:1 sl\", \"bar-\"]}";

    TEST_F(PmcTests, ValidSet)
    {
        const std::map<std::string, std::string> textResults =
        {
            {"command -v apt-get", ""},
            {"command -v apt-cache", ""},
            {"command -v dpkg-query", ""},
            {"apt-get update", ""},
            {"apt-get install cowsay=3.03+dfsg2-7:1 sl -y --allow-downgrades --auto-remove", ""},
            {"apt-get install bar- -y --allow-downgrades --auto-remove", ""}
        };

        testModule->SetTextResult(textResults);

        int status = testModule->Set(componentName, desiredObjectName, validJsonPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, MMI_OK);
    }

    TEST_F(PmcTests, ValidGetInitialValues)
    {
        const std::map<std::string, std::string> textResults =
        {
            {"command -v apt-get", ""},
            {"command -v apt-cache", ""},
            {"command -v dpkg-query", ""},
            {"dpkg-query --showformat='${Package} (=${Version})\n' --show | sha256sum | head -c 64", "25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4"}
        };
        char reportedJsonPayload[] = "{\"PackagesFingerprint\":\"25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4\",\"Packages\":[],\"ExecutionState\":0,\"ExecutionSubState\":0,\"ExecutionSubStateDetails\":\"\"}";
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;
        testModule->SetTextResult(textResults);

        int status = testModule->Get(componentName, reportedObjectName, &payload, &payloadSizeBytes);
        EXPECT_EQ(status, MMI_OK);

        std::string payloadString(payload, payloadSizeBytes);
        ASSERT_STREQ(reportedJsonPayload, payloadString.c_str());
    }

    TEST_F(PmcTests, ValidSetGet)
    {
        const std::map<std::string, std::string> textResults =
        {
            {"command -v apt-get", ""},
            {"command -v apt-cache", ""},
            {"command -v dpkg-query", ""},
            {"apt-get update", ""},
            {"apt-get install cowsay=3.03+dfsg2-7:1 sl -y --allow-downgrades --auto-remove", ""},
            {"apt-get install bar- -y --allow-downgrades --auto-remove", ""},
            {"dpkg-query --showformat='${Package} (=${Version})\n' --show | sha256sum | head -c 64", "25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4"},
            {"apt-cache policy cowsay | grep Installed", "  Installed: 3.03+dfsg2-7:1 "},
            {"apt-cache policy sl | grep Installed", "  Installed: 5.02-1 "},
            {"apt-cache policy bar | grep Installed", "  Installed: (none) "}           
        };
        char reportedJsonPayload[] = "{\"PackagesFingerprint\":\"25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4\",\"Packages\":[\"cowsay=3.03+dfsg2-7:1\",\"sl=5.02-1\",\"bar=(none)\"],\"ExecutionState\":2,\"ExecutionSubState\":0,\"ExecutionSubStateDetails\":\"\"}";
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;
        int status;
        testModule->SetTextResult(textResults);

        status = testModule->Set(componentName, desiredObjectName, validJsonPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, MMI_OK);

        status = testModule->Get(componentName, reportedObjectName, &payload, &payloadSizeBytes);
        EXPECT_EQ(status, MMI_OK);

        std::string payloadString(payload, payloadSizeBytes);
        ASSERT_STREQ(reportedJsonPayload, payloadString.c_str());
    }

    TEST_F(PmcTests, SetGetPackageInstallationTimeoutFailure)
    {
        const std::map<std::string, std::string> textResults =
        {
            {"command -v apt-get", ""},
            {"command -v apt-cache", ""},
            {"command -v dpkg-query", ""},
            {"apt-get update", ""},
            {"apt-get install cowsay=3.03+dfsg2-7:1 sl -y --allow-downgrades --auto-remove", "TIMEOUT_ERROR"},
            {"dpkg-query --showformat='${Package} (=${Version})\n' --show | sha256sum | head -c 64", "25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4"},
            {"apt-cache policy cowsay | grep Installed", "  Installed: (none) "},
            {"apt-cache policy sl | grep Installed", "  Installed: (none) "},
            {"apt-cache policy bar | grep Installed", "  Installed: (none) "}
        };
        char reportedJsonPayload[] = "{\"PackagesFingerprint\":\"25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4\",\"Packages\":[\"cowsay=(none)\",\"sl=(none)\",\"bar=(none)\"],\"ExecutionState\":4,\"ExecutionSubState\":5,\"ExecutionSubStateDetails\":\"cowsay=3.03+dfsg2-7:1 sl\"}";
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;
        int status;
        testModule->SetTextResult(textResults);

        status = testModule->Set(componentName, desiredObjectName, validJsonPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, ETIME);

        status = testModule->Get(componentName, reportedObjectName, &payload, &payloadSizeBytes);
        EXPECT_EQ(status, MMI_OK);

        std::string payloadString(payload, payloadSizeBytes);
        ASSERT_STREQ(reportedJsonPayload, payloadString.c_str());
    }

    TEST_F(PmcTests, InvalidPackageInputSet)
    {
        const std::map<std::string, std::string> textResults = 
        {
            {"command -v apt-get", ""},
            {"command -v apt-cache", ""},
            {"command -v dpkg-query", ""}
        };
        int status;
        testModule->SetTextResult(textResults);

        char invalidJsonPayloadAmpersand[] = "{\"Packages\":[\"cowsay=3.03+dfsg2-7 sl && echo foo\", \"bar-\"]}";
        status = testModule->Set(componentName, desiredObjectName, invalidJsonPayloadAmpersand, strlen(invalidJsonPayloadAmpersand));
        EXPECT_EQ(status, EINVAL);

        char invalidJsonPayloadDollar[] = "{\"Packages\":[\"cowsay=3.03+dfsg2-7 sl $(echo bar)\", \"bar-\"]}";
        status = testModule->Set(componentName, desiredObjectName, invalidJsonPayloadDollar, strlen(invalidJsonPayloadDollar));
        EXPECT_EQ(status, EINVAL);

        char invalidJsonPayloadSemicolon[] = "{\"Packages\":[\"cowsay=3.03+dfsg2-7 sl ; echo foo\", \"bar-\"]}";
        status = testModule->Set(componentName, desiredObjectName, invalidJsonPayloadSemicolon, strlen(invalidJsonPayloadSemicolon));
        EXPECT_EQ(status, EINVAL);

        char invalidJsonPayloadNewLine[] = "{\"Packages\":[\"cowsay=3.03+dfsg2-7 sl \n echo foo\", \"bar-\"]}";
        status = testModule->Set(componentName, desiredObjectName, invalidJsonPayloadNewLine, strlen(invalidJsonPayloadNewLine));
        EXPECT_EQ(status, EINVAL);
    }

    TEST_F(PmcTests, InvalidPackageInputSetGet)
    {
        const std::map<std::string, std::string> textResults =
        {
            {"command -v apt-get", ""},
            {"command -v apt-cache", ""},
            {"command -v dpkg-query", ""},
            {"dpkg-query --showformat='${Package} (=${Version})\n' --show | sha256sum | head -c 64", "25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4"}
        };
        
        char invalidJsonPayload[] = "{\"Packages\":[\"cowsay=3.03+dfsg2-7 sl && echo foo\", \"bar-\"]}";
        char reportedJsonPayload[] = "{\"PackagesFingerprint\":\"25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4\",\"Packages\":[],\"ExecutionState\":3,\"ExecutionSubState\":3,\"ExecutionSubStateDetails\":\"cowsay=3.03+dfsg2-7 sl && echo foo\"}";
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;
        int status;
        testModule->SetTextResult(textResults);

        status = testModule->Set(componentName, desiredObjectName, invalidJsonPayload, strlen(invalidJsonPayload));
        EXPECT_EQ(status, EINVAL);
        
        status = testModule->Get(componentName, reportedObjectName, &payload, &payloadSizeBytes);
        EXPECT_EQ(status, MMI_OK);
        
        std::string payloadString(payload, payloadSizeBytes);
        ASSERT_STREQ(reportedJsonPayload, payloadString.c_str());
    }

    TEST_F(PmcTests, SetInvalidComponentObjectName)
    {
        const std::map<std::string, std::string> textResults =
        {
            {"command -v apt-get", ""},
            {"command -v apt-cache", ""},
            {"command -v dpkg-query", ""}
        };
        std::string invalidName = "invalid";
        int status;
        testModule->SetTextResult(textResults);

        status = testModule->Set(invalidName.c_str(), desiredObjectName, validJsonPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, EINVAL);
        status = testModule->Set(componentName, invalidName.c_str(), validJsonPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, EINVAL);
    }

    TEST_F(PmcTests, GetInvalidComponentObjectName)
    {
        const std::map<std::string, std::string> textResults =
        {
            {"command -v apt-get", ""},
            {"command -v apt-cache", ""},
            {"command -v dpkg-query", ""}
        };
        std::string invalidName = "invalid";

        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;
        int status;
        testModule->SetTextResult(textResults);

        status = testModule->Get(invalidName.c_str(), reportedObjectName, &payload, &payloadSizeBytes);
        EXPECT_EQ(status, EINVAL);
        status = testModule->Get(componentName, invalidName.c_str(), &payload, &payloadSizeBytes);
        EXPECT_EQ(status, EINVAL);
    }

    TEST_F(PmcTests, SetInvalidPayloadString)
    {
        const std::map<std::string, std::string> textResults =
        {
            {"command -v apt-get", ""},
            {"command -v apt-cache", ""},
            {"command -v dpkg-query", ""},
        };
        char invalidPayload[] = "C++ PackageManagerConfiguration Module";
        testModule->SetTextResult(textResults);

        //test invalid length
        int status = testModule->Set(componentName, desiredObjectName, validJsonPayload, strlen(validJsonPayload) - 1);
        EXPECT_EQ(status, EINVAL);

        //test invalid payload
        status = testModule->Set(componentName, desiredObjectName, invalidPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, EINVAL);
    }
}