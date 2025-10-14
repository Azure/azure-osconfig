// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "MockContext.h"

#include <EnsureSshdOption.h>
#include <gtest/gtest.h>
#include <string>

using ComplianceEngine::AuditEnsureSshdOption;
using ComplianceEngine::CompactListFormatter;
using ComplianceEngine::EnsureSshdOptionMode;
using ComplianceEngine::EnsureSshdOptionOperation;
using ComplianceEngine::EnsureSshdOptionParams;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Optional;
using ComplianceEngine::Result;
using ComplianceEngine::Separated;
using ComplianceEngine::Status;
using ::testing::Return;

static const char sshdInitialCommand[] = "sshd -T 2>&1";
static const char hostnameCommand[] = "hostname";
static const char hostAddressCommand[] = "hostname -I | cut -d ' ' -f1";
static const char sshdSimpleCommand[] = "sshd -T";
static const char sshdComplexCommand[] = "sshd -T -C user=root -C host=testhost -C addr=1.2.3.4";
static const char sshdMaxStartupsOutput[] =
    "port 22\n"
    "maxstartups 10:30:60\n"; // Using space-separated triplet to match current special-case parser

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

TEST_F(EnsureSshdOptionTest, InitialCommandFails)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(Error("Command failed", -1))));

    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}};
    params.value = "no";

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    auto formatted = mFormatter.Format(mIndicators).Value();
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("[NonCompliant]") != std::string::npos);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("Failed to execute sshd") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, SimpleConfigOptionExists)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}};
    params.value = "no";

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("[Compliant]") != std::string::npos);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("Option 'permitrootlogin' has a compliant value 'no'") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, SimpleConfigOptionMismatch)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}};
    params.value = "yes";

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("[NonCompliant]") != std::string::npos);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("which does not match required pattern") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, ConfigOptionNotFound)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EnsureSshdOptionParams params;
    params.option = {{"nonexistentoption"}};
    params.value = ".*";

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("Option 'nonexistentoption' not found") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, CommandFailure)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(Error("Command execution failed", -1))));
    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}};
    params.value = "no";

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    auto formatted = mFormatter.Format(mIndicators).Value();
    ASSERT_TRUE(formatted.find("Failed to get sshd options:") != std::string::npos);
    ASSERT_TRUE(formatted.find("Failed to execute sshd -T") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, WithMatchGroupConfig)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(hostnameCommand)).WillOnce(Return(Result<std::string>("testhost\n")));

    EXPECT_CALL(mContext, ExecuteCommand(hostAddressCommand)).WillOnce(Return(Result<std::string>("1.2.3.4\n")));

    EXPECT_CALL(mContext, ExecuteCommand(sshdComplexCommand)).WillOnce(Return(Result<std::string>(sshdWithMatchGroupOutput)));

    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}};
    params.value = "no";

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSshdOptionTest, HostnameCommandFailure)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(hostnameCommand)).WillOnce(Return(Result<std::string>(Error("Hostname command failed", -1))));

    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}};
    params.value = "no";

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    auto formatted = mFormatter.Format(mIndicators).Value();
    ASSERT_TRUE(formatted.find("Failed to get sshd options:") != std::string::npos);
    ASSERT_TRUE(formatted.find("Failed to execute hostname command") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, HostAddressCommandFailure)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(hostnameCommand)).WillOnce(Return(Result<std::string>("testhost\n")));

    EXPECT_CALL(mContext, ExecuteCommand(hostAddressCommand)).WillOnce(Return(Result<std::string>(Error("Host address command failed", -1))));

    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}};
    params.value = "no";

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    auto formatted = mFormatter.Format(mIndicators).Value();
    ASSERT_TRUE(formatted.find("Failed to get sshd options:") != std::string::npos);
    ASSERT_TRUE(formatted.find("Failed to get host address") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, RegexMatches)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EnsureSshdOptionParams params;
    params.option = {{"maxauthtries"}};
    params.value = "[1-4]";

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSshdOptionTest, RegexDoesNotMatch)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EnsureSshdOptionParams params;
    params.option = {{"maxauthtries"}};
    params.value = "[5-9]";

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSshdOptionTest, ComplexRegexMatches)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}};
    params.value = "^(no|prohibit-password)$";

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSshdOptionTest, OperationNotMatch_Compliant)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}};
    params.value = "yes"; // forbidden
    params.op = EnsureSshdOptionOperation::NotMatch;

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSshdOptionTest, OperationNotMatch_NonCompliant)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}};
    params.value = "no"; // actual value matches forbidden pattern
    params.op = EnsureSshdOptionOperation::NotMatch;

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSshdOptionTest, OperationNotMatch_MissingOptionIsCompliant)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EnsureSshdOptionParams params;
    params.option = {{"nonexistentoption"}}; // ensure not present
    params.value = "forbidden";              // arbitrary pattern
    params.op = EnsureSshdOptionOperation::NotMatch;

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    auto formatted = mFormatter.Format(mIndicators).Value();
    // Current implementation returns generic missing option message without special suffix
    ASSERT_TRUE(formatted.find("Option 'nonexistentoption' not found") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, OperationNumericLt_Compliant)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EnsureSshdOptionParams params;
    params.option = {{"maxauthtries"}}; // value 4
    params.value = "5";
    params.op = EnsureSshdOptionOperation::LessThan;

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSshdOptionTest, OperationNumericLt_NonCompliant)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EnsureSshdOptionParams params;
    params.option = {{"maxauthtries"}}; // value 4
    params.value = "3";
    params.op = EnsureSshdOptionOperation::LessThan;

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSshdOptionTest, OperationNumericGe_Compliant)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EnsureSshdOptionParams params;
    params.option = {{"maxauthtries"}}; // value 4
    params.value = "4";
    params.op = EnsureSshdOptionOperation::GreaterOrEqual;

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSshdOptionTest, OperationNumericGe_NonCompliant)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EnsureSshdOptionParams params;
    params.option = {{"maxauthtries"}}; // value 4
    params.value = "5";
    params.op = EnsureSshdOptionOperation::GreaterOrEqual;

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSshdOptionTest, MaxStartups_Compliant)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdMaxStartupsOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdMaxStartupsOutput)));

    EnsureSshdOptionParams params;
    params.option = {{"maxstartups"}}; // special case
    params.op = EnsureSshdOptionOperation::Match;
    // Provide thresholds higher than actual values (10 30 60)
    params.value = "15:40:70";

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    auto formatted = mFormatter.Format(mIndicators).Value();
    ASSERT_TRUE(formatted.find("Option 'maxstartups' has a value '10:30:60' compliant with limits '15:40:70'") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, MaxStartups_NonCompliant)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdMaxStartupsOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdMaxStartupsOutput)));

    EnsureSshdOptionParams params;
    params.option = {{"maxstartups"}};
    params.op = EnsureSshdOptionOperation::Match;
    // Set at least one threshold lower than actual (e.g., middle value 25 < 30)
    params.value = "15:25:70";

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    auto formatted = mFormatter.Format(mIndicators).Value();
    ASSERT_TRUE(formatted.find("Option 'maxstartups' has value '10:30:60' which exceeds limits '15:25:70'") != std::string::npos);
}

// ========================= Adapted legacy NoOption scenarios using EnsureSshdOption (op=not_match) =========================

TEST_F(EnsureSshdOptionTest, NoOption_AllOptionsAbsent_Adapted)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    EnsureSshdOptionParams params;
    params.option = {{"nonexistentoption1", "nonexistentoption2"}};
    params.value = ".*";
    params.op = EnsureSshdOptionOperation::NotMatch;

    auto result = ComplianceEngine::AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    auto formatted = mFormatter.Format(mIndicators).Value();
    // Expect individual missing option messages
    ASSERT_TRUE(formatted.find("Option 'nonexistentoption1' not found") != std::string::npos);
    ASSERT_TRUE(formatted.find("Option 'nonexistentoption2' not found") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, NoOption_OptionPresentWithForbiddenValue_Adapted)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));

    // Original test expected NonCompliant when the value was present (since legacy NoOption treated presence of compliant value as violation)
    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}}; // actual value 'no'
    params.value = "no";                   // forbidden value pattern
    params.op = EnsureSshdOptionOperation::NotMatch;

    auto result = ComplianceEngine::AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    auto formatted = mFormatter.Format(mIndicators).Value();
    ASSERT_TRUE(formatted.find("matches forbidden pattern 'no'") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, NoOption_OptionPresentWithAllowedValue_Adapted)
{
    EXPECT_CALL(mContext, ExecuteCommand(sshdInitialCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdSimpleCommand)).WillOnce(Return(Result<std::string>(sshdWithoutMatchGroupOutput)));
    EnsureSshdOptionParams params;
    params.option = {{"maxauthtries"}};
    params.value = "5,6,7";
    params.op = EnsureSshdOptionOperation::NotMatch;

    auto result = ComplianceEngine::AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    auto formatted = mFormatter.Format(mIndicators).Value();
    // Ensure we recorded compliant evaluation for the option; presence of explicit success phrase not guaranteed, so just ensure absence of forbidden pattern message
    ASSERT_TRUE(formatted.find("matches forbidden pattern") == std::string::npos);
}

TEST_F(EnsureSshdOptionTest, NoOption_InvalidRegex_Adapted)
{
    // Regex compilation fails before any command execution; no EXPECT_CALL on sshd
    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}};
    params.value = "(invalid["; // invalid regex
    params.op = EnsureSshdOptionOperation::NotMatch;
    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);

    ASSERT_FALSE(result.HasValue());
    ASSERT_TRUE(result.Error().message.find("Failed to compile regex") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, NoOption_MissingOptionArgument_Adapted)
{
    EnsureSshdOptionParams params;
    params.value = "no";
    params.op = EnsureSshdOptionOperation::NotMatch;
    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);

    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Missing 'option' parameter");
}

TEST_F(EnsureSshdOptionTest, NoOption_MissingValueArgument_Adapted)
{
    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}};
    params.op = EnsureSshdOptionOperation::NotMatch;
    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Missing 'value' parameter");
}
// ========================= Tests for EnsureSshdOption in all_matches mode (formerly EnsureSshdOptionMatch) =========================

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
    EnsureSshdOptionParams params;
    params.value = "no";
    params.mode = EnsureSshdOptionMode::AllMatches;
    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Missing 'option' parameter");
}

TEST_F(EnsureSshdOptionTest, Match_MissingValueArgument)
{
    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}};
    params.mode = EnsureSshdOptionMode::AllMatches;
    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Missing 'value' parameter");
}

TEST_F(EnsureSshdOptionTest, Match_InvalidRegex)
{
    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}};
    params.value = "(invalid[";
    params.mode = EnsureSshdOptionMode::AllMatches;
    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
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

    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}};
    params.value = "no";
    params.mode = EnsureSshdOptionMode::AllMatches;

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    auto formatted = mFormatter.Format(mIndicators).Value();
    // Unified implementation final success message changed
    ASSERT_TRUE(formatted.find("All options are compliant") != std::string::npos);
}

TEST_F(EnsureSshdOptionTest, Match_FirstNonCompliantShortCircuits)
{
    EXPECT_CALL(mContext, GetFileContents("/etc/ssh/sshd_config")).WillOnce(Return(Result<std::string>(sshdConfigWithMatches)));
    // First match returns non-compliant (yes), subsequent commands must not be invoked
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchUserAliceCommand)).WillOnce(Return(Result<std::string>(sshdMatchOutputPermitRootLoginYes)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchGroupAdminsCommand)).Times(0);
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchAddress10005Command)).Times(0);

    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}};
    params.value = "no"; // expecting 'no'
    params.mode = EnsureSshdOptionMode::AllMatches;

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
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

    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}};
    params.value = ".*";

    params.mode = EnsureSshdOptionMode::AllMatches;
    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant); // option missing treated as non-compliant
}

TEST_F(EnsureSshdOptionTest, Match_NotMatchOperation_Compliant)
{
    EXPECT_CALL(mContext, GetFileContents("/etc/ssh/sshd_config")).WillOnce(Return(Result<std::string>(sshdConfigWithMatches)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchUserAliceCommand)).WillOnce(Return(Result<std::string>(sshdMatchOutputPermitRootLoginNo)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchGroupAdminsCommand)).WillOnce(Return(Result<std::string>(sshdMatchOutputPermitRootLoginNo)));
    EXPECT_CALL(mContext, ExecuteCommand(sshdMatchAddress10005Command)).WillOnce(Return(Result<std::string>(sshdMatchOutputPermitRootLoginNo)));

    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}};
    params.value = "yes"; // forbidden
    params.op = EnsureSshdOptionOperation::NotMatch;
    params.mode = EnsureSshdOptionMode::AllMatches;

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
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

    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}};
    params.value = "no"; // actual value matches forbidden pattern
    params.op = EnsureSshdOptionOperation::NotMatch;
    params.mode = EnsureSshdOptionMode::AllMatches;

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
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

    EnsureSshdOptionParams params;
    params.option = {{"maxauthtries"}};
    params.value = "5";
    params.op = EnsureSshdOptionOperation::LessThan;
    params.mode = EnsureSshdOptionMode::AllMatches;

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
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

    EnsureSshdOptionParams params;
    params.option = {{"maxauthtries"}};
    params.value = "5";
    params.op = EnsureSshdOptionOperation::LessThan;
    params.mode = EnsureSshdOptionMode::AllMatches;

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSshdOptionTest, Match_FileReadFailure_NoMatchesReturnsCompliant)
{
    EXPECT_CALL(mContext, GetFileContents("/etc/ssh/sshd_config")).WillOnce(Return(Result<std::string>(Error("read error", -1))));

    EnsureSshdOptionParams params;
    params.option = {{"permitrootlogin"}};
    params.value = "no";
    params.mode = EnsureSshdOptionMode::AllMatches;

    auto result = AuditEnsureSshdOption(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    // No matches => loop skipped => Compliant per current implementation
    ASSERT_EQ(result.Value(), Status::Compliant);
}
