// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <Optional.h>
#include <fstream>

using ComplianceEngine::AuditEnsureDefaultShellTimeoutIsConfigured;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Optional;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using std::map;
using std::string;

class EnsureDefaultShellTimeoutIsConfiguredTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;

    string mTempDir;

    void SetUp() override
    {
        mIndicators.Push("EnsurePasswordChangeIsInPast");
        mContext.SetSpecialFilePath("/etc/bashrc", "/tmp/somenonexistentfilename");
        mContext.SetSpecialFilePath("/etc/bash.bashrc", "/tmp/somenonexistentfilename");
        mContext.SetSpecialFilePath("/etc/profile", "/tmp/somenonexistentfilename");
        mContext.SetSpecialFilePath("/etc/profile.d/", "/tmp/somenonexistentdirectoryname");
    }

    void TearDown() override
    {
    }
};

TEST_F(EnsureDefaultShellTimeoutIsConfiguredTest, NoSpecialFiles)
{
    auto result = AuditEnsureDefaultShellTimeoutIsConfigured({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_GE(root->indicators.size(), 1);
    EXPECT_EQ(root->indicators.back().status, Status::NonCompliant);
    EXPECT_EQ(root->indicators.back().message, string("TMOUT is not set"));
}

TEST_F(EnsureDefaultShellTimeoutIsConfiguredTest, IncorrectValue)
{
    auto path = mContext.MakeTempfile("TMOUT=901"); // Acceptance threshold is 900
    mContext.SetSpecialFilePath("/etc/bashrc", path);
    auto result = AuditEnsureDefaultShellTimeoutIsConfigured({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_GE(root->indicators.size(), 1);
    EXPECT_EQ(root->indicators.back().status, Status::NonCompliant);
    EXPECT_EQ(root->indicators.back().message, string("TMOUT is set to an incorrect value in ") + path);
}

TEST_F(EnsureDefaultShellTimeoutIsConfiguredTest, NoReadonly)
{
    auto path = mContext.MakeTempfile("TMOUT=900\n");
    mContext.SetSpecialFilePath("/etc/bashrc", path);
    auto result = AuditEnsureDefaultShellTimeoutIsConfigured({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_GE(root->indicators.size(), 1);
    EXPECT_EQ(root->indicators.back().status, Status::NonCompliant);
    EXPECT_EQ(root->indicators.back().message, string("TMOUT is not readonly in ") + path);
}

TEST_F(EnsureDefaultShellTimeoutIsConfiguredTest, NoExport)
{
    auto path = mContext.MakeTempfile("TMOUT=900\nreadonly TMOUT\n");
    mContext.SetSpecialFilePath("/etc/bashrc", path);
    auto result = AuditEnsureDefaultShellTimeoutIsConfigured({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_GE(root->indicators.size(), 1);
    EXPECT_EQ(root->indicators.back().status, Status::NonCompliant);
    EXPECT_EQ(root->indicators.back().message, string("TMOUT is not exported in ") + path);
}

TEST_F(EnsureDefaultShellTimeoutIsConfiguredTest, ProperlyConfigured)
{
    auto path = mContext.MakeTempfile("TMOUT=900\nreadonly TMOUT\nexport TMOUT\n");
    mContext.SetSpecialFilePath("/etc/bashrc", path);
    auto result = AuditEnsureDefaultShellTimeoutIsConfigured({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_GE(root->indicators.size(), 1);
    EXPECT_EQ(root->indicators.back().status, Status::Compliant);
    EXPECT_EQ(root->indicators.back().message, string("TMOUT variable is properly defined"));
}

TEST_F(EnsureDefaultShellTimeoutIsConfiguredTest, MultipleEntries)
{
    auto path = mContext.MakeTempfile("TMOUT=100\nTMOUT=200\n");
    mContext.SetSpecialFilePath("/etc/bashrc", path);
    auto result = AuditEnsureDefaultShellTimeoutIsConfigured({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_GE(root->indicators.size(), 1);
    EXPECT_EQ(root->indicators.back().status, Status::NonCompliant);
    EXPECT_EQ(root->indicators.back().message, string("TMOUT is set multiple times in ") + path);
}

TEST_F(EnsureDefaultShellTimeoutIsConfiguredTest, MultipleEntriesInDifferentFiles)
{
    auto path1 = mContext.MakeTempfile("TMOUT=900\nreadonly TMOUT\nexport TMOUT\n");
    mContext.SetSpecialFilePath("/etc/bashrc", path1);
    auto path2 = mContext.MakeTempfile("TMOUT=900\nreadonly TMOUT\nexport TMOUT\n");
    mContext.SetSpecialFilePath("/etc/profile", path2);
    auto result = AuditEnsureDefaultShellTimeoutIsConfigured({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_GE(root->indicators.size(), 1);
    EXPECT_EQ(root->indicators.back().status, Status::NonCompliant);
    EXPECT_EQ(root->indicators.back().message, string("TMOUT is set in multiple locations"));
}
