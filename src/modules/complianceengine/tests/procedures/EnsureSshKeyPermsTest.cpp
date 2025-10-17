// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Clean minimal test file begins
#include "Evaluator.h"
#include "MockContext.h"

#include <EnsureSshKeyPerms.h>
#include <fstream>
#include <gtest/gtest.h>
#include <map>
#include <sstream>
#include <sys/stat.h>

using ComplianceEngine::AuditEnsureSshKeyPerms;
using ComplianceEngine::EnsureSshKeyPermsParams;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::RemediateEnsureSshKeyPerms;
using ComplianceEngine::SshKeyType;
using ComplianceEngine::Status;

static const char kPublicKeySample[] = "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIFakeKeyMaterial user@example\n";
static const char kPrivateKeyHeader[] = "-----BEGIN OPENSSH PRIVATE KEY-----\n";

class EnsureSshKeyPermsTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;
    std::string mSshDir;
    void SetUp() override
    {
        if (0 != getuid())
        {
            GTEST_SKIP() << "This test suite requires root privileges or fakeroot";
        }

        mIndicators.Push("EnsureSshKeyPerms");
        mSshDir = mContext.GetFilesystemScannerRoot() + std::string("/etc/ssh");
        ::mkdir((mContext.GetFilesystemScannerRoot() + std::string("/etc")).c_str(), 0755);
        ::mkdir(mSshDir.c_str(), 0755);
        mContext.SetSpecialFilePath("/etc/ssh", mSshDir);
        using ::testing::Invoke;
        // Generic default: return file contents if exists, else empty string.
        ON_CALL(mContext, GetFileContents(::testing::_)).WillByDefault(Invoke([&](const std::string& path) -> ComplianceEngine::Result<std::string> {
            std::string physical = path;
            if (physical.rfind("/etc/ssh/", 0) == 0)
            {
                physical = mSshDir + physical.substr(std::string("/etc/ssh").size());
            }
            std::ifstream in(physical);
            if (!in.good())
            {
                return std::string();
            }
            std::ostringstream ss;
            ss << in.rdbuf();
            return ss.str();
        }));
    }
};

TEST_F(EnsureSshKeyPermsTest, PublicKeyCompliant)
{
    std::string keyPath = mSshDir + "/id_ed25519.pub";
    {
        std::ofstream f(keyPath);
        f << kPublicKeySample;
    }
    EXPECT_CALL(mContext, GetFileContents(keyPath)).WillOnce(::testing::Return(ComplianceEngine::Result<std::string>(kPublicKeySample)));
    chmod(keyPath.c_str(), (0644 & ~0133));
    EnsureSshKeyPermsParams params;
    params.type = SshKeyType::Public;

    auto result = AuditEnsureSshKeyPerms(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSshKeyPermsTest, PublicKeyNonCompliantBitMask)
{
    std::string keyPath = mSshDir + "/id_test.pub";
    {
        std::ofstream f(keyPath);
        f << kPublicKeySample;
    }
    EXPECT_CALL(mContext, GetFileContents(keyPath)).WillOnce(::testing::Return(ComplianceEngine::Result<std::string>(kPublicKeySample)));
    chmod(keyPath.c_str(), 0644 | 0020); // forbidden group write bit
    EnsureSshKeyPermsParams params;
    params.type = SshKeyType::Public;
    auto result = AuditEnsureSshKeyPerms(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSshKeyPermsTest, PrivateKeyCompliant)
{
    std::string keyPath = mSshDir + "/id_ed25519";
    {
        std::ofstream f(keyPath);
        f << kPrivateKeyHeader << "...payload...\n";
    }
    EXPECT_CALL(mContext, GetFileContents(keyPath)).WillOnce(::testing::Return(ComplianceEngine::Result<std::string>(std::string(kPrivateKeyHeader) + "...payload...\n")));
    chmod(keyPath.c_str(), 0600);
    EnsureSshKeyPermsParams params;
    params.type = SshKeyType::Private;
    auto result = AuditEnsureSshKeyPerms(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSshKeyPermsTest, PrivateKeyRemediation)
{
    std::string keyPath = mSshDir + "/id_ed25519";
    {
        std::ofstream f(keyPath);
        f << kPrivateKeyHeader << "...payload...\n";
    }
    EXPECT_CALL(mContext, GetFileContents(keyPath)).WillRepeatedly(::testing::Return(ComplianceEngine::Result<std::string>(std::string(kPrivateKeyHeader) + "...payload...\n")));
    chmod(keyPath.c_str(), 0777); // bad perms
    EnsureSshKeyPermsParams params;
    params.type = SshKeyType::Private;
    auto remediate = RemediateEnsureSshKeyPerms(params, mIndicators, mContext);
    ASSERT_TRUE(remediate.HasValue());
    ASSERT_EQ(remediate.Value(), Status::Compliant);
    struct stat st;
    ASSERT_EQ(0, stat(keyPath.c_str(), &st));
    ASSERT_EQ(0u, (st.st_mode & 0137));
    auto post = AuditEnsureSshKeyPerms(params, mIndicators, mContext);
    ASSERT_TRUE(post.HasValue());
    ASSERT_EQ(post.Value(), Status::Compliant);
}

TEST_F(EnsureSshKeyPermsTest, RemediationNoKeys)
{
    EnsureSshKeyPermsParams params;
    params.type = SshKeyType::Public;
    auto result = RemediateEnsureSshKeyPerms(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
