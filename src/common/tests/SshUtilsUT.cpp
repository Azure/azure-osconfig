// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <fcntl.h>
#include <fstream>
#include <system_error>
#include <gtest/gtest.h>
#include "Helper.h"

class SshTest : public testing::Test
{
protected:
    SshTest()
    {
        char tmpl[] = "/tmp/sshXXXXXX";
        char* dir = ::mkdtemp(tmpl);
        if (dir == NULL)
        {
            throw std::system_error(errno, std::generic_category(), "mkdtemp failed");
        }
        tmpdir = dir;
        sshdir = tmpdir + "/ssh";
        ::mkdir(sshdir.c_str(), 0755);
        sshd_config = sshdir + "/sshd_config";
        sshd_config_backup = sshdir + "/sshd_config.bak";
        sshd_config_remediation = sshdir + "/osconfig_remediation.conf";
        g_sshServerConfiguration_original = sshd_config.c_str();
        g_sshServerConfigurationBackup_original = sshd_config_backup.c_str();
        g_osconfigRemediationConf_original = sshd_config_remediation.c_str();
        SwapGlobalSshServerConfigs(&g_sshServerConfiguration_original, &g_sshServerConfigurationBackup_original, &g_osconfigRemediationConf_original);
    }

    ~SshTest()
    {
        SwapGlobalSshServerConfigs(&g_sshServerConfiguration_original, &g_sshServerConfigurationBackup_original, &g_osconfigRemediationConf_original);
        ClearDirs();
        ::rmdir(tmpdir.c_str());
    }

    void ClearDirs() const
    {
        ::remove(sshd_config.c_str());
        ::remove(sshd_config_backup.c_str());
        ::remove(sshd_config_remediation.c_str());
        ::rmdir(sshdir.c_str());
    }

    std::string tmpdir;
    std::string sshdir;
    std::string sshd_config;
    std::string sshd_config_backup;
    std::string sshd_config_remediation;
    const char* g_sshServerConfiguration_original;
    const char* g_sshServerConfigurationBackup_original;
    const char* g_osconfigRemediationConf_original;
};

static std::string getFileContents(const std::string& filename)
{
    std::ifstream file(filename);
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

TEST_F(SshTest, BackupSshdSuccess)
{
    std::string success = "success\n";
    int result = BackupSshdConfigTest(success.c_str());

    EXPECT_EQ(0, result);
    EXPECT_EQ(success, getFileContents(sshd_config_backup));
}

TEST_F(SshTest, BackupSshdFail)
{
    // Force an error by removing an intermediate directory
    ClearDirs();
    int result = BackupSshdConfigTest("fail");

    EXPECT_NE(0, result);
}
