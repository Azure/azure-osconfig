// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "MockContext.h"

#include <EnsureUserIsOnlyAccountWith.h>
#include <Optional.h>
#include <fstream>

using ComplianceEngine::AuditEnsureUserIsOnlyAccountWith;
using ComplianceEngine::EnsureUserIsOnlyAccountWithParams;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::NestedListFormatter;
using ComplianceEngine::Optional;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using std::map;
using std::string;

class EnsureUserIsOnlyAccountWithTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree mIndicators;
    NestedListFormatter mFormatter;
    string mTempDir;

    void SetUp() override
    {
        mIndicators.Push("UfwStatus");
        char tempDirTemplate[] = "/tmp/EnsureUserIsOnlyAccountWithTestXXXXXX";
        char* tempDir = mkdtemp(tempDirTemplate);
        ASSERT_NE(tempDir, nullptr);
        mTempDir = tempDir;
    }

    void TearDown() override
    {
        if (!mTempDir.empty())
        {
            if (0 != remove(mTempDir.c_str()))
            {
                OsConfigLogError(mContext.GetLogHandle(), "Failed to remove temporary directory %s: %s", mTempDir.c_str(), strerror(errno));
            }
            mTempDir.clear();
        }
    }

    string CreateTestPasswdFile(string username, Optional<string> password, Optional<int> uid = Optional<int>(), Optional<int> gid = Optional<int>(),
        Optional<string> home = Optional<string>(), Optional<string> shell = Optional<string>())
    {
        auto content = std::move(username);
        content += ":" + (password.HasValue() ? password.Value() : "");
        content += ":" + (uid.HasValue() ? std::to_string(uid.Value()) : "");
        content += ":" + (gid.HasValue() ? std::to_string(gid.Value()) : "");
        content += ":" + (home.HasValue() ? home.Value() : "");
        content += ":" + (shell.HasValue() ? shell.Value() : "");

        return CreateTestPasswdFile(std::move(content));
    }

    string CreateTestPasswdFile(string content)
    {
        string shadowFilePath = mTempDir + "/passwd";
        std::ofstream shadowFile(shadowFilePath);
        if (!shadowFile.is_open())
        {
            OsConfigLogError(mContext.GetLogHandle(), "Failed to create test shadow file %s: %s", shadowFilePath.c_str(), strerror(errno));
            return string();
        }
        shadowFile << std::move(content);
        shadowFile.close();
        return shadowFilePath;
    }

    void RemoveTestShadowFile(const string& shadowFilePath)
    {
        if (shadowFilePath.empty())
        {
            return;
        }

        if (0 != remove(shadowFilePath.c_str()))
        {
            OsConfigLogError(mContext.GetLogHandle(), "Failed to remove test shadow file %s: %s", shadowFilePath.c_str(), strerror(errno));
        }
    }
};

TEST_F(EnsureUserIsOnlyAccountWithTest, NoParameter)
{
    auto path = CreateTestPasswdFile(string("foo"), string("x"), 8888, 1000, string("/home/foo"), string("/bin/bash"));
    EnsureUserIsOnlyAccountWithParams params;
    params.username = "foo";
    params.test_etcPasswdPath = path;
    auto result = AuditEnsureUserIsOnlyAccountWith(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureUserIsOnlyAccountWithTest, EmptyFile)
{
    auto path = CreateTestPasswdFile("");
    EnsureUserIsOnlyAccountWithParams params;
    params.username = "foo";
    params.uid = 8888;
    params.test_etcPasswdPath = path;
    auto result = AuditEnsureUserIsOnlyAccountWith(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureUserIsOnlyAccountWithTest, SingleUID)
{
    auto path = CreateTestPasswdFile("foo:x:8888:9999:/home/foo:/bin/bash");
    EnsureUserIsOnlyAccountWithParams params;
    params.username = "foo";
    params.uid = 8888;
    params.test_etcPasswdPath = path;
    auto result = AuditEnsureUserIsOnlyAccountWith(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureUserIsOnlyAccountWithTest, DuplicatedUID)
{
    auto path = CreateTestPasswdFile(
        "foo:x:8888:9999:/home/foo:/bin/bash\n"
        "bar:x:8888:9999:/home/bar:/bin/bash");
    EnsureUserIsOnlyAccountWithParams params;
    params.username = "foo";
    params.uid = 8888;
    params.test_etcPasswdPath = path;
    auto result = AuditEnsureUserIsOnlyAccountWith(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureUserIsOnlyAccountWithTest, SingleGID)
{
    auto path = CreateTestPasswdFile("foo:x:8888:9999:/home/foo:/bin/bash");
    EnsureUserIsOnlyAccountWithParams params;
    params.username = "foo";
    params.gid = 9999;
    params.test_etcPasswdPath = path;
    auto result = AuditEnsureUserIsOnlyAccountWith(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureUserIsOnlyAccountWithTest, DuplicatedGID)
{
    auto path = CreateTestPasswdFile(
        "foo:x:8888:9999:/home/foo:/bin/bash\n"
        "bar:x:8888:9999:/home/bar:/bin/bash");
    EnsureUserIsOnlyAccountWithParams params;
    params.username = "foo";
    params.gid = 9999;
    params.test_etcPasswdPath = path;
    auto result = AuditEnsureUserIsOnlyAccountWith(params, mIndicators, mContext);
    RemoveTestShadowFile(path);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}
