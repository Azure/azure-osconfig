#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

using compliance::AuditExecuteCommandGrep;
using compliance::Error;
using compliance::IndicatorsTree;
using compliance::Result;
using compliance::Status;
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
    EXPECT_CALL(mContext, ExecuteCommand("iptables -L -n | grep -P -- \"test\"")).WillOnce(Return(Result<std::string>(Error("Command execution failed", -1))));

    std::map<std::string, std::string> args;
    args["command"] = "iptables -L -n";
    args["regex"] = "test";

    auto result = AuditExecuteCommandGrep(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(ExecuteCommandGrepTest, AuditCommandMatches)
{
    EXPECT_CALL(mContext, ExecuteCommand("iptables -L -n | grep -P -- \"test\"")).WillOnce(Return(Result<std::string>("test output")));

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
    EXPECT_CALL(mContext, ExecuteCommand("iptables -L -n | grep -E -- \"test\"")).WillOnce(Return(Result<std::string>("test output")));

    std::map<std::string, std::string> args;
    args["command"] = "iptables -L -n";
    args["regex"] = "test";
    args["type"] = "E";

    auto result = AuditExecuteCommandGrep(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
