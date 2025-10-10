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
