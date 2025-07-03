// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <gtest/gtest.h>

using ComplianceEngine::AuditEnsureDconf;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Result;
using ComplianceEngine::Status;

class EnsureDconf : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;

    std::map<std::string, std::string> args;

    std::string DconfRead()
    {
        return "dconf read " + args["key"];
    }

    void SetUp() override
    {
        mIndicators.Push("EnsureGsettings");
    }

    void TearDown() override
    {
        args.clear();
    }
};

TEST_F(EnsureDconf, AuditFailNoArgs)
{

    auto result = AuditEnsureDconf(args, mIndicators, mContext);
    ASSERT_TRUE(!result.HasValue());
    ASSERT_EQ(result.Error().message, "No key arg provided");
}

TEST_F(EnsureDconf, AuditFailNoArgsOperation)
{
    args["key"] = "/org/gnome/login-screen/banner-message-text";
    auto result = AuditEnsureDconf(args, mIndicators, mContext);
    ASSERT_TRUE(!result.HasValue());
    ASSERT_EQ(result.Error().message, "No operation arg provided");
}

TEST_F(EnsureDconf, AuditFailInvalidOperation)
{
    args["key"] = "/org/gnome/login-screen/banner-message-text";
    args["operation"] = "spaceship equal";
    auto result = AuditEnsureDconf(args, mIndicators, mContext);
    ASSERT_TRUE(!result.HasValue());
    ASSERT_EQ(result.Error().message, "Not supported operation '" + args["operation"] + "'");
}

TEST_F(EnsureDconf, AuditFailNoValueArg)
{
    args["key"] = "/org/gnome/login-screen/banner-message-text";
    args["operation"] = "eq";
    auto result = AuditEnsureDconf(args, mIndicators, mContext);
    ASSERT_TRUE(!result.HasValue());
    ASSERT_EQ(result.Error().message, "No value arg provided");
}

TEST_F(EnsureDconf, AuditNoCompliantValueNotEqual)
{
    args["key"] = "/org/gnome/login-screen/banner-message-text";
    args["operation"] = "eq";
    args["value"] = "You *SHALL NOT PASS* (this login creen)";
    EXPECT_CALL(mContext, ExecuteCommand(DconfRead())).WillOnce(::testing::Return(Result<std::string>("You *SHALL* PASSS")));
    auto result = AuditEnsureDconf(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureDconf, AuditCompliantValueNotEqual)
{
    args["key"] = "/org/gnome/login-screen/banner-message-text";
    args["operation"] = "ne";
    args["value"] = "You *SHALL NOT PASS* (this login creen)";
    EXPECT_CALL(mContext, ExecuteCommand(DconfRead())).WillOnce(::testing::Return(Result<std::string>("You *SHALL* PASSS")));
    auto result = AuditEnsureDconf(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureDconf, AuditCompliantValueEqual)
{
    args["key"] = "/org/gnome/login-screen/banner-message-text";
    args["operation"] = "eq";
    args["value"] = "You *SHALL NOT PASS* (this login creen)";
    EXPECT_CALL(mContext, ExecuteCommand(DconfRead())).WillOnce(::testing::Return(Result<std::string>(args["value"])));
    auto result = AuditEnsureDconf(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
