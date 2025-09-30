// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "MockContext.h"

#include <EnsureDconf.h>
#include <gtest/gtest.h>

using ComplianceEngine::AuditEnsureDconf;
using ComplianceEngine::AuditEnsureDconfParams;
using ComplianceEngine::DConfOperation;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Result;
using ComplianceEngine::Status;

class EnsureDconf : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;

    AuditEnsureDconfParams mParams;

    std::string DconfRead() const
    {
        return "dconf read \"" + mParams.key + "\"";
    }

    void SetUp() override
    {
        mIndicators.Push("EnsureGsettings");
    }
};

TEST_F(EnsureDconf, AuditNonCompliantValueNotEqual)
{
    mParams.key = "/org/gnome/login-screen/banner-message-text";
    mParams.operation = DConfOperation::Eq;
    mParams.value = "You *SHALL NOT PASS* (this login screen)";
    EXPECT_CALL(mContext, ExecuteCommand(DconfRead())).WillOnce(::testing::Return(Result<std::string>("You *SHALL* PASSS")));
    auto result = AuditEnsureDconf(mParams, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureDconf, AuditCompliantValueNotEqual)
{
    mParams.key = "/org/gnome/login-screen/banner-message-text";
    mParams.operation = DConfOperation::Ne;
    mParams.value = "You *SHALL NOT PASS* (this login screen)";
    EXPECT_CALL(mContext, ExecuteCommand(DconfRead())).WillOnce(::testing::Return(Result<std::string>("You *SHALL* PASSS")));
    auto result = AuditEnsureDconf(mParams, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureDconf, AuditCompliantValueEqual)
{
    mParams.key = "/org/gnome/login-screen/banner-message-text";
    mParams.operation = DConfOperation::Eq;
    mParams.value = "You *SHALL NOT PASS* (this login screen)";
    EXPECT_CALL(mContext, ExecuteCommand(DconfRead())).WillOnce(::testing::Return(Result<std::string>(mParams.value)));
    auto result = AuditEnsureDconf(mParams, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
