// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <vector>

#include <Mmi.h>
#include <PackageManagerConfiguration.h>

class PackageManagerConfigurationTests : public PackageManagerConfigurationBase
{
public:
    PackageManagerConfigurationTests(const std::map<std::string, std::string> &textResults, unsigned int maxPayloadSizeBytes);
    ~PackageManagerConfigurationTests() = default;
    int RunCommand(const char* command, bool replaceEol, std::string* textResult, unsigned int timeoutSeconds) override;


private:
    const std::map<std::string, std::string> &m_textResults;
};

PackageManagerConfigurationTests::PackageManagerConfigurationTests(const std::map<std::string, std::string> &textResults, unsigned int maxPayloadSizeBytes)
    : PackageManagerConfigurationBase(maxPayloadSizeBytes), m_textResults(textResults)
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

    TEST(PackageManagerConfigurationTests, ValidSet)
    {
        const std::map<std::string, std::string> textResults =
        {
            {"sudo apt-get update", ""},
            {"sudo apt-get install cowsay=3.03+dfsg2-7 sl -y --allow-downgrades --auto-remove", ""},
            {"sudo apt-get install bar- -y --allow-downgrades --auto-remove", ""},
        };

        PackageManagerConfigurationTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Set(componentName, desiredObjectName, validJsonPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, MMI_OK);
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

        PackageManagerConfigurationTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Get(componentName, reportedObjectName, &payload, &payloadSizeBytes);
        EXPECT_EQ(status, MMI_OK);

        std::string payloadString(payload, payloadSizeBytes);
        ASSERT_STREQ(reportedJsonPayload, payloadString.c_str());
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
            {"apt-cache policy bar | grep Installed", "  Installed: (none) "}
        };
        char reportedJsonPayload[] = "{\"PackagesFingerprint\":\"25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4\",\"Packages\":{\"bar\":\"(none)\",\"cowsay\":\"3.03+dfsg2-7\",\"sl\":\"5.02-1\"},\"ExecutionState\":2,\"SourcesFingerprint\":{}}";

        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        int status;
        PackageManagerConfigurationTests testModule(textResults, g_maxPayloadSizeBytes);
        status = testModule.Set(componentName, desiredObjectName, validJsonPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, MMI_OK);

        status = testModule.Get(componentName, reportedObjectName, &payload, &payloadSizeBytes);
        EXPECT_EQ(status, MMI_OK);
        
        std::string payloadString(payload, payloadSizeBytes);
        ASSERT_STREQ(reportedJsonPayload, payloadString.c_str());
    }

    TEST(PackageManagerConfigurationTests, SetInvalidComponentObjectName)
    {
        const std::map<std::string, std::string> textResults =
        {
            {"sudo apt-get update", ""},
            {"sudo apt-get install cowsay sl -y --allow-downgrades --auto-remove", ""},
            {"sudo apt-get install bar -y --allow-downgrades --auto-remove", ""}
        };
        std::string invalidName = "invalid";

        PackageManagerConfigurationTests testModule(textResults, g_maxPayloadSizeBytes);
        int status;
        
        status = testModule.Set(invalidName.c_str(), desiredObjectName, validJsonPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, EINVAL);
        status = testModule.Set(componentName, invalidName.c_str(), validJsonPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, EINVAL);
    }
    
    TEST(PackageManagerConfigurationTests, GetInvalidComponentObjectName)
    {
        const std::map<std::string, std::string> textResults =
        {
            {"dpkg-query --showformat='${Package} (=${Version})\n' --show | sha256sum | head -c 64", "25beefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4"},
        };
        std::string invalidName = "invalid";

        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        PackageManagerConfigurationTests testModule(textResults, g_maxPayloadSizeBytes);
        int status;
       
        status = testModule.Get(invalidName.c_str(), reportedObjectName, &payload, &payloadSizeBytes);
        EXPECT_EQ(status, EINVAL);
        status = testModule.Get(componentName, invalidName.c_str(), &payload, &payloadSizeBytes);
        EXPECT_EQ(status, EINVAL);
    }

    TEST(PackageManagerConfigurationTests, SetInvalidPayloadString)
    {
        const std::map<std::string, std::string> textResults =
        {
            {"sudo apt-get update", ""},
            {"sudo apt-get install cowsay sl -y --allow-downgrades --auto-remove", ""},
            {"sudo apt-get install bar -y --allow-downgrades --auto-remove", ""},
        };

        char invalidPayload[] = "C++ PackageManagerConfiguration Module";
        PackageManagerConfigurationTests testModule(textResults, g_maxPayloadSizeBytes);

        //test invalid length
        int status = testModule.Set(componentName, desiredObjectName, validJsonPayload, strlen(validJsonPayload)-1); 
        EXPECT_EQ(status, EINVAL);
        //test invalid payload
        status = testModule.Set(componentName, desiredObjectName, invalidPayload, strlen(validJsonPayload)); 
        EXPECT_EQ(status, EINVAL);
    }
}