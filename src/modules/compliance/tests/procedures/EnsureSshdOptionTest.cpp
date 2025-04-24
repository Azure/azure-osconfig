// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <gtest/gtest.h>
#include <string>

using compliance::AuditEnsureSshdOption;
using compliance::CompactListFormatter;
using compliance::Error;
using compliance::IndicatorsTree;
using compliance::Result;
using compliance::Status;
using ::testing::Return;

static const char sshdInitialCommand[] = "sshd -T 2>&1";
static const char hostnameCommand[] = "hostname";
static const char hostAddressCommand[] = "hostname -I | cut -d ' ' -f1";
static const char sshdSimpleCommand[] = "sshd -T";
static const char sshdComplexCommand[] = "sshd -T -C user=root -C host=testhost -C addr=1.2.3.4";

static const char sshdWithoutMatchGroupOutput[] =
    "port 22\n"
    "addressfamily any\n"
    "listenaddress 0.0.0.0\n"
    "permitrootlogin no\n"
    "maxauthtries 4\n"
    "pubkeyauthentication yes\n"
    "passwordauthentication no\n"
    "permitemptypasswords no\n"
    "kbdinteractiveauthentication no\n"
    "usepam yes\n"
    "x11forwarding no\n"
    "permituserpam no\n";

static const char sshdWithMatchGroupOutput[] =
    "port 22\n"
    "addressfamily any\n"
    "listenaddress 0.0.0.0\n"
    "match group admins\n"
    "permitrootlogin no\n"
    "maxauthtries 4\n"
    "pubkeyauthentication yes\n"
    "passwordauthentication no\n"
    "permitemptypasswords no\n"
    "kbdinteractiveauthentication no\n"
    "usepam yes\n"
    "x11forwarding no\n"
    "permituserpam no\n";

class EnsureSshdOptionTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;
    CompactListFormatter mFormatter;

    void SetUp() override
    {
        mIndicators.Push("EnsureSshdOption");
    }
};

TEST_F(EnsureSshdOptionTest, MissingOptionNameArgument)
{
    std::map<std::string, std::string> args;
    args["optionRegex"] = "no";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Missing 'optionName' parameter");
}

TEST_F(EnsureSshdOptionTest, MissingOptionRegexArgument)
{
    std::map<std::string, std::string> args;
    args["optionName"] = "permitrootlogin";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Missing 'optionRegex' parameter");
}

TEST_F(EnsureSshdOptionTest, InvalidRegex)
{
    std::map<std::string, std::string> args;
    args["optionName"] = "permitrootlogin";
    args["optionRegex"] = "(invalid[regex";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_TRUE(result.Error().message.find("Failed to compile regex") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, InitialCommandFails)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(Error("Command failed", -1))));

    std::map<std::string, std::string> args;
    args["optionName"] = "permitrootlogin";
    args["optionRegex"] = "no";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_TRUE(result.Error().message.find("Failed to execute sshd -T command") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, SimpleConfigOptionExists)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["optionName"] = "permitrootlogin";
    args["optionRegex"] = "no";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("[Compliant]") != std::string::npos);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("Option 'permitrootlogin' has a compliant value 'no'") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, SimpleConfigOptionMismatch)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["optionName"] = "permitrootlogin";
    args["optionRegex"] = "yes";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("[NonCompliant]") != std::string::npos);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("which does not match required pattern 'yes'") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, ConfigOptionNotFound)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["optionName"] = "nonexistentoption";
    args["optionRegex"] = ".*";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("Option 'nonexistentoption' not found") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, CommandFailure)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(Error("Command execution failed", -1))));

    std::map<std::string, std::string> args;
    args["optionName"] = "permitrootlogin";
    args["optionRegex"] = "no";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_TRUE(result.Error().message.find("Failed to execute sshd -T") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, WithMatchGroupConfig)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(hostnameCommand)).WillOnce(Return(Result<std::string>("testhost\n")));

    EXPECT_CALL(mContext, ExecuteCommand(hostAddressCommand)).WillOnce(Return(Result<std::string>("1.2.3.4\n")));

    EXPECT_CALL(mContext, ExecuteCommand(sshdComplexCommand)).WillOnce(Return(Result<std::string>(sshdWithMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["optionName"] = "permitrootlogin";
    args["optionRegex"] = "no";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSshdOptionTest, HostnameCommandFailure)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(hostnameCommand)).WillOnce(Return(Result<std::string>(Error("Hostname command failed", -1))));

    std::map<std::string, std::string> args;
    args["optionName"] = "permitrootlogin";
    args["optionRegex"] = "no";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_TRUE(result.Error().message.find("Failed to execute hostname command") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, HostAddressCommandFailure)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(hostnameCommand)).WillOnce(Return(Result<std::string>("testhost\n")));

    EXPECT_CALL(mContext, ExecuteCommand(hostAddressCommand)).WillOnce(Return(Result<std::string>(Error("Host address command failed", -1))));

    std::map<std::string, std::string> args;
    args["optionName"] = "permitrootlogin";
    args["optionRegex"] = "no";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_TRUE(result.Error().message.find("Failed to get host address") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, RegexMatches)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["optionName"] = "maxauthtries";
    args["optionRegex"] = "[1-4]";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSshdOptionTest, RegexDoesNotMatch)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["optionName"] = "maxauthtries";
    args["optionRegex"] = "[5-9]";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSshdOptionTest, ComplexRegexMatches)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["optionName"] = "permitrootlogin";
    args["optionRegex"] = "^(no|prohibit-password)$";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
