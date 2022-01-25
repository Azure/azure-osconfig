// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <vector>

#include <Mmi.h>
#include <AptInstall.h>

class AptInstallTests : public AptInstallBase
{
public:
    AptInstallTests(const std::map<std::string, std::string> &textResults, unsigned int maxPayloadSizeBytes);
    ~AptInstallTests() = default;
    int RunCommand(const char* command, bool replaceEol, std::string* textResult, unsigned int timeoutSeconds) override;


private:
    const std::map<std::string, std::string> &m_textResults;
};

AptInstallTests::AptInstallTests(const std::map<std::string, std::string> &textResults, unsigned int maxPayloadSizeBytes)
    : AptInstallBase(maxPayloadSizeBytes), m_textResults(textResults)
{
}

int AptInstallTests::RunCommand(const char* command, bool replaceEol, std::string* textResult, unsigned int timeoutSeconds)
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
    static char validJsonPayload[] = "{\"StateId\":\"my-id-1\",\"Packages\":[\"cowsay sl\", \"bar\"]}";
    static const char* componentName = "AptInstall";
    static const char* desiredObjectName = "DesiredPackages";
    static const char* reportedObjectName = "State";

    TEST(AptInstallTests, ValidSet)
    {
        const std::map<std::string, std::string> textResults =
        {
            {"sudo apt-get update", ""},
            {"sudo apt-get install cowsay sl -y --allow-downgrades --auto-remove", ""},
            {"sudo apt-get install bar -y --allow-downgrades --auto-remove", ""},
        };

        AptInstallTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Set(componentName, desiredObjectName, validJsonPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, MMI_OK);
    }

    TEST(AptInstallTests, ValidGet)
    {
        const std::map<std::string, std::string> textResults =
        {
            {"dpkg-query --showformat='${Package} (=${Version})\n' --show | sha256sum | head -c 64", "25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4"},
        };
        char reportedJsonPayload[] = "{\"StateId\":\"\",\"PackagesFingerprint\":\"25abefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4\",\"CurrentState\":0}";

        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        AptInstallTests testModule(textResults, g_maxPayloadSizeBytes);
        int status = testModule.Get(componentName, reportedObjectName, &payload, &payloadSizeBytes);
        EXPECT_EQ(status, MMI_OK);

        std::string payloadString(payload, payloadSizeBytes);
        ASSERT_STREQ(reportedJsonPayload, payloadString.c_str());
    }

    TEST(AptInstallTests, InvalidComponentObjectName)
    {
        const std::map<std::string, std::string> textResults =
        {
            {"sudo apt-get update", ""},
            {"sudo apt-get install cowsay sl -y --allow-downgrades --auto-remove", ""},
            {"sudo apt-get install bar -y --allow-downgrades --auto-remove", ""},
            {"dpkg-query --showformat='${Package} (=${Version})\n' --show | sha256sum | head -c 64", "25beefbfdb34fd48872dea4e2339f2a17e395196945c77a6c7098c203b87fca4"},
        };
        std::string invalidName = "invalid";

        int payloadSizeBytes = 0;
        MMI_JSON_STRING payload = nullptr;

        AptInstallTests testModule(textResults, g_maxPayloadSizeBytes);
        int status;
        
        status = testModule.Set(invalidName.c_str(), desiredObjectName, validJsonPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, EINVAL);
        status = testModule.Set(componentName, invalidName.c_str(), validJsonPayload, strlen(validJsonPayload));
        EXPECT_EQ(status, EINVAL);

        status = testModule.Get(invalidName.c_str(), reportedObjectName, &payload, &payloadSizeBytes);
        EXPECT_EQ(status, EINVAL);
        status = testModule.Get(componentName, invalidName.c_str(), &payload, &payloadSizeBytes);
        EXPECT_EQ(status, EINVAL);
    }

    TEST(AptInstallTests, SetInvalidPayloadString)
    {
        const std::map<std::string, std::string> textResults =
        {
            {"sudo apt-get update", ""},
            {"sudo apt-get install cowsay sl -y --allow-downgrades --auto-remove", ""},
            {"sudo apt-get install bar -y --allow-downgrades --auto-remove", ""},
        };

        char invalidPayload[] = "C++ AptInstall Module";
        AptInstallTests testModule(textResults, g_maxPayloadSizeBytes);

        //test invalid length
        int status = testModule.Set(componentName, desiredObjectName, validJsonPayload, strlen(validJsonPayload)-1); 
        EXPECT_EQ(status, EINVAL);
        //test invalid payload
        status = testModule.Set(componentName, desiredObjectName, invalidPayload, strlen(validJsonPayload)); 
        EXPECT_EQ(status, EINVAL);
    }
}