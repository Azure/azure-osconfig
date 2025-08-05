// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <Optional.h>
#include <fstream>

using ComplianceEngine::AuditEnsureSystemAccountsDoNotHaveValidShell;
using ComplianceEngine::CompactListFormatter;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Optional;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using std::map;
using std::string;

class EnsureSystemAccountsDoNotHaveValidShellTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;
    CompactListFormatter mFormatter;
    string mTempDir;

    void SetUp() override
    {
        mIndicators.Push("EnsureAccountsWithoutShellAreLocked");
        auto filename = mContext.MakeTempfile("/bin/bash\n/bin/nologin");
        mContext.SetSpecialFilePath("/etc/shells", filename);
        filename = mContext.MakeTempfile("UID_MIN 100");
        mContext.SetSpecialFilePath("/etc/login.defs", filename);
        filename = CreateTestPasswdFile(101, "/bin/bash");
        mContext.SetSpecialFilePath("/etc/passwd", filename);
    }

    void TearDown() override
    {
    }

    string CreateTestPasswdFile(uid_t uid, string shell, std::string username = "testuser")
    {
        auto content = username + ":x";
        content += ":" + std::to_string(uid);
        content += ":" + std::to_string(uid);
        content += ":::";
        content += shell;
        return mContext.MakeTempfile(std::move(content));
    }
};

TEST_F(EnsureSystemAccountsDoNotHaveValidShellTest, NoEtcPasswdFile)
{
    mContext.SetSpecialFilePath("/etc/passwd", "/tmp/somenonexistentfilename");
    auto result = AuditEnsureSystemAccountsDoNotHaveValidShell({}, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
}

TEST_F(EnsureSystemAccountsDoNotHaveValidShellTest, NoLoginDefsFile_1)
{
    mContext.SetSpecialFilePath("/etc/login.defs", "/tmp/somenonexistentfilename");
    mContext.SetSpecialFilePath("/etc/passwd", mContext.MakeTempfile(""));
    auto result = AuditEnsureSystemAccountsDoNotHaveValidShell({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    // No system accounts found
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSystemAccountsDoNotHaveValidShellTest, NoLoginDefsFile_2)
{
    mContext.SetSpecialFilePath("/etc/login.defs", "/tmp/somenonexistentfilename");
    auto filename = CreateTestPasswdFile(1001, "/bin/bash");
    mContext.SetSpecialFilePath("/etc/passwd", filename);
    auto result = AuditEnsureSystemAccountsDoNotHaveValidShell({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    // UID_MIN defaults to 1000 in case there's no /etc/login.defs file
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSystemAccountsDoNotHaveValidShellTest, NoLoginDefsFile_3)
{
    mContext.SetSpecialFilePath("/etc/login.defs", "/tmp/somenonexistentfilename");
    auto result = AuditEnsureSystemAccountsDoNotHaveValidShell({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    // Min UID is 1000 as there's no /etc/login.defs file
    // We currently have one user: 101 with /bin/bash
    EXPECT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSystemAccountsDoNotHaveValidShellTest, LoginDefs_1)
{
    auto filename = mContext.MakeTempfile("UID_MIN -1");
    mContext.SetSpecialFilePath("/etc/login.defs", filename);
    auto result = AuditEnsureSystemAccountsDoNotHaveValidShell({}, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
}

TEST_F(EnsureSystemAccountsDoNotHaveValidShellTest, LoginDefs_2)
{
    auto filename = mContext.MakeTempfile("#UID_MIN -1");
    mContext.SetSpecialFilePath("/etc/login.defs", filename);
    auto result = AuditEnsureSystemAccountsDoNotHaveValidShell({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
}

TEST_F(EnsureSystemAccountsDoNotHaveValidShellTest, LoginDefs_3)
{
    auto filename = mContext.MakeTempfile("UID_MIN  foo bar");
    mContext.SetSpecialFilePath("/etc/login.defs", filename);
    auto result = AuditEnsureSystemAccountsDoNotHaveValidShell({}, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
}

TEST_F(EnsureSystemAccountsDoNotHaveValidShellTest, LoginDefs_4)
{
    auto filename = mContext.MakeTempfile("UID_MIN\t0 foo");
    mContext.SetSpecialFilePath("/etc/login.defs", filename);
    auto result = AuditEnsureSystemAccountsDoNotHaveValidShell({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
}

TEST_F(EnsureSystemAccountsDoNotHaveValidShellTest, WhitelistedAccount_1)
{
    auto filename = CreateTestPasswdFile(0, "/bin/bash", "root");
    mContext.SetSpecialFilePath("/etc/passwd", filename);
    auto result = AuditEnsureSystemAccountsDoNotHaveValidShell({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    // 'root' is whitelisted
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSystemAccountsDoNotHaveValidShellTest, WhitelistedAccount_2)
{
    auto filename = CreateTestPasswdFile(0, "/bin/bash", "halt");
    mContext.SetSpecialFilePath("/etc/passwd", filename);
    auto result = AuditEnsureSystemAccountsDoNotHaveValidShell({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    // 'halt' is whitelisted
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSystemAccountsDoNotHaveValidShellTest, WhitelistedAccount_3)
{
    auto filename = CreateTestPasswdFile(0, "/bin/bash", "shutdown");
    mContext.SetSpecialFilePath("/etc/passwd", filename);
    auto result = AuditEnsureSystemAccountsDoNotHaveValidShell({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    // 'shutdown' is whitelisted
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSystemAccountsDoNotHaveValidShellTest, WhitelistedAccount_4)
{
    auto filename = CreateTestPasswdFile(0, "/bin/bash", "nfsnobody");
    mContext.SetSpecialFilePath("/etc/passwd", filename);
    auto result = AuditEnsureSystemAccountsDoNotHaveValidShell({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    // 'nfsnobody' is whitelisted
    EXPECT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSystemAccountsDoNotHaveValidShellTest, SystemUser_1)
{
    auto filename = CreateTestPasswdFile(99, "/bin/bash");
    mContext.SetSpecialFilePath("/etc/passwd", filename);
    auto result = AuditEnsureSystemAccountsDoNotHaveValidShell({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);
    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_FALSE(root->indicators.empty());
    EXPECT_EQ(root->indicators.back().message, "System user 99 has a valid login shell");
    EXPECT_EQ(root->indicators.back().status, Status::NonCompliant);
}

TEST_F(EnsureSystemAccountsDoNotHaveValidShellTest, SystemUser_2)
{
    auto filename = CreateTestPasswdFile(99, "/bin/nologin");
    mContext.SetSpecialFilePath("/etc/passwd", filename);
    auto result = AuditEnsureSystemAccountsDoNotHaveValidShell({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_FALSE(root->indicators.empty());
    EXPECT_EQ(root->indicators.back().message, "System user 99 does not have a valid login shell");
    EXPECT_EQ(root->indicators.back().status, Status::Compliant);
}

TEST_F(EnsureSystemAccountsDoNotHaveValidShellTest, RegularUser_1)
{
    auto filename = CreateTestPasswdFile(100, "/bin/bash");
    mContext.SetSpecialFilePath("/etc/passwd", filename);
    auto result = AuditEnsureSystemAccountsDoNotHaveValidShell({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    EXPECT_TRUE(root->indicators.empty());
}

TEST_F(EnsureSystemAccountsDoNotHaveValidShellTest, RegularUser_2)
{
    auto filename = CreateTestPasswdFile(100, "/bin/nologin");
    mContext.SetSpecialFilePath("/etc/passwd", filename);
    auto result = AuditEnsureSystemAccountsDoNotHaveValidShell({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);
    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    EXPECT_TRUE(root->indicators.empty());
}
