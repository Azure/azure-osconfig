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

    void prepareMinimalSshdConfig() const
    {
        std::ofstream ofs(sshd_config);
        ofs << "Port 22\n";
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

class SshExecutableTest : public SshTest
{
protected:
    SshExecutableTest()
    {
        systemctl = tmpdir + "/systemctl";
        sshd = tmpdir + "/sshd";
        std::string path = getenv("PATH");
        std::string newpath = tmpdir + ":" + path;
        setenv("PATH", newpath.c_str(), 1);
        ::close(::creat(systemctl.c_str(), 0755));
        ::close(::creat(sshd.c_str(), 0755));
    }

    void storeSshd(const char *s) const
    {
        std::ofstream ofs(sshd);
        ofs << "cat <<EOF\n";
        ofs << s;
        ofs << "EOF\n";
    }

    void prepareUbuntu1804() const
    {
        const char* s = "unknown option -- V\r\n"
                        "OpenSSH_7.6p1 Ubuntu-4ubuntu0.7, OpenSSL 1.0.2n  7 Dec 2017\n"
                        "usage: sshd [-46DdeiqTt] [-C connection_spec] [-c host_cert_file]\n"
                        "            [-E log_file] [-f config_file] [-g login_grace_time]\n"
                        "[-h host_key_file][-o option][-p port][-u len]\n ";
        storeSshd(s);
    }

    void prepareUbuntu2004() const
    {
        const char* s =
            "unknown option -- V\r\n"
            "OpenSSH_8.2p1 Ubuntu-4ubuntu0.11, OpenSSL 1.1.1f  31 Mar 2020\n"
            "usage: sshd [-46DdeiqTt] [-C connection_spec] [-c host_cert_file]\n"
            "            [-E log_file] [-f config_file] [-g login_grace_time]\n"
            "            [-h host_key_file] [-o option] [-p port] [-u len]\n";
        storeSshd(s);
    }

    void prepareUbuntu2204() const
    {
        const char* s =
            "unknown option -- V\r\n"
            "OpenSSH_8.9p1 Ubuntu-3ubuntu0.10, OpenSSL 3.0.2 15 Mar 2022\n"
            "usage: sshd [-46DdeiqTt] [-C connection_spec] [-c host_cert_file]\n"
            "            [-E log_file] [-f config_file] [-g login_grace_time]\n"
            "            [-h host_key_file] [-o option] [-p port] [-u len]\n";
        storeSshd(s);
    }

    void prepareUbuntu2404() const
    {
        const char* s = "OpenSSH_9.6p1 Ubuntu-3ubuntu13.5, OpenSSL 3.0.13 30 Jan 2024\n";
        storeSshd(s);
    }

    void prepareDebian12() const
    {
        const char* s = "OpenSSH_9.2, OpenSSL 3.0.15 3 Sep 2024\n";
        storeSshd(s);
    }

    void prepareCustomSsh91() const
    {
        const char* s = "OpenSSH_9.1, OpenSSL 3.0.15 3 Sep 2024\n";
        storeSshd(s);
    }

    ~SshExecutableTest()
    {
        ::remove(systemctl.c_str());
        ::remove(sshd.c_str());
    }

    std::string systemctl;
    std::string sshd;
};

TEST_F(SshExecutableTest, CheckSshVersionUbuntu18)
{
    int major = 0;
    int minor = 0;
    prepareUbuntu1804();
    int result = GetSshdVersionTest(&major, &minor);
    EXPECT_EQ(0, result);
    EXPECT_EQ(7, major);
    EXPECT_EQ(6, minor);
    EXPECT_NE(0, IsSshConfigIncludeSupportedTest());
}

TEST_F(SshExecutableTest, CheckSshVersionUbuntu20)
{
    int major = 0;
    int minor = 0;
    prepareUbuntu2004();
    int result = GetSshdVersionTest(&major, &minor);
    EXPECT_EQ(0, result);
    EXPECT_EQ(8, major);
    EXPECT_EQ(2, minor);
    EXPECT_EQ(0, IsSshConfigIncludeSupportedTest());
}

TEST_F(SshExecutableTest, CheckSshVersionUbuntu22)
{
    int major = 0;
    int minor = 0;
    prepareUbuntu2204();
    int result = GetSshdVersionTest(&major, &minor);
    EXPECT_EQ(0, result);
    EXPECT_EQ(8, major);
    EXPECT_EQ(9, minor);
    EXPECT_EQ(0, IsSshConfigIncludeSupportedTest());
}

TEST_F(SshExecutableTest, CheckSshVersionUbuntu24)
{
    int major = 0;
    int minor = 0;
    prepareUbuntu2404();
    int result = GetSshdVersionTest(&major, &minor);
    EXPECT_EQ(0, result);
    EXPECT_EQ(9, major);
    EXPECT_EQ(6, minor);
    EXPECT_EQ(0, IsSshConfigIncludeSupportedTest());
}

TEST_F(SshExecutableTest, CheckSshVersionDebian12)
{
    int major = 0;
    int minor = 0;
    prepareDebian12();
    int result = GetSshdVersionTest(&major, &minor);
    EXPECT_EQ(0, result);
    EXPECT_EQ(9, major);
    EXPECT_EQ(2, minor);
    EXPECT_EQ(0, IsSshConfigIncludeSupportedTest());
}

TEST_F(SshExecutableTest, CheckSshVersionCustom91)
{
    int major = 0;
    int minor = 0;
    prepareCustomSsh91();
    int result = GetSshdVersionTest(&major, &minor);
    EXPECT_EQ(0, result);
    EXPECT_EQ(9, major);
    EXPECT_EQ(1, minor);
    EXPECT_EQ(1, IsSshConfigIncludeSupportedTest());
}

TEST_F(SshExecutableTest, SaveRemediation)
{
    prepareMinimalSshdConfig();
    int result = SaveRemediationToSshdConfigTest();
    EXPECT_EQ(0, result);

    std::string config = getFileContents(sshd_config);
    int nulls = 0;
    for (char c : config)
    {
        if (c == '\0')
        {
            nulls++;
        }
    }
    EXPECT_EQ(0, nulls) << "Null bytes found in the config file:\n" << config;
}