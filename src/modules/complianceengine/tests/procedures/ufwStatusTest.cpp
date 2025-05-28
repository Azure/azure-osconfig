// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <dirent.h>
#include <fstream>
#include <gtest/gtest.h>
#include <linux/limits.h>
#include <regex>
#include <string>
#include <unistd.h>

using ComplianceEngine::AuditUfwStatus;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::NestedListFormatter;
using ComplianceEngine::Result;
using ComplianceEngine::Status;

static const std::string ufwCommand = "ufw status verbose";
static const std::string ufwActiveOutput =
    "Status: active\n"
    "Logging: on (low)\n"
    "Default: deny (incoming), allow (outgoing), disabled (routed)\n"
    "New profiles: skip\n\n"
    "To                         Action      From\n"
    "--                         ------      ----\n"
    "22/tcp                     ALLOW IN    Anywhere\n"
    "80/tcp                     ALLOW IN    Anywhere\n"
    "443/tcp                    ALLOW IN    Anywhere\n"
    "22/tcp (v6)                ALLOW IN    Anywhere (v6)\n"
    "80/tcp (v6)                ALLOW IN    Anywhere (v6)\n"
    "443/tcp (v6)               ALLOW IN    Anywhere (v6)\n";

static const std::string ufwInactiveOutput = "Status: inactive\n";

class UfwStatusTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;
    NestedListFormatter mFormatter;

    void SetUp() override
    {
        mIndicators.Push("UfwStatus");
    }

    void TearDown() override
    {
    }
};

TEST_F(UfwStatusTest, MissingRegexParameter)
{
    std::map<std::string, std::string> args;
    // No statusRegex parameter provided

    auto result = AuditUfwStatus(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Missing 'statusRegex' parameter");
    ASSERT_EQ(result.Error().code, EINVAL);
}

TEST_F(UfwStatusTest, UfwActiveStatusMatches)
{
    // Setup the expectation for the ufw status command to return active status
    EXPECT_CALL(mContext, ExecuteCommand(ufwCommand)).WillOnce(::testing::Return(Result<std::string>(ufwActiveOutput)));

    std::map<std::string, std::string> args;
    args["statusRegex"] = "Status:\\s*active";

    auto result = AuditUfwStatus(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);

    auto formattedResult = mFormatter.Format(mIndicators);
    ASSERT_TRUE(formattedResult.HasValue());
    ASSERT_TRUE(formattedResult.Value().find("found") != std::string::npos);
}

TEST_F(UfwStatusTest, UfwNotActiveStatusMismatch)
{
    // Setup the expectation for the ufw status command to return inactive status
    EXPECT_CALL(mContext, ExecuteCommand(ufwCommand)).WillOnce(::testing::Return(Result<std::string>(ufwInactiveOutput)));

    std::map<std::string, std::string> args;
    args["statusRegex"] = "Status:\\s*active";

    auto result = AuditUfwStatus(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);

    auto formattedResult = mFormatter.Format(mIndicators);
    ASSERT_TRUE(formattedResult.HasValue());
    ASSERT_TRUE(formattedResult.Value().find("not found") != std::string::npos);
}

TEST_F(UfwStatusTest, UfwFirewallRuleMatches)
{
    // Setup the expectation for the ufw status command to return output with firewall rules
    EXPECT_CALL(mContext, ExecuteCommand((ufwCommand))).WillOnce(::testing::Return(Result<std::string>(ufwActiveOutput)));

    std::map<std::string, std::string> args;
    args["statusRegex"] = "22/tcp\\s+ALLOW IN\\s+Anywhere";

    auto result = AuditUfwStatus(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);

    auto formattedResult = mFormatter.Format(mIndicators);
    ASSERT_TRUE(formattedResult.HasValue());
    ASSERT_TRUE(formattedResult.Value().find("found") != std::string::npos);
}

TEST_F(UfwStatusTest, UfwFirewallRuleMissing)
{
    // Setup the expectation for the ufw status command to return output with firewall rules
    EXPECT_CALL(mContext, ExecuteCommand(ufwCommand)).WillOnce(::testing::Return(Result<std::string>(ufwActiveOutput)));

    std::map<std::string, std::string> args;
    args["statusRegex"] = "8080/tcp\\s+ALLOW IN\\s+Anywhere"; // Rule not present in output

    auto result = AuditUfwStatus(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);

    auto formattedResult = mFormatter.Format(mIndicators);
    ASSERT_TRUE(formattedResult.HasValue());
    ASSERT_TRUE(formattedResult.Value().find("not found") != std::string::npos);
}

TEST_F(UfwStatusTest, UfwNotFound)
{
    // Setup the expectation for the ufw status command to fail
    EXPECT_CALL(mContext, ExecuteCommand(::testing::StrEq(ufwCommand))).WillOnce(::testing::Return(Result<std::string>(Error("Command not found", 127))));

    std::map<std::string, std::string> args;
    args["statusRegex"] = "Status:\\s*active";

    auto result = AuditUfwStatus(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);

    // Verify the error message is propagated in the indicators
    auto formattedResult = mFormatter.Format(mIndicators);
    ASSERT_TRUE(formattedResult.HasValue());
    ASSERT_TRUE(formattedResult.Value().find("ufw not found") != std::string::npos);
}

TEST_F(UfwStatusTest, InvalidRegex)
{
    std::map<std::string, std::string> args;
    args["statusRegex"] = "Status:*["; // Invalid regex

    auto result = AuditUfwStatus(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_TRUE(result.Error().message.find("Failed to compile regex") != std::string::npos);
}
