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
#include <vector>

using compliance::AuditEnsureFilePermissions;
using compliance::AuditEnsureFilePermissionsCollection;
using compliance::Error;
using compliance::IndicatorsTree;
using compliance::RemediateEnsureFilePermissions;
using compliance::RemediateEnsureFilePermissionsCollection;
using compliance::Result;
using compliance::Status;

class EnsureFilePermissionsTest : public ::testing::Test
{
protected:
    char fileTemplate[PATH_MAX] = "/tmp/permTest.XXXXXX";
    std::vector<std::string> files;
    char dirTemplate[PATH_MAX] = "/tmp/permCollectionTest.XXXXXX";
    std::string testDir;
    MockContext mContext;
    IndicatorsTree indicators;
    compliance::NestedListFormatter mFormatter;

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
        for (auto& file : files)
        {
            unlink(file.c_str());
        }
        rmdir(testDir.c_str());
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
};

TEST_F(EnsureFilePermissionsTest, AuditFileMissing)
{
    std::map<std::string, std::string> args;
    args["filename"] = "/this_doesnt_exist_for_sure";

    auto result = AuditEnsureFilePermissions(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFilePermissionsTest, AuditWrongOwner)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 1, 0, 0610);
    args["owner"] = "root";
    args["group"] = "root";
    args["permissions"] = "0400";
    args["mask"] = "0066";
    auto result = AuditEnsureFilePermissions(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("owner") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, RemediateWrongOwner)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 1, 0, 0610);
    args["owner"] = "root";
    args["group"] = "root";
    args["permissions"] = "0400";
    args["mask"] = "0066";

    auto result = RemediateEnsureFilePermissions(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    struct stat st;
    ASSERT_EQ(stat(args["filename"].c_str(), &st), 0);
    ASSERT_EQ(st.st_uid, 0);
    ASSERT_EQ(st.st_gid, 0);
    ASSERT_EQ(st.st_mode & 0777, 0610);
}

TEST_F(EnsureFilePermissionsTest, AuditWrongGroup)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 0, 1, 0610);
    args["owner"] = "root";
    args["group"] = "root";
    args["permissions"] = "0400";
    args["mask"] = "0066";

    auto result = AuditEnsureFilePermissions(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("Invalid group") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, RemediateWrongGroup)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 0, 1, 0610);
    args["owner"] = "root";
    args["group"] = "root";
    args["permissions"] = "0400";
    args["mask"] = "0066";

    auto result = RemediateEnsureFilePermissions(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    struct stat st;
    ASSERT_EQ(stat(args["filename"].c_str(), &st), 0);
    ASSERT_EQ(st.st_uid, 0);
    ASSERT_EQ(st.st_gid, 0);
    ASSERT_EQ(st.st_mode & 0777, 0610);
}

TEST_F(EnsureFilePermissionsTest, AuditWrongPermissions)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 0, 0, 0210);
    args["owner"] = "root";
    args["group"] = "root";
    args["permissions"] = "0400";
    args["mask"] = "0066";

    auto result = AuditEnsureFilePermissions(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("Invalid permissions") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, RemediateWrongPermissions)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 0, 0, 0210);
    args["owner"] = "root";
    args["group"] = "root";
    args["permissions"] = "0400";
    args["mask"] = "0066";

    auto result = RemediateEnsureFilePermissions(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    struct stat st;
    ASSERT_EQ(stat(args["filename"].c_str(), &st), 0);
    ASSERT_EQ(st.st_uid, 0);
    ASSERT_EQ(st.st_gid, 0);
    ASSERT_EQ(st.st_mode & 0777, 0610);
}

TEST_F(EnsureFilePermissionsTest, AuditWrongMask)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 0, 0, 0654);
    args["owner"] = "root";
    args["group"] = "root";
    args["permissions"] = "0400";
    args["mask"] = "0066";

    auto result = AuditEnsureFilePermissions(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("Invalid permissions") != std::string::npos);
}

TEST_F(EnsureFilePermissionsTest, RemediateWrongMask)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 0, 0, 0654);
    args["owner"] = "root";
    args["group"] = "root";
    args["permissions"] = "0400";
    args["mask"] = "0066";

    auto result = RemediateEnsureFilePermissions(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    struct stat st;
    ASSERT_EQ(stat(args["filename"].c_str(), &st), 0);
    ASSERT_EQ(st.st_uid, 0);
    ASSERT_EQ(st.st_gid, 0);
    ASSERT_EQ(st.st_mode & 0777, 0610);
}

TEST_F(EnsureFilePermissionsTest, AuditAllWrong)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 1, 1, 0276);
    args["owner"] = "root";
    args["group"] = "root";
    args["permissions"] = "0400";
    args["mask"] = "0066";

    auto result = AuditEnsureFilePermissions(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFilePermissionsTest, RemediateAllWrong)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 1, 1, 0276);
    args["owner"] = "root";
    args["group"] = "root";
    args["permissions"] = "0400";
    args["mask"] = "0066";
    auto result = RemediateEnsureFilePermissions(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    struct stat st;
    ASSERT_EQ(stat(args["filename"].c_str(), &st), 0);
    ASSERT_EQ(st.st_uid, 0);
    ASSERT_EQ(st.st_gid, 0);
    ASSERT_EQ(st.st_mode & 0777, 0610);
}

TEST_F(EnsureFilePermissionsTest, AuditAllOk)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 0, 0, 0610);
    args["owner"] = "root";
    args["group"] = "root";
    args["permissions"] = "0400";
    args["mask"] = "0066";
    auto result = AuditEnsureFilePermissions(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFilePermissionsTest, RemediateAllOk)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 0, 0, 0610);
    args["owner"] = "root";
    args["group"] = "root";
    args["permissions"] = "0400";
    args["mask"] = "0066";
    auto result = RemediateEnsureFilePermissions(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    struct stat st;
    ASSERT_EQ(stat(args["filename"].c_str(), &st), 0);
    ASSERT_EQ(st.st_uid, 0);
    ASSERT_EQ(st.st_gid, 0);
    ASSERT_EQ(st.st_mode & 0777, 0610);
}

TEST_F(EnsureFilePermissionsTest, AuditMissingFilename)
{
    std::map<std::string, std::string> args;
    auto result = AuditEnsureFilePermissions(args, indicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "No filename provided");
}

TEST_F(EnsureFilePermissionsTest, RemediateMissingFilename)
{
    std::map<std::string, std::string> args;
    auto result = RemediateEnsureFilePermissions(args, indicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "No filename provided");
}

TEST_F(EnsureFilePermissionsTest, AuditBadFileOwner)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 15213, 0, 0600);
    args["owner"] = "boohoonotarealuser";
    auto result = AuditEnsureFilePermissions(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFilePermissionsTest, RemediateBadFileOwner)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 15213, 0, 0600);
    args["owner"] = "boohoonotarealuser";
    auto result = RemediateEnsureFilePermissions(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFilePermissionsTest, AuditBadFileGroup)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 0, 15213, 0600);
    args["group"] = "boohoonotarealgroup";
    auto result = AuditEnsureFilePermissions(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFilePermissionsTest, RemediateBadFileGroup)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 0, 15213, 0600);
    args["group"] = "boohoonotarealgroup";
    auto result = RemediateEnsureFilePermissions(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFilePermissionsTest, AuditBadPermissions)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 0, 0, 0600);
    args["permissions"] = "999";
    auto result = AuditEnsureFilePermissions(args, indicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Invalid permissions argument: 999");
}

TEST_F(EnsureFilePermissionsTest, RemediateBadPermissions)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 0, 0, 0600);
    args["permissions"] = "999";
    auto result = RemediateEnsureFilePermissions(args, indicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Invalid permissions argument: 999");
}

TEST_F(EnsureFilePermissionsTest, AuditBadMask)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 0, 0, 0600);
    args["mask"] = "999";
    auto result = AuditEnsureFilePermissions(args, indicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Invalid mask argument: 999");
}

TEST_F(EnsureFilePermissionsTest, RemediateBadMask)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 0, 0, 0600);
    args["mask"] = "999";
    auto result = RemediateEnsureFilePermissions(args, indicators, mContext);
    ASSERT_FALSE(result.HasValue());
}

TEST_F(EnsureFilePermissionsTest, AuditSameBitsSet)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 0, 0, 0600);
    args["permissions"] = "600";
    args["mask"] = "600";
    auto result = AuditEnsureFilePermissions(args, indicators, mContext);
    ASSERT_FALSE(result.HasValue());
}

TEST_F(EnsureFilePermissionsTest, RemediateSameBitsSet)
{
    std::map<std::string, std::string> args;
    CreateFile(args["filename"], 0, 0, 0600);
    args["permissions"] = "600";
    args["mask"] = "600";
    auto result = RemediateEnsureFilePermissions(args, indicators, mContext);
    ASSERT_FALSE(result.HasValue());
}

TEST_F(EnsureFilePermissionsTest, AuditCollectionAllCompliant)
{
    CreateFileInDir("file1.txt", 0, 0, 0644);
    CreateFileInDir("file2.txt", 0, 0, 0644);

    std::map<std::string, std::string> args;
    args["directory"] = testDir;
    args["ext"] = "*.txt";
    args["owner"] = "root";
    args["group"] = "root";
    args["permissions"] = "0644";

    auto result = AuditEnsureFilePermissionsCollection(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(mFormatter.Format(indicators).Value().find("file1.txt") != std::string::npos);
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFilePermissionsTest, AuditCollectionExplicitFile)
{
    CreateFileInDir("file1.txt", 0, 0, 0644);

    std::map<std::string, std::string> args;
    args["directory"] = testDir;
    args["ext"] = "file1.txt";
    args["owner"] = "root";
    args["group"] = "root";
    args["permissions"] = "0644";

    auto result = AuditEnsureFilePermissionsCollection(args, indicators, mContext);
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

    std::map<std::string, std::string> args;
    args["directory"] = testDir;
    args["ext"] = "file?.txt";
    args["owner"] = "root";
    args["group"] = "root";
    args["permissions"] = "0644";

    auto result = AuditEnsureFilePermissionsCollection(args, indicators, mContext);
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

    std::map<std::string, std::string> args;
    args["directory"] = testDir;
    args["ext"] = "*.txt";
    args["owner"] = "root";
    args["group"] = "root";
    args["permissions"] = "0644";

    auto result = AuditEnsureFilePermissionsCollection(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFilePermissionsTest, RemediateCollectionNonCompliantFile)
{
    CreateFileInDir("file1.txt", 0, 0, 0644);
    CreateFileInDir("file2.txt", 1000, 0, 0600);

    std::map<std::string, std::string> args;
    args["directory"] = testDir;
    args["ext"] = "*.txt";
    args["owner"] = "root";
    args["group"] = "root";
    args["permissions"] = "0644";

    auto result = RemediateEnsureFilePermissionsCollection(args, indicators, mContext);
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

    std::map<std::string, std::string> args;
    args["directory"] = testDir;
    args["ext"] = "*.txt";
    args["owner"] = "root";
    args["group"] = "root";
    args["permissions"] = "0644";

    auto result = AuditEnsureFilePermissionsCollection(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
