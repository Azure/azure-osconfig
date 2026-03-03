// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "MockContext.h"

#include <SystemdConfig.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using ComplianceEngine::AuditSystemdParameter;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using ComplianceEngine::SystemdParameterOperator;
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

// --- Tests for op + value comparisons ---

TEST_F(SystemdConfigTest, NeitherValueNorValueRegexProvided)
{
    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.file = "test.conf";
    // Neither value nor valueRegex set

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "'value' (or 'valueRegex') must be provided");
}

TEST_F(SystemdConfigTest, OpWithoutValueProvided)
{
    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.file = "test.conf";
    params.op = SystemdParameterOperator::Equal;
    // Neither value nor valueRegex set

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "'value' (or 'valueRegex') must be provided");
}

TEST_F(SystemdConfigTest, OpWithValueRegexNotAllowed)
{
    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.file = "test.conf";
    params.op = SystemdParameterOperator::LessThan;
    params.valueRegex = regex("^[0-9]+$");
    // op requires value, not valueRegex

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "'op' requires 'value' (not 'valueRegex')");
}

TEST_F(SystemdConfigTest, BothValueAndValueRegexProvided)
{
    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.file = "test.conf";
    params.value = std::string("99");
    params.valueRegex = regex("^42$");

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Both 'value' and 'valueRegex' are provided, only one is allowed");
}

TEST_F(SystemdConfigTest, ValueWithoutOpTreatedAsRegexCompliant)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=correctvalue\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.value = std::string("^correctvalue$");
    params.file = "test.conf";
    // No op, so value is treated as regex

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, ValueWithoutOpTreatedAsRegexNonCompliant)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=wrongvalue\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.value = std::string("^correctvalue$");
    params.file = "test.conf";
    // No op, so value is treated as regex

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdConfigTest, ValueWithoutOpRegexPattern)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=65536\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.value = std::string("^[0-9]+$");
    params.file = "test.conf";
    // No op, so value is treated as regex

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, OperatorEqualCompliant)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=hello\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.op = SystemdParameterOperator::Equal;
    params.value = std::string("hello");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, OperatorEqualNonCompliant)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=world\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.op = SystemdParameterOperator::Equal;
    params.value = std::string("hello");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdConfigTest, OperatorLessThanCompliant)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=5\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.op = SystemdParameterOperator::LessThan;
    params.value = std::string("10");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, OperatorLessThanNonCompliant)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=10\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.op = SystemdParameterOperator::LessThan;
    params.value = std::string("10");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdConfigTest, OperatorLessOrEqualCompliant)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=10\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.op = SystemdParameterOperator::LessOrEqual;
    params.value = std::string("10");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, OperatorLessOrEqualNonCompliant)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=11\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.op = SystemdParameterOperator::LessOrEqual;
    params.value = std::string("10");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdConfigTest, OperatorGreaterThanCompliant)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=100\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.op = SystemdParameterOperator::GreaterThan;
    params.value = std::string("50");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, OperatorGreaterThanNonCompliant)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=50\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.op = SystemdParameterOperator::GreaterThan;
    params.value = std::string("50");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdConfigTest, OperatorGreaterOrEqualCompliant)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=50\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.op = SystemdParameterOperator::GreaterOrEqual;
    params.value = std::string("50");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, OperatorGreaterOrEqualNonCompliant)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=49\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.op = SystemdParameterOperator::GreaterOrEqual;
    params.value = std::string("50");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdConfigTest, NumericComparisonWithNonNumericActualValue)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=notanumber\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.op = SystemdParameterOperator::LessThan;
    params.value = std::string("10");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    EXPECT_NE(result.Error().message.find("Failed to convert values to numbers"), std::string::npos);
}

TEST_F(SystemdConfigTest, NumericComparisonWithNonNumericExpectedValue)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=42\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.op = SystemdParameterOperator::GreaterThan;
    params.value = std::string("abc");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    EXPECT_NE(result.Error().message.find("Failed to convert values to numbers"), std::string::npos);
}

TEST_F(SystemdConfigTest, OperatorEqualWithNumericStrings)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "TestParam=42\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "TestParam";
    params.op = SystemdParameterOperator::Equal;
    params.value = std::string("42");
    params.file = "test.conf";

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

// --- Tests for block parameter ---

TEST_F(SystemdConfigTest, BlockParameterFoundInCorrectBlock)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[Service]\n"
        "ExecStart=/usr/bin/test\n"
        "Restart=always\n"
        "[Unit]\n"
        "Description=Test Unit\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "Restart";
    params.valueRegex = regex("^always$");
    params.file = "test.conf";
    params.block = std::string("[Service]");

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, BlockParameterNotFoundInWrongBlock)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[Service]\n"
        "ExecStart=/usr/bin/test\n"
        "Restart=always\n"
        "[Unit]\n"
        "Description=Test Unit\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "Restart";
    params.valueRegex = regex("^always$");
    params.file = "test.conf";
    params.block = std::string("[Unit]");

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdConfigTest, BlockParameterNotFoundInNonexistentBlock)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[Service]\n"
        "ExecStart=/usr/bin/test\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "ExecStart";
    params.valueRegex = regex(".*");
    params.file = "test.conf";
    params.block = std::string("[Install]");

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(SystemdConfigTest, SameParameterInDifferentBlocksWithBlockFilter)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[Service]\n"
        "Type=simple\n"
        "[Socket]\n"
        "Type=stream\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "Type";
    params.op = SystemdParameterOperator::Equal;
    params.value = std::string("stream");
    params.file = "test.conf";
    params.block = std::string("[Socket]");

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, SameParameterInDifferentBlocksWithoutBlockFilter)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[Service]\n"
        "Type=simple\n"
        "[Socket]\n"
        "Type=stream\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "Type";
    params.op = SystemdParameterOperator::Equal;
    params.value = std::string("simple");
    params.file = "test.conf";
    // No block filter - should find the first occurrence

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, BlockWithOperatorComparison)
{
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "[Service]\n"
        "LimitNOFILE=65536\n"
        "[Unit]\n"
        "StartLimitBurst=5\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "LimitNOFILE";
    params.op = SystemdParameterOperator::GreaterOrEqual;
    params.value = std::string("1024");
    params.file = "test.conf";
    params.block = std::string("[Service]");

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(SystemdConfigTest, ParameterWithoutBlockHeaderFoundWithoutBlockFilter)
{
    // Parameters before any block header have an empty block
    std::string systemdOutput =
        "# /etc/systemd/test.conf\n"
        "GlobalParam=globalvalue\n"
        "[Service]\n"
        "ServiceParam=servicevalue\n";

    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("/usr/bin/systemd-analyze cat-config test.conf"))).WillOnce(Return(Result<std::string>(systemdOutput)));

    SystemdParameterParams params;
    params.parameter = "GlobalParam";
    params.valueRegex = regex("^globalvalue$");
    params.file = "test.conf";
    // No block filter

    auto result = AuditSystemdParameter(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
