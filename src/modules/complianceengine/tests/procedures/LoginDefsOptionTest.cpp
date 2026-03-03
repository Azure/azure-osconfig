// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "MockContext.h"

#include <LoginDefsOption.h>

using ComplianceEngine::AuditLoginDefsOption;
using ComplianceEngine::ComparisonOperation;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::LoginDefsOptionParams;
using ComplianceEngine::NestedListFormatter;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using std::string;
using testing::Return;

static const char* cLoginDefsPath = "/etc/login.defs";

class LoginDefsOptionTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;
    NestedListFormatter mFormatter;

    void SetUp() override
    {
        mIndicators.Push("LoginDefsOption");
    }

    void SetLoginDefsContent(const string& content)
    {
        EXPECT_CALL(mContext, GetFileContents(cLoginDefsPath)).WillOnce(Return(Result<string>(content)));
    }

    void SetLoginDefsError()
    {
        EXPECT_CALL(mContext, GetFileContents(cLoginDefsPath)).WillOnce(Return(Result<string>(Error("Failed to load file contents"))));
    }
};

TEST_F(LoginDefsOptionTest, PassMaxDays_LessOrEqual_Compliant)
{
    SetLoginDefsContent(
        "# This is a comment\n"
        "PASS_MAX_DAYS\t365\n"
        "PASS_MIN_DAYS\t7\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::LessOrEqual;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, PassMaxDays_LessOrEqual_CompliantWhenLess)
{
    SetLoginDefsContent("PASS_MAX_DAYS 180\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::LessOrEqual;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, PassMaxDays_LessOrEqual_NonCompliant)
{
    SetLoginDefsContent("PASS_MAX_DAYS 99999\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::LessOrEqual;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(LoginDefsOptionTest, PassMaxDays_GreaterOrEqual_Compliant)
{
    SetLoginDefsContent("PASS_MAX_DAYS 365\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "1";
    params.comparison = ComparisonOperation::GreaterOrEqual;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, PassMaxDays_GreaterOrEqual_NonCompliant)
{
    SetLoginDefsContent("PASS_MAX_DAYS 0\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "1";
    params.comparison = ComparisonOperation::GreaterOrEqual;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(LoginDefsOptionTest, PassMinDays_GreaterOrEqual_Compliant)
{
    SetLoginDefsContent(
        "PASS_MAX_DAYS 365\n"
        "PASS_MIN_DAYS 7\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MIN_DAYS";
    params.value = "1";
    params.comparison = ComparisonOperation::GreaterOrEqual;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, PassWarnAge_GreaterOrEqual_Compliant)
{
    SetLoginDefsContent("PASS_WARN_AGE 7\n");

    LoginDefsOptionParams params;
    params.option = "PASS_WARN_AGE";
    params.value = "7";
    params.comparison = ComparisonOperation::GreaterOrEqual;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, EncryptMethod_Equal_Compliant)
{
    SetLoginDefsContent("ENCRYPT_METHOD SHA512\n");

    LoginDefsOptionParams params;
    params.option = "ENCRYPT_METHOD";
    params.value = "SHA512";
    params.comparison = ComparisonOperation::Equal;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, EncryptMethod_Equal_NonCompliant)
{
    SetLoginDefsContent("ENCRYPT_METHOD MD5\n");

    LoginDefsOptionParams params;
    params.option = "ENCRYPT_METHOD";
    params.value = "SHA512";
    params.comparison = ComparisonOperation::Equal;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(LoginDefsOptionTest, OptionNotFound_NonCompliant)
{
    SetLoginDefsContent(
        "# Only comments\n"
        "SOME_OTHER_OPTION 42\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::LessOrEqual;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(LoginDefsOptionTest, FileNotFound_ReturnsError)
{
    SetLoginDefsError();

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::LessOrEqual;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
}

TEST_F(LoginDefsOptionTest, CommentSkipped)
{
    SetLoginDefsContent(
        "# PASS_MAX_DAYS 99999\n"
        "PASS_MAX_DAYS 180\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::LessOrEqual;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, LastOccurrenceWins)
{
    SetLoginDefsContent(
        "PASS_MAX_DAYS 99999\n"
        "PASS_MAX_DAYS 180\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::LessOrEqual;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, TabSeparated)
{
    SetLoginDefsContent("PASS_MAX_DAYS\t\t365\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::Equal;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, LeadingWhitespace)
{
    SetLoginDefsContent("   PASS_MAX_DAYS 365\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::Equal;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, EmptyFile)
{
    SetLoginDefsContent("");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::LessOrEqual;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(LoginDefsOptionTest, LessThan_Compliant)
{
    SetLoginDefsContent("PASS_MAX_DAYS 364\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::LessThan;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, LessThan_NonCompliant_WhenEqual)
{
    SetLoginDefsContent("PASS_MAX_DAYS 365\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MAX_DAYS";
    params.value = "365";
    params.comparison = ComparisonOperation::LessThan;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(LoginDefsOptionTest, GreaterThan_Compliant)
{
    SetLoginDefsContent("PASS_MIN_DAYS 2\n");

    LoginDefsOptionParams params;
    params.option = "PASS_MIN_DAYS";
    params.value = "1";
    params.comparison = ComparisonOperation::GreaterThan;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, NotEqual_Compliant)
{
    SetLoginDefsContent("ENCRYPT_METHOD SHA512\n");

    LoginDefsOptionParams params;
    params.option = "ENCRYPT_METHOD";
    params.value = "MD5";
    params.comparison = ComparisonOperation::NotEqual;

    auto result = AuditLoginDefsOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(LoginDefsOptionTest, RealisticLoginDefs)
{
    const string content =
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
        "ENCRYPT_METHOD SHA512\n";

    // Check PASS_MAX_DAYS <= 365
    {
        EXPECT_CALL(mContext, GetFileContents(cLoginDefsPath)).WillOnce(Return(Result<string>(content)));

        LoginDefsOptionParams params;
        params.option = "PASS_MAX_DAYS";
        params.value = "365";
        params.comparison = ComparisonOperation::LessOrEqual;

        auto result = AuditLoginDefsOption(params, mIndicators, mContext);
        ASSERT_TRUE(result.HasValue());
        EXPECT_EQ(result.Value(), Status::Compliant);
    }

    // Check PASS_MAX_DAYS >= 1
    {
        EXPECT_CALL(mContext, GetFileContents(cLoginDefsPath)).WillOnce(Return(Result<string>(content)));

        LoginDefsOptionParams params;
        params.option = "PASS_MAX_DAYS";
        params.value = "1";
        params.comparison = ComparisonOperation::GreaterOrEqual;

        auto result = AuditLoginDefsOption(params, mIndicators, mContext);
        ASSERT_TRUE(result.HasValue());
        EXPECT_EQ(result.Value(), Status::Compliant);
    }

    // Check ENCRYPT_METHOD == SHA512
    {
        EXPECT_CALL(mContext, GetFileContents(cLoginDefsPath)).WillOnce(Return(Result<string>(content)));

        LoginDefsOptionParams params;
        params.option = "ENCRYPT_METHOD";
        params.value = "SHA512";
        params.comparison = ComparisonOperation::Equal;

        auto result = AuditLoginDefsOption(params, mIndicators, mContext);
        ASSERT_TRUE(result.HasValue());
        EXPECT_EQ(result.Value(), Status::Compliant);
    }
}
