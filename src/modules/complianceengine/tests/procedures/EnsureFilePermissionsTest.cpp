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
using ComplianceEngine::Behavior;
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
    ComplianceEngine::CompactListFormatter mFormatter;

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
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("does not exist but it should") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, AuditFileMissingNoneExist)
{
    EnsureFilePermissionsParams params;
    params.filename = "/this_doesnt_exist_for_sure";
    params.behavior = Behavior::NoneExist;

    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("does not exist as it should") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, AuditFileExistsNoneExist)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 0, 0, 0600);
    params.behavior = Behavior::NoneExist;

    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("exist but it should not") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, AuditFileMissingAllExist)
{
    EnsureFilePermissionsParams params;
    params.filename = "/this_doesnt_exist_for_sure";
    params.behavior = Behavior::AllExist;

    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("does not exist but it should") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, AuditFileMissingOnlyOneExists)
{
    EnsureFilePermissionsParams params;
    params.filename = "/this_doesnt_exist_for_sure";
    params.behavior = Behavior::OnlyOneExists;

    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("does not exist but it should") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, AuditFileMissingAnyExist)
{
    EnsureFilePermissionsParams params;
    params.filename = "/this_doesnt_exist_for_sure";
    params.behavior = Behavior::AnyExist;

    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("does not exist but it should") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, AuditFileExistsAllExist)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 0, 0, 0600);
    params.behavior = Behavior::AllExist;

    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);

    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("correct permissions and ownership as expected") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, AuditFileExistsBadPermssionsBehaviorNoneExist)
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
    params.behavior = Behavior::NoneExist;
    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
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
    ASSERT_EQ(st.st_uid, 0u);
    ASSERT_EQ(st.st_gid, 0u);
    ASSERT_EQ(st.st_mode & 0777, 0610u);
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
    ASSERT_EQ(st.st_uid, 0u);
    ASSERT_EQ(st.st_gid, 0u);
    ASSERT_EQ(st.st_mode & 0777, 0610u);
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
    ASSERT_EQ(st.st_uid, 0u);
    ASSERT_EQ(st.st_gid, 0u);
    ASSERT_EQ(st.st_mode & 0777, 0610u);
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
    ASSERT_EQ(st.st_uid, 0u);
    ASSERT_EQ(st.st_gid, 0u);
    ASSERT_EQ(st.st_mode & 0777, 0610u);
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
    ASSERT_EQ(st.st_uid, 0u);
    ASSERT_EQ(st.st_gid, 0u);
    ASSERT_EQ(st.st_mode & 0777, 0610u);
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
    ASSERT_EQ(st.st_uid, 0u);
    ASSERT_EQ(st.st_gid, 0u);
    ASSERT_EQ(st.st_mode & 0777, 0610u);
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
        ASSERT_EQ(st.st_uid, 0u);
        ASSERT_EQ(st.st_gid, 0u);
        ASSERT_EQ(st.st_mode & 0777, 0644u);
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
    ASSERT_EQ(result.Value(), Status::NonCompliant);

    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("At least one file in") != std::string::npos);
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

// When recurse is false, and Behavior is AtLeastOneExists, nested non-compliant files should *not* be ignored and result be NonCompliant.
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

TEST_F(EnsureFilePermissionsTest, AuditCollectionRecurseTrueAllExistsFailOneBad)
{
    // Create top-level compliant file
    CreateFileInDir("top.txt", 0, 0, 0644);
    // Create nested directory and a non-compliant file inside it (wrong owner)
    std::string nestedDir = testDir + "/nested2";
    ASSERT_EQ(mkdir(nestedDir.c_str(), 0755), 0);

    {
        std::string nestedFile = nestedDir + "/bad.txt";
        std::ofstream nf(nestedFile);
        nf << "content";
        nf.close();
        ASSERT_EQ(chmod(nestedFile.c_str(), 0644), 0);
        ASSERT_EQ(chown(nestedFile.c_str(), 1, 0), 0);
        files.push_back(nestedFile);
    }

    {
        std::string nestedFile = nestedDir + "/good.txt";
        std::ofstream nf(nestedFile);
        nf << "content";
        nf.close();
        ASSERT_EQ(chmod(nestedFile.c_str(), 0644), 0);
        ASSERT_EQ(chown(nestedFile.c_str(), 0, 0), 0);
        files.push_back(nestedFile);
    }
    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "*.txt"; // Matches both files but recurse true and Behavior AllExist
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    params.permissions = 0644;
    params.recurse = true;
    params.behavior = Behavior::AllExist;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    std::cout << "AuditCollectionRecurseTrueAllExistsFailOneBad " << mFormatter.Format(indicators).Value() << std::endl;
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("Invalid owner") != std::string::npos);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("bad.txt") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, AuditCollectionRecurseTrueAllExistsAllgood)
{
    // Create top-level compliant file
    CreateFileInDir("top.txt", 0, 0, 0644);
    // Create nested directory and a non-compliant file inside it (wrong owner)
    std::string nestedDir = testDir + "/nested2";
    ASSERT_EQ(mkdir(nestedDir.c_str(), 0755), 0);

    {
        std::string nestedFile = nestedDir + "/good1.txt";
        std::ofstream nf(nestedFile);
        nf << "content";
        nf.close();
        ASSERT_EQ(chmod(nestedFile.c_str(), 0644), 0);
        ASSERT_EQ(chown(nestedFile.c_str(), 0, 0), 0);
        files.push_back(nestedFile);
    }

    {
        std::string nestedFile = nestedDir + "/good2.txt";
        std::ofstream nf(nestedFile);
        nf << "content";
        nf.close();
        ASSERT_EQ(chmod(nestedFile.c_str(), 0644), 0);
        ASSERT_EQ(chown(nestedFile.c_str(), 0, 0), 0);
        files.push_back(nestedFile);
    }
    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "*.txt"; // Matches both files but recurse true and Behavior AllExist
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    params.permissions = 0644;
    params.recurse = true;
    params.behavior = Behavior::AllExist;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);

    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);

    std::cout << "AuditCollectionRecurseTrueAllExistsAllgood:" << mFormatter.Format(indicators).Value() << std::endl;
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("good1.txt") != std::string::npos);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("good2.txt") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, AuditCollectionRecurseTrueNoneExists)
{
    // Create top-level compliant file
    CreateFileInDir("top.txt", 0, 0, 0644);
    // Create nested directory and a non-compliant file inside it (wrong owner)
    std::string nestedDir = testDir + "/nested2";
    ASSERT_EQ(mkdir(nestedDir.c_str(), 0755), 0);

    {
        std::string nestedFile = nestedDir + "/good1.txt";
        std::ofstream nf(nestedFile);
        nf << "content";
        nf.close();
        ASSERT_EQ(chmod(nestedFile.c_str(), 0644), 0);
        ASSERT_EQ(chown(nestedFile.c_str(), 0, 0), 0);
        files.push_back(nestedFile);
    }

    {
        std::string nestedFile = nestedDir + "/good2.txt";
        std::ofstream nf(nestedFile);
        nf << "content";
        nf.close();
        ASSERT_EQ(chmod(nestedFile.c_str(), 0644), 0);
        ASSERT_EQ(chown(nestedFile.c_str(), 0, 0), 0);
        files.push_back(nestedFile);
    }
    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "*.txt"; // Matches both files but recurse true and Behavior AllExist
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    params.permissions = 0644;
    params.recurse = true;
    params.behavior = Behavior::NoneExist;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFilePermissionsTest, AuditCollectionRecurseTrueOnlyOneExists)
{
    // Create top-level compliant file
    CreateFileInDir("top.txt", 0, 0, 0644);
    // Create nested directory and two copmpliant files
    std::string nestedDir = testDir + "/nested2";
    ASSERT_EQ(mkdir(nestedDir.c_str(), 0755), 0);

    {
        std::string nestedFile = nestedDir + "/good1.txt";
        std::ofstream nf(nestedFile);
        nf << "content";
        nf.close();
        ASSERT_EQ(chmod(nestedFile.c_str(), 0644), 0);
        ASSERT_EQ(chown(nestedFile.c_str(), 0, 0), 0);
        files.push_back(nestedFile);
    }

    {
        std::string nestedFile = nestedDir + "/good2.txt";
        std::ofstream nf(nestedFile);
        nf << "content";
        nf.close();
        ASSERT_EQ(chmod(nestedFile.c_str(), 0644), 0);
        ASSERT_EQ(chown(nestedFile.c_str(), 0, 0), 0);
        files.push_back(nestedFile);
    }
    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "*.txt"; // Matches both files but recurse true and Behavior OnlyOneExists
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    params.permissions = 0644;
    params.recurse = true;
    params.behavior = Behavior::OnlyOneExists;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFilePermissionsTest, AuditCollectionRecurseTrueOnlyOneExistsOneBad)
{
    // Create top-level compliant file
    CreateFileInDir("top.txt", 0, 0, 0644);
    // Create nested directory and a non-compliant file inside it (wrong owner)
    std::string nestedDir = testDir + "/nested2";
    ASSERT_EQ(mkdir(nestedDir.c_str(), 0755), 0);

    {
        std::string nestedFile = nestedDir + "/good1.txt";
        std::ofstream nf(nestedFile);
        nf << "content";
        nf.close();
        ASSERT_EQ(chmod(nestedFile.c_str(), 0644), 0);
        ASSERT_EQ(chown(nestedFile.c_str(), 0, 0), 0);
        files.push_back(nestedFile);
    }

    {
        std::string nestedFile = nestedDir + "/bad.txt";
        std::ofstream nf(nestedFile);
        nf << "content";
        nf.close();
        ASSERT_EQ(chmod(nestedFile.c_str(), 0644), 0);
        ASSERT_EQ(chown(nestedFile.c_str(), 1, 0), 0);
        files.push_back(nestedFile);
    }
    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "*.txt"; // Matches both files but recurse true and Behavior OnlyOneExists
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    params.permissions = 0644;
    params.recurse = true;
    params.behavior = Behavior::OnlyOneExists;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFilePermissionsTest, AuditCollectionRecurseTrueAtLeastOneExists)
{
    // Create top-level compliant file
    CreateFileInDir("top.txt", 0, 0, 0644);
    // Create nested directory and a non-compliant file inside it (wrong owner)
    std::string nestedDir = testDir + "/nested2";
    ASSERT_EQ(mkdir(nestedDir.c_str(), 0755), 0);

    {
        std::string nestedFile = nestedDir + "/good1.txt";
        std::ofstream nf(nestedFile);
        nf << "content";
        nf.close();
        ASSERT_EQ(chmod(nestedFile.c_str(), 0644), 0);
        ASSERT_EQ(chown(nestedFile.c_str(), 0, 0), 0);
        files.push_back(nestedFile);
    }

    {
        std::string nestedFile = nestedDir + "/good2.txt";
        std::ofstream nf(nestedFile);
        nf << "content";
        nf.close();
        ASSERT_EQ(chmod(nestedFile.c_str(), 0644), 0);
        ASSERT_EQ(chown(nestedFile.c_str(), 0, 0), 0);
        files.push_back(nestedFile);
    }
    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "*.txt"; // Matches both files but recurse true and Behavior OnlyOneExists
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    params.permissions = 0644;
    params.recurse = true;
    params.behavior = Behavior::AtLeastOneExists;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFilePermissionsTest, AuditCollectionRecurseTrueAtLeastOneExistsOneBad)
{
    // Create top-level compliant file
    CreateFileInDir("top.txt", 0, 0, 0644);
    // Create nested directory and a non-compliant file inside it (wrong owner)
    std::string nestedDir = testDir + "/nested2";
    ASSERT_EQ(mkdir(nestedDir.c_str(), 0755), 0);

    {
        std::string nestedFile = nestedDir + "/good1.txt";
        std::ofstream nf(nestedFile);
        nf << "content";
        nf.close();
        ASSERT_EQ(chmod(nestedFile.c_str(), 0644), 0);
        ASSERT_EQ(chown(nestedFile.c_str(), 0, 0), 0);
        files.push_back(nestedFile);
    }

    {
        std::string nestedFile = nestedDir + "/bad.txt";
        std::ofstream nf(nestedFile);
        nf << "content";
        nf.close();
        ASSERT_EQ(chmod(nestedFile.c_str(), 0644), 0);
        ASSERT_EQ(chown(nestedFile.c_str(), 1, 0), 0);
        files.push_back(nestedFile);
    }
    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "*.txt"; // Matches both files but recurse true and Behavior OnlyOneExists
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    params.permissions = 0644;
    params.recurse = true;
    params.behavior = Behavior::AtLeastOneExists;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

// ── Single-file: file exists, all Behavior values that are not NoneExist ──────

TEST_F(EnsureFilePermissionsTest, AuditFileExistsAnyExist)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 0, 0, 0600);
    params.behavior = Behavior::AnyExist;

    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("correct permissions and ownership as expected") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, AuditFileExistsOnlyOneExists)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 0, 0, 0600);
    params.behavior = Behavior::OnlyOneExists;

    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("correct permissions and ownership as expected") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, AuditFileExistsAtLeastOneExistsExplicit)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 0, 0, 0600);
    params.behavior = Behavior::AtLeastOneExists;

    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("correct permissions and ownership as expected") != std::string::npos);
}

// ── Single-file: file exists with wrong owner — behavior must not skip perms ──

TEST_F(EnsureFilePermissionsTest, AuditFileExistsBadPermsAnyExist)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 1, 0, 0600); // owner=bin, not root
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    params.behavior = Behavior::AnyExist;

    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("owner") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, AuditFileExistsBadPermsOnlyOneExists)
{
    EnsureFilePermissionsParams params;
    CreateFile(params.filename, 1, 0, 0600); // owner=bin, not root
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    params.behavior = Behavior::OnlyOneExists;

    auto result = AuditEnsureFilePermissions(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("owner") != std::string::npos);
}

// ── Collection: no matching files, all Behavior values ────────────────────────

TEST_F(EnsureFilePermissionsTest, AuditCollectionNoMatchingFilesNoneExist)
{
    CreateFileInDir("file1.log", 0, 0, 0644);

    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "*.txt"; // no txt files exist
    params.behavior = Behavior::NoneExist;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);

    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("No files in") != std::string::npos);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("match the pattern as expected") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, AuditCollectionNoMatchingFilesAllExist)
{
    CreateFileInDir("file1.log", 0, 0, 0644);

    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "*.txt";
    params.behavior = Behavior::AllExist;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("At least one file") != std::string::npos);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("but they should") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, AuditCollectionNoMatchingFilesOnlyOneExists)
{
    CreateFileInDir("file1.log", 0, 0, 0644);

    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "*.txt";
    params.behavior = Behavior::OnlyOneExists;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("Expected exactly one file") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, AuditCollectionNoMatchingFilesAnyExist)
{
    CreateFileInDir("file1.log", 0, 0, 0644);

    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "*.txt";
    params.behavior = Behavior::AnyExist;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("At least one file in") != std::string::npos);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("but it should") != std::string::npos);
}

// ── Collection: AnyExist with matching files ──────────────────────────────────

TEST_F(EnsureFilePermissionsTest, AuditCollectionAnyExistAllGood)
{
    CreateFileInDir("file1.txt", 0, 0, 0644);
    CreateFileInDir("file2.txt", 0, 0, 0644);

    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "*.txt";
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    params.permissions = 0644;
    params.behavior = Behavior::AnyExist;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFilePermissionsTest, AuditCollectionAnyExistOneBad)
{
    CreateFileInDir("file1.txt", 0, 0, 0644); // compliant
    CreateFileInDir("file2.txt", 1, 0, 0644); // wrong owner

    EnsureFilePermissionsCollectionParams params;
    params.directory = testDir;
    params.ext = "*.txt";
    auto owner = Pattern::Make("root");
    ASSERT_TRUE(owner.HasValue());
    params.owner = {{std::move(owner).Value()}};
    params.permissions = 0644;
    params.behavior = Behavior::AnyExist;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("owner") != std::string::npos);
}

// ── Collection: missing directory, NoneExist vs AtLeastOneExists ──────────────

TEST_F(EnsureFilePermissionsTest, AuditCollectionMissingDirectoryNoneExist)
{
    EnsureFilePermissionsCollectionParams params;
    params.directory = "/tmp/this_dir_does_not_exist_efp_test_noneexist";
    params.ext = "*.conf";
    params.behavior = Behavior::NoneExist;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("match the pattern as expected") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, AuditCollectionMissingDirectoryAtLeastOneExists)
{
    EnsureFilePermissionsCollectionParams params;
    params.directory = "/tmp/this_dir_does_not_exist_efp_test_atleastone";
    params.ext = "*.conf";
    params.behavior = Behavior::AtLeastOneExists;

    auto result = AuditEnsureFilePermissionsCollection(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("At least one file") != std::string::npos);

    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("did not match required permissions but it should") != std::string::npos);
}
