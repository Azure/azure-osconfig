// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <dirent.h>
#include <fstream>
#include <grp.h>
#include <gtest/gtest.h>
#include <linux/limits.h>
#include <pwd.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

using ComplianceEngine::AuditEnsureLogfileAccess;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::RemediateEnsureLogfileAccess;
using ComplianceEngine::Result;
using ComplianceEngine::Status;

class EnsureLogfileAccessTest : public ::testing::Test
{
protected:
    char dirTemplate[PATH_MAX] = "/tmp/logfileTest.XXXXXX";
    std::string testDir;
    std::vector<std::string> createdFiles;
    std::vector<std::string> createdDirs;
    MockContext mContext;
    IndicatorsTree indicators;
    ComplianceEngine::NestedListFormatter mFormatter;

    void SetUp() override
    {
        if (0 != getuid())
        {
            GTEST_SKIP() << "This test suite requires root privileges or fakeroot";
        }
        SetLoggingLevel(LoggingLevelDebug);
        // Create necessary groups/users for testing
        system("groupadd bin >/dev/null");
        system("useradd -g bin bin >/dev/null");
        system("groupadd adm >/dev/null");
        system("groupadd utmp >/dev/null");
        system("useradd -g adm syslog >/dev/null");
        system("groupadd systemd-journal >/dev/null");

        testDir = mkdtemp(dirTemplate);
        ASSERT_FALSE(testDir.empty());
        indicators.Push("EnsureLogfileAccess");
    }

    void TearDown() override
    {
        // Clean up created files
        for (const auto& file : createdFiles)
        {
            unlink(file.c_str());
        }

        // Clean up created directories (in reverse order)
        for (auto it = createdDirs.rbegin(); it != createdDirs.rend(); ++it)
        {
            rmdir(it->c_str());
        }

        rmdir(testDir.c_str());
    }

    void CreateLogFile(const std::string& filename, const std::string& owner, const std::string& group, mode_t permissions)
    {
        std::string filePath = testDir + "/" + filename;
        std::ofstream file(filePath);
        file << "test log content\n";
        file.close();

        struct passwd* pwd = getpwnam(owner.c_str());
        uid_t ownerId = (pwd != nullptr) ? pwd->pw_uid : 0; // Default to root if user not found

        struct group* grp = getgrnam(group.c_str());
        gid_t groupId = (grp != nullptr) ? grp->gr_gid : 0; // Default to root if group not found

        ASSERT_EQ(chmod(filePath.c_str(), permissions), 0);
        ASSERT_EQ(chown(filePath.c_str(), ownerId, groupId), 0);
        createdFiles.push_back(filePath);
    }

    void CreateSubdir(const std::string& dirname)
    {
        std::string dirPath = testDir + "/" + dirname;
        ASSERT_EQ(mkdir(dirPath.c_str(), 0755), 0);
        createdDirs.push_back(dirPath);
    }

    void CreateSymlink(const std::string& linkname, const std::string& target)
    {
        std::string linkPath = testDir + "/" + linkname;
        ASSERT_EQ(symlink(target.c_str(), linkPath.c_str()), 0);
        createdFiles.push_back(linkPath);
    }

    void VerifyFilePermissions(const std::string& filename, uid_t expectedOwner, gid_t expectedGroup, mode_t expectedPerms)
    {
        std::string filePath = testDir + "/" + filename;
        struct stat statInfo;
        ASSERT_EQ(stat(filePath.c_str(), &statInfo), 0);
        EXPECT_EQ(statInfo.st_uid, expectedOwner);
        EXPECT_EQ(statInfo.st_gid, expectedGroup);
        EXPECT_EQ(statInfo.st_mode & 0777, expectedPerms);
    }
};

// Test audit with missing directory
TEST_F(EnsureLogfileAccessTest, AuditMissingDirectory)
{
    std::map<std::string, std::string> args;
    args["path"] = "/nonexistent/log/directory";

    auto result = AuditEnsureLogfileAccess(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant); // Missing directory should be compliant
}

// Test audit with default path (using test directory)
TEST_F(EnsureLogfileAccessTest, AuditEmptyDirectory)
{
    std::map<std::string, std::string> args;
    args["path"] = testDir;

    auto result = AuditEnsureLogfileAccess(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

// Test audit with correct mask for various log file patterns
TEST_F(EnsureLogfileAccessTest, AuditCorrectPermissions)
{
    // Create files with correct mask according to patterns
    CreateLogFile("syslog", "syslog", "adm", 0640);   // syslog user, adm group, 640
    CreateLogFile("auth.log", "syslog", "adm", 0640); // matches *auth* pattern
    CreateLogFile("secure", "syslog", "adm", 0640);   // matches *secure* pattern
    CreateLogFile("messages", "syslog", "adm", 0640); // matches *message* pattern
    CreateLogFile("test.log", "syslog", "adm", 0640); // matches *.log pattern (default)
    CreateLogFile("wtmp", "root", "utmp", 0664);      // matches wtmp pattern
    CreateLogFile("lastlog", "root", "utmp", 0664);   // matches lastlog pattern

    std::map<std::string, std::string> args;
    args["path"] = testDir;

    auto result = AuditEnsureLogfileAccess(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

// Test audit with incorrect mask
TEST_F(EnsureLogfileAccessTest, AuditIncorrectPermissions)
{
    // Create file with wrong mask
    CreateLogFile("auth.log", "bin", "bin", 0777); // Wrong owner, group, and mask

    std::map<std::string, std::string> args;
    args["path"] = testDir;

    auto result = AuditEnsureLogfileAccess(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

// Test audit with mixed correct and incorrect mask
TEST_F(EnsureLogfileAccessTest, AuditMixedPermissions)
{
    // Create some files with correct mask
    CreateLogFile("syslog", "syslog", "adm", 0640);
    CreateLogFile("messages", "syslog", "adm", 0640);

    // Create some files with incorrect mask
    CreateLogFile("auth.log", "bin", "bin", 0777);
    CreateLogFile("secure", "bin", "bin", 0666);

    std::map<std::string, std::string> args;
    args["path"] = testDir;

    auto result = AuditEnsureLogfileAccess(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

// Test audit ignores directories
TEST_F(EnsureLogfileAccessTest, AuditIgnoresDirectories)
{
    CreateSubdir("subdir");
    CreateLogFile("syslog", "syslog", "adm", 0640);

    std::map<std::string, std::string> args;
    args["path"] = testDir;

    auto result = AuditEnsureLogfileAccess(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

// Test audit ignores symbolic links
TEST_F(EnsureLogfileAccessTest, AuditIgnoresSymlinks)
{
    CreateLogFile("real.log", "syslog", "adm", 0640);
    CreateSymlink("link.log", "real.log");

    std::map<std::string, std::string> args;
    args["path"] = testDir;

    auto result = AuditEnsureLogfileAccess(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

// Test pattern matching for specific log file types
TEST_F(EnsureLogfileAccessTest, AuditPatternMatching)
{
    // Test various patterns
    CreateLogFile("secure", "syslog", "adm", 0640);                    // Should match *secure* pattern
    CreateLogFile("secure.1", "syslog", "adm", 0640);                  // Should match *secure* pattern
    CreateLogFile("auth.log", "syslog", "adm", 0640);                  // Should match *auth* pattern
    CreateLogFile("something.log", "syslog", "adm", 0640);             // Should match *.log pattern
    CreateLogFile("journal.journal", "root", "systemd-journal", 0640); // Should match *.journal pattern
    CreateLogFile("random_file", "syslog", "adm", 0640);               // Should match default pattern

    std::map<std::string, std::string> args;
    args["path"] = testDir;

    auto result = AuditEnsureLogfileAccess(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

// Test recursive directory traversal
TEST_F(EnsureLogfileAccessTest, AuditRecursiveDirectories)
{
    CreateSubdir("apache2");
    CreateLogFile("apache2/access.log", "syslog", "adm", 0640);
    CreateLogFile("apache2/error.log", "syslog", "adm", 0640);

    CreateSubdir("mail");
    CreateLogFile("mail/mail.log", "syslog", "adm", 0640);

    std::map<std::string, std::string> args;
    args["path"] = testDir;

    auto result = AuditEnsureLogfileAccess(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

// Test remediation of incorrect mask
TEST_F(EnsureLogfileAccessTest, RemediateIncorrectPermissions)
{
    // Create file with wrong mask
    CreateLogFile("auth.log", "bin", "bin", 0777);

    std::map<std::string, std::string> args;
    args["path"] = testDir;

    auto result = RemediateEnsureLogfileAccess(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);

    // Verify the file now has correct mask (auth.log should be root|syslog:root|adm:640)
    std::string filePath = testDir + "/auth.log";
    struct stat statInfo;
    ASSERT_EQ(stat(filePath.c_str(), &statInfo), 0);

    // Check that mask is now 640
    EXPECT_EQ(statInfo.st_mode & 0777, 0640);
    // Owner should be root(0) or syslog(101)
    EXPECT_TRUE(statInfo.st_uid == 0 || statInfo.st_uid == 101);
    // Group should be root(0) or adm(4)
    EXPECT_TRUE(statInfo.st_gid == 0 || statInfo.st_gid == 4);
}

// Test remediation with missing directory
TEST_F(EnsureLogfileAccessTest, RemediateMissingDirectory)
{
    std::map<std::string, std::string> args;
    args["path"] = "/nonexistent/log/directory";

    auto result = RemediateEnsureLogfileAccess(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant); // Missing directory should be compliant
}

// Test remediation with already correct mask
TEST_F(EnsureLogfileAccessTest, RemediateAlreadyCorrect)
{
    CreateLogFile("syslog", "syslog", "adm", 0640);

    std::map<std::string, std::string> args;
    args["path"] = testDir;

    auto result = RemediateEnsureLogfileAccess(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);

    // Verify mask haven't changed
    struct passwd* pwd = getpwnam("syslog");
    uid_t syslogUid = (pwd != nullptr) ? pwd->pw_uid : 0;
    struct group* grp = getgrnam("adm");
    gid_t admGid = (grp != nullptr) ? grp->gr_gid : 0;
    VerifyFilePermissions("syslog", syslogUid, admGid, 0640);
}

// Test remediation of multiple files
TEST_F(EnsureLogfileAccessTest, RemediateMultipleFiles)
{
    // Create files with various wrong mask
    CreateLogFile("auth.log", "bin", "bin", 0777);
    CreateLogFile("secure", "bin", "bin", 0666);
    CreateLogFile("syslog", "bin", "bin", 0644);
    CreateLogFile("test.log", "bin", "bin", 0755);

    std::map<std::string, std::string> args;
    args["path"] = testDir;

    auto result = RemediateEnsureLogfileAccess(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);

    // Verify all files now have appropriate mask
    std::vector<std::string> files = {"auth.log", "secure", "syslog", "test.log"};
    for (const auto& file : files)
    {
        std::string filePath = testDir + "/" + file;
        struct stat statInfo;
        ASSERT_EQ(stat(filePath.c_str(), &statInfo), 0);
        EXPECT_EQ(statInfo.st_mode & 0777, 0640);
    }
}

// Test remediation preserves files that should be ignored
TEST_F(EnsureLogfileAccessTest, RemediateIgnoresSpecialFiles)
{
    CreateSubdir("subdir");
    CreateSymlink("link.log", "/tmp/target");
    CreateLogFile("regular.log", "bin", "bin", 0777); // This should be fixed

    std::map<std::string, std::string> args;
    args["path"] = testDir;

    auto result = RemediateEnsureLogfileAccess(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);

    // Verify regular.log was fixed to valid owner/group combination
    std::string filePath = testDir + "/regular.log";
    struct stat statInfo;
    ASSERT_EQ(stat(filePath.c_str(), &statInfo), 0);
    // Owner should be root(0) or syslog(101)
    EXPECT_TRUE(statInfo.st_uid == 0 || statInfo.st_uid == 101);
    // Group should be root(0) or adm(4)
    EXPECT_TRUE(statInfo.st_gid == 0 || statInfo.st_gid == 4);
    // Mask should be 640
    EXPECT_EQ(statInfo.st_mode & 0777, 0640);

    // Verify symlink and directory are unchanged
    std::string linkPath = testDir + "/link.log";
    std::string dirPath = testDir + "/subdir";
    struct stat linkStat, dirStat;
    ASSERT_EQ(lstat(linkPath.c_str(), &linkStat), 0);
    ASSERT_EQ(stat(dirPath.c_str(), &dirStat), 0);
    EXPECT_TRUE(S_ISLNK(linkStat.st_mode));
    EXPECT_TRUE(S_ISDIR(dirStat.st_mode));
}

// Test default path behavior
TEST_F(EnsureLogfileAccessTest, DefaultPath)
{
    std::map<std::string, std::string> args; // No path specified, should use /var/log

    // Since we can't create files in /var/log in tests, this should just not crash
    auto auditResult = AuditEnsureLogfileAccess(args, indicators, mContext);
    ASSERT_TRUE(auditResult.HasValue()); // Should handle missing /var/log gracefully

    auto remediateResult = RemediateEnsureLogfileAccess(args, indicators, mContext);
    ASSERT_TRUE(remediateResult.HasValue()); // Should handle missing /var/log gracefully
}

// Test specific pattern edge cases
TEST_F(EnsureLogfileAccessTest, SpecificPatternEdgeCases)
{
    // Test case sensitivity (fnmatch with FNM_CASEFOLD should be case-insensitive)
    CreateLogFile("AUTH.LOG", "syslog", "adm", 0640);
    CreateLogFile("SECURE", "syslog", "adm", 0640);
    CreateLogFile("TEST.LOG", "syslog", "adm", 0640);

    std::map<std::string, std::string> args;
    args["path"] = testDir;

    auto result = AuditEnsureLogfileAccess(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

// Test wtmp/btmp special mask
TEST_F(EnsureLogfileAccessTest, SpecialSystemLogFiles)
{
    CreateLogFile("wtmp", "root", "utmp", 0664);    // root:utmp:664
    CreateLogFile("btmp", "root", "utmp", 0664);    // root:utmp:664
    CreateLogFile("lastlog", "root", "utmp", 0664); // root:root|utmp:664
    CreateLogFile("faillog", "root", "adm", 0640);  // root:adm:640 (uses default pattern)

    std::map<std::string, std::string> args;
    args["path"] = testDir;

    auto result = AuditEnsureLogfileAccess(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
