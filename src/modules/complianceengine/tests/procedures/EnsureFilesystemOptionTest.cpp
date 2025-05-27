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

using ComplianceEngine::AuditEnsureFilesystemOption;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::RemediateEnsureFilesystemOption;
using ComplianceEngine::Result;
using ComplianceEngine::Status;

class EnsureFilesystemOptionTest : public ::testing::Test
{
protected:
    char dirTemplate[PATH_MAX] = "/tmp/fsoptionTest.XXXXXX";
    std::string dir;
    std::string fstabFile;
    std::string mtabFile;
    MockContext mContext;
    IndicatorsTree indicators;

    void SetUp() override
    {
        dir = mkdtemp(dirTemplate);
        ASSERT_TRUE(dir != "");
        fstabFile = dir + "/fstab";
        mtabFile = dir + "/mtab";
        indicators.Push("EnsureFilesystemOption");
    }
    void CreateTabs()
    {
        std::ofstream fstab(fstabFile);
        fstab << "# Leave the comment alone!\n";
        fstab << "/dev/sda1 / ext4 rw,nodev,noatime 0 1\n";
        fstab << "/dev/sda2 /home ext4 rw,relatime,data=ordered 0 2\n";
        fstab.close();

        std::ofstream mtab(mtabFile);
        mtab << "/dev/sda1 / ext4 rw,nodev,noatime 0 0\n";
        mtab << "/dev/sda2 /home ext4 rw,relatime,data=ordered 0 0\n";
        mtab.close();
    }

    void TearDown() override
    {
        remove(fstabFile.c_str());
        remove(mtabFile.c_str());
    }
};

TEST_F(EnsureFilesystemOptionTest, AuditEnsureFilesystemOptionSuccess)
{
    CreateTabs();
    std::map<std::string, std::string> args;
    args["mountpoint"] = "/";
    args["test_fstab"] = fstabFile;
    args["test_mtab"] = mtabFile;
    args["optionsSet"] = "rw,noatime";
    args["optionsNotSet"] = "noreltime";

    auto result = AuditEnsureFilesystemOption(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFilesystemOptionTest, AuditEnsureFilesystemOptionMissing)
{
    CreateTabs();
    std::map<std::string, std::string> args;
    args["mountpoint"] = "/";
    args["test_fstab"] = fstabFile;
    args["test_mtab"] = mtabFile;
    args["optionsSet"] = "rw,noatime,noexec";
    args["optionsNotSet"] = "noreltime";

    auto result = AuditEnsureFilesystemOption(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFilesystemOptionTest, AuditEnsureFilesystemOptionForbidden)
{
    CreateTabs();
    std::map<std::string, std::string> args;
    args["mountpoint"] = "/";
    args["test_fstab"] = fstabFile;
    args["test_mtab"] = mtabFile;
    args["optionsSet"] = "rw";
    args["optionsNotSet"] = "nodev";

    auto result = AuditEnsureFilesystemOption(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFilesystemOptionTest, RemediateEnsureFilesystemOption)
{
    CreateTabs();
    std::map<std::string, std::string> args;
    args["mountpoint"] = "/home";
    args["test_fstab"] = fstabFile;
    args["test_mtab"] = mtabFile;
    args["optionsSet"] = "rw,noatime";
    args["optionsNotSet"] = "relatime";
    args["test_mount"] = "touch " + dir + " /remounted;/bin/true ";

    auto result = RemediateEnsureFilesystemOption(args, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);

    std::ifstream fstab(fstabFile);
    std::string fstabContents;
    std::string line;
    while (std::getline(fstab, line))
    {
        fstabContents += line + "\n";
    }
    ASSERT_EQ(fstabContents, "# Leave the comment alone!\n/dev/sda1 / ext4 rw,nodev,noatime 0 1\n/dev/sda2 /home ext4 rw,data=ordered,noatime 0 2\n");
    DIR* d = opendir(dir.c_str());
    ASSERT_NE(d, nullptr);
    struct dirent* de;
    std::string backupFilename;
    while ((de = readdir(d)) != NULL)
    {
        std::string filename = de->d_name;
        std::string prefix = "fstab.bak.";
        auto prefixLen = prefix.length();
        if (filename.substr(0, prefixLen) == prefix)
        {
            backupFilename = filename;
            break;
        }
    }
    closedir(d);
    ASSERT_NE(backupFilename, "");
}
