#include "MockContext.h"

#include <ExecuteCommandGrep.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using ComplianceEngine::AuditExecuteCommandGrep;
using ComplianceEngine::Error;
using ComplianceEngine::ExecuteCommandGrepParams;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::RegexType;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using ::testing::Return;

class ExecuteCommandGrepTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree indicators;

    void SetUp() override
    {
        indicators.Push("ExecuteCommandGrep");
    }
};

TEST_F(ExecuteCommandGrepTest, AuditInvalidCommand)
{
    ExecuteCommandGrepParams params;
    params.command = "invalid command";
    params.regex = "test";

    auto result = AuditExecuteCommandGrep(params, indicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Command invalid command is not allowed");
}

TEST_F(ExecuteCommandGrepTest, AuditCommandFails)
{
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("iptables -L -n"))).WillOnce(Return(Result<std::string>(Error("Command execution failed", -1))));

    ExecuteCommandGrepParams params;
    params.command = "iptables -L -n";
    params.regex = "test";

    auto result = AuditExecuteCommandGrep(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(ExecuteCommandGrepTest, AuditCommandMatches)
{
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("iptables -L -n"))).WillOnce(Return(Result<std::string>("test output")));

    ExecuteCommandGrepParams params;
    params.command = "iptables -L -n";
    params.regex = "test";

    auto result = AuditExecuteCommandGrep(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ExecuteCommandGrepTest, AuditExtendedRegex)
{
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("iptables -L -n"))).WillOnce(Return(Result<std::string>("test output")));

    ExecuteCommandGrepParams params;
    params.command = "iptables -L -n";
    params.regex = "test";
    params.type = RegexType::Extended;

    auto result = AuditExecuteCommandGrep(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ExecuteCommandGrepTest, AuditWithAwkTransformation)
{
    EXPECT_CALL(mContext,
        ExecuteCommand(::testing::HasSubstr("iptables -L -n | awk -S \"{print \\$1}\"  | grep -P -- \"test\" || (echo -n 'No match found'; exit 1)")))
        .WillOnce(Return(Result<std::string>("test output")));

    ExecuteCommandGrepParams params;
    params.command = "iptables -L -n";
    params.awk = "{print $1}";
    params.regex = "test";

    auto result = AuditExecuteCommandGrep(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ExecuteCommandGrepTest, AuditWithAwkAndExtendedRegex)
{
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(
                              "iptables -L -n | awk -S \"{print \\$2}\"  | grep -E -- \"test.*pattern\" || (echo -n 'No match found'; exit 1)")))
        .WillOnce(Return(Result<std::string>("test matched pattern")));

    ExecuteCommandGrepParams params;
    params.command = "iptables -L -n";
    params.awk = "{print $2}";
    params.regex = "test.*pattern";
    params.type = RegexType::Extended;

    auto result = AuditExecuteCommandGrep(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ExecuteCommandGrepTest, AuditWithAwkSpecialCharactersEscaping)
{
    // Test that special characters in awk command are properly escaped
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(
                              "iptables -L -n | awk -S \"/^Chain/ {print \\$2}\"  | grep -P -- \"INPUT\" || (echo -n 'No match found'; exit 1)")))
        .WillOnce(Return(Result<std::string>("INPUT")));

    ExecuteCommandGrepParams params;
    params.command = "iptables -L -n";
    params.awk = "/^Chain/ {print $2}";
    params.regex = "INPUT";

    auto result = AuditExecuteCommandGrep(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ExecuteCommandGrepTest, AuditWithAwkComplexTransformation)
{
    EXPECT_CALL(mContext,
        ExecuteCommand(::testing::HasSubstr("uname | awk -S \"BEGIN{FS=\\\"\\\\n\\\"} {gsub(/\\\\s+/, \\\"\\\", \\$1); print \\$1}\"  | "
                                            "grep -P -- \"Linux\" || (echo -n 'No match found'; exit 1)")))
        .WillOnce(Return(Result<std::string>("Linux")));

    ExecuteCommandGrepParams params;
    params.command = "uname";
    params.awk = "BEGIN{FS=\"\\n\"} {gsub(/\\s+/, \"\", $1); print $1}";
    params.regex = "Linux";

    auto result = AuditExecuteCommandGrep(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ExecuteCommandGrepTest, AuditWithEmptyAwkParameter)
{
    // Test that empty awk parameter is handled correctly (should not add awk to pipeline)
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("iptables -L -n | grep -P -- \"test\" || (echo -n 'No match found'; exit 1)")))
        .WillOnce(Return(Result<std::string>("test output")));

    ExecuteCommandGrepParams params;
    params.command = "iptables -L -n";
    params.awk = "";
    params.regex = "test";

    auto result = AuditExecuteCommandGrep(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ExecuteCommandGrepTest, AuditWithAwkFailsAtGrep)
{
    // Test that when awk transforms output and grep doesn't match, it returns NonCompliant
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(
                              "iptables -L -n | awk -S \"{print \\$3}\"  | grep -P -- \"nonexistent\" || (echo -n 'No match found'; exit 1)")))
        .WillOnce(Return(Result<std::string>(Error("Command execution failed", 1))));

    ExecuteCommandGrepParams params;
    params.command = "iptables -L -n";
    params.awk = "{print $3}";
    params.regex = "nonexistent";

    auto result = AuditExecuteCommandGrep(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}
