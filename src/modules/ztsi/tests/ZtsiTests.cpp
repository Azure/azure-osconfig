// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <fstream>
#include <gtest/gtest.h>
#include <list>
#include <sstream>
#include <string>
#include <sys/file.h>

#include <Ztsi.h>

#define MAX_PAYLOAD_SIZE 256

static const Ztsi::EnabledState g_defaultEnabledState = Ztsi::EnabledState::Unknown;
static const std::string g_defaultServiceUrl = "";

#define STRFTIME_DATE_FORMAT "%Y%m%d"
#define SSCANF_DATE_FORMAT "%4d%2d%2d"
#define DATE_FORMAT_LENGTH 9

namespace OSConfig::Platform::Tests
{
    class ZtsiTests : public testing::Test
    {
    public:
        static bool FileExists()
        {
            std::ifstream ifile(ZtsiTests::filename);
            return ifile.good();
        }

        static std::string BuildFileContents(bool enabled, const std::string& serviceUrl)
        {
            std::stringstream expected;
            expected << "{\n";
            expected << "    \"enabled\": " << (enabled ? "true" : "false") << ",\n";
            expected << "    \"serviceUrl\": \"" << serviceUrl << "\"\n";
            expected << "}";
            return expected.str();
        }

        static std::string ReadFileContents()
        {
            std::ifstream ifile(ZtsiTests::filename);
            if (ifile.good())
            {
                std::stringstream buffer;
                buffer << ifile.rdbuf();
                return buffer.str();
            }
            else
            {
                return "";
            }
        }

    protected:
        void SetUp() override
        {
            ztsi = new Ztsi(filename, MAX_PAYLOAD_SIZE);
        }

        void TearDown() override
        {
            delete ztsi;
            remove(filename.c_str());
        }

        static Ztsi* ztsi;
        static std::string filename;
    };

    Ztsi* ZtsiTests::ztsi;
    std::string ZtsiTests::filename = "./ztsi/config.temp.json";

    TEST_F(ZtsiTests, GetWithoutConfigurationFile)
    {
        // Defaults are returned when no configuration file exists
        ASSERT_EQ(g_defaultEnabledState, ZtsiTests::ztsi->GetEnabledState());
        ASSERT_EQ(g_defaultServiceUrl, ZtsiTests::ztsi->GetServiceUrl());

        ASSERT_FALSE(ZtsiTests::FileExists());
    }

    TEST_F(ZtsiTests, SetEnabledTrueWithoutConfigurationFile)
    {
        // Enabled can only be set to true when no configuration file exists since serviceUrl is empty string by default
        // No file is created for invalid configurations
        ASSERT_EQ(EINVAL, ZtsiTests::ztsi->SetEnabled(true));
        ASSERT_FALSE(ZtsiTests::FileExists());

        // Default values are returned
        ASSERT_EQ(g_defaultEnabledState, ZtsiTests::ztsi->GetEnabledState());
        ASSERT_STREQ(g_defaultServiceUrl.c_str(), ZtsiTests::ztsi->GetServiceUrl().c_str());
    }

    TEST_F(ZtsiTests, SetEnabledFalseWithoutConfigurationFile)
    {
        ASSERT_EQ(0, ZtsiTests::ztsi->SetEnabled(false));
        ASSERT_TRUE(ZtsiTests::FileExists());

        ASSERT_EQ(Ztsi::EnabledState::Disabled, ZtsiTests::ztsi->GetEnabledState());
        ASSERT_STREQ(g_defaultServiceUrl.c_str(), ZtsiTests::ztsi->GetServiceUrl().c_str());

        std::string expected = ZtsiTests::BuildFileContents(false, g_defaultServiceUrl);
        std::string actual = ZtsiTests::ReadFileContents();
        ASSERT_STREQ(expected.c_str(), actual.c_str());
    }

    TEST_F(ZtsiTests, SetServiceUrlWithoutConfigurationFile)
    {
        std::string serviceUrl = "https://www.example.com/";

        ASSERT_EQ(0, ZtsiTests::ztsi->SetServiceUrl(serviceUrl));
        ASSERT_TRUE(ZtsiTests::FileExists());

        ASSERT_EQ(Ztsi::EnabledState::Disabled, ZtsiTests::ztsi->GetEnabledState());
        ASSERT_STREQ(serviceUrl.c_str(), ZtsiTests::ztsi->GetServiceUrl().c_str());

        std::string expected = ZtsiTests::BuildFileContents(false, serviceUrl);
        std::string actual = ZtsiTests::ReadFileContents();
        ASSERT_STREQ(expected.c_str(), actual.c_str());
    }

    TEST_F(ZtsiTests, MultipleSet)
    {
        std::string serviceUrl1 = "https://www.example.com/";
        std::string serviceUrl2 = "https://www.test.com/";

        std::string expected;
        std::string actual;

        for (int i = 0; i < 10; i++)
        {
            ASSERT_EQ(0, ZtsiTests::ztsi->SetServiceUrl(serviceUrl1));
            ASSERT_TRUE(ZtsiTests::FileExists());
            expected = ZtsiTests::BuildFileContents(false, serviceUrl1);
            actual = ZtsiTests::ReadFileContents();
            ASSERT_STREQ(expected.c_str(), actual.c_str());

            ASSERT_EQ(0, ZtsiTests::ztsi->SetEnabled(true));
            ASSERT_TRUE(ZtsiTests::FileExists());
            expected = ZtsiTests::BuildFileContents(true, serviceUrl1);
            actual = ZtsiTests::ReadFileContents();
            ASSERT_STREQ(expected.c_str(), actual.c_str());

            ASSERT_EQ(0, ZtsiTests::ztsi->SetServiceUrl(serviceUrl2));
            ASSERT_TRUE(ZtsiTests::FileExists());
            expected = ZtsiTests::BuildFileContents(true, serviceUrl2);
            actual = ZtsiTests::ReadFileContents();
            ASSERT_STREQ(expected.c_str(), actual.c_str());

            ASSERT_EQ(0, ZtsiTests::ztsi->SetEnabled(false));
            ASSERT_TRUE(ZtsiTests::FileExists());
            expected = ZtsiTests::BuildFileContents(false, serviceUrl2);
            actual = ZtsiTests::ReadFileContents();
            ASSERT_STREQ(expected.c_str(), actual.c_str());
        }
    }

    TEST_F(ZtsiTests, SetSameValue)
    {
        std::string serviceUrl = "https://www.example.com/";
        std::string expected;
        std::string actual;

        for (int i = 0; i < 10; i++)
        {
            ASSERT_EQ(0, ZtsiTests::ztsi->SetServiceUrl(serviceUrl));
            ASSERT_TRUE(ZtsiTests::FileExists());
            ASSERT_EQ(0, ZtsiTests::ztsi->SetEnabled(true));
            ASSERT_TRUE(ZtsiTests::FileExists());

            expected = ZtsiTests::BuildFileContents(true, serviceUrl);
            actual = ZtsiTests::ReadFileContents();
            ASSERT_STREQ(expected.c_str(), actual.c_str());
        }
    }

    TEST_F(ZtsiTests, ValidServiceUrl)
    {
        ASSERT_EQ(0, ZtsiTests::ztsi->SetEnabled(false));

        std::list<std::string> validServiceUrls = {
            "",
            "http://example.com",
            "https://example.com",
            "http://example.com/",
            "https://example.com/",
            "http://www.example.com",
            "https://www.example.com",
            "https://www.example.com/path/to/something/",
            "https://www.example.com/params?a=1",
            "https://www.example.com/params?a=1&b=2",
        };

        for (const auto& validServiceUrl : validServiceUrls)
        {
            ASSERT_EQ(0, ZtsiTests::ztsi->SetServiceUrl(validServiceUrl));
            std::string expected = ZtsiTests::BuildFileContents(false, validServiceUrl);
            std::string actual = ZtsiTests::ReadFileContents();
            ASSERT_STREQ(expected.c_str(), actual.c_str());
        }
    }

    TEST_F(ZtsiTests, InvalidServiceUrl)
    {
        ASSERT_EQ(0, ZtsiTests::ztsi->SetEnabled(false));

        std::string expected = ZtsiTests::BuildFileContents(false, g_defaultServiceUrl);
        std::list<std::string> invalidServiceUrls = {
            "http://",
            "https://",
            "http:\\\\example.com",
            "htp://example.com",
            "//example.com",
            "www.example.com",
            "example.com",
            "example.com/params?a=1",
            "/example",
            "localhost",
            "localhost:5000",
        };

        for (const auto& invalidServiceUrl : invalidServiceUrls)
        {
            ASSERT_EQ(EINVAL, ZtsiTests::ztsi->SetServiceUrl(invalidServiceUrl));
            std::string actual = ZtsiTests::ReadFileContents();
            ASSERT_STREQ(expected.c_str(), actual.c_str());
        }
    }

    TEST_F(ZtsiTests, InvalidConfiguration)
    {
        ASSERT_EQ(0, ZtsiTests::ztsi->SetServiceUrl(g_defaultServiceUrl));
        ASSERT_EQ(0, ZtsiTests::ztsi->SetEnabled(false));
        std::string expected = ZtsiTests::BuildFileContents(false, g_defaultServiceUrl);
        std::string actual = ZtsiTests::ReadFileContents();
        ASSERT_STREQ(expected.c_str(), actual.c_str());

        // Cannot enable when serviceUrl is empty
        ASSERT_EQ(EINVAL, ZtsiTests::ztsi->SetEnabled(true));
        actual = ZtsiTests::ReadFileContents();
        ASSERT_STREQ(expected.c_str(), actual.c_str());

        std::string serviceUrl = "https://www.example.com/";
        ASSERT_EQ(0, ZtsiTests::ztsi->SetServiceUrl(serviceUrl));
        ASSERT_EQ(0, ZtsiTests::ztsi->SetEnabled(true));
        expected = ZtsiTests::BuildFileContents(true, serviceUrl);
        actual = ZtsiTests::ReadFileContents();
        ASSERT_STREQ(expected.c_str(), actual.c_str());

        // Cannot set serviceUrl to empty string when enabled
        ASSERT_EQ(EINVAL, ZtsiTests::ztsi->SetServiceUrl(""));
        actual = ZtsiTests::ReadFileContents();
        ASSERT_STREQ(expected.c_str(), actual.c_str());
    }

    TEST_F(ZtsiTests, GetAfterModifiedValidData)
    {
        std::string serviceUrl1 = "https://www.example.com/";
        std::string serviceUrl2 = "https://www.test.com/";

        ASSERT_EQ(0, ZtsiTests::ztsi->SetServiceUrl(serviceUrl1));
        ASSERT_TRUE(ZtsiTests::FileExists());
        ASSERT_EQ(0, ZtsiTests::ztsi->SetEnabled(true));
        ASSERT_TRUE(ZtsiTests::FileExists());

        std::string expected = ZtsiTests::BuildFileContents(true, serviceUrl1);
        std::string actual = ZtsiTests::ReadFileContents();
        ASSERT_STREQ(expected.c_str(), actual.c_str());
        ASSERT_EQ(Ztsi::EnabledState::Enabled, ZtsiTests::ztsi->GetEnabledState());
        ASSERT_STREQ(serviceUrl1.c_str(), ZtsiTests::ztsi->GetServiceUrl().c_str());

        // Modify JSON contents with valid data
        std::ofstream file(ZtsiTests::filename);
        file << ZtsiTests::BuildFileContents(false, serviceUrl2);
        file.close();

        // Get should return the new contents
        ASSERT_EQ(Ztsi::EnabledState::Disabled, ZtsiTests::ztsi->GetEnabledState());
        ASSERT_STREQ(serviceUrl2.c_str(), ZtsiTests::ztsi->GetServiceUrl().c_str());
    }

    TEST_F(ZtsiTests, GetAfterModifiedInvalidData)
    {
        std::string serviceUrl = "https://www.example.com/";

        // Overwrite with valid data
        ASSERT_EQ(0, ZtsiTests::ztsi->SetServiceUrl(serviceUrl));
        ASSERT_EQ(0, ZtsiTests::ztsi->SetEnabled(true));

        std::string expected = ZtsiTests::BuildFileContents(true, serviceUrl);
        std::string actual = ZtsiTests::ReadFileContents();
        ASSERT_STREQ(expected.c_str(), actual.c_str());
        ASSERT_EQ(Ztsi::EnabledState::Enabled, ZtsiTests::ztsi->GetEnabledState());
        ASSERT_STREQ(serviceUrl.c_str(), ZtsiTests::ztsi->GetServiceUrl().c_str());

        // Modify JSON contents with invalid data
        std::ofstream file2(ZtsiTests::filename);
        file2 << "invalid json";
        file2.close();

        // Get should return the default contents
        ASSERT_EQ(g_defaultEnabledState, ZtsiTests::ztsi->GetEnabledState());
        ASSERT_STREQ(g_defaultServiceUrl.c_str(), ZtsiTests::ztsi->GetServiceUrl().c_str());
    }

    TEST_F(ZtsiTests, ValidClientName)
    {
        std::list<std::string> validClientNames = {
            "Azure OSConfig 5;0.0.0.20210927",
            "Azure OSConfig 5;1.1.1.20210927",
            "Azure OSConfig 5;11.11.11.20210927",
            "Azure OSConfig 6;0.0.0.20210927",
            "Azure OSConfig 5;0.0.0.20210927abc123"
        };

        for (const auto& validClientName : validClientNames)
        {
            ASSERT_TRUE(IsValidClientName(validClientName));
        }

        time_t t = time(0);
        char dateNow[DATE_FORMAT_LENGTH] = {0};
        strftime(dateNow, DATE_FORMAT_LENGTH, STRFTIME_DATE_FORMAT, localtime(&t));

        std::string clientNameWithCurrentDate = "Azure OSConfig 5;0.0.0." + std::string(dateNow);
        ASSERT_TRUE(IsValidClientName(clientNameWithCurrentDate));
    }

    TEST_F(ZtsiTests, InvalidClientName)
    {
        std::list<std::string> invalidClientNames = {
            "AzureOSConfig 5;0.0.0.20210927",
            "Azure OSConfig5;0.0.0.20210927",
            "azure osconfig 5;0.0.0.20210927",
            "AzureOSConfig 5;0.0.0.20210927",
            "Azure  OSConfig5;0.0.0.20210927",
            "Azure OSConfig  5;0.0.0.20210927",
            "Azure OSConfig 5:0.0.0.20210927",
            "Azure OSConfig 5;0,0,0,20210927",
            "Azure OSConfig 5;0.0.0.2021927",
            "Azure OSConfig -5;-1.-1.-1.20210927",
            "Azure OSConfig 1;0.0.0.20210927",
            "Azure OSConfig 2;0.0.0.20210927",
            "Azure OSConfig 3;0.0.0.20210927",
            "Azure OSConfig 4;0.0.0.20210927",
            "Azure OSConfig 5;0.0.0.20210827",
            "Azure OSConfig 5;0.0.0.20210926",
            "Azure OSConfig 5;0.0.0.20200927"
            "Azure OSConfig 5;0.0.0.20200927"
        };

        for (const auto& invalidClientName : invalidClientNames)
        {
            ASSERT_FALSE(IsValidClientName(invalidClientName));
        }

        time_t t = time(0);
        char dateNow[DATE_FORMAT_LENGTH] = {0};
        strftime(dateNow, DATE_FORMAT_LENGTH, STRFTIME_DATE_FORMAT, localtime(&t));

        int yearNow, monthNow, dayNow;
        sscanf(dateNow, SSCANF_DATE_FORMAT, &yearNow, &monthNow, &dayNow);

        std::string clientNameWithYearAfterCurrentDate = "Azure OSConfig 5;0.0.0." + std::to_string(yearNow + 1) + std::to_string(monthNow) + std::to_string(dayNow);
        std::string clientNameWithMonthAfterCurrentDate = "Azure OSConfig 5;0.0.0." + std::to_string(yearNow) + std::to_string(monthNow + 1) + std::to_string(dayNow);
        std::string clientNameWithDayAfterCurrentDate = "Azure OSConfig 5;0.0.0." + std::to_string(yearNow) + std::to_string(monthNow) + std::to_string(dayNow + 1);

        ASSERT_FALSE(IsValidClientName(clientNameWithMonthAfterCurrentDate));
        ASSERT_FALSE(IsValidClientName(clientNameWithDayAfterCurrentDate));
        ASSERT_FALSE(IsValidClientName(clientNameWithYearAfterCurrentDate));
    }
}