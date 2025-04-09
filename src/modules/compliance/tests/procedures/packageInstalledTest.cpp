// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <dirent.h>
#include <fstream>
#include <gtest/gtest.h>
#include <linux/limits.h>
#include <string>
#include <unistd.h>

using compliance::AuditPackageInstalled;
using compliance::Error;
using compliance::Result;

static const char rpmCommand[] = "rpm -qa --qf='%{NAME}\n'";
static const char rpmWithPackageOutput[] = "package1\npackage2\nsample-package\nmysql-server\npackage5\n";
static const char rpmWithoutPackageOutput[] = "package1\npackage2\nother-package\npackage5\n";

static const char dpkgCommand[] = "dpkg -l";
static const char dpkgWithPackageOutput[] =
    "Desired=Unknown/Install/Remove/Purge/Hold\n"
    "| Status=Not/Inst/Conf-files/Unpacked/halF-conf/Half-inst/trig-aWait/Trig-pend\n"
    "|/ Err?=(none)/Reinst-required (Status,Err: uppercase=bad)\n"
    "||/ Name                      Version                  Architecture Description\n"
    "+++-=========================-========================-============-===============================\n"
    "ii  package1                  1.2.3-4                  amd64        Package 1 description\n"
    "ii  package2                  2.0.0-1                  amd64        Package 2 description\n"
    "ii  sample-package            3.1.4-2                  amd64        Sample package description\n"
    "rc  removed-package           1.0.0-1                  amd64        Removed package\n"
    "ii  mysql-server              5.7.32-1                 amd64        MySQL server package\n";

static const char dpkgWithoutPackageOutput[] =
    "Desired=Unknown/Install/Remove/Purge/Hold\n"
    "| Status=Not/Inst/Conf-files/Unpacked/halF-conf/Half-inst/trig-aWait/Trig-pend\n"
    "|/ Err?=(none)/Reinst-required (Status,Err: uppercase=bad)\n"
    "||/ Name                      Version                  Architecture Description\n"
    "+++-=========================-========================-============-===============================\n"
    "ii  package1                  1.2.3-4                  amd64        Package 1 description\n"
    "ii  package2                  2.0.0-1                  amd64        Package 2 description\n"
    "rc  removed-package           1.0.0-1                  amd64        Removed package\n"
    "ii  mysql-server              5.7.32-1                 amd64        MySQL server package\n";

// Package manager detection commands
static const char dpkgDetectCommand[] = "dpkg -l dpkg";
static const char rpmDetectCommand[] = "rpm -qa rpm";
static const char dpkgDetectOutput[] =
    "Desired=Unknown/Install/Remove/Purge/Hold\n"
    "| Status=Not/Inst/Conf-files/Unpacked/halF-conf/Half-inst/trig-aWait/Trig-pend\n"
    "|/ Err?=(none)/Reinst-required (Status,Err: uppercase=bad)\n"
    "||/ Name                      Version                  Architecture Description\n"
    "+++-=========================-========================-============-===============================\n"
    "ii  dpkg                      1.19.7                   amd64        Debian package management system\n";
static const char rpmDetectOutput[] = "rpm-4.14.2.1-1.el8\n";

class PackageInstalledTest : public ::testing::Test
{
protected:
    char dirTemplate[PATH_MAX] = "/tmp/packageCacheTest.XXXXXX";
    std::string dir;
    std::string cacheFile;
    MockContext mContext;

    void SetUp() override
    {
        char* tempDir = mkdtemp(dirTemplate);
        ASSERT_NE(tempDir, nullptr);
        dir = tempDir;
        cacheFile = dir + "/packageCache";
    }

    void TearDown() override
    {
        CleanupMockCommands();
        rmdir(dir.c_str());
    }

    void CreateCacheFile(const std::string& packageManager, time_t timestamp, const std::vector<std::string>& packages)
    {
        std::ofstream cache(cacheFile);
        ASSERT_TRUE(cache.is_open());
        cache << "# PackageCache " << packageManager << "@" << timestamp << std::endl;
        for (const auto& pkg : packages)
        {
            cache << pkg << std::endl;
        }
        cache.close();
    }
};

TEST_F(PackageInstalledTest, DetectDpkgPackageManager)
{
    CleanupMockCommands();
    AddMockCommand(dpkgDetectCommand, true, dpkgDetectOutput, 0);
    AddMockCommand(rpmDetectCommand, true, NULL, 1);
    AddMockCommand(dpkgCommand, true, dpkgWithPackageOutput, 0);

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["test_cachePath"] = cacheFile;

    std::ostringstream logstream;
    Result<bool> result = AuditPackageInstalled(args, mContext);

    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(result.Value());
    ASSERT_TRUE(logstream.str().find("sample-package") != std::string::npos);
}

TEST_F(PackageInstalledTest, DetectRpmPackageManager)
{
    CleanupMockCommands();
    AddMockCommand(dpkgDetectCommand, true, NULL, 1);
    AddMockCommand(rpmDetectCommand, true, rpmDetectOutput, 0);
    AddMockCommand(rpmCommand, true, rpmWithPackageOutput, 0);

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["test_cachePath"] = cacheFile;

    std::ostringstream logstream;
    Result<bool> result = AuditPackageInstalled(args, mContext);

    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(result.Value());
    ASSERT_TRUE(logstream.str().find("sample-package") != std::string::npos);
}

TEST_F(PackageInstalledTest, NoPackageManagerDetected)
{
    CleanupMockCommands();
    AddMockCommand(dpkgDetectCommand, true, NULL, 1);
    AddMockCommand(rpmDetectCommand, true, NULL, 1);

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["test_cachePath"] = cacheFile;

    std::ostringstream logstream;
    Result<bool> result = AuditPackageInstalled(args, mContext);

    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "No package manager found");
}

TEST_F(PackageInstalledTest, SpecifiedPackageManagerOverridesDetection)
{
    CleanupMockCommands();
    AddMockCommand(dpkgDetectCommand, true, NULL, 1);
    AddMockCommand(rpmDetectCommand, true, NULL, 1);
    AddMockCommand(rpmCommand, true, rpmWithPackageOutput, 0);

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm";
    args["test_cachePath"] = cacheFile;

    std::ostringstream logstream;
    Result<bool> result = AuditPackageInstalled(args, mContext);

    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(result.Value());
}

TEST_F(PackageInstalledTest, NoPackageName)
{
    CleanupMockCommands();
    std::map<std::string, std::string> args;

    std::ostringstream logstream;
    Result<bool> result = AuditPackageInstalled(args, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "No package name provided");
}

TEST_F(PackageInstalledTest, UnsupportedPackageManager)
{
    CleanupMockCommands();
    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "apt";
    args["test_cachePath"] = cacheFile;

    std::ostringstream logstream;
    Result<bool> result = AuditPackageInstalled(args, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_TRUE(result.Error().message.find("Unsupported package manager") != std::string::npos);
}

TEST_F(PackageInstalledTest, RpmPackageExists)
{
    CleanupMockCommands();
    AddMockCommand(rpmCommand, true, rpmWithPackageOutput, 0);
    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm";
    args["test_cachePath"] = cacheFile;

    std::ostringstream logstream;
    Result<bool> result = AuditPackageInstalled(args, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(result.Value());
}

TEST_F(PackageInstalledTest, RpmPackageDoesNotExist)
{
    CleanupMockCommands();
    AddMockCommand(rpmCommand, true, rpmWithoutPackageOutput, 0);
    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm";
    args["test_cachePath"] = cacheFile;

    std::ostringstream logstream;
    Result<bool> result = AuditPackageInstalled(args, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_FALSE(result.Value());
}

TEST_F(PackageInstalledTest, DpkgPackageExists)
{
    CleanupMockCommands();
    AddMockCommand(dpkgCommand, true, dpkgWithPackageOutput, 0);
    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "dpkg";
    args["test_cachePath"] = cacheFile;

    std::ostringstream logstream;
    Result<bool> result = AuditPackageInstalled(args, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(result.Value());
}

TEST_F(PackageInstalledTest, DpkgPackageDoesNotExist)
{
    CleanupMockCommands();
    AddMockCommand(dpkgCommand, true, dpkgWithoutPackageOutput, 0);
    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "dpkg";
    args["test_cachePath"] = cacheFile;

    std::ostringstream logstream;
    Result<bool> result = AuditPackageInstalled(args, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_FALSE(result.Value());
}

TEST_F(PackageInstalledTest, RpmCommandFails)
{
    CleanupMockCommands();
    AddMockCommand(rpmCommand, true, NULL, -1);
    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm";
    args["test_cachePath"] = cacheFile;

    std::ostringstream logstream;
    Result<bool> result = AuditPackageInstalled(args, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_TRUE(result.Error().message.find("Failed to get installed packages") != std::string::npos);
}

TEST_F(PackageInstalledTest, DpkgCommandFails)
{
    CleanupMockCommands();
    AddMockCommand(dpkgCommand, true, NULL, -1);
    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "dpkg";
    args["test_cachePath"] = cacheFile;

    std::ostringstream logstream;
    Result<bool> result = AuditPackageInstalled(args, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_TRUE(result.Error().message.find("Failed to get installed packages") != std::string::npos);
}

TEST_F(PackageInstalledTest, UseCacheWhenAvailable)
{
    CleanupMockCommands();
    time_t now = time(nullptr);
    AddMockCommand(rpmCommand, true, NULL, -1); // it should never be called
    CreateCacheFile("rpm", now, {"package1", "package2", "sample-package", "mysql-server"});

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm";
    args["test_cachePath"] = cacheFile;

    std::ostringstream logstream;
    Result<bool> result = AuditPackageInstalled(args, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(result.Value());
}

TEST_F(PackageInstalledTest, UseStaleCache)
{
    CleanupMockCommands();
    time_t staleTime = time(nullptr) - 4000; // Over PACKAGELIST_TTL (3000)
    CreateCacheFile("rpm", staleTime, {"sample-package", "package1", "package2", "old-package", "mysql-server"});

    AddMockCommand(rpmCommand, true, NULL, -1);

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm";
    args["test_cachePath"] = cacheFile;

    std::ostringstream logstream;
    Result<bool> result = AuditPackageInstalled(args, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(result.Value());
}

TEST_F(PackageInstalledTest, RefreshStaleCache)
{
    CleanupMockCommands();
    time_t staleTime = time(nullptr) - 4000; // Over PACKAGELIST_TTL (3000)
    CreateCacheFile("rpm", staleTime, {"package1", "package2", "old-package", "mysql-server"});

    AddMockCommand(rpmCommand, true, rpmWithPackageOutput, 0);

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm";
    args["test_cachePath"] = cacheFile;

    std::ostringstream logstream;
    Result<bool> result = AuditPackageInstalled(args, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(result.Value());
}

TEST_F(PackageInstalledTest, PackageManagerMismatch)
{
    CleanupMockCommands();
    time_t now = time(nullptr);
    CreateCacheFile("dpkg", now, {"package1", "package2", "sample-package", "mysql-server"});

    AddMockCommand(rpmCommand, true, rpmWithPackageOutput, 0);

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm"; // Mismatch with cache
    args["test_cachePath"] = cacheFile;

    std::ostringstream logstream;
    Result<bool> result = AuditPackageInstalled(args, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(result.Value());
}

TEST_F(PackageInstalledTest, InvalidCacheFormat)
{
    CleanupMockCommands();
    std::ofstream cache(cacheFile);
    ASSERT_TRUE(cache.is_open());
    cache << "This is not a valid cache file format" << std::endl;
    cache.close();

    AddMockCommand(rpmCommand, true, rpmWithPackageOutput, 0);

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm";
    args["test_cachePath"] = cacheFile;

    std::ostringstream logstream;
    Result<bool> result = AuditPackageInstalled(args, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(result.Value());
}

TEST_F(PackageInstalledTest, CacheWithInvalidTimestamp)
{
    CleanupMockCommands();
    std::ofstream cache(cacheFile);
    ASSERT_TRUE(cache.is_open());
    cache << "# PackageCache rpm@notanumber" << std::endl;
    cache << "package1" << std::endl;
    cache << "sample-package" << std::endl;
    cache.close();

    AddMockCommand(rpmCommand, true, rpmWithPackageOutput, 0);

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm";
    args["test_cachePath"] = cacheFile;

    std::ostringstream logstream;
    Result<bool> result = AuditPackageInstalled(args, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(result.Value());
}

TEST_F(PackageInstalledTest, CacheTooStale)
{
    CleanupMockCommands();
    time_t veryStaleTime = time(nullptr) - 13000; // Over PACKAGELIST_STALE_TTL (12600)
    CreateCacheFile("rpm", veryStaleTime, {"package1", "package2", "sample-package", "mysql-server"});

    AddMockCommand(rpmCommand, true, rpmWithPackageOutput, 0);

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm";
    args["test_cachePath"] = cacheFile;

    std::ostringstream logstream;
    Result<bool> result = AuditPackageInstalled(args, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(result.Value());
}

TEST_F(PackageInstalledTest, CachePathBroken)
{
    CleanupMockCommands();
    AddMockCommand(dpkgCommand, true, dpkgWithPackageOutput, 0);
    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "dpkg";
    args["test_cachePath"] = "/invalid/path/to/cache"; // Invalid path

    std::ostringstream logstream;
    Result<bool> result = AuditPackageInstalled(args, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_TRUE(result.Value());
}
