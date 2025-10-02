// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <Optional.h>
#include <fstream>

using ComplianceEngine::AuditEnsureAccountsWithoutShellAreLocked;
using ComplianceEngine::CompactListFormatter;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Optional;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using std::map;
using std::string;

class EnsureAccountsWithoutShellAreLockedTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;
    CompactListFormatter mFormatter;
    string mTempDir;

    void SetUp() override
    {
        mIndicators.Push("EnsureAccountsWithoutShellAreLocked");
        auto filename = CreateTestPasswdFile("testuser", "x", "/bin/bash");
        mContext.SetSpecialFilePath("/etc/passwd", filename);
        filename = mContext.MakeTempfile("# comment\n/bin/bash\n/bin/nologin");
        mContext.SetSpecialFilePath("/etc/shells", filename);
    }

    void TearDown() override
    {
    }

    string CreateTestShadowFile(string username, string password)
    {
        auto content = std::move(username);
        content += ":" + password;
        content += ":::::::";
        return mContext.MakeTempfile(std::move(content));
    }

    string CreateTestPasswdFile(string username, string password, string shell)
    {
        auto content = std::move(username);
        content += ":" + password;
        content += ":" + std::to_string(9999);
        content += ":" + std::to_string(9999);
        content += ":::";
        content += shell;
        return mContext.MakeTempfile(std::move(content));
    }
};

TEST_F(EnsureAccountsWithoutShellAreLockedTest, NoEtcShadowFile)
{
    mContext.SetSpecialFilePath("/etc/shadow", "/tmp/somenonexistentfilename");
    auto result = AuditEnsureAccountsWithoutShellAreLocked({}, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
}

TEST_F(EnsureAccountsWithoutShellAreLockedTest, NoEtcPasswdFile)
{
    mContext.SetSpecialFilePath("/etc/shadow", "/tmp/somenonexistentfilename");
    mContext.SetSpecialFilePath("/etc/passwd", "/tmp/somenonexistentfilename");
    auto result = AuditEnsureAccountsWithoutShellAreLocked({}, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
}

TEST_F(EnsureAccountsWithoutShellAreLockedTest, ValidShell_RegularPassword)
{
    auto filename = CreateTestShadowFile("testuser", "$y$");
    mContext.SetSpecialFilePath("/etc/shadow", filename);
    auto result = AuditEnsureAccountsWithoutShellAreLocked({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureAccountsWithoutShellAreLockedTest, ValidShell_NoPassword)
{
    auto filename = CreateTestPasswdFile("testuser", "x", "/bin/bash");
    mContext.SetSpecialFilePath("/etc/passwd", filename);
    filename = CreateTestShadowFile("testuser", "");
    mContext.SetSpecialFilePath("/etc/shadow", filename);
    auto result = AuditEnsureAccountsWithoutShellAreLocked({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureAccountsWithoutShellAreLockedTest, ValidShell_LockedUser_1)
{
    auto filename = CreateTestShadowFile("testuser", "!");
    mContext.SetSpecialFilePath("/etc/shadow", filename);
    auto result = AuditEnsureAccountsWithoutShellAreLocked({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureAccountsWithoutShellAreLockedTest, ValidShell_LockedUser_2)
{
    auto filename = CreateTestShadowFile("testuser", "*");
    mContext.SetSpecialFilePath("/etc/shadow", filename);
    auto result = AuditEnsureAccountsWithoutShellAreLocked({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureAccountsWithoutShellAreLockedTest, InvalidShell_RegularPassword)
{
    auto filename = CreateTestShadowFile("testuser", "$y$");
    mContext.SetSpecialFilePath("/etc/shadow", filename);
    filename = CreateTestPasswdFile("testuser", "$y$", "/bin/x");
    mContext.SetSpecialFilePath("/etc/passwd", filename);
    auto result = AuditEnsureAccountsWithoutShellAreLocked({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_TRUE(root->indicators.size() > 0);
    EXPECT_EQ(root->indicators.back().message, string("User 9999 does not have a valid shell, but the account is not locked"));
}

TEST_F(EnsureAccountsWithoutShellAreLockedTest, InvalidShell_NoPassword)
{
    auto filename = CreateTestShadowFile("testuser", "");
    mContext.SetSpecialFilePath("/etc/shadow", filename);
    filename = CreateTestPasswdFile("testuser", "$y$", "/bin/x");
    mContext.SetSpecialFilePath("/etc/passwd", filename);
    auto result = AuditEnsureAccountsWithoutShellAreLocked({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_TRUE(root->indicators.size() > 0);
    EXPECT_EQ(root->indicators.back().message, string("User 9999 does not have a valid shell, but the account is not locked"));
}

TEST_F(EnsureAccountsWithoutShellAreLockedTest, InvalidShell_LockedUser_1)
{
    auto filename = CreateTestShadowFile("testuser", "!");
    mContext.SetSpecialFilePath("/etc/shadow", filename);
    filename = CreateTestPasswdFile("testuser", "$y$", "/bin/x");
    mContext.SetSpecialFilePath("/etc/passwd", filename);
    auto result = AuditEnsureAccountsWithoutShellAreLocked({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_EQ(root->indicators.size(), 2);
    EXPECT_EQ(root->indicators[0].message, string("User 9999 does not have a valid shell, but the account is locked"));
    EXPECT_EQ(root->indicators[1].message, string("All non-root users without a login shell are locked"));
}

TEST_F(EnsureAccountsWithoutShellAreLockedTest, InvalidShell_LockedUser_2)
{
    auto filename = CreateTestShadowFile("testuser", "*");
    mContext.SetSpecialFilePath("/etc/shadow", filename);
    filename = CreateTestPasswdFile("testuser", "$y$", "/bin/x");
    mContext.SetSpecialFilePath("/etc/passwd", filename);
    auto result = AuditEnsureAccountsWithoutShellAreLocked({}, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_EQ(root->indicators.size(), 2);
    EXPECT_EQ(root->indicators[0].message, string("User 9999 does not have a valid shell, but the account is locked"));
    EXPECT_EQ(root->indicators[1].message, string("All non-root users without a login shell are locked"));
}
