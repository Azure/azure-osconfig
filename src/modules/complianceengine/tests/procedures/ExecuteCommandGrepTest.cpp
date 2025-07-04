#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

using ComplianceEngine::AuditExecuteCommandGrep;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
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

TEST_F(ExecuteCommandGrepTest, AuditNoCommand)
{
    std::map<std::string, std::string> args;
    args["regex"] = "test";

    auto result = AuditExecuteCommandGrep(args, indicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "No command name provided");
}

TEST_F(ExecuteCommandGrepTest, AuditNoRegex)
{
    std::map<std::string, std::string> args;
    args["command"] = "iptables -L -n";

    auto result = AuditExecuteCommandGrep(args, indicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "No regex provided");
}

TEST_F(ExecuteCommandGrepTest, AuditInvalidCommand)
{
    std::map<std::string, std::string> args;
    args["command"] = "invalid command";
    args["regex"] = "test";

    auto result = AuditExecuteCommandGrep(args, indicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Command invalid command is not allowed");
}

TEST_F(ExecuteCommandGrepTest, AuditCommandFails)
{
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("iptables -L -n"))).WillOnce(Return(Result<std::string>(Error("Command execution failed", -1))));

    std::map<std::string, std::string> args;
    args["command"] = "iptables -L -n";
    args["regex"] = "test";

    auto result = AuditExecuteCommandGrep(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(ExecuteCommandGrepTest, AuditCommandMatches)
{
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("iptables -L -n"))).WillOnce(Return(Result<std::string>("test output")));

    std::map<std::string, std::string> args;
    args["command"] = "iptables -L -n";
    args["regex"] = "test";

    auto result = AuditExecuteCommandGrep(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ExecuteCommandGrepTest, AuditInvalidRegexType)
{
    std::map<std::string, std::string> args;
    args["command"] = "iptables -L -n";
    args["regex"] = "test";
    args["type"] = "X";

    auto result = AuditExecuteCommandGrep(args, indicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Invalid regex type, only P(erl) and E(xtended) are allowed");
}

TEST_F(ExecuteCommandGrepTest, AuditExtendedRegex)
{
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("iptables -L -n"))).WillOnce(Return(Result<std::string>("test output")));

    std::map<std::string, std::string> args;
    args["command"] = "iptables -L -n";
    args["regex"] = "test";
    args["type"] = "E";

    auto result = AuditExecuteCommandGrep(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ExecuteCommandGrepTest, AuditWithAwkTransformation)
{
    EXPECT_CALL(mContext,
        ExecuteCommand(::testing::HasSubstr("iptables -L -n | awk -S \"{print \\$1}\"  | grep -P -- \"test\" || (echo -n 'No match found'; exit 1)")))
        .WillOnce(Return(Result<std::string>("test output")));

    std::map<std::string, std::string> args;
    args["command"] = "iptables -L -n";
    args["awk"] = "{print $1}";
    args["regex"] = "test";

    auto result = AuditExecuteCommandGrep(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ExecuteCommandGrepTest, AuditWithAwkAndExtendedRegex)
{
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(
                              "iptables -L -n | awk -S \"{print \\$2}\"  | grep -E -- \"test.*pattern\" || (echo -n 'No match found'; exit 1)")))
        .WillOnce(Return(Result<std::string>("test matched pattern")));

    std::map<std::string, std::string> args;
    args["command"] = "iptables -L -n";
    args["awk"] = "{print $2}";
    args["regex"] = "test.*pattern";
    args["type"] = "E";

    auto result = AuditExecuteCommandGrep(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ExecuteCommandGrepTest, AuditWithAwkSpecialCharactersEscaping)
{
    // Test that special characters in awk command are properly escaped
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(
                              "iptables -L -n | awk -S \"/^Chain/ {print \\$2}\"  | grep -P -- \"INPUT\" || (echo -n 'No match found'; exit 1)")))
        .WillOnce(Return(Result<std::string>("INPUT")));

    std::map<std::string, std::string> args;
    args["command"] = "iptables -L -n";
    args["awk"] = "/^Chain/ {print $2}";
    args["regex"] = "INPUT";

    auto result = AuditExecuteCommandGrep(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ExecuteCommandGrepTest, AuditWithAwkComplexTransformation)
{
    EXPECT_CALL(mContext,
        ExecuteCommand(::testing::HasSubstr("uname | awk -S \"BEGIN{FS=\\\"\\\\n\\\"} {gsub(/\\\\s+/, \\\"\\\", \\$1); print \\$1}\"  | "
                                            "grep -P -- \"Linux\" || (echo -n 'No match found'; exit 1)")))
        .WillOnce(Return(Result<std::string>("Linux")));

    std::map<std::string, std::string> args;
    args["command"] = "uname";
    args["awk"] = "BEGIN{FS=\"\\n\"} {gsub(/\\s+/, \"\", $1); print $1}";
    args["regex"] = "Linux";

    auto result = AuditExecuteCommandGrep(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ExecuteCommandGrepTest, AuditWithEmptyAwkParameter)
{
    // Test that empty awk parameter is handled correctly (should not add awk to pipeline)
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr("iptables -L -n | grep -P -- \"test\" || (echo -n 'No match found'; exit 1)")))
        .WillOnce(Return(Result<std::string>("test output")));

    std::map<std::string, std::string> args;
    args["command"] = "iptables -L -n";
    args["awk"] = "";
    args["regex"] = "test";

    auto result = AuditExecuteCommandGrep(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(ExecuteCommandGrepTest, AuditWithAwkFailsAtGrep)
{
    // Test that when awk transforms output and grep doesn't match, it returns NonCompliant
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(
                              "iptables -L -n | awk -S \"{print \\$3}\"  | grep -P -- \"nonexistent\" || (echo -n 'No match found'; exit 1)")))
        .WillOnce(Return(Result<std::string>(Error("Command execution failed", 1))));

    std::map<std::string, std::string> args;
    args["command"] = "iptables -L -n";
    args["awk"] = "{print $3}";
    args["regex"] = "nonexistent";

    auto result = AuditExecuteCommandGrep(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}
