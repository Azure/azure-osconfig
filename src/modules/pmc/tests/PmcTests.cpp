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
    PmcTestImpl(unsigned int maxPayloadSizeBytes, const char* sourcesDirectory);
    void SetTextResult(const std::map<std::string, std::tuple<int, std::string>> &textResults);

private:
    int RunCommand(const char* command, std::string* textResult, bool isLongRunning = false) override;
    std::string GetPackagesFingerprint() override;
    std::string GetSourcesFingerprint(const char* sourcesDirectory) override;
    std::map<std::string, std::tuple<int, std::string>> m_textResults;
};

PmcTestImpl::PmcTestImpl(unsigned int maxPayloadSizeBytes, const char* sourcesDirectory)
: PmcBase(maxPayloadSizeBytes, sourcesDirectory)
{
}

void PmcTestImpl::SetTextResult(const std::map<std::string, std::tuple<int, std::string>> &textResults)
{
    m_textResults = textResults;
}

int PmcTestImpl::RunCommand(const char* command, std::string* textResult, bool isLongRunning)
{
    UNUSED(isLongRunning);

    std::map<std::string, std::tuple<int, std::string>>::const_iterator it = m_textResults.find(command);
    if (it != m_textResults.end())
    {
        if (textResult)
        {
            *textResult = std::get<1>(it->second);
        }
        return std::get<0>(it->second);
    }
    return ENOSYS;
}

std::string PmcTestImpl::GetPackagesFingerprint()
{
    return "25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4";
}

std::string PmcTestImpl::GetSourcesFingerprint(const char* sourcesDirectory)
{
    UNUSED(sourcesDirectory);
    return "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b877";
}

namespace OSConfig::Platform::Tests
{
    class PmcTests : public testing::Test
    {
    protected:
        void SetUp() override
        {
            mkdir(sourcesDirectory, 0775);
            testModule = new PmcTestImpl(g_maxPayloadSizeBytes, sourcesDirectory);
        }

        void TearDown() override
        {
            delete testModule;
            const std::string command = std::string("rm -r ") + sourcesDirectory;
            int status = ExecuteCommand(nullptr, command.c_str(), true, true, 0, 0, nullptr, nullptr, PmcLog::Get());
            if (status != MMI_OK)
            {
                throw std::runtime_error("Failed to execute command " + command);
            }
        }

        static PmcTestImpl* testModule;
        static constexpr const unsigned int g_maxPayloadSizeBytes = 4000;
        static constexpr const char* componentName = "PackageManagerConfiguration";
        static constexpr const char* desiredObjectName = "desiredState";
        static constexpr const char* reportedObjectName = "state";
        static constexpr const char* sourcesDirectory = "sources/";
        static char validJsonPayload[];
    };

    PmcTestImpl* PmcTests::testModule;
    char PmcTests::validJsonPayload[] = "{\"packages\":[\"cowsay=3.03+dfsg2-7:1 sl\", \"bar-\"], \"sources\":{\"key\":\"value\",\"sourceToDelete\":null}}";

    TEST_F(PmcTests, ValidSet)
    {
        const std::map<std::string, std::tuple<int, std::string>> textResults =
        {
            {"command -v apt-get",  std::tuple<int, std::string>(MMI_OK, "")},
            {"command -v apt-cache", std::tuple<int, std::string>(MMI_OK, "")},
            {"command -v dpkg-query", std::tuple<int, std::string>(MMI_OK, "")},
            {"apt-get update", std::tuple<int, std::string>(MMI_OK, "")},
            {"apt-get install cowsay=3.03+dfsg2-7:1 sl -y --allow-downgrades --auto-remove", std::tuple<int, std::string>(MMI_OK, "")},
            {"apt-get install bar- -y --allow-downgrades --auto-remove", std::tuple<int, std::string>(MMI_OK, "")}
        };
        const std::string testFileToDeletePath = sourcesDirectory + std::string("sourceToDelete.list");
        const std::string testData = "test data";
        const std::string expectedFilePath = sourcesDirectory + std::string("key.list");

        std::ofstream sourceFileToDelete(testFileToDeletePath);
        sourceFileToDelete << testData << std::endl;
        sourceFileToDelete.close();
        testModule->SetTextResult(textResults);

        int status = testModule->Set(componentName, desiredObjectName, validJsonPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, MMI_OK);
        ASSERT_TRUE(FileExists(expectedFilePath.c_str()));
        ASSERT_FALSE(FileExists(testFileToDeletePath.c_str()));
    }

    TEST_F(PmcTests, ValidGetInitialValues)
    {
        const std::map<std::string, std::tuple<int, std::string>> textResults =
        {
            {"command -v apt-get", std::tuple<int, std::string>(MMI_OK, "")},
            {"command -v apt-cache", std::tuple<int, std::string>(MMI_OK, "")},
            {"command -v dpkg-query", std::tuple<int, std::string>(MMI_OK, "")},
        };
        char reportedJsonPayload[] = "{\"packagesFingerprint\":\"25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4\","
            "\"packages\":[],"
            "\"executionState\":0,\"executionSubState\":0,\"executionSubStateDetails\":\"\","
            "\"sourcesFingerprint\":\"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b877\","
            "\"sourcesFilenames\":[]}";
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
        const std::map<std::string, std::tuple<int, std::string>> textResults =
        {
            {"command -v apt-get", std::tuple<int, std::string>(MMI_OK, "")},
            {"command -v apt-cache", std::tuple<int, std::string>(MMI_OK, "")},
            {"command -v dpkg-query", std::tuple<int, std::string>(MMI_OK, "")},
            {"apt-get update", std::tuple<int, std::string>(MMI_OK, "")},
            {"apt-get install cowsay=3.03+dfsg2-7:1 sl -y --allow-downgrades --auto-remove", std::tuple<int, std::string>(MMI_OK, "")},
            {"apt-get install bar- -y --allow-downgrades --auto-remove", std::tuple<int, std::string>(MMI_OK, "")},
            {"apt-cache policy cowsay | grep Installed", std::tuple<int, std::string>(MMI_OK, "  Installed: 3.03+dfsg2-7:1 ")},
            {"apt-cache policy sl | grep Installed", std::tuple<int, std::string>(MMI_OK, "  Installed: 5.02-1 ")},
            {"apt-cache policy bar | grep Installed", std::tuple<int, std::string>(MMI_OK, "  Installed: (none) ")},
        };
        char reportedJsonPayload[] = "{\"packagesFingerprint\":\"25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4\","
            "\"packages\":[\"cowsay=3.03+dfsg2-7:1\",\"sl=5.02-1\",\"bar=(none)\"],"
            "\"executionState\":2,\"executionSubState\":0,\"executionSubStateDetails\":\"\","
            "\"sourcesFingerprint\":\"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b877\","
            "\"sourcesFilenames\":[\"key.list\"]}";
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

    TEST_F(PmcTests, SetGetUpdatingPackagesSourcesFailure)
    {
        const std::map<std::string, std::tuple<int, std::string>> textResults =
        {
            {"command -v apt-get", std::tuple<int, std::string>(MMI_OK, "")},
            {"command -v apt-cache", std::tuple<int, std::string>(MMI_OK, "")},
            {"command -v dpkg-query", std::tuple<int, std::string>(MMI_OK, "")},
            {"apt-get update", std::tuple<int, std::string>(EBUSY, "")},
            {"apt-cache policy cowsay | grep Installed", std::tuple<int, std::string>(MMI_OK, "  Installed: (none) ")},
            {"apt-cache policy sl | grep Installed", std::tuple<int, std::string>(MMI_OK, "  Installed: (none) ")},
            {"apt-cache policy bar | grep Installed", std::tuple<int, std::string>(MMI_OK, "  Installed: (none) ")},
        };
        char reportedJsonPayload[] = "{\"packagesFingerprint\":\"25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4\","
            "\"packages\":[\"cowsay=(none)\",\"sl=(none)\",\"bar=(none)\"],"
            "\"executionState\":3,\"executionSubState\":6,\"executionSubStateDetails\":\"\","
            "\"sourcesFingerprint\":\"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b877\","
            "\"sourcesFilenames\":[\"key.list\"]}";
        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;
        int status;
        testModule->SetTextResult(textResults);

        status = testModule->Set(componentName, desiredObjectName, validJsonPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, EBUSY);

        status = testModule->Get(componentName, reportedObjectName, &payload, &payloadSizeBytes);
        EXPECT_EQ(status, MMI_OK);

        std::string payloadString(payload, payloadSizeBytes);
        ASSERT_STREQ(reportedJsonPayload, payloadString.c_str());
    }

    TEST_F(PmcTests, SetGetPackageInstallationTimeoutFailure)
    {
        const std::map<std::string, std::tuple<int, std::string>> textResults =
        {
            {"command -v apt-get", std::tuple<int, std::string>(MMI_OK, "")},
            {"command -v apt-cache", std::tuple<int, std::string>(MMI_OK, "")},
            {"command -v dpkg-query", std::tuple<int, std::string>(MMI_OK, "")},
            {"apt-get update", std::tuple<int, std::string>(MMI_OK, "")},
            {"apt-get install cowsay=3.03+dfsg2-7:1 sl -y --allow-downgrades --auto-remove", std::tuple<int, std::string>(ETIME,"")},
            {"apt-cache policy cowsay | grep Installed", std::tuple<int, std::string>(MMI_OK, "  Installed: (none) ")},
            {"apt-cache policy sl | grep Installed", std::tuple<int, std::string>(MMI_OK, "  Installed: (none) ")},
            {"apt-cache policy bar | grep Installed", std::tuple<int, std::string>(MMI_OK, "  Installed: (none) ")},
        };
        char reportedJsonPayload[] = "{\"packagesFingerprint\":\"25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4\","
            "\"packages\":[\"cowsay=(none)\",\"sl=(none)\",\"bar=(none)\"],"
            "\"executionState\":4,\"executionSubState\":7,\"executionSubStateDetails\":\"cowsay=3.03+dfsg2-7:1 sl\","
            "\"sourcesFingerprint\":\"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b877\","
            "\"sourcesFilenames\":[\"key.list\"]}";
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
        const std::map<std::string, std::tuple<int, std::string>> textResults = 
        {
            {"command -v apt-get", std::tuple<int, std::string>(MMI_OK, "")},
            {"command -v apt-cache", std::tuple<int, std::string>(MMI_OK, "")},
            {"command -v dpkg-query", std::tuple<int, std::string>(MMI_OK, "")}
        };
        int status;
        testModule->SetTextResult(textResults);

        char invalidJsonPayloadAmpersand[] = "{\"packages\":[\"cowsay=3.03+dfsg2-7 sl && echo foo\", \"bar-\"]}";
        status = testModule->Set(componentName, desiredObjectName, invalidJsonPayloadAmpersand, strlen(invalidJsonPayloadAmpersand));
        EXPECT_EQ(status, EINVAL);

        char invalidJsonPayloadDollar[] = "{\"packages\":[\"cowsay=3.03+dfsg2-7 sl $(echo bar)\", \"bar-\"]}";
        status = testModule->Set(componentName, desiredObjectName, invalidJsonPayloadDollar, strlen(invalidJsonPayloadDollar));
        EXPECT_EQ(status, EINVAL);

        char invalidJsonPayloadSemicolon[] = "{\"packages\":[\"cowsay=3.03+dfsg2-7 sl ; echo foo\", \"bar-\"]}";
        status = testModule->Set(componentName, desiredObjectName, invalidJsonPayloadSemicolon, strlen(invalidJsonPayloadSemicolon));
        EXPECT_EQ(status, EINVAL);

        char invalidJsonPayloadNewLine[] = "{\"packages\":[\"cowsay=3.03+dfsg2-7 sl \n echo foo\", \"bar-\"]}";
        status = testModule->Set(componentName, desiredObjectName, invalidJsonPayloadNewLine, strlen(invalidJsonPayloadNewLine));
        EXPECT_EQ(status, EINVAL);
    }

    TEST_F(PmcTests, InvalidPackageInputSetGet)
    {
        const std::map<std::string, std::tuple<int, std::string>> textResults =
        {
            {"command -v apt-get", std::tuple<int, std::string>(MMI_OK, "")},
            {"command -v apt-cache", std::tuple<int, std::string>(MMI_OK, "")},
            {"command -v dpkg-query", std::tuple<int, std::string>(MMI_OK, "")},
        };
        
        char invalidJsonPayload[] = "{\"packages\":[\"cowsay=3.03+dfsg2-7 sl && echo foo\", \"bar-\"]}";
        char reportedJsonPayload[] = "{\"packagesFingerprint\":\"25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4\","
            "\"packages\":[],"
            "\"executionState\":3,\"executionSubState\":4,\"executionSubStateDetails\":\"cowsay=3.03+dfsg2-7 sl && echo foo\","
            "\"sourcesFingerprint\":\"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b877\","
            "\"sourcesFilenames\":[]}";
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
        const std::map<std::string, std::tuple<int, std::string>> textResults =
        {
            {"command -v apt-get", std::tuple<int, std::string>(MMI_OK, "")},
            {"command -v apt-cache", std::tuple<int, std::string>(MMI_OK, "")},
            {"command -v dpkg-query", std::tuple<int, std::string>(MMI_OK, "")}
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
        const std::map<std::string, std::tuple<int, std::string>> textResults =
        {
            {"command -v apt-get", std::tuple<int, std::string>(MMI_OK, "")},
            {"command -v apt-cache", std::tuple<int, std::string>(MMI_OK, "")},
            {"command -v dpkg-query", std::tuple<int, std::string>(MMI_OK, "")}
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
        const std::map<std::string, std::tuple<int, std::string>> textResults =
        {
            {"command -v apt-get", std::tuple<int, std::string>(MMI_OK, "")},
            {"command -v apt-cache", std::tuple<int, std::string>(MMI_OK, "")},
            {"command -v dpkg-query", std::tuple<int, std::string>(MMI_OK, "")},
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