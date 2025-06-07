// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <fstream>
#include <gtest/gtest.h>
#include <list>
#include <sstream>
#include <string>
#include <sys/file.h>

#include <CommonUtils.h>
#include <Mmi.h>
#include <Ztsi.h>

#define MAX_PAYLOAD_SIZE 256

static const char g_componentName[] = "Ztsi";
static const char g_desiredEnabled[] = "desiredEnabled";
static const char g_desiredMaxScheduledAttestationsPerDay[] ="desiredMaxScheduledAttestationsPerDay";
static const char g_desiredMaxManualAttestationsPerDay[] = "desiredMaxManualAttestationsPerDay";
static const char g_reportedEnabled[] = "enabled";
static const char g_reportedMaxScheduledAttestationsPerDay[] ="maxScheduledAttestationsPerDay";
static const char g_reportedMaxManualAttestationsPerDay[] = "maxManualAttestationsPerDay";

static const Ztsi::EnabledState g_defaultEnabledState = Ztsi::EnabledState::Unknown;
static const int g_defaultMaxScheduledAttestationsPerDay = 10;
static const int g_defaultMaxManualAttestationsPerDay = 10;

#define STRFTIME_DATE_FORMAT "%Y%m%d"
#define SSCANF_DATE_FORMAT "%4d%2d%2d"
#define DATE_FORMAT_LENGTH 9

namespace OSConfig::Platform::Tests
{
    class ZtsiTests : public testing::Test
    {
    public:
        static std::string BuildFileContents(bool enabled, const int maxScheduledAttestationsPerDay, const int maxManualAttestationsPerDay)
        {
            std::stringstream expected;
            expected << "{\n";
            expected << "    \"enabled\": " << (enabled ? "true" : "false") << ",\n";
            expected << "    \"maxScheduledAttestationsPerDay\": " << maxScheduledAttestationsPerDay << ",\n";
            expected << "    \"maxManualAttestationsPerDay\": " << maxManualAttestationsPerDay << "\n";
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

    TEST_F(ZtsiTests, GetSetMaxScheduledAttestationsPerDay)
    {
        char maxScheduledAttestationsPerDay[] = "24";
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        ASSERT_EQ(MMI_OK, ztsi->Set(g_componentName, g_desiredMaxScheduledAttestationsPerDay, maxScheduledAttestationsPerDay, sizeof(maxScheduledAttestationsPerDay)));
        ASSERT_EQ(MMI_OK, ztsi->Get(g_componentName, g_reportedMaxScheduledAttestationsPerDay, &payload, &payloadSizeBytes));

        std::string payloadStr(payload, payloadSizeBytes);
        ASSERT_STREQ(maxScheduledAttestationsPerDay, payloadStr.c_str());
        ASSERT_EQ(payloadStr.length(), payloadSizeBytes);
    }

    TEST_F(ZtsiTests, GetSetMaxManualAttestationsPerDay)
    {
        char maxManualAttestationsPerDay[] = "24";
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        ASSERT_EQ(MMI_OK, ztsi->Set(g_componentName, g_desiredMaxManualAttestationsPerDay, maxManualAttestationsPerDay, sizeof(maxManualAttestationsPerDay)));
        ASSERT_EQ(MMI_OK, ztsi->Get(g_componentName, g_reportedMaxManualAttestationsPerDay, &payload, &payloadSizeBytes));

        std::string payloadStr(payload, payloadSizeBytes);
        ASSERT_STREQ(maxManualAttestationsPerDay, payloadStr.c_str());
        ASSERT_EQ(payloadStr.length(), payloadSizeBytes);
    }

    TEST_F(ZtsiTests, GetSetEnabled)
    {
        char enabled[] = "false";
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        ASSERT_EQ(MMI_OK, ztsi->Set(g_componentName, g_desiredEnabled, enabled, sizeof(enabled)));
        ASSERT_EQ(MMI_OK, ztsi->Get(g_componentName, g_reportedEnabled, &payload, &payloadSizeBytes));

        std::string payloadStr(payload, payloadSizeBytes);
        ASSERT_STREQ("2", payloadStr.c_str());
        ASSERT_EQ(payloadStr.length(), payloadSizeBytes);
    }

    TEST_F(ZtsiTests, InvalidSet)
    {
        char payload[] = "invalid payload";

        // Set with invalid arguments
        ASSERT_EQ(EINVAL, ztsi->Set("invalid component", g_desiredMaxScheduledAttestationsPerDay, payload, sizeof(payload)));
        ASSERT_EQ(EINVAL, ztsi->Set(g_componentName, "invalid component", payload, sizeof(payload)));
        ASSERT_EQ(EINVAL, ztsi->Set(g_componentName, g_desiredEnabled, payload, sizeof(payload)));
        ASSERT_EQ(EINVAL, ztsi->Set(g_componentName, g_reportedEnabled, payload, -1));
    }

    TEST_F(ZtsiTests, InvalidGet)
    {
        MMI_JSON_STRING payload = nullptr;
        int payloadSizeBytes = 0;

        // Get with invalid arguments
        ASSERT_EQ(EINVAL, ztsi->Get("invalid component", g_desiredEnabled, &payload, &payloadSizeBytes));
        ASSERT_EQ(EINVAL, ztsi->Get(g_componentName, "invalid object", &payload, &payloadSizeBytes));
    }

    TEST_F(ZtsiTests, GetWithoutConfigurationFile)
    {
        // Defaults are returned when no configuration file exists
        ASSERT_EQ(g_defaultEnabledState, ZtsiTests::ztsi->GetEnabledState());
        ASSERT_EQ(g_defaultMaxScheduledAttestationsPerDay, ZtsiTests::ztsi->GetMaxScheduledAttestationsPerDay());
        ASSERT_EQ(g_defaultMaxManualAttestationsPerDay, ZtsiTests::ztsi->GetMaxManualAttestationsPerDay());

        ASSERT_FALSE(FileExists(ZtsiTests::filename.c_str()));
    }

    TEST_F(ZtsiTests, SetEnabledFalseWithoutConfigurationFile)
    {
        ASSERT_EQ(MMI_OK, ZtsiTests::ztsi->SetEnabled(false));
        ASSERT_TRUE(FileExists(ZtsiTests::filename.c_str()));

        ASSERT_EQ(Ztsi::EnabledState::Disabled, ZtsiTests::ztsi->GetEnabledState());
    }

    TEST_F(ZtsiTests, MultipleSet)
    {
        int maxScheduledAttestationsPerDay1 = 24;
        int maxScheduledAttestationsPerDay2 = 48;

        std::string expected;
        std::string actual;

        for (int i = 0; i < 10; i++)
        {
            ASSERT_EQ(MMI_OK, ZtsiTests::ztsi->SetMaxScheduledAttestationsPerDay(maxScheduledAttestationsPerDay1));
            ASSERT_TRUE(FileExists(ZtsiTests::filename.c_str()));
            expected = ZtsiTests::BuildFileContents(false, maxScheduledAttestationsPerDay1, g_defaultMaxManualAttestationsPerDay);
            actual = ZtsiTests::ReadFileContents();
            ASSERT_STREQ(expected.c_str(), actual.c_str());

            ASSERT_EQ(MMI_OK, ZtsiTests::ztsi->SetEnabled(true));
            ASSERT_TRUE(FileExists(ZtsiTests::filename.c_str()));
            expected = ZtsiTests::BuildFileContents(true, maxScheduledAttestationsPerDay1, g_defaultMaxManualAttestationsPerDay);
            actual = ZtsiTests::ReadFileContents();
            ASSERT_STREQ(expected.c_str(), actual.c_str());

            ASSERT_EQ(MMI_OK, ZtsiTests::ztsi->SetMaxScheduledAttestationsPerDay(maxScheduledAttestationsPerDay2));
            ASSERT_TRUE(FileExists(ZtsiTests::filename.c_str()));
            expected = ZtsiTests::BuildFileContents(true, maxScheduledAttestationsPerDay2, g_defaultMaxManualAttestationsPerDay);
            actual = ZtsiTests::ReadFileContents();
            ASSERT_STREQ(expected.c_str(), actual.c_str());

            ASSERT_EQ(MMI_OK, ZtsiTests::ztsi->SetEnabled(false));
            ASSERT_TRUE(FileExists(ZtsiTests::filename.c_str()));
            expected = ZtsiTests::BuildFileContents(false, maxScheduledAttestationsPerDay2, g_defaultMaxManualAttestationsPerDay);
            actual = ZtsiTests::ReadFileContents();
            ASSERT_STREQ(expected.c_str(), actual.c_str());
        }
    }

    TEST_F(ZtsiTests, SetSameValue)
    {
        int maxScheduledAttestationsPerDay = 10;
        std::string expected;
        std::string actual;

        for (int i = 0; i < 10; i++)
        {
            ASSERT_EQ(MMI_OK, ZtsiTests::ztsi->SetMaxScheduledAttestationsPerDay(maxScheduledAttestationsPerDay));
            ASSERT_TRUE(FileExists(ZtsiTests::filename.c_str()));
            ASSERT_EQ(MMI_OK, ZtsiTests::ztsi->SetEnabled(true));
            ASSERT_TRUE(FileExists(ZtsiTests::filename.c_str()));

            expected = ZtsiTests::BuildFileContents(true, maxScheduledAttestationsPerDay, g_defaultMaxManualAttestationsPerDay);
            actual = ZtsiTests::ReadFileContents();
            ASSERT_STREQ(expected.c_str(), actual.c_str());
        }
    }


    TEST_F(ZtsiTests, InvalidConfiguration)
    {
        //Cannot set negative MaxScheduledAttestations
        ASSERT_EQ(EINVAL, ZtsiTests::ztsi->SetMaxScheduledAttestationsPerDay(-1));

        //Cannot set negative MaxManualAttestations
        ASSERT_EQ(EINVAL, ZtsiTests::ztsi->SetMaxManualAttestationsPerDay(-1));

    }

    TEST_F(ZtsiTests, GetAfterModifiedValidData)
    {
        int maxManualAttestationsPerDay1 = 24;
        int maxManualAttestationsPerDay2 = 48;

        ASSERT_EQ(MMI_OK, ZtsiTests::ztsi->SetMaxManualAttestationsPerDay(maxManualAttestationsPerDay1));
        ASSERT_TRUE(FileExists(ZtsiTests::filename.c_str()));
        ASSERT_EQ(MMI_OK, ZtsiTests::ztsi->SetEnabled(true));
        ASSERT_TRUE(FileExists(ZtsiTests::filename.c_str()));

        std::string expected = ZtsiTests::BuildFileContents(true, g_defaultMaxScheduledAttestationsPerDay, maxManualAttestationsPerDay1);
        std::string actual = ZtsiTests::ReadFileContents();
        ASSERT_EQ(maxManualAttestationsPerDay1, ZtsiTests::ztsi->GetMaxManualAttestationsPerDay());
        ASSERT_STREQ(expected.c_str(), actual.c_str());
        ASSERT_EQ(Ztsi::EnabledState::Enabled, ZtsiTests::ztsi->GetEnabledState());
        ASSERT_EQ(maxManualAttestationsPerDay1, ZtsiTests::ztsi->GetMaxManualAttestationsPerDay());

        // Modify JSON contents with valid data
        std::ofstream file(ZtsiTests::filename);
        file << ZtsiTests::BuildFileContents(false, g_defaultMaxScheduledAttestationsPerDay, maxManualAttestationsPerDay2);
        file.close();

        // Get should return the new contents
        ASSERT_EQ(Ztsi::EnabledState::Disabled, ZtsiTests::ztsi->GetEnabledState());
        ASSERT_EQ(maxManualAttestationsPerDay2, ZtsiTests::ztsi->GetMaxManualAttestationsPerDay());
    }

    TEST_F(ZtsiTests, GetAfterModifiedInvalidData)
    {
        int maxManualAttestationsPerDay = 24;

        // Overwrite with valid data
        ASSERT_EQ(MMI_OK, ZtsiTests::ztsi->SetMaxManualAttestationsPerDay(maxManualAttestationsPerDay));
        ASSERT_EQ(MMI_OK, ZtsiTests::ztsi->SetEnabled(true));

        std::string expected = ZtsiTests::BuildFileContents(true, g_defaultMaxScheduledAttestationsPerDay, maxManualAttestationsPerDay);
        std::string actual = ZtsiTests::ReadFileContents();

        ASSERT_EQ(maxManualAttestationsPerDay, ZtsiTests::ztsi->GetMaxManualAttestationsPerDay());
        ASSERT_STREQ(expected.c_str(), actual.c_str());
        ASSERT_EQ(Ztsi::EnabledState::Enabled, ZtsiTests::ztsi->GetEnabledState());
        ASSERT_EQ(maxManualAttestationsPerDay, ZtsiTests::ztsi->GetMaxManualAttestationsPerDay());

        // Modify JSON contents with invalid data
        std::ofstream file2(ZtsiTests::filename);
        file2 << "invalid json";
        file2.close();

        // Get should return the default contents
        ASSERT_EQ(g_defaultEnabledState, ZtsiTests::ztsi->GetEnabledState());
        ASSERT_EQ(g_defaultMaxManualAttestationsPerDay, ZtsiTests::ztsi->GetMaxManualAttestationsPerDay());
    }

    TEST_F(ZtsiTests, CachedEnabledBeforeSetMaxAttestations)
    {
        int maxScheduledAttestationsPerDay = 24;
        int maxManualAttestationsPerDay = 24;

        ASSERT_EQ(MMI_OK, ZtsiTests::ztsi->SetEnabled(true));
        ASSERT_EQ(Ztsi::EnabledState::Enabled, ZtsiTests::ztsi->GetEnabledState());
        ASSERT_EQ(g_defaultMaxScheduledAttestationsPerDay, ZtsiTests::ztsi->GetMaxScheduledAttestationsPerDay());
        ASSERT_EQ(g_defaultMaxManualAttestationsPerDay, ZtsiTests::ztsi->GetMaxManualAttestationsPerDay());
        ASSERT_TRUE(FileExists(ZtsiTests::filename.c_str()));

        // Should return max attestations per day and cached enabled state
        ASSERT_EQ(MMI_OK, ZtsiTests::ztsi->SetMaxScheduledAttestationsPerDay(maxScheduledAttestationsPerDay));
        ASSERT_EQ(MMI_OK, ZtsiTests::ztsi->SetMaxManualAttestationsPerDay(maxManualAttestationsPerDay));
        ASSERT_EQ(Ztsi::EnabledState::Enabled, ZtsiTests::ztsi->GetEnabledState());
        ASSERT_EQ(maxScheduledAttestationsPerDay, ZtsiTests::ztsi->GetMaxScheduledAttestationsPerDay());
        ASSERT_EQ(maxManualAttestationsPerDay, ZtsiTests::ztsi->GetMaxManualAttestationsPerDay());
        ASSERT_TRUE(FileExists(ZtsiTests::filename.c_str()));

        // Modify JSON contents with default JSON values
        std::string defaultJson = ZtsiTests::BuildFileContents(false, g_defaultMaxScheduledAttestationsPerDay,g_defaultMaxManualAttestationsPerDay);
        std::ofstream file(ZtsiTests::filename);
        file << defaultJson;
        file.close();
        ASSERT_STREQ(defaultJson.c_str(), ZtsiTests::ReadFileContents().c_str());

        // Get should return the JSON contents
        ASSERT_EQ(Ztsi::EnabledState::Disabled, ZtsiTests::ztsi->GetEnabledState());
        ASSERT_EQ(g_defaultMaxScheduledAttestationsPerDay, ZtsiTests::ztsi->GetMaxScheduledAttestationsPerDay());
        ASSERT_EQ(g_defaultMaxManualAttestationsPerDay, ZtsiTests::ztsi->GetMaxManualAttestationsPerDay());

        ASSERT_EQ(MMI_OK, ZtsiTests::ztsi->SetEnabled(true));
        ASSERT_EQ(Ztsi::EnabledState::Enabled, ZtsiTests::ztsi->GetEnabledState());
        ASSERT_EQ(g_defaultMaxScheduledAttestationsPerDay, ZtsiTests::ztsi->GetMaxScheduledAttestationsPerDay());
        ASSERT_EQ(g_defaultMaxManualAttestationsPerDay, ZtsiTests::ztsi->GetMaxManualAttestationsPerDay());

        // Should return max attestations per day and cached enabled state
        ASSERT_EQ(MMI_OK, ZtsiTests::ztsi->SetMaxScheduledAttestationsPerDay(maxScheduledAttestationsPerDay));
        ASSERT_EQ(MMI_OK, ZtsiTests::ztsi->SetMaxManualAttestationsPerDay(maxManualAttestationsPerDay));
        ASSERT_EQ(Ztsi::EnabledState::Enabled, ZtsiTests::ztsi->GetEnabledState());
        ASSERT_EQ(maxScheduledAttestationsPerDay, ZtsiTests::ztsi->GetMaxScheduledAttestationsPerDay());
        ASSERT_EQ(maxManualAttestationsPerDay, ZtsiTests::ztsi->GetMaxManualAttestationsPerDay());
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
            ASSERT_TRUE(IsValidClientName(validClientName.c_str()));
        }

        time_t t = time(0);
        char dateNow[DATE_FORMAT_LENGTH] = {0};
        strftime(dateNow, DATE_FORMAT_LENGTH, STRFTIME_DATE_FORMAT, localtime(&t));

        std::string clientNameWithCurrentDate = "Azure OSConfig 5;0.0.0." + std::string(dateNow);
        ASSERT_TRUE(IsValidClientName(clientNameWithCurrentDate.c_str()));
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
            ASSERT_FALSE(IsValidClientName(invalidClientName.c_str()));
        }

        time_t t = time(0);
        char dateNow[DATE_FORMAT_LENGTH] = {0};
        strftime(dateNow, DATE_FORMAT_LENGTH, STRFTIME_DATE_FORMAT, localtime(&t));

        int yearNow, monthNow, dayNow;
        sscanf(dateNow, SSCANF_DATE_FORMAT, &yearNow, &monthNow, &dayNow);

        std::string clientNameWithYearAfterCurrentDate = "Azure OSConfig 5;0.0.0." + std::to_string(yearNow + 1) + std::to_string(monthNow) + std::to_string(dayNow);
        std::string clientNameWithMonthAfterCurrentDate = "Azure OSConfig 5;0.0.0." + std::to_string(yearNow) + std::to_string(monthNow + 1) + std::to_string(dayNow);
        std::string clientNameWithDayAfterCurrentDate = "Azure OSConfig 5;0.0.0." + std::to_string(yearNow) + std::to_string(monthNow) + std::to_string(dayNow + 1);

        ASSERT_FALSE(IsValidClientName(clientNameWithMonthAfterCurrentDate.c_str()));
        ASSERT_FALSE(IsValidClientName(clientNameWithDayAfterCurrentDate.c_str()));
        ASSERT_FALSE(IsValidClientName(clientNameWithYearAfterCurrentDate.c_str()));
    }
}
