// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "MockContext.h"

#include <EnsureXdmcp.h>
#include <dirent.h>
#include <fstream>
#include <gtest/gtest.h>
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
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    auto result = AuditEnsureXdmcp(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureXdmcp, AuditFailureGdm3Custom)
{
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>(failureXdmcpEnabled)));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    auto result = AuditEnsureXdmcp(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureXdmcp, AuditFailureGdm3Daemon)
{
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>(failureXdmcpEnabled)));

    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    auto result = AuditEnsureXdmcp(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureXdmcp, AuditFailureGdmCustom)
{
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>(failureXdmcpEnabled)));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    auto result = AuditEnsureXdmcp(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureXdmcp, AuditFailureGdmDaemon)
{
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>(failureXdmcpEnabled)));
    auto result = AuditEnsureXdmcp(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureXdmcp, AuditSuccessNoSection)
{
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>(successXdmcpSectionNoEnable)));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    auto result = AuditEnsureXdmcp(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureXdmcp, AuditFailureOneSection)
{
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>(successXdmcpSectionNoEnable)));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>(failureXdmcpEnabled)));
    auto result = AuditEnsureXdmcp(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureXdmcp, AuditSuccessEmptySection)
{
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>(successXdmcpSectionNoEnable)));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm3/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>(successXdmcpSectionEmpty)));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/custom.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    EXPECT_CALL(mContext, GetFileContents("/etc/gdm/daemon.conf")).WillRepeatedly(::testing::Return(Result<std::string>("\n")));
    auto result = AuditEnsureXdmcp(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
