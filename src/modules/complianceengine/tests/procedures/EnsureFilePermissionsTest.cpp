// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "MockContext.h"

#include <EnsureFilePermissions.h>
#include <dirent.h>
#include <fstream>
#include <gtest/gtest.h>
#include <linux/limits.h>
#include <string>
#include <unistd.h>
#include <vector>

using ComplianceEngine::AuditEnsureFilePermissions;
using ComplianceEngine::AuditEnsureFilePermissionsCollection;
using ComplianceEngine::EnsureFilePermissionsCollectionParams;
using ComplianceEngine::EnsureFilePermissionsParams;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Optional;
using ComplianceEngine::Pattern;
using ComplianceEngine::RemediateEnsureFilePermissions;
using ComplianceEngine::RemediateEnsureFilePermissionsCollection;
using ComplianceEngine::Result;
using ComplianceEngine::Separated;
using ComplianceEngine::Status;

class EnsureFilePermissionsTest : public ::testing::Test
{
protected:
    char fileTemplate[PATH_MAX] = "/tmp/permTest.XXXXXX";
    std::vector<std::string> files;
    char dirTemplate[PATH_MAX] = "/tmp/permCollectionTest.XXXXXX";
    std::string testDir;
    MockContext mContext;
    IndicatorsTree indicators;
    ComplianceEngine::NestedListFormatter mFormatter;

    void SetUp() override
    {
        if (0 != getuid())
        {
            GTEST_SKIP() << "This test suite requires root privileges or fakeroot";
        }
        // SLES15 docker image doesn't have the bin group/user, create if it doesn't exist.
        system("groupadd -g 1 bin >/dev/null 2>&1");
        system("useradd -g 1 -u 1 bin >/dev/null 2>&1");
        testDir = mkdtemp(dirTemplate);
        ASSERT_FALSE(testDir.empty());
        indicators.Push("EnsureFilePermissions");
    }

    void TearDown() override
    {
        // Remove all tracked files; some may be in nested directories. Remove directories afterwards.
        for (auto& file : files)
        {
            unlink(file.c_str());
        }
        // Attempt recursive removal of any nested directories created during tests.
        std::string cmd = std::string("rm -rf ") + testDir;
        system(cmd.c_str());
    }

    void CreateFile(std::string& filename, int owner, int group, short permissions)
    {
        char* newFileName = strdup(fileTemplate);
        EXPECT_NE(newFileName, nullptr);
        int f = mkstemp(newFileName);
        if (f < 0)
        {
            free(newFileName);
            GTEST_FAIL() << "Failed to create temporary file";
        }
        close(f);
        filename = newFileName;
        files.push_back(filename);
        free(newFileName);
        ASSERT_EQ(chown(filename.c_str(), owner, group), 0);
        ASSERT_EQ(chmod(filename.c_str(), permissions), 0);
    }

    void CreateFileInDir(const std::string& filename, int owner, int group, short permissions)
    {
        std::string filePath = testDir + "/" + filename;
        std::ofstream file(filePath);
        file << "test content";
        file.close();
        ASSERT_EQ(chmod(filePath.c_str(), permissions), 0);
        ASSERT_EQ(chown(filePath.c_str(), owner, group), 0);
        files.push_back(filePath);
    }

    static void MakeSeparatedList(const std::string& input, Optional<Separated<std::string, '|'>>& output)
    {
        auto result = Separated<std::string, '|'>::Parse(input);
        ASSERT_TRUE(result.HasValue());
        output = std::move(result.Value());
    }
};

TEST_F(EnsureFilePermissionsTest, AuditFileMissing)
{
    EnsureFilePermissionsParams params;
    params.filename = "/this_doesnt_exist_for_sure";

    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFilePermissionsTest, AuditWrongOwner)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 1, 0, 0610);
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    auto group = Pattern::Make("root");
    ASSERT_TRUE(group.HasValue());
    params.group = {{std::move(group.Value())}};
    params.permissions = 0400;
    params.mask = 0066;
    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("owner") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, RemediateWrongOwner)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 1, 0, 0610);
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    auto group = Pattern::Make("root");
    ASSERT_TRUE(group.HasValue());
    params.permissions = 0400;
    params.mask = 0066;

    auto result = RemediateEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    struct stat st;
    ASSERT_EQ(stat(params.filename.c_str(), &st), 0);
    ASSERT_EQ(st.st_uid, 0);
    ASSERT_EQ(st.st_gid, 0);
    ASSERT_EQ(st.st_mode & 0777, 0610);
}

TEST_F(EnsureFilePermissionsTest, AuditWrongGroup)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 0, 1, 0610);
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    auto group = Pattern::Make("root");
    ASSERT_TRUE(group.HasValue());
    params.group = {{std::move(group).Value()}};
    params.permissions = 0400;
    params.mask = 0066;

    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("Invalid group") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, RemediateWrongGroup)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 0, 1, 0610);
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    auto group = Pattern::Make("root");
    ASSERT_TRUE(group.HasValue());
    params.group = {{std::move(group).Value()}};
    params.permissions = 0400;
    params.mask = 0066;

    auto result = RemediateEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    struct stat st;
    ASSERT_EQ(stat(params.filename.c_str(), &st), 0);
    ASSERT_EQ(st.st_uid, 0);
    ASSERT_EQ(st.st_gid, 0);
    ASSERT_EQ(st.st_mode & 0777, 0610);
}

TEST_F(EnsureFilePermissionsTest, AuditWrongPermissions)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 0, 0, 0210);
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    auto group = Pattern::Make("root");
    ASSERT_TRUE(group.HasValue());
    params.group = {{std::move(group).Value()}};
    params.permissions = 0400;
    params.mask = 0066;

    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("Invalid permissions") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, RemediateWrongPermissions)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 0, 0, 0210);
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    auto group = Pattern::Make("root");
    ASSERT_TRUE(group.HasValue());
    params.group = {{std::move(group).Value()}};
    params.permissions = 0400;
    params.mask = 0066;

    auto result = RemediateEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    struct stat st;
    ASSERT_EQ(stat(params.filename.c_str(), &st), 0);
    ASSERT_EQ(st.st_uid, 0);
    ASSERT_EQ(st.st_gid, 0);
    ASSERT_EQ(st.st_mode & 0777, 0610);
}

TEST_F(EnsureFilePermissionsTest, AuditWrongMask)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 0, 0, 0654);
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    auto group = Pattern::Make("root");
    ASSERT_TRUE(group.HasValue());
    params.group = {{std::move(group).Value()}};
    params.permissions = 0400;
    params.mask = 0066;

    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("Invalid permissions") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, RemediateWrongMask)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 0, 0, 0654);
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    auto group = Pattern::Make("root");
    ASSERT_TRUE(group.HasValue());
    params.group = {{std::move(group).Value()}};
    params.permissions = 0400;
    params.mask = 0066;

    auto result = RemediateEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    struct stat st;
    ASSERT_EQ(stat(params.filename.c_str(), &st), 0);
    ASSERT_EQ(st.st_uid, 0);
    ASSERT_EQ(st.st_gid, 0);
    ASSERT_EQ(st.st_mode & 0777, 0610);
}

TEST_F(EnsureFilePermissionsTest, AuditAllWrong)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 1, 1, 0276);
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    auto group = Pattern::Make("root");
    ASSERT_TRUE(group.HasValue());
    params.group = {{std::move(group).Value()}};
    params.permissions = 0400;
    params.mask = 0066;

    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFilePermissionsTest, RemediateAllWrong)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 1, 1, 0276);
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    auto group = Pattern::Make("root");
    ASSERT_TRUE(group.HasValue());
    params.group = {{std::move(group).Value()}};
    params.permissions = 0400;
    params.mask = 0066;
    auto result = RemediateEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    struct stat st;
    ASSERT_EQ(stat(params.filename.c_str(), &st), 0);
    ASSERT_EQ(st.st_uid, 0);
    ASSERT_EQ(st.st_gid, 0);
    ASSERT_EQ(st.st_mode & 0777, 0610);
}

TEST_F(EnsureFilePermissionsTest, AuditAllOk)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 0, 0, 0610);
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    auto group = Pattern::Make("root");
    ASSERT_TRUE(group.HasValue());
    params.group = {{std::move(group).Value()}};
    params.permissions = 0400;
    params.mask = 0066;
    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFilePermissionsTest, RemediateAllOk)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 0, 0, 0610);
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    auto group = Pattern::Make("root");
    ASSERT_TRUE(group.HasValue());
    params.group = {{std::move(group).Value()}};
    params.permissions = 0400;
    params.mask = 0066;
    auto result = RemediateEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    struct stat st;
    ASSERT_EQ(stat(params.filename.c_str(), &st), 0);
    ASSERT_EQ(st.st_uid, 0);
    ASSERT_EQ(st.st_gid, 0);
    ASSERT_EQ(st.st_mode & 0777, 0610);
}

TEST_F(EnsureFilePermissionsTest, AuditBadFileOwner)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 15213, 0, 0600);
    auto owner = Pattern::Make("boohoonotarealuser");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFilePermissionsTest, RemediateBadFileOwner)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 15213, 0, 0600);
    auto owner = Pattern::Make("boohoonotarealuser");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    auto result = RemediateEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFilePermissionsTest, AuditBadFileGroup)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 0, 15213, 0600);
    auto group = Pattern::Make("boohoonotarealgroup");
    ASSERT_TRUE(group.HasValue());
    params.group = {{std::move(group.Value())}};
    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFilePermissionsTest, RemediateBadFileGroup)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 0, 15213, 0600);
    auto group = Pattern::Make("boohoonotarealgroup");
    ASSERT_TRUE(group.HasValue());
    params.group = {{std::move(group.Value())}};
    auto result = RemediateEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFilePermissionsTest, AuditSameBitsSet)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 0, 0, 0600);
    params.permissions = 600;
    params.mask = 600;
    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_FALSE(result.HasValue());
}

TEST_F(EnsureFilePermissionsTest, RemediateSameBitsSet)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 0, 0, 0600);
    params.permissions = 600;
    params.mask = 600;
    auto result = RemediateEnsureFilePermissions(params, indicators, mContext);
    ASSERT_FALSE(result.HasValue());
}

TEST_F(EnsureFilePermissionsTest, AuditCollectionAllCompliant)
{
    CreateFileInDir("file1.txt", 0, 0, 0644);
    CreateFileInDir("file2.txt", 0, 0, 0644);

    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "*.txt";
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    auto group = Pattern::Make("root");
    ASSERT_TRUE(group.HasValue());
    params.permissions = 0644;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("file1.txt") != std::string::npos);
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFilePermissionsTest, AuditCollectionExplicitFile)
{
    CreateFileInDir("file1.txt", 0, 0, 0644);

    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "file1.txt";
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    auto group = Pattern::Make("root");
    ASSERT_TRUE(group.HasValue());
    params.permissions = 0644;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("file1.txt owner") != std::string::npos);
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFilePermissionsTest, AuditCollectionQuestionMark)
{
    CreateFileInDir("file1.txt", 0, 0, 0644);
    CreateFileInDir("file2.txt", 0, 0, 0644);
    CreateFileInDir("file1.log", 0, 0, 0644);
    CreateFileInDir("file13.txt", 0, 0, 0644);

    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "file?.txt";
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    auto group = Pattern::Make("root");
    ASSERT_TRUE(group.HasValue());
    params.permissions = 0644;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("file1.txt") != std::string::npos);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("file2.txt") != std::string::npos);
    ASSERT_FALSE(mFormatter.Format(indicators).Value().find("file1.log") != std::string::npos);
    ASSERT_FALSE(mFormatter.Format(indicators).Value().find("file13.txt") != std::string::npos);
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFilePermissionsTest, AuditCollectionNonCompliantFile)
{
    CreateFileInDir("file1.txt", 0, 0, 0644);
    CreateFileInDir("file2.txt", 1000, 0, 0644);

    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "*.txt";
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    auto group = Pattern::Make("root");
    ASSERT_TRUE(group.HasValue());
    params.permissions = 0644;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFilePermissionsTest, RemediateCollectionNonCompliantFile)
{
    CreateFileInDir("file1.txt", 0, 0, 0644);
    CreateFileInDir("file2.txt", 1000, 0, 0600);

    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "*.txt";
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    auto group = Pattern::Make("root");
    ASSERT_TRUE(group.HasValue());
    params.permissions = 0644;

    auto result = RemediateEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);

    struct stat st;
    for (const auto& file : files)
    {
        ASSERT_EQ(stat(file.c_str(), &st), 0);
        ASSERT_EQ(st.st_uid, 0);
        ASSERT_EQ(st.st_gid, 0);
        ASSERT_EQ(st.st_mode & 0777, 0644);
    }
}

TEST_F(EnsureFilePermissionsTest, AuditCollectionNoMatchingFiles)
{
    CreateFileInDir("file1.log", 0, 0, 0644);
    CreateFileInDir("file2.log", 0, 0, 0644);

    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "*.txt";
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    auto group = Pattern::Make("root");
    ASSERT_TRUE(group.HasValue());
    params.permissions = 0644;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

// New tests to validate recurse flag behavior.
// When recurse is default (true), nested non-compliant files should trigger NonCompliant.
TEST_F(EnsureFilePermissionsTest, AuditCollectionRecurseDefaultTrue)
{
    // Create top-level compliant file
    CreateFileInDir("top.txt", 0, 0, 0644);
    // Create nested directory and a non-compliant file inside it (wrong owner)
    std::string nestedDir = testDir + "/nested";
    ASSERT_EQ(mkdir(nestedDir.c_str(), 0755), 0);
    std::string nestedFile = nestedDir + "/bad.txt";
    std::ofstream nf(nestedFile);
    nf << "content";
    nf.close();
    ASSERT_EQ(chmod(nestedFile.c_str(), 0644), 0);
    // Set owner to uid 1 (bin) so it differs if expecting root
    ASSERT_EQ(chown(nestedFile.c_str(), 1, 0), 0);
    files.push_back(nestedFile);

    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "*.txt"; // Matches both files
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    params.permissions = 0644;
    // Do not set recurse explicitly; default should be true

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

// When recurse is false, nested non-compliant files should be ignored and result remain Compliant.
TEST_F(EnsureFilePermissionsTest, AuditCollectionRecurseFalse)
{
    // Create top-level compliant file
    CreateFileInDir("top.txt", 0, 0, 0644);
    // Create nested directory and a non-compliant file inside it (wrong owner)
    std::string nestedDir = testDir + "/nested2";
    ASSERT_EQ(mkdir(nestedDir.c_str(), 0755), 0);
    std::string nestedFile = nestedDir + "/bad.txt";
    std::ofstream nf(nestedFile);
    nf << "content";
    nf.close();
    ASSERT_EQ(chmod(nestedFile.c_str(), 0644), 0);
    ASSERT_EQ(chown(nestedFile.c_str(), 1, 0), 0);
    files.push_back(nestedFile);

    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "*.txt"; // Matches both files but recurse false should skip nested
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    params.permissions = 0644;
    params.recurse = false;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
