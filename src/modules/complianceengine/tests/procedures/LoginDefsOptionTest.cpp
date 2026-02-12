// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "MockContext.h"

#include <LoginDefsOption.h>
#include <Optional.h>
#include <fstream>

using ComplianceEngine::AuditLoginDefsOption;
using ComplianceEngine::ComparisonOperation;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::LoginDefsOptionParams;
using ComplianceEngine::NestedListFormatter;
using ComplianceEngine::Optional;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using std::string;

class LoginDefsOptionTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;
    NestedListFormatter mFormatter;
    string mTempDir;

    void SetUp() override
    {
        mIndicators.Push("LoginDefsOption");
        char tempDirTemplate[] = "/tmp/LoginDefsOptionTestXXXXXX";
        char* tempDir = mkdtemp(tempDirTemplate);
        ASSERT_NE(tempDir, nullptr);
        mTempDir = tempDir;
    }

    void TearDown() override
    {
        if (!mTempDir.empty())
        {
            // Remove all files in temp dir, then the dir itself
            string cmd = "rm -rf " + mTempDir;
            system(cmd.c_str());
            mTempDir.clear();
        }
    }

    string CreateLoginDefsFile(const string& content)
    {
        string filePath = mTempDir + "/login.defs";
        std::ofstream file(filePath);
        EXPECT_TRUE(file.is_open());
        file << content;
        file.close();
        return filePath;
    }
};

TEST_F(LoginDefsOptionTest, PassMaxDays_LessOrEqual_Compliant)
{
    string filePath = CreateLoginDefsFile(
        "# This is a comment\n"
        "PASS_MAX_DAYS\t365\n"
        "PASS_MIN_DAYS\t7\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::LessOrEqual;
    params.test_loginDefsPath = filePath;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, PassMaxDays_LessOrEqual_CompliantWhenLess)
{
    string filePath = CreateLoginDefsFile("PASS_MAX_DAYS 180\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::LessOrEqual;
    params.test_loginDefsPath = filePath;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, PassMaxDays_LessOrEqual_NonCompliant)
{
    string filePath = CreateLoginDefsFile("PASS_MAX_DAYS 99999\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::LessOrEqual;
    params.test_loginDefsPath = filePath;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(LoginDefsOptionTest, PassMaxDays_GreaterOrEqual_Compliant)
{
    string filePath = CreateLoginDefsFile("PASS_MAX_DAYS 365\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "1";
    params.comparison = ComparisonOperation::GreaterOrEqual;
    params.test_loginDefsPath = filePath;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, PassMaxDays_GreaterOrEqual_NonCompliant)
{
    string filePath = CreateLoginDefsFile("PASS_MAX_DAYS 0\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "1";
    params.comparison = ComparisonOperation::GreaterOrEqual;
    params.test_loginDefsPath = filePath;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(LoginDefsOptionTest, PassMinDays_GreaterOrEqual_Compliant)
{
    string filePath = CreateLoginDefsFile(
        "PASS_MAX_DAYS 365\n"
        "PASS_MIN_DAYS 7\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MIN_DAYS";
    params.value = "1";
    params.comparison = ComparisonOperation::GreaterOrEqual;
    params.test_loginDefsPath = filePath;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, PassWarnAge_GreaterOrEqual_Compliant)
{
    string filePath = CreateLoginDefsFile("PASS_WARN_AGE 7\n");

    LoginDefsOptionParams params;
    params.option = "PASS_WARN_AGE";
    params.value = "7";
    params.comparison = ComparisonOperation::GreaterOrEqual;
    params.test_loginDefsPath = filePath;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, EncryptMethod_Equal_Compliant)
{
    string filePath = CreateLoginDefsFile("ENCRYPT_METHOD SHA512\n");

    LoginDefsOptionParams params;
    params.option = "ENCRYPT_METHOD";
    params.value = "SHA512";
    params.comparison = ComparisonOperation::Equal;
    params.test_loginDefsPath = filePath;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, EncryptMethod_Equal_NonCompliant)
{
    string filePath = CreateLoginDefsFile("ENCRYPT_METHOD MD5\n");

    LoginDefsOptionParams params;
    params.option = "ENCRYPT_METHOD";
    params.value = "SHA512";
    params.comparison = ComparisonOperation::Equal;
    params.test_loginDefsPath = filePath;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(LoginDefsOptionTest, OptionNotFound_NonCompliant)
{
    string filePath = CreateLoginDefsFile(
        "# Only comments\n"
        "SOME_OTHER_OPTION 42\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::LessOrEqual;
    params.test_loginDefsPath = filePath;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(LoginDefsOptionTest, FileNotFound_NonCompliant)
{
    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::LessOrEqual;
    params.test_loginDefsPath = mTempDir + "/nonexistent";

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(LoginDefsOptionTest, CommentSkipped)
{
    string filePath = CreateLoginDefsFile(
        "# PASS_MAX_DAYS 99999\n"
        "PASS_MAX_DAYS 180\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::LessOrEqual;
    params.test_loginDefsPath = filePath;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, LastOccurrenceWins)
{
    string filePath = CreateLoginDefsFile(
        "PASS_MAX_DAYS 99999\n"
        "PASS_MAX_DAYS 180\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::LessOrEqual;
    params.test_loginDefsPath = filePath;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, TabSeparated)
{
    string filePath = CreateLoginDefsFile("PASS_MAX_DAYS\t\t365\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::Equal;
    params.test_loginDefsPath = filePath;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, LeadingWhitespace)
{
    string filePath = CreateLoginDefsFile("   PASS_MAX_DAYS 365\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::Equal;
    params.test_loginDefsPath = filePath;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, EmptyFile)
{
    string filePath = CreateLoginDefsFile("");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::LessOrEqual;
    params.test_loginDefsPath = filePath;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(LoginDefsOptionTest, LessThan_Compliant)
{
    string filePath = CreateLoginDefsFile("PASS_MAX_DAYS 364\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::LessThan;
    params.test_loginDefsPath = filePath;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, LessThan_NonCompliant_WhenEqual)
{
    string filePath = CreateLoginDefsFile("PASS_MAX_DAYS 365\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::LessThan;
    params.test_loginDefsPath = filePath;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(LoginDefsOptionTest, GreaterThan_Compliant)
{
    string filePath = CreateLoginDefsFile("PASS_MIN_DAYS 2\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MIN_DAYS";
    params.value = "1";
    params.comparison = ComparisonOperation::GreaterThan;
    params.test_loginDefsPath = filePath;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, NotEqual_Compliant)
{
    string filePath = CreateLoginDefsFile("ENCRYPT_METHOD SHA512\n");

    LoginDefsOptionParams params;
    params.option = "ENCRYPT_METHOD";
    params.value = "MD5";
    params.comparison = ComparisonOperation::NotEqual;
    params.test_loginDefsPath = filePath;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, RealisticLoginDefs)
{
    string filePath = CreateLoginDefsFile(
        "#\n"
        "# /etc/login.defs - Configuration control definitions for the login package.\n"
        "#\n"
        "\n"
        "MAIL_DIR        /var/mail\n"
        "\n"
        "# Password aging controls:\n"
        "#\n"
        "PASS_MAX_DAYS   365\n"
        "PASS_MIN_DAYS   7\n"
        "PASS_WARN_AGE   7\n"
        "\n"
        "#\n"
        "# Min/max values for automatic uid selection in useradd\n"
        "#\n"
        "UID_MIN                  1000\n"
        "UID_MAX                 60000\n"
        "\n"
        "ENCRYPT_METHOD SHA512\n");

    // Check PASS_MAX_DAYS <= 365
    {
        LoginDefsOptionParams params;
        params.option = "PASS_MAX_DAYS";
        params.value = "365";
        params.comparison = ComparisonOperation::LessOrEqual;
        params.test_loginDefsPath = filePath;

        auto result = AuditLoginDefsOption(params, mIndicators, mContext);
        ASSERT_TRUE(result.HasValue());
        EXPECT_EQ(result.Value(), Status::Compliant);
    }

    // Check PASS_MAX_DAYS >= 1
    {
        LoginDefsOptionParams params;
        params.option = "PASS_MAX_DAYS";
        params.value = "1";
        params.comparison = ComparisonOperation::GreaterOrEqual;
        params.test_loginDefsPath = filePath;

        auto result = AuditLoginDefsOption(params, mIndicators, mContext);
        ASSERT_TRUE(result.HasValue());
        EXPECT_EQ(result.Value(), Status::Compliant);
    }

    // Check ENCRYPT_METHOD == SHA512
    {
        LoginDefsOptionParams params;
        params.option = "ENCRYPT_METHOD";
        params.value = "SHA512";
        params.comparison = ComparisonOperation::Equal;
        params.test_loginDefsPath = filePath;

        auto result = AuditLoginDefsOption(params, mIndicators, mContext);
        ASSERT_TRUE(result.HasValue());
        EXPECT_EQ(result.Value(), Status::Compliant);
    }
}
