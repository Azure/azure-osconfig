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
using compliance::CompactListFormatter;
using compliance::Error;
using compliance::IndicatorsTree;
using compliance::Result;
using compliance::Status;

using testing::HasSubstr;
using testing::Return;

static const std::string rpmCommand = "rpm -qa --qf='%{NAME}\n'";
static const std::string rpmWithPackageOutput = "package1\npackage2\nsample-package\nmysql-server\npackage5\n";
static const std::string rpmWithoutPackageOutput = "package1\npackage2\nother-package\npackage5\n";

static const std::string dpkgCommand = "dpkg -l";
static const std::string dpkgWithPackageOutput =
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

static const std::string dpkgWithoutPackageOutput =
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
static const std::string dpkgDetectCommand = "dpkg -l dpkg";
static const std::string rpmDetectCommand = "rpm -qa rpm";
static const std::string dpkgDetectOutput =
    "Desired=Unknown/Install/Remove/Purge/Hold\n"
    "| Status=Not/Inst/Conf-files/Unpacked/halF-conf/Half-inst/trig-aWait/Trig-pend\n"
    "|/ Err?=(none)/Reinst-required (Status,Err: uppercase=bad)\n"
    "||/ Name                      Version                  Architecture Description\n"
    "+++-=========================-========================-============-===============================\n"
    "ii  dpkg                      1.19.7                   amd64        Debian package management system\n";
static const std::string rpmDetectOutput = "rpm-4.14.2.1-1.el8\n";

class PackageInstalledTest : public ::testing::Test
{
protected:
    char dirTemplate[PATH_MAX] = "/tmp/packageCacheTest.XXXXXX";
    std::string dir;
    std::string cacheFile;
    MockContext mContext;
    CompactListFormatter mFormatter;
    IndicatorsTree mIndicators;

    void SetUp() override
    {
        char* tempDir = mkdtemp(dirTemplate);
        ASSERT_NE(tempDir, nullptr);
        dir = tempDir;
        cacheFile = dir + "/packageCache";
        mIndicators.Push("PackageInstalled");
    }

    void TearDown() override
    {
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
    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(dpkgDetectCommand))).WillRepeatedly(Return(Result<std::string>(dpkgDetectOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(rpmDetectCommand))).WillRepeatedly(Return(Result<std::string>(Error("Command failed", 1))));
    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(dpkgCommand))).WillRepeatedly(Return(Result<std::string>(dpkgWithPackageOutput)));

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["test_cachePath"] = cacheFile;

    auto result = AuditPackageInstalled(args, mIndicators, mContext);

    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("sample-package") != std::string::npos);
}

TEST_F(PackageInstalledTest, DetectRpmPackageManager)
{
    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(dpkgDetectCommand))).WillRepeatedly(Return(Result<std::string>(Error("Command failed", 1))));
    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(rpmDetectCommand))).WillRepeatedly(Return(Result<std::string>(rpmDetectOutput)));
    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(rpmCommand))).WillRepeatedly(Return(Result<std::string>(rpmWithPackageOutput)));

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["test_cachePath"] = cacheFile;

    auto result = AuditPackageInstalled(args, mIndicators, mContext);

    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("sample-package") != std::string::npos);
}

TEST_F(PackageInstalledTest, NoPackageManagerDetected)
{
    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(dpkgDetectCommand))).WillRepeatedly(Return(Result<std::string>(Error("Command failed", 1))));
    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(rpmDetectCommand))).WillRepeatedly(Return(Result<std::string>(Error("Command failed", 1))));

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["test_cachePath"] = cacheFile;

    auto result = AuditPackageInstalled(args, mIndicators, mContext);

    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "No package manager found");
}

TEST_F(PackageInstalledTest, SpecifiedPackageManagerOverridesDetection)
{
    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(dpkgDetectCommand))).WillRepeatedly(Return(Result<std::string>(Error("Command failed", 1))));
    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(rpmDetectCommand))).WillRepeatedly(Return(Result<std::string>(Error("Command failed", 1))));
    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(rpmCommand))).WillRepeatedly(Return(Result<std::string>(rpmWithPackageOutput)));

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm";
    args["test_cachePath"] = cacheFile;

    auto result = AuditPackageInstalled(args, mIndicators, mContext);

    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(PackageInstalledTest, NoPackageName)
{
    std::map<std::string, std::string> args;

    auto result = AuditPackageInstalled(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "No package name provided");
}

TEST_F(PackageInstalledTest, UnsupportedPackageManager)
{
    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "apt";
    args["test_cachePath"] = cacheFile;

    auto result = AuditPackageInstalled(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_TRUE(result.Error().message.find("Unsupported package manager") != std::string::npos);
}

TEST_F(PackageInstalledTest, RpmPackageExists)
{
    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(rpmCommand))).WillRepeatedly(Return(Result<std::string>(rpmWithPackageOutput)));
    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm";
    args["test_cachePath"] = cacheFile;

    auto result = AuditPackageInstalled(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(PackageInstalledTest, RpmPackageDoesNotExist)
{
    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(rpmCommand))).WillRepeatedly(Return(Result<std::string>(rpmWithoutPackageOutput)));
    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm";
    args["test_cachePath"] = cacheFile;

    auto result = AuditPackageInstalled(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(PackageInstalledTest, DpkgPackageExists)
{
    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(dpkgCommand))).WillRepeatedly(Return(Result<std::string>(dpkgWithPackageOutput)));
    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "dpkg";
    args["test_cachePath"] = cacheFile;

    auto result = AuditPackageInstalled(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(PackageInstalledTest, DpkgPackageDoesNotExist)
{
    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(dpkgCommand))).WillRepeatedly(Return(Result<std::string>(dpkgWithoutPackageOutput)));
    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "dpkg";
    args["test_cachePath"] = cacheFile;

    auto result = AuditPackageInstalled(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(PackageInstalledTest, RpmCommandFails)
{
    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(rpmCommand))).WillRepeatedly(Return(Result<std::string>(Error("Command failed", 1))));
    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm";
    args["test_cachePath"] = cacheFile;

    auto result = AuditPackageInstalled(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_TRUE(result.Error().message.find("Failed to get installed packages") != std::string::npos);
}

TEST_F(PackageInstalledTest, DpkgCommandFails)
{
    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(dpkgCommand))).WillRepeatedly(Return(Result<std::string>(Error("Command failed", 1))));
    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "dpkg";
    args["test_cachePath"] = cacheFile;

    auto result = AuditPackageInstalled(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_TRUE(result.Error().message.find("Failed to get installed packages") != std::string::npos);
}

TEST_F(PackageInstalledTest, UseCacheWhenAvailable)
{
    time_t now = time(nullptr);
    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(rpmCommand))).Times(0); // it should never be called
    CreateCacheFile("rpm", now, {"package1", "package2", "sample-package", "mysql-server"});

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm";
    args["test_cachePath"] = cacheFile;

    auto result = AuditPackageInstalled(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(PackageInstalledTest, UseStaleCache)
{
    time_t staleTime = time(nullptr) - 4000; // Over PACKAGELIST_TTL (3000)
    CreateCacheFile("rpm", staleTime, {"sample-package", "package1", "package2", "old-package", "mysql-server"});

    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(rpmCommand))).WillRepeatedly(Return(Result<std::string>(Error("Command failed", 1))));
    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm";
    args["test_cachePath"] = cacheFile;

    auto result = AuditPackageInstalled(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(PackageInstalledTest, RefreshStaleCache)
{
    time_t staleTime = time(nullptr) - 4000; // Over PACKAGELIST_TTL (3000)
    CreateCacheFile("rpm", staleTime, {"package1", "package2", "old-package", "mysql-server"});

    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(rpmCommand))).WillRepeatedly(Return(Result<std::string>(rpmWithPackageOutput)));

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm";
    args["test_cachePath"] = cacheFile;

    auto result = AuditPackageInstalled(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(PackageInstalledTest, PackageManagerMismatch)
{
    time_t now = time(nullptr);
    CreateCacheFile("dpkg", now, {"package1", "package2", "sample-package", "mysql-server"});

    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(rpmCommand))).WillRepeatedly(Return(Result<std::string>(rpmWithPackageOutput)));

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm"; // Mismatch with cache
    args["test_cachePath"] = cacheFile;

    auto result = AuditPackageInstalled(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(PackageInstalledTest, InvalidCacheFormat)
{
    std::ofstream cache(cacheFile);
    ASSERT_TRUE(cache.is_open());
    cache << "This is not a valid cache file format" << std::endl;
    cache.close();

    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(rpmCommand))).WillRepeatedly(Return(Result<std::string>(rpmWithPackageOutput)));

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm";
    args["test_cachePath"] = cacheFile;

    auto result = AuditPackageInstalled(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(PackageInstalledTest, CacheWithInvalidTimestamp)
{
    std::ofstream cache(cacheFile);
    ASSERT_TRUE(cache.is_open());
    cache << "# PackageCache rpm@notanumber" << std::endl;
    cache << "package1" << std::endl;
    cache << "sample-package" << std::endl;
    cache.close();

    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(rpmCommand))).WillRepeatedly(Return(Result<std::string>(rpmWithPackageOutput)));

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm";
    args["test_cachePath"] = cacheFile;

    auto result = AuditPackageInstalled(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(PackageInstalledTest, CacheTooStale)
{
    time_t veryStaleTime = time(nullptr) - 13000; // Over PACKAGELIST_STALE_TTL (12600)
    CreateCacheFile("rpm", veryStaleTime, {"package1", "package2", "sample-package", "mysql-server"});

    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(rpmCommand))).WillRepeatedly(Return(Result<std::string>(rpmWithPackageOutput)));

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "rpm";
    args["test_cachePath"] = cacheFile;

    auto result = AuditPackageInstalled(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(PackageInstalledTest, CachePathBroken)
{
    EXPECT_CALL(mContext, ExecuteCommand(HasSubstr(dpkgCommand))).WillRepeatedly(Return(Result<std::string>(dpkgWithPackageOutput)));

    std::map<std::string, std::string> args;
    args["packageName"] = "sample-package";
    args["packageManager"] = "dpkg";
    args["test_cachePath"] = "/invalid/path/to/cache"; // Invalid path

    auto result = AuditPackageInstalled(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
