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
#include <string>
#include <unistd.h>

using compliance::AuditUfwStatus;
using compliance::Error;
using compliance::IndicatorsTree;
using compliance::NestedListFormatter;
using compliance::Result;
using compliance::Status;

static const std::string ufwCommand = "ufw status";
static const std::string ufwActiveOutput =
    "Status: active\n\n"
    "To                         Action      From\n"
    "--                         ------      ----\n"
    "22/tcp                     ALLOW       Anywhere\n"
    "80/tcp                     ALLOW       Anywhere\n"
    "443/tcp                    ALLOW       Anywhere\n"
    "22/tcp (v6)                ALLOW       Anywhere (v6)\n"
    "80/tcp (v6)                ALLOW       Anywhere (v6)\n"
    "443/tcp (v6)               ALLOW       Anywhere (v6)\n";

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

TEST_F(UfwStatusTest, UfwActive)
{
    // Setup the expectation for the ufw status command to return active status
    EXPECT_CALL(mContext, ExecuteCommand(::testing::StrEq(ufwCommand))).WillOnce(::testing::Return(Result<std::string>(ufwActiveOutput)));

    std::map<std::string, std::string> args;

    auto result = AuditUfwStatus(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(UfwStatusTest, UfwNotActive)
{
    // Setup the expectation for the ufw status command to return inactive status
    EXPECT_CALL(mContext, ExecuteCommand(::testing::StrEq(ufwCommand))).WillOnce(::testing::Return(Result<std::string>(ufwInactiveOutput)));

    std::map<std::string, std::string> args;

    auto result = AuditUfwStatus(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(UfwStatusTest, UfwNotFound)
{
    // Setup the expectation for the ufw status command to fail
    EXPECT_CALL(mContext, ExecuteCommand(::testing::StrEq(ufwCommand))).WillOnce(::testing::Return(Result<std::string>(Error("Command not found", 127))));

    std::map<std::string, std::string> args;

    auto result = AuditUfwStatus(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);

    // Verify the error message is propagated in the indicators
    auto formattedResult = mFormatter.Format(mIndicators);
    ASSERT_TRUE(formattedResult.HasValue());
    ASSERT_TRUE(formattedResult.Value().find("ufw not found") != std::string::npos);
}
