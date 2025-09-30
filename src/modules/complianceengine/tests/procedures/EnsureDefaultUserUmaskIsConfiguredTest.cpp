// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "Evaluator.h"
#include "MockContext.h"

#include <EnsureDefaultUserUmaskIsConfigured.h>
#include <Optional.h>
#include <fstream>

using ComplianceEngine::AuditEnsureDefaultUserUmaskIsConfigured;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Optional;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using std::map;
using std::string;

class EnsureDefaultUserUmaskIsConfiguredTest : public ::testing::Test
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
        mContext.SetSpecialFilePath("/etc/profile.d/", mContext.GetTempdirPath());
        mContext.SetSpecialFilePath("/etc/pam.d/postlogin", "/tmp/somenonexistentfilename");
        mContext.SetSpecialFilePath("/etc/login.defs", "/tmp/somenonexistentfilename");
        mContext.SetSpecialFilePath("/etc/default/login", "/tmp/somenonexistentfilename");
    }

    void TearDown() override
    {
    }
};

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, CorrectValue_1)
{
    auto filename = mContext.MakeTempfile("umask 027");
    mContext.SetSpecialFilePath("/etc/bashrc", filename);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_FALSE(root->indicators.empty());
    EXPECT_EQ(root->indicators.back().message, string("umask is correctly set in ") + filename);
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, CorrectValue_2)
{
    auto filename = mContext.MakeTempfile("umask u=rwx,g=rx,o=");
    mContext.SetSpecialFilePath("/etc/bashrc", filename);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_FALSE(root->indicators.empty());
    EXPECT_EQ(root->indicators.back().message, string("umask is correctly set in ") + filename);
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, CorrectValue_3)
{
    auto filename = mContext.MakeTempfile("   umask\t\tu=rwx,g=rx,o=");
    mContext.SetSpecialFilePath("/etc/bashrc", filename);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_FALSE(root->indicators.empty());
    EXPECT_EQ(root->indicators.back().message, string("umask is correctly set in ") + filename);
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, CorrectValue_4)
{
    auto filename = mContext.MakeTempfile("UMASK 027");
    mContext.SetSpecialFilePath("/etc/login.defs", filename);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_FALSE(root->indicators.empty());
    EXPECT_EQ(root->indicators.back().message, string("umask is correctly set in ") + filename);
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, CorrectValue_MoreRestrictive_1)
{
    auto filename = mContext.MakeTempfile("umask u=rwx,g=r,o=");
    mContext.SetSpecialFilePath("/etc/bashrc", filename);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_FALSE(root->indicators.empty());
    EXPECT_EQ(root->indicators.back().message, string("umask is correctly set in ") + filename);
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, CorrectValue_MoreRestrictive_2)
{
    auto filename = mContext.MakeTempfile("umask u=rwx,g=x,o=");
    mContext.SetSpecialFilePath("/etc/bashrc", filename);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_FALSE(root->indicators.empty());
    EXPECT_EQ(root->indicators.back().message, string("umask is correctly set in ") + filename);
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, CorrectValue_MoreRestrictive_3)
{
    auto filename = mContext.MakeTempfile("umask u=rx,g=rx,o=");
    mContext.SetSpecialFilePath("/etc/bashrc", filename);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_FALSE(root->indicators.empty());
    EXPECT_EQ(root->indicators.back().message, string("umask is correctly set in ") + filename);
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, CorrectValue_MoreRestrictive_4)
{
    auto filename = mContext.MakeTempfile("umask 037");
    mContext.SetSpecialFilePath("/etc/bashrc", filename);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_FALSE(root->indicators.empty());
    EXPECT_EQ(root->indicators.back().message, string("umask is correctly set in ") + filename);
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, CorrectValue_MoreRestrictive_5)
{
    auto filename = mContext.MakeTempfile("umask 127");
    mContext.SetSpecialFilePath("/etc/bashrc", filename);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_FALSE(root->indicators.empty());
    EXPECT_EQ(root->indicators.back().message, string("umask is correctly set in ") + filename);
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, IncorrectValue_1)
{
    auto filename = mContext.MakeTempfile("umask 026");
    mContext.SetSpecialFilePath("/etc/bashrc", filename);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_FALSE(root->indicators.empty());
    EXPECT_EQ(root->indicators.back().message, string("umask is incorrectly set in ") + filename);
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, IncorrectValue_2)
{
    auto filename = mContext.MakeTempfile("umask 017");
    mContext.SetSpecialFilePath("/etc/bashrc", filename);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_FALSE(root->indicators.empty());
    EXPECT_EQ(root->indicators.back().message, string("umask is incorrectly set in ") + filename);
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, IncorrectValue_3)
{
    auto filename = mContext.MakeTempfile("umask u=rwx,g=rwx,o=");
    mContext.SetSpecialFilePath("/etc/bashrc", filename);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_FALSE(root->indicators.empty());
    EXPECT_EQ(root->indicators.back().message, string("umask is incorrectly set in ") + filename);
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, IncorrectValue_4)
{
    auto filename = mContext.MakeTempfile("umask u=rwx,g=rx,o=r");
    mContext.SetSpecialFilePath("/etc/bashrc", filename);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_FALSE(root->indicators.empty());
    EXPECT_EQ(root->indicators.back().message, string("umask is incorrectly set in ") + filename);
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, IncorrectValue_5)
{
    auto filename = mContext.MakeTempfile("umask u=rwx,g=rx,o=w");
    mContext.SetSpecialFilePath("/etc/bashrc", filename);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_FALSE(root->indicators.empty());
    EXPECT_EQ(root->indicators.back().message, string("umask is incorrectly set in ") + filename);
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, IncorrectValue_6)
{
    auto filename = mContext.MakeTempfile("umask u=rwx,g=rx,o=x");
    mContext.SetSpecialFilePath("/etc/bashrc", filename);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_FALSE(root->indicators.empty());
    EXPECT_EQ(root->indicators.back().message, string("umask is incorrectly set in ") + filename);
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, IncorrectValue_7)
{
    auto filename = mContext.MakeTempfile("umask 028");
    mContext.SetSpecialFilePath("/etc/bashrc", filename);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_FALSE(root->indicators.empty());
    EXPECT_EQ(root->indicators.back().message, string("umask is not set"));
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, NoUmask)
{
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_FALSE(root->indicators.empty());
    EXPECT_EQ(root->indicators.back().message, string("umask is not set"));
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, Precedence_1)
{
    auto filename1 = mContext.MakeTempfile("umask u=rwx,g=rx,o=x", ".sh");
    auto filename2 = mContext.MakeTempfile("umask u=rwx,g=rx,o=");
    mContext.SetSpecialFilePath("/etc/bashrc", filename2);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_EQ(root->indicators.size(), 2);
    EXPECT_EQ(root->indicators[0].message, string("umask is incorrectly set in ") + filename1);
    EXPECT_EQ(root->indicators[1].message, string("umask is correctly set in ") + filename2);
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, Precedence_2)
{
    auto filename1 = mContext.MakeTempfile("umask u=rwx,g=rx,o=", ".sh");
    auto filename2 = mContext.MakeTempfile("umask u=rwx,g=rx,o=x");
    mContext.SetSpecialFilePath("/etc/bashrc", filename2);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_EQ(root->indicators.size(), 1);
    EXPECT_EQ(root->indicators[0].message, string("umask is correctly set in ") + filename1);
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, CorrectValue_PAM_1)
{
    auto filename = mContext.MakeTempfile("session pam_umask.so umask=027");
    mContext.SetSpecialFilePath("/etc/pam.d/postlogin", filename);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_EQ(root->indicators.size(), 1);
    EXPECT_EQ(root->indicators[0].message, string("umask is correctly set in ") + filename);
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, CorrectValue_PAM_2)
{
    auto filename = mContext.MakeTempfile("session\tpam_umask.so\t \tumask=027");
    mContext.SetSpecialFilePath("/etc/pam.d/postlogin", filename);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::Compliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_EQ(root->indicators.size(), 1);
    EXPECT_EQ(root->indicators[0].message, string("umask is correctly set in ") + filename);
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, IncorrectValue_PAM_1)
{
    auto filename = mContext.MakeTempfile("session\tpam_umask.so\t \tumask=026");
    mContext.SetSpecialFilePath("/etc/pam.d/postlogin", filename);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_EQ(root->indicators.size(), 1);
    EXPECT_EQ(root->indicators[0].message, string("umask is incorrectly set in ") + filename);
}

TEST_F(EnsureDefaultUserUmaskIsConfiguredTest, IncorrectValue_PAM_2)
{
    auto filename = mContext.MakeTempfile("session\tpam_umask.so\t \tumask=007");
    mContext.SetSpecialFilePath("/etc/pam.d/postlogin", filename);
    auto result = AuditEnsureDefaultUserUmaskIsConfigured(mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(result.Value(), Status::NonCompliant);

    const auto* root = mIndicators.GetRootNode();
    ASSERT_NE(nullptr, root);
    ASSERT_EQ(root->indicators.size(), 1);
    EXPECT_EQ(root->indicators[0].message, string("umask is incorrectly set in ") + filename);
}
