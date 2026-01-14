// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "MockContext.h"

#include <EnsureSystemdParameter.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using ComplianceEngine::AuditEnsureSystemdParameterV4;
using ComplianceEngine::AuditSystemdParameter;
using ComplianceEngine::CompactListFormatter;
using ComplianceEngine::EnsureSystemdParameterV4Params;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using ComplianceEngine::SystemdParameterExpression;
using ComplianceEngine::SystemdParameterParams;
using ::testing::Return;

class SystemdConfigTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;

    void SetUp() override
    {
        mIndicators.Push("SystemdParameter");
        EXPECT_CALL(mContext, ExecuteCommand("readlink -e /bin/systemd-analyze"))
            .WillRepeatedly(Return(Result<std::string>("/usr/bin/systemd-analyze")));
        EXPECT_CALL(mContext, ExecuteCommand("readlink -e /usr/bin/systemd-analyze"))
            .WillRepeatedly(Return(Result<std::string>("/usr/bin/systemd-analyze")));
    }

    void TearDown() override
    {
    }
};

TEST_F(SystemdConfigTest, NeitherFileNorDirProvided)
{
    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.valueRegex = regex(".*");

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Neither 'file' nor 'dir' argument is provided");
}

TEST_F(SystemdConfigTest, BothFileAndDirProvided)
{
    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.valueRegex = regex(".*");
    params.file = "test.conf";
    params.dir = "/etc/systemd";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Both 'file' and 'dir' arguments are provided, only one is allowed");
}

TEST_F(SystemdConfigTest, FileCommandExecutionFails)
{
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf")))
        .WillOnce(Return(Result<std::string>(Error("Command execution failed", -1))));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.valueRegex = regex(".*");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Command execution failed");
}

TEST_F(SystemdConfigTest, FileParameterNotFound)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "OtherParam=value1\n"
        "AnotherParam=value2\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.valueRegex = regex(".*");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdConfigTest, FileParameterFoundButRegexMismatch)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=wrongvalue\n"
        "OtherParam=value1\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.valueRegex = regex("^correctvalue$");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdConfigTest, FileParameterFoundAndRegexMatches)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=correctvalue\n"
        "OtherParam=value1\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.valueRegex = regex("^correctvalue$");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, FileParameterWithComplexRegex)
{
    std::string systemdOutput =
        "# /etc/systemd/system.conf\n"
        "DefaultLimitNOFILE=65536\n"
        "DefaultTimeoutStopSec=90s\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config system.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "DefaultLimitNOFILE";
    params.valueRegex = regex("^[0-9]+$");
    params.file = "system.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, FileWithMultipleConfigSections)
{
    std::string systemdOutput =
        "# /etc/systemd/system.conf\n"
        "DefaultLimitNOFILE=1024\n"
        "# /usr/lib/systemd/system.conf\n"
        "DefaultLimitNOFILE=65536\n"
        "DefaultTimeoutStopSec=90s\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config system.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "DefaultLimitNOFILE";
    params.valueRegex = regex("^65536$");
    params.file = "system.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, FileWithCommentsAndEmptyLines)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "\n"
        "# This is a comment\n"
        "TestParam=value123\n"
        "\n"
        "# Another comment\n"
        "OtherParam=othervalue\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.valueRegex = regex("value[0-9]+");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, FileWithInvalidLineFormat)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=correctvalue\n"
        "InvalidLineWithoutEquals\n"
        "OtherParam=value1\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.valueRegex = regex("correctvalue");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, FileParameterWithAnyValueRegex)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=any_value_should_match\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.valueRegex = regex(".*");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, FileParameterWithEmptyValue)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=\n"
        "OtherParam=value1\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.valueRegex = regex("^$");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, FileParameterWithSpecialCharacters)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=/path/to/file with spaces\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.valueRegex = regex("/path/to/file with spaces");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, V4_NonExistentFile)
{
    EnsureSystemdParameterV4Params params;
    params.file = "nonexistentfile";
    params.section = "test";
    params.option = "foo";
    params.expression = SystemdParameterExpression::Equal;
    params.value = "1";

    const auto result = AuditEnsureSystemdParameterV4(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdConfigTest, V4_FileCommandExecutionFails)
{
    const auto filename = mContext.MakeTempfile("");
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config " + filename)))
        .WillOnce(Return(Result<std::string>(Error("Command execution failed", -1))));
    // EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config " + filename)))
    //     .WillOnce(Return(Result<std::string>("[test]\nfoo=1")));

    EnsureSystemdParameterV4Params params;
    params.file = filename;
    params.section = "test";
    params.option = "foo";
    params.expression = SystemdParameterExpression::Equal;
    params.value = "bar";

    const auto result = AuditEnsureSystemdParameterV4(params, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Command execution failed");
}

TEST_F(SystemdConfigTest, V4_SectionNotFound)
{
    const auto filename = mContext.MakeTempfile("");
    const std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[foo]\n"
        "bar=baz\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config " + filename))).WillOnce(Return(Result<std::string>(systemdOutput)));

    EnsureSystemdParameterV4Params params;
    params.file = filename;
    params.section = "test";
    params.option = "foo";
    params.expression = SystemdParameterExpression::Equal;
    params.value = "bar";

    const auto result = AuditEnsureSystemdParameterV4(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdConfigTest, V4_OptionNotFound)
{
    const auto filename = mContext.MakeTempfile("");
    const std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[test]\n"
        "bar=baz\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config " + filename))).WillOnce(Return(Result<std::string>(systemdOutput)));

    EnsureSystemdParameterV4Params params;
    params.file = filename;
    params.section = "test";
    params.option = "foo";
    params.expression = SystemdParameterExpression::Equal;
    params.value = "bar";

    const auto result = AuditEnsureSystemdParameterV4(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdConfigTest, V4_Match_1)
{
    const auto filename = mContext.MakeTempfile("");
    const std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[test]\n"
        "foo=bar\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config " + filename))).WillOnce(Return(Result<std::string>(systemdOutput)));

    EnsureSystemdParameterV4Params params;
    params.file = filename;
    params.section = "test";
    params.option = "foo";
    params.expression = SystemdParameterExpression::Equal;
    params.value = "bar";

    const auto result = AuditEnsureSystemdParameterV4(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, V4_Mismatch_1)
{
    const auto filename = mContext.MakeTempfile("");
    const std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[test]\n"
        "foo=baz\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config " + filename))).WillOnce(Return(Result<std::string>(systemdOutput)));

    EnsureSystemdParameterV4Params params;
    params.file = filename;
    params.section = "test";
    params.option = "foo";
    params.expression = SystemdParameterExpression::Equal;
    params.value = "bar";

    const auto result = AuditEnsureSystemdParameterV4(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdConfigTest, V4_Mismatch_2)
{
    const auto filename = mContext.MakeTempfile("");
    const std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[test]\n"
        "foo=bar\n"
        "foo=baz\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config " + filename))).WillOnce(Return(Result<std::string>(systemdOutput)));

    EnsureSystemdParameterV4Params params;
    params.file = filename;
    params.section = "test";
    params.option = "foo";
    params.expression = SystemdParameterExpression::Equal;
    params.value = "bar";

    const auto result = AuditEnsureSystemdParameterV4(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdConfigTest, V4_Match_2)
{
    const auto filename = mContext.MakeTempfile("");
    const std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[test]\n"
        "foo=baz\n"
        "foo=bar\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config " + filename))).WillOnce(Return(Result<std::string>(systemdOutput)));

    EnsureSystemdParameterV4Params params;
    params.file = filename;
    params.section = "test";
    params.option = "foo";
    params.expression = SystemdParameterExpression::Equal;
    params.value = "bar";

    const auto result = AuditEnsureSystemdParameterV4(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, V4_Match_4)
{
    const auto filename = mContext.MakeTempfile("");
    const std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[test]\n"
        "foo=\"baz\"\n"
        " foo = \"bar\"\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config " + filename))).WillOnce(Return(Result<std::string>(systemdOutput)));

    EnsureSystemdParameterV4Params params;
    params.file = filename;
    params.section = "test";
    params.option = "foo";
    params.expression = SystemdParameterExpression::Equal;
    params.value = "bar";

    const auto result = AuditEnsureSystemdParameterV4(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, V4_Match_5)
{
    const auto filename = mContext.MakeTempfile("");
    const std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[test]\n"
        "foo=3\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config " + filename))).WillOnce(Return(Result<std::string>(systemdOutput)));

    EnsureSystemdParameterV4Params params;
    params.file = filename;
    params.section = "test";
    params.option = "foo";
    params.expression = SystemdParameterExpression::LessThan;
    params.value = "4";

    const auto result = AuditEnsureSystemdParameterV4(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, V4_Match_6)
{
    const auto filename = mContext.MakeTempfile("");
    const std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[test]\n"
        "foo=5\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config " + filename))).WillOnce(Return(Result<std::string>(systemdOutput)));

    EnsureSystemdParameterV4Params params;
    params.file = filename;
    params.section = "test";
    params.option = "foo";
    params.expression = SystemdParameterExpression::GreaterThan;
    params.value = "4";

    const auto result = AuditEnsureSystemdParameterV4(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, V4_Match_7)
{
    const auto filename = mContext.MakeTempfile("");
    const std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[test]\n"
        "foo=4\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config " + filename))).WillOnce(Return(Result<std::string>(systemdOutput)));

    EnsureSystemdParameterV4Params params;
    params.file = filename;
    params.section = "test";
    params.option = "foo";
    params.expression = SystemdParameterExpression::LessOrEqual;
    params.value = "4";

    const auto result = AuditEnsureSystemdParameterV4(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, V4_Match_8)
{
    const auto filename = mContext.MakeTempfile("");
    const std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[test]\n"
        "foo=4\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config " + filename))).WillOnce(Return(Result<std::string>(systemdOutput)));

    EnsureSystemdParameterV4Params params;
    params.file = filename;
    params.section = "test";
    params.option = "foo";
    params.expression = SystemdParameterExpression::GreaterOrEqual;
    params.value = "4";

    const auto result = AuditEnsureSystemdParameterV4(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, V4_Misatch_3)
{
    const auto filename = mContext.MakeTempfile("");
    const std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[test]\n"
        "foo=4\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config " + filename))).WillOnce(Return(Result<std::string>(systemdOutput)));

    EnsureSystemdParameterV4Params params;
    params.file = filename;
    params.section = "test";
    params.option = "foo";
    params.expression = SystemdParameterExpression::LessThan;
    params.value = "4";

    const auto result = AuditEnsureSystemdParameterV4(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdConfigTest, V4_Misatch_4)
{
    const auto filename = mContext.MakeTempfile("");
    const std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[test]\n"
        "foo=4\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config " + filename))).WillOnce(Return(Result<std::string>(systemdOutput)));

    EnsureSystemdParameterV4Params params;
    params.file = filename;
    params.section = "test";
    params.option = "foo";
    params.expression = SystemdParameterExpression::GreaterThan;
    params.value = "4";

    const auto result = AuditEnsureSystemdParameterV4(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdConfigTest, V4_Misatch_5)
{
    const auto filename = mContext.MakeTempfile("");
    const std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[test]\n"
        "foo=5\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config " + filename))).WillOnce(Return(Result<std::string>(systemdOutput)));

    EnsureSystemdParameterV4Params params;
    params.file = filename;
    params.section = "test";
    params.option = "foo";
    params.expression = SystemdParameterExpression::LessOrEqual;
    params.value = "4";

    const auto result = AuditEnsureSystemdParameterV4(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdConfigTest, V4_Misatch_6)
{
    const auto filename = mContext.MakeTempfile("");
    const std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[test]\n"
        "foo=3\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config " + filename))).WillOnce(Return(Result<std::string>(systemdOutput)));

    EnsureSystemdParameterV4Params params;
    params.file = filename;
    params.section = "test";
    params.option = "foo";
    params.expression = SystemdParameterExpression::GreaterOrEqual;
    params.value = "4";

    const auto result = AuditEnsureSystemdParameterV4(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}
