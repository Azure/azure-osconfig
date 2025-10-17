// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Evaluator.h"
#include "MockContext.h"

#include <EnsureFilesystemOption.h>
#include <dirent.h>
#include <fstream>
#include <gtest/gtest.h>
#include <linux/limits.h>
#include <string>
#include <unistd.h>

using ComplianceEngine::AuditEnsureFilesystemOption;
using ComplianceEngine::EnsureFilesystemOptionParams;
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
    EnsureFilesystemOptionParams params;
    params.mountpoint = "/";
    params.test_fstab = fstabFile;
    params.test_mtab = mtabFile;
    params.optionsSet = {{"rw", "noatime"}};
    params.optionsNotSet = {{"noreltime"}};

    auto result = AuditEnsureFilesystemOption(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureFilesystemOptionTest, AuditEnsureFilesystemOptionMissing)
{
    CreateTabs();
    EnsureFilesystemOptionParams params;
    params.mountpoint = "/";
    params.test_fstab = fstabFile;
    params.test_mtab = mtabFile;
    params.optionsSet = {{"rw", "noatime", "noexec"}};
    params.optionsNotSet = {{"noreltime"}};

    auto result = AuditEnsureFilesystemOption(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFilesystemOptionTest, AuditEnsureFilesystemOptionForbidden)
{
    CreateTabs();
    EnsureFilesystemOptionParams params;
    params.mountpoint = "/";
    params.test_fstab = fstabFile;
    params.test_mtab = mtabFile;
    params.optionsSet = {{"rw"}};
    params.optionsNotSet = {{"nodev"}};

    auto result = AuditEnsureFilesystemOption(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureFilesystemOptionTest, RemediateEnsureFilesystemOption)
{
    CreateTabs();
    EnsureFilesystemOptionParams params;
    params.mountpoint = "/home";
    params.test_fstab = fstabFile;
    params.test_mtab = mtabFile;
    params.optionsSet = {{"rw", "noatime"}};
    params.optionsNotSet = {{"relatime"}};
    params.test_mount = "touch " + dir + " /remounted;/bin/true ";

    auto result = RemediateEnsureFilesystemOption(params, indicators, mContext);
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
    struct dirent* de = nullptr;
    std::string backupFilename;
    while ((de = readdir(d)) != nullptr)
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
