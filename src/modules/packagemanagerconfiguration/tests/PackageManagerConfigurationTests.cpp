// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <filesystem>
#include <gtest/gtest.h>
#include <vector>

#include <Mmi.h>
#include <PackageManagerConfiguration.h>

class PackageManagerConfigurationTests : public PackageManagerConfigurationBase
{
public:
    PackageManagerConfigurationTests(const std::map<std::string, std::string> &textResults, unsigned int maxPayloadSizeBytes, std::string sourcesDir);
    ~PackageManagerConfigurationTests() = default;
    int RunCommand(const char* command, bool replaceEol, std::string* textResult, unsigned int timeoutSeconds) override;

private:
    const std::map<std::string, std::string> &m_textResults;
};

PackageManagerConfigurationTests::PackageManagerConfigurationTests(const std::map<std::string, std::string> &textResults, unsigned int maxPayloadSizeBytes, std::string sourcesDir)
    : PackageManagerConfigurationBase(maxPayloadSizeBytes, sourcesDir), m_textResults(textResults)
{
}

int PackageManagerConfigurationTests::RunCommand(const char* command, bool replaceEol, std::string* textResult, unsigned int timeoutSeconds)
{
    UNUSED(replaceEol);
    UNUSED(timeoutSeconds);

    std::map<std::string, std::string>::const_iterator it = m_textResults.find(command);
    if (it != m_textResults.end())
    {
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
    constexpr const unsigned int g_maxPayloadSizeBytes = 4000;
    static char validJsonPayload[] = "{\"Packages\":[\"cowsay=3.03+dfsg2-7 sl\", \"bar-\"], \"Sources\":{\"key\":\"value\"}}";
    static const char* componentName = "PackageManagerConfiguration";
    static const char* desiredObjectName = "DesiredState";
    static const char* reportedObjectName = "State";
    std::string sourcesDir = "sources";

    TEST(PackageManagerConfigurationTests, ValidSet)
    {
        const std::map<std::string, std::string> textResults =
        {
            {"sudo apt-get update", ""},
            {"sudo apt-get install cowsay=3.03+dfsg2-7 sl -y --allow-downgrades --auto-remove", ""},
            {"sudo apt-get install bar- -y --allow-downgrades --auto-remove", ""},
        };

        mkdir(sourcesDir.c_str(), 0775);

        PackageManagerConfigurationTests testModule(textResults, g_maxPayloadSizeBytes, sourcesDir + "/");

        int status = testModule.Set(componentName, desiredObjectName, validJsonPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, MMI_OK);
        ASSERT_TRUE(FileExists("./sources/key.list"));
        std::filesystem::remove_all(sourcesDir.c_str());
    }

    TEST(PackageManagerConfigurationTests, ValidGetInitialValues)
    {
        const std::map<std::string, std::string> textResults =
        {
            {"dpkg-query --showformat='${Package} (=${Version})\n' --show | sha256sum | head -c 64", "25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4"},
        };
        char reportedJsonPayload[] = "{\"PackagesFingerprint\":\"25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4\",\"Packages\":{},\"ExecutionState\":0,\"SourcesFingerprint\":{}}";

        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;
        mkdir(sourcesDir.c_str(), 0775);

        PackageManagerConfigurationTests testModule(textResults, g_maxPayloadSizeBytes, sourcesDir + "/");
        int status = testModule.Get(componentName, reportedObjectName, &payload, &payloadSizeBytes);
        EXPECT_EQ(status, MMI_OK);

        std::string payloadString(payload, payloadSizeBytes);
        ASSERT_STREQ(reportedJsonPayload, payloadString.c_str());
        std::filesystem::remove_all(sourcesDir.c_str());
    }

    TEST(PackageManagerConfigurationTests, ValidSetGet)
    {
        const std::map<std::string, std::string> textResults =
        {
            {"sudo apt-get update", ""},
            {"sudo apt-get install cowsay=3.03+dfsg2-7 sl -y --allow-downgrades --auto-remove", ""},
            {"sudo apt-get install bar- -y --allow-downgrades --auto-remove", ""},
            {"dpkg-query --showformat='${Package} (=${Version})\n' --show | sha256sum | head -c 64", "25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4"},
            {"apt-cache policy cowsay | grep Installed", "  Installed: 3.03+dfsg2-7 "},
            {"apt-cache policy sl | grep Installed", "  Installed: 5.02-1 "},
            {"apt-cache policy bar | grep Installed", "  Installed: (none) "},
            {"cat sources/key.list | sha256sum | head -c 64", "75c083f0e21449c1fd860cb64b12ea7442b479464d0aae1ade5ec4d15483b870"}
        };
        char reportedJsonPayload[] = "{\"PackagesFingerprint\":\"25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4\",\"Packages\":{\"bar\":\"(none)\",\"cowsay\":\"3.03+dfsg2-7\",\"sl\":\"5.02-1\"},\"ExecutionState\":2,\"SourcesFingerprint\":{\"key\":\"75c083f0e21449c1fd860cb64b12ea7442b479464d0aae1ade5ec4d15483b870\"}}";

        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;
        mkdir(sourcesDir.c_str(), 0775);

        int status;
        PackageManagerConfigurationTests testModule(textResults, g_maxPayloadSizeBytes, sourcesDir + "/");
        status = testModule.Set(componentName, desiredObjectName, validJsonPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, MMI_OK);

        status = testModule.Get(componentName, reportedObjectName, &payload, &payloadSizeBytes);
        EXPECT_EQ(status, MMI_OK);
        
        std::string payloadString(payload, payloadSizeBytes);
        ASSERT_STREQ(reportedJsonPayload, payloadString.c_str());
        std::filesystem::remove_all(sourcesDir.c_str());
    }

    TEST(PackageManagerConfigurationTests, SetInvalidComponentObjectName)
    {
        const std::map<std::string, std::string> textResults;
        std::string invalidName = "invalid";

        PackageManagerConfigurationTests testModule(textResults, g_maxPayloadSizeBytes, sourcesDir);
        int status;
        
        status = testModule.Set(invalidName.c_str(), desiredObjectName, validJsonPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, EINVAL);
        status = testModule.Set(componentName, invalidName.c_str(), validJsonPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, EINVAL);
    }
    
    TEST(PackageManagerConfigurationTests, GetInvalidComponentObjectName)
    {
        const std::map<std::string, std::string> textResults;
        std::string invalidName = "invalid";

        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        PackageManagerConfigurationTests testModule(textResults, g_maxPayloadSizeBytes, sourcesDir);
        int status;
       
        status = testModule.Get(invalidName.c_str(), reportedObjectName, &payload, &payloadSizeBytes);
        EXPECT_EQ(status, EINVAL);
        status = testModule.Get(componentName, invalidName.c_str(), &payload, &payloadSizeBytes);
        EXPECT_EQ(status, EINVAL);
    }

    TEST(PackageManagerConfigurationTests, SetInvalidPayloadString)
    {
        const std::map<std::string, std::string> textResults;

        char invalidPayload[] = "C++ PackageManagerConfiguration Module";
        PackageManagerConfigurationTests testModule(textResults, g_maxPayloadSizeBytes, sourcesDir);

        //test invalid length
        int status = testModule.Set(componentName, desiredObjectName, validJsonPayload, strlen(validJsonPayload) - 1); 
        EXPECT_EQ(status, EINVAL);
        
        //test invalid payload
        status = testModule.Set(componentName, desiredObjectName, invalidPayload, strlen(validJsonPayload)); 
        EXPECT_EQ(status, EINVAL);
    }
}