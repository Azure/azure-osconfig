// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <gtest/gtest.h>
#include <string>

using ComplianceEngine::AuditEnsureSshdOption;
using ComplianceEngine::CompactListFormatter;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
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

TEST_F(EnsureSshdOptionTest, MissingoptionArgument)
{
    std::map<std::string, std::string> args;
    args["value"] = "no";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Missing 'option' parameter");
}

TEST_F(EnsureSshdOptionTest, MissingvalueArgument)
{
    std::map<std::string, std::string> args;
    args["option"] = "permitrootlogin";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Missing 'value' parameter");
}

TEST_F(EnsureSshdOptionTest, InvalidRegex)
{
    std::map<std::string, std::string> args;
    args["option"] = "permitrootlogin";
    args["value"] = "(invalid[regex";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_TRUE(result.Error().message.find("Failed to compile regex") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, InitialCommandFails)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(Error("Command failed", -1))));

    std::map<std::string, std::string> args;
    args["option"] = "permitrootlogin";
    args["value"] = "no";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_TRUE(result.Error().message.find("Failed to execute sshd -T command") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, SimpleConfigOptionExists)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["option"] = "permitrootlogin";
    args["value"] = "no";

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
    args["option"] = "permitrootlogin";
    args["value"] = "yes";

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
    args["option"] = "nonexistentoption";
    args["value"] = ".*";

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
    args["option"] = "permitrootlogin";
    args["value"] = "no";

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
    args["option"] = "permitrootlogin";
    args["value"] = "no";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSshdOptionTest, HostnameCommandFailure)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(hostnameCommand)).WillOnce(Return(Result<std::string>(Error("Hostname command failed", -1))));

    std::map<std::string, std::string> args;
    args["option"] = "permitrootlogin";
    args["value"] = "no";

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
    args["option"] = "permitrootlogin";
    args["value"] = "no";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_TRUE(result.Error().message.find("Failed to get host address") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, RegexMatches)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["option"] = "maxauthtries";
    args["value"] = "[1-4]";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSshdOptionTest, RegexDoesNotMatch)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["option"] = "maxauthtries";
    args["value"] = "[5-9]";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSshdOptionTest, ComplexRegexMatches)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["option"] = "permitrootlogin";
    args["value"] = "^(no|prohibit-password)$";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSshdOptionTest, OperationNotMatch_Compliant)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["option"] = "permitrootlogin";
    args["value"] = "yes"; // forbidden
    args["op"] = "not_match";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSshdOptionTest, OperationNotMatch_NonCompliant)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["option"] = "permitrootlogin";
    args["value"] = "no"; // actual value matches forbidden pattern
    args["op"] = "not_match";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSshdOptionTest, OperationNumericLt_Compliant)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["option"] = "maxauthtries"; // value 4
    args["value"] = "5";
    args["op"] = "lt";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSshdOptionTest, OperationNumericLt_NonCompliant)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["option"] = "maxauthtries"; // value 4
    args["value"] = "3";
    args["op"] = "lt";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSshdOptionTest, OperationNumericGe_Compliant)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["option"] = "maxauthtries"; // value 4
    args["value"] = "4";
    args["op"] = "ge";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSshdOptionTest, OperationNumericGe_NonCompliant)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["option"] = "maxauthtries"; // value 4
    args["value"] = "5";
    args["op"] = "ge";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSshdOptionTest, OperationUnsupported)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["option"] = "permitrootlogin";
    args["value"] = "no";
    args["op"] = "invalidOp";

    auto result = AuditEnsureSshdOption(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_TRUE(result.Error().message.find("Unsupported op") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, NoOption_AllOptionsAbsent)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["options"] = "nonexistentoption1,nonexistentoption2";
    args["values"] = ".*no.*,.*yes.*";

    auto result = ComplianceEngine::AuditEnsureSshdNoOption(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    auto formatted = mFormatter.Format(mIndicators).Value();
    ASSERT_TRUE(formatted.find("Option 'nonexistentoption1' not found") != std::string::npos);
    ASSERT_TRUE(formatted.find("Option 'nonexistentoption2' not found") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, NoOption_OptionPresentWithCompliantValue)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["options"] = "permitrootlogin";
    args["values"] = "no";

    auto result = ComplianceEngine::AuditEnsureSshdNoOption(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    auto formatted = mFormatter.Format(mIndicators).Value();
    ASSERT_TRUE(formatted.find("Option 'permitrootlogin' has a compliant value 'no'") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, NoOption_OptionPresentWithNonCompliantValue)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["options"] = "maxauthtries";
    args["values"] = "5,6,7";

    auto result = ComplianceEngine::AuditEnsureSshdNoOption(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    auto formatted = mFormatter.Format(mIndicators).Value();
    ASSERT_TRUE(formatted.find("Option 'maxauthtries' has no compliant value in SSH daemon configuration") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, NoOption_InvalidRegex)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    std::map<std::string, std::string> args;
    args["options"] = "permitrootlogin";
    args["values"] = "(invalid[";

    auto result = ComplianceEngine::AuditEnsureSshdNoOption(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_TRUE(result.Error().message.find("Failed to compile regex") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, NoOption_MissingOptionsArgument)
{
    std::map<std::string, std::string> args;
    args["values"] = "no";

    auto result = ComplianceEngine::AuditEnsureSshdNoOption(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Missing 'options' parameter");
}

TEST_F(EnsureSshdOptionTest, NoOption_MissingValuesArgument)
{
    std::map<std::string, std::string> args;
    args["options"] = "permitrootlogin";

    auto result = ComplianceEngine::AuditEnsureSshdNoOption(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Missing 'values' parameter");
}

// ========================= Tests for EnsureSshdOptionMatch =========================

static const char sshdConfigWithMatches[] =
    "Port 22\n"
    "Match User alice\n"
    "Match Group admins\n"
    "Match Address 10.0.0.5/24\n"; // address truncated to 10.0.0.5

static const char sshdMatchUserAliceCommand[] = "sshd -T -C user=alice";
static const char sshdMatchGroupAdminsCommand[] = "sshd -T -C group=admins";
static const char sshdMatchAddress10005Command[] = "sshd -T -C address=10.0.0.5";

static const char sshdMatchOutputPermitRootLoginNo[] =
    "permitrootlogin no\n"
    "maxauthtries 4\n";

static const char sshdMatchOutputPermitRootLoginYes[] = "permitrootlogin yes\n";

TEST_F(EnsureSshdOptionTest, Match_MissingOptionArgument)
{
    std::map<std::string, std::string> args;
    args["value"] = "no";
    auto result = ComplianceEngine::AuditEnsureSshdOptionMatch(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Missing 'option' parameter");
}

TEST_F(EnsureSshdOptionTest, Match_MissingValueArgument)
{
    std::map<std::string, std::string> args;
    args["option"] = "permitrootlogin";
    auto result = ComplianceEngine::AuditEnsureSshdOptionMatch(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Missing 'value' parameter");
}

TEST_F(EnsureSshdOptionTest, Match_InvalidRegex)
{
    std::map<std::string, std::string> args;
    args["option"] = "permitrootlogin";
    args["value"] = "(invalid[";
    auto result = ComplianceEngine::AuditEnsureSshdOptionMatch(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_TRUE(result.Error().message.find("Failed to compile regex") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, Match_AllCompliant)
{
    // GetAllMatches() file read
    EXPECT_CALL(mContext, GetFileContents("/etc/ssh/sshd_config")).WillOnce(Return(Result<std::string>(sshdConfigWithMatches)));

    // For each match context we expect a compliant option value
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchUserAliceCommand)).WillOnce(Return(Result<std::string>(sshdMatchOutputPermitRootLoginNo)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchGroupAdminsCommand)).WillOnce(Return(Result<std::string>(sshdMatchOutputPermitRootLoginNo)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchAddress10005Command)).WillOnce(Return(Result<std::string>(sshdMatchOutputPermitRootLoginNo)));

    std::map<std::string, std::string> args;
    args["option"] = "permitrootlogin";
    args["value"] = "no";

    auto result = ComplianceEngine::AuditEnsureSshdOptionMatch(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    auto formatted = mFormatter.Format(mIndicators).Value();
    ASSERT_TRUE(formatted.find("All possible match options are compliant") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, Match_FirstNonCompliantShortCircuits)
{
    EXPECT_CALL(mContext, GetFileContents("/etc/ssh/sshd_config")).WillOnce(Return(Result<std::string>(sshdConfigWithMatches)));
    // First match returns non-compliant (yes), subsequent commands must not be invoked
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchUserAliceCommand)).WillOnce(Return(Result<std::string>(sshdMatchOutputPermitRootLoginYes)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchGroupAdminsCommand)).Times(0);
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchAddress10005Command)).Times(0);

    std::map<std::string, std::string> args;
    args["option"] = "permitrootlogin";
    args["value"] = "no"; // expecting 'no'

    auto result = ComplianceEngine::AuditEnsureSshdOptionMatch(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSshdOptionTest, Match_OptionMissingInOneContext)
{
    EXPECT_CALL(mContext, GetFileContents("/etc/ssh/sshd_config")).WillOnce(Return(Result<std::string>(sshdConfigWithMatches)));
    // Return config that does not contain the option
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchUserAliceCommand)).WillOnce(Return(Result<std::string>("maxauthtries 4\n")));
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchGroupAdminsCommand)).Times(0);
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchAddress10005Command)).Times(0);

    std::map<std::string, std::string> args;
    args["option"] = "permitrootlogin";
    args["value"] = ".*";

    auto result = ComplianceEngine::AuditEnsureSshdOptionMatch(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant); // option missing treated as non-compliant
}

TEST_F(EnsureSshdOptionTest, Match_NotMatchOperation_Compliant)
{
    EXPECT_CALL(mContext, GetFileContents("/etc/ssh/sshd_config")).WillOnce(Return(Result<std::string>(sshdConfigWithMatches)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchUserAliceCommand)).WillOnce(Return(Result<std::string>(sshdMatchOutputPermitRootLoginNo)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchGroupAdminsCommand)).WillOnce(Return(Result<std::string>(sshdMatchOutputPermitRootLoginNo)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchAddress10005Command)).WillOnce(Return(Result<std::string>(sshdMatchOutputPermitRootLoginNo)));

    std::map<std::string, std::string> args;
    args["option"] = "permitrootlogin";
    args["value"] = "yes"; // forbidden
    args["op"] = "not_match";

    auto result = ComplianceEngine::AuditEnsureSshdOptionMatch(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSshdOptionTest, Match_NotMatchOperation_NonCompliant)
{
    EXPECT_CALL(mContext, GetFileContents("/etc/ssh/sshd_config")).WillOnce(Return(Result<std::string>(sshdConfigWithMatches)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchUserAliceCommand)).WillOnce(Return(Result<std::string>(sshdMatchOutputPermitRootLoginNo)));
    // Since first context already matches forbidden, short circuit; no further commands expected
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchGroupAdminsCommand)).Times(0);
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchAddress10005Command)).Times(0);

    std::map<std::string, std::string> args;
    args["option"] = "permitrootlogin";
    args["value"] = "no"; // actual value matches forbidden pattern
    args["op"] = "not_match";

    auto result = ComplianceEngine::AuditEnsureSshdOptionMatch(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSshdOptionTest, Match_NumericLt_Compliant)
{
    EXPECT_CALL(mContext, GetFileContents("/etc/ssh/sshd_config")).WillOnce(Return(Result<std::string>(sshdConfigWithMatches)));
    const char numericConfig[] = "maxauthtries 3\n";
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchUserAliceCommand)).WillOnce(Return(Result<std::string>(numericConfig)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchGroupAdminsCommand)).WillOnce(Return(Result<std::string>(numericConfig)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchAddress10005Command)).WillOnce(Return(Result<std::string>(numericConfig)));

    std::map<std::string, std::string> args;
    args["option"] = "maxauthtries";
    args["value"] = "5";
    args["op"] = "lt";

    auto result = ComplianceEngine::AuditEnsureSshdOptionMatch(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSshdOptionTest, Match_NumericLt_NonCompliant)
{
    EXPECT_CALL(mContext, GetFileContents("/etc/ssh/sshd_config")).WillOnce(Return(Result<std::string>(sshdConfigWithMatches)));
    const char numericConfig[] = "maxauthtries 6\n"; // 6 !< 5
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchUserAliceCommand)).WillOnce(Return(Result<std::string>(numericConfig)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchGroupAdminsCommand)).Times(0);
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchAddress10005Command)).Times(0);

    std::map<std::string, std::string> args;
    args["option"] = "maxauthtries";
    args["value"] = "5";
    args["op"] = "lt";

    auto result = ComplianceEngine::AuditEnsureSshdOptionMatch(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSshdOptionTest, Match_UnsupportedOp)
{
    EXPECT_CALL(mContext, GetFileContents("/etc/ssh/sshd_config")).WillOnce(Return(Result<std::string>(sshdConfigWithMatches)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchUserAliceCommand)).WillOnce(Return(Result<std::string>(sshdMatchOutputPermitRootLoginNo)));

    std::map<std::string, std::string> args;
    args["option"] = "permitrootlogin";
    args["value"] = "no";
    args["op"] = "someInvalid";

    auto result = ComplianceEngine::AuditEnsureSshdOptionMatch(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_TRUE(result.Error().message.find("Unsupported op") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, Match_FileReadFailure_NoMatchesReturnsCompliant)
{
    EXPECT_CALL(mContext, GetFileContents("/etc/ssh/sshd_config")).WillOnce(Return(Result<std::string>(Error("read error", -1))));

    std::map<std::string, std::string> args;
    args["option"] = "permitrootlogin";
    args["value"] = "no";

    auto result = ComplianceEngine::AuditEnsureSshdOptionMatch(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    // No matches => loop skipped => Compliant per current implementation
    ASSERT_EQ(result.Value(), Status::Compliant);
}
