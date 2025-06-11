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
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("[NonCompliant]") != std::string::npos);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("Failed to execute sshd") != std::string::npos);
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
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("[NonCompliant]") != std::string::npos);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("Failed to execute sshd") != std::string::npos);
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
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("[NonCompliant]") != std::string::npos);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("Failed to execute sshd") != std::string::npos);
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
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("[NonCompliant]") != std::string::npos);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("Failed to execute sshd") != std::string::npos);
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
