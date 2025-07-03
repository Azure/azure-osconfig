// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <dirent.h>
#include <fstream>
#include <gtest/gtest.h>
#include <linux/limits.h>
#include <string>
#include <unistd.h>

using ComplianceEngine::AuditEnsureXdmcp;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Result;
using ComplianceEngine::Status;

class EnsureXdmcp : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;
    std::string failureXdmcpEnabled = "[xdmcp]\nEnable = true\n";
    std::string successXdmcpSectionNoEnable = "[xdmcp]\nFobable = true\n";
    std::string successXdmcpSectionEmpty = "[xdmcp]\n[OtherSection]xdmcp\nEnable = true\n";

    void SetUp() override
    {
        mIndicators.Push("EnsureXdmcp");
    }

    void TearDown() override
    {
    }
};

TEST_F(EnsureXdmcp, AuditSuccess)
{
    std::map<std::string, std::string> args;

    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    auto result = AuditEnsureXdmcp(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureXdmcp, AuditFailureGdm3Custom)
{
    std::map<std::string, std::string> args;

    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>(failureXdmcpEnabled)));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    auto result = AuditEnsureXdmcp(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureXdmcp, AuditFailureGdm3Daemon)
{
    std::map<std::string, std::string> args;

    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>(failureXdmcpEnabled)));

    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    auto result = AuditEnsureXdmcp(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureXdmcp, AuditFailureGdmCustom)
{
    std::map<std::string, std::string> args;

    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>(failureXdmcpEnabled)));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    auto result = AuditEnsureXdmcp(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureXdmcp, AuditFailureGdmDaemon)
{
    std::map<std::string, std::string> args;

    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>(failureXdmcpEnabled)));
    auto result = AuditEnsureXdmcp(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureXdmcp, AuditSuccessNoSection)
{
    std::map<std::string, std::string> args;

    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>(successXdmcpSectionNoEnable)));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    auto result = AuditEnsureXdmcp(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureXdmcp, AuditFailureOneSection)
{
    std::map<std::string, std::string> args;

    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>(successXdmcpSectionNoEnable)));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>(failureXdmcpEnabled)));
    auto result = AuditEnsureXdmcp(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureXdmcp, AuditSuccessEmptySection)
{
    std::map<std::string, std::string> args;

    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>(successXdmcpSectionNoEnable)));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>(successXdmcpSectionEmpty)));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    auto result = AuditEnsureXdmcp(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
