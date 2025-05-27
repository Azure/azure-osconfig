// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "Evaluator.h"
#include "MockContext.h"
#include "ProcedureMap.h"

#include <algorithm>
#include <dirent.h>
#include <fstream>
#include <gtest/gtest.h>
#include <linux/limits.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <vector>

using ComplianceEngine::AuditEnsureSysctl;
using ComplianceEngine::CompactListFormatter;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Result;
using ComplianceEngine::Status;

bool startsWith(const std::string& str, const std::string& prefix)
{
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

static const char systemdSysctlCat[] = "/lib/systemd/systemd-sysctl --cat-config";
static const char sysctlIpForward0[] = "net.ipv4.ip_forward = 0";
static const char sysctlIpForward1[] = "net.ipv4.ip_forward = 1";
static const char sysctlIpForward0Comment[] = "                          # net.ipv4.ip_forward = 0";

struct SysctlNameValue
{
    std::string name;
    std::string value;
    SysctlNameValue(std::string a_name, std::string a_value)
        : name(a_name),
          value(a_value)
    {
    }
    std::string CfgOutput() const
    {
        auto fname = name;
        std::replace(fname.begin(), fname.end(), '.', '/');
        fname = std::string("# /etc/") + fname + ".conf\n";
        auto nameEqVal = name + " = " + value + "\n";
        return fname + nameEqVal;
    }
};

// sysctl names that should be matchec correctly using regexp in ensureSysctl
static const std::vector<SysctlNameValue> unsuportedSysctlTests = {
    // This is sysctl with multiline output as stated in SysctlNameValue.value string, currenlty due to regex limitation we can handle it
    SysctlNameValue(std::string("fs.binfmt_misc.python3/10"),
        std::string("enabled\ninterpreter /usr/bin/python3.10\nflags:\noffset 0\nmagic 6f0d0d0a\n")),
};

static const std::vector<SysctlNameValue> cisSsysctlNames = {
    SysctlNameValue(std::string("net.ipv4.conf.all.accept_redirects"), std::string("0")),
    SysctlNameValue(std::string("net.ipv4.conf.all.accept_source_route"), std::string("0")),
    SysctlNameValue(std::string("net.ipv4.conf.all.log_martians"), std::string("1")),
    SysctlNameValue(std::string("net.ipv4.conf.all.rp_filter"), std::string("1")),
    SysctlNameValue(std::string("net.ipv4.conf.all.secure_redirects"), std::string("0")),
    SysctlNameValue(std::string("net.ipv4.conf.all.send_redirects"), std::string("0")),
    SysctlNameValue(std::string("net.ipv4.conf.default.accept_redirects"), std::string("0")),
    SysctlNameValue(std::string("net.ipv4.conf.default.accept_source_route"), std::string("0")),
    SysctlNameValue(std::string("net.ipv4.conf.default.log_martians"), std::string("1")),
    SysctlNameValue(std::string("net.ipv4.conf.default.rp_filter"), std::string("1")),
    SysctlNameValue(std::string("net.ipv4.conf.default.secure_redirects"), std::string("0")),
    SysctlNameValue(std::string("net.ipv4.conf.default.send_redirects"), std::string("0")),
    SysctlNameValue(std::string("net.ipv4.icmp_echo_ignore_broadcasts"), std::string("1")),
    SysctlNameValue(std::string("net.ipv4.icmp_ignore_bogus_error_responses"), std::string("1")),
    SysctlNameValue(std::string("net.ipv4.ip_forward"), std::string("0")),
    SysctlNameValue(std::string("net.ipv4.tcp_syncookies"), std::string("1")),
    SysctlNameValue(std::string("net.ipv6.conf.all.accept_ra"), std::string("0")),
    SysctlNameValue(std::string("net.ipv6.conf.all.accept_redirects"), std::string("0")),
    SysctlNameValue(std::string("net.ipv6.conf.all.accept_source_route"), std::string("0")),
    SysctlNameValue(std::string("net.ipv6.conf.all.forwarding"), std::string("0")),
    SysctlNameValue(std::string("net.ipv6.conf.default.accept_ra"), std::string("0")),
    SysctlNameValue(std::string("net.ipv6.conf.default.accept_redirects"), std::string("0")),
    SysctlNameValue(std::string("net.ipv6.conf.default.accept_source_route"), std::string("0")),
};

static const char sysctlIpForward1Then0Than1Than0[] =
    "net.ipv4.ip_forward = 1\n"
    "net.ipv4.ip_forward = 0\n"
    "net.ipv4.ip_forward = 1\n"
    "net.ipv4.ip_forward = 0";

static const char emptyOutput[] = "";

static const char sysctlIpForward0FilenameExtraSpaces[] =
    ""
    "# /etc/sysctl.d/foo.conf\n"
    "     net.ipv4.ip_forward    =          0     \n"
    "     \n";

static const char sysctlIpForward0FilenameTabs[] =
    ""
    "# /etc/sysctl.d/foo.conf\n"
    " \t net.ipv4.ip_forward    =\t0\t     \n"
    "     \n";

static const char sysctlIpForward1Then0Than1Than0WithFilenames[] =
    "# /etc/sysctl.d/fwd_1.conf\n"
    "   net.ipv4.ip_forward = 1\n"
    "# /etc/sysctl.d/fwd_0.conf\n"
    "   net.ipv4.ip_forward = 0\n"
    "# /etc/sysctl.d/fwd_1_v2.conf\n"
    "   net.ipv4.ip_forward = 1\n"
    "# /etc/sysctl.d/fwd_0_v2.conf\n"
    "   net.ipv4.ip_forward = 0\n";

class EnsureSysctlTest : public ::testing::Test
{
    struct LengthComparator
    {
        bool operator()(const std::string& lhs, const std::string& rhs) const
        {
            if (lhs.size() == rhs.size())
            { // If lengths are equal, sort lexicographically
                return lhs < rhs;
            }
            return lhs.size() > rhs.size(); // Sort by length, longest first
        }
    };

protected:
    char dirTemplate[PATH_MAX] = "/tmp/ensureSysctlTest.XXXXXX";
    std::string dir;
    std::set<std::string, LengthComparator> sysctlDirs;
    std::set<std::string, LengthComparator> sysctlFiles;
    MockContext mContext;
    IndicatorsTree mIndicators;
    CompactListFormatter mFormatter;

    void SetUp() override
    {
        dir = mkdtemp(dirTemplate);
        ASSERT_TRUE(dir != "");
        mIndicators.Push("EnsureSysctl");
    }

    // sysctlName: is sysctl name in dot notation e.g net.net/ipv4/ip_forwardipv4.ip_forward
    void CreateSysctlFile(std::string sysctlName, std::string data)
    {
        int ret;
        std::replace(sysctlName.begin(), sysctlName.end(), '.', '/');
        std::string path = dir + std::string("/") + sysctlName;

        size_t pos = 0;
        while ((pos = path.find('/', pos)) != std::string::npos)
        {
            auto pathPart = path.substr(0, pos);
            pos++;
            if (startsWith(dir, pathPart))
            {
                continue;
            }
            ret = ::mkdir(pathPart.c_str(), 0777);
            ASSERT_TRUE((ret == 0) || (ret != 0 && (errno == EEXIST)))
                << "ERROR creating directory " << pathPart << " errno " << errno << ": " << strerror(errno);
            sysctlDirs.insert(pathPart);
        }

        std::ofstream sysctlFile(path);
        sysctlFile << data;
        sysctlFile.close();
        sysctlFiles.insert(path);
    }

    void TearDown() override
    {
        for (auto& file : sysctlFiles)
        {
            unlink(file.c_str());
        }
        for (auto& dir : sysctlDirs)
        {
            rmdir(dir.c_str());
        }
        rmdir(dir.c_str());
    }
};

TEST_F(EnsureSysctlTest, HappyPathSysctlValueEqualConfiruationNoOverride)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    CreateSysctlFile(sysctlName, sysctlName + " = 0\n");
    AddMockCommand(systemdSysctlCat, true, sysctlIpForward0, 0);
    std::map<std::string, std::string> args;
    args["sysctlName"] = sysctlName;
    args["value"] = "0";
    args["test_procfs"] = dir;

    auto result = AuditEnsureSysctl(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSysctlTest, UnHappyPathSysctlValueConfigurationEqualEmptyOuput)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    CreateSysctlFile(sysctlName, sysctlName + " = 0\n");
    AddMockCommand(systemdSysctlCat, true, emptyOutput, 0);
    std::map<std::string, std::string> args;
    args["sysctlName"] = sysctlName;
    args["value"] = "0";
    args["test_procfs"] = dir;

    auto result = AuditEnsureSysctl(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSysctlTest, HappyPathSysctlValueEqualConfiruationOverrideLastOneWins)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    CreateSysctlFile(sysctlName, sysctlName + " = 0\n");
    AddMockCommand(systemdSysctlCat, true, sysctlIpForward1Then0Than1Than0, 0);
    std::map<std::string, std::string> args;
    args["sysctlName"] = sysctlName;
    args["value"] = "0";
    args["test_procfs"] = dir;

    auto result = AuditEnsureSysctl(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSysctlTest, UnHappyPathSysctlValueEqualConfiruationComment)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    CreateSysctlFile(sysctlName, sysctlName + " = 0\n");
    AddMockCommand(systemdSysctlCat, true, sysctlIpForward0Comment, 0);
    std::map<std::string, std::string> args;
    args["sysctlName"] = sysctlName;
    args["value"] = "0";
    args["test_procfs"] = dir;

    auto result = AuditEnsureSysctl(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSysctlTest, UnHappyPathSysctlValueNotEqual)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    CreateSysctlFile(sysctlName, sysctlName + " = 1\n");
    AddMockCommand(systemdSysctlCat, true, sysctlIpForward1, 0);
    std::map<std::string, std::string> args;
    args["sysctlName"] = sysctlName;
    args["value"] = "0";
    args["test_procfs"] = dir;

    auto result = AuditEnsureSysctl(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSysctlTest, UnHappyPathSysctlValueEqualConfiruationOverride)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    CreateSysctlFile(sysctlName, sysctlName + " = 0\n");
    AddMockCommand(systemdSysctlCat, true, sysctlIpForward1, 0);
    std::map<std::string, std::string> args;
    args["sysctlName"] = sysctlName;
    args["value"] = "0";
    args["test_procfs"] = dir;

    auto result = AuditEnsureSysctl(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

// Regexp value tests
TEST_F(EnsureSysctlTest, HappyPathSysctlValueRegexpDotEqualConfiruationNoOverride)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    CreateSysctlFile(sysctlName, sysctlName + " = 0\n");
    AddMockCommand(systemdSysctlCat, true, sysctlIpForward0, 0);
    std::map<std::string, std::string> args;
    args["sysctlName"] = sysctlName;
    args["value"] = ".";
    args["test_procfs"] = dir;

    auto result = AuditEnsureSysctl(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSysctlTest, HappyPathSysctlValueRegexpRangeEqualConfiruationNoOverride)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    CreateSysctlFile(sysctlName, sysctlName + " = 0\n");
    AddMockCommand(systemdSysctlCat, true, sysctlIpForward0, 0);
    std::map<std::string, std::string> args;
    args["sysctlName"] = sysctlName;
    args["value"] = "[0]";
    args["test_procfs"] = dir;

    auto result = AuditEnsureSysctl(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSysctlTest, UnHappyPathSysctlValueRegexpRangeEqualConfiruationNoOverride)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    CreateSysctlFile(sysctlName, sysctlName + " = 0\n");
    AddMockCommand(systemdSysctlCat, true, sysctlIpForward1, 0);
    std::map<std::string, std::string> args;
    args["sysctlName"] = sysctlName;
    args["value"] = "[0]";
    args["test_procfs"] = dir;

    auto result = AuditEnsureSysctl(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSysctlTest, UnHappyPathSysctlValueRegexpRangeNotEqual)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    CreateSysctlFile(sysctlName, sysctlName + " = 1\n");
    AddMockCommand(systemdSysctlCat, true, sysctlIpForward0, 0);
    std::map<std::string, std::string> args;
    args["sysctlName"] = sysctlName;
    args["value"] = "[0]";
    args["test_procfs"] = dir;

    auto result = AuditEnsureSysctl(args, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}
// Invalid Args tests
TEST_F(EnsureSysctlTest, UnHappyPathRegexError)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    CreateSysctlFile(sysctlName, sysctlName + " = 1\n");
    AddMockCommand(systemdSysctlCat, true, sysctlIpForward1, 0);
    std::map<std::string, std::string> args;
    args["sysctlName"] = sysctlName;
    args["value"] = "(?)[1]";
    args["test_procfs"] = dir;

    auto result = AuditEnsureSysctl(args, mIndicators, mContext);
    ASSERT_FALSE(result.HasValue());
    auto should_contain = result.Error().message.find(std::string("Failed to compile regex '" + args["value"] + "' error:"));
    ASSERT_TRUE(should_contain != std::string::npos);
}

TEST_F(EnsureSysctlTest, UnHappyPathSysctlValueEqualConfiruationNotEqualTabs)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    CreateSysctlFile(sysctlName, sysctlName + " = 1\n");
    AddMockCommand(systemdSysctlCat, true, sysctlIpForward0FilenameTabs, 0);
    std::map<std::string, std::string> args;
    args["sysctlName"] = sysctlName;
    args["value"] = "1";
    args["test_procfs"] = dir;

    auto result = AuditEnsureSysctl(args, mIndicators, mContext);
    ASSERT_EQ(mFormatter.Format(mIndicators).Value(),
        std::string("[NonCompliant] Expected 'net.ipv4.ip_forward' value: '1' got '0' found in: '/etc/sysctl.d/foo.conf'\n"));
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSysctlTest, UnHappyPathSysctlValueEqualConfiruationNotEqualExtraSpacesFilenameReportCheck)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    CreateSysctlFile(sysctlName, sysctlName + " = 1\n");
    AddMockCommand(systemdSysctlCat, true, sysctlIpForward0FilenameExtraSpaces, 0);
    std::map<std::string, std::string> args;
    args["sysctlName"] = sysctlName;
    args["value"] = "1";
    args["test_procfs"] = dir;

    auto result = AuditEnsureSysctl(args, mIndicators, mContext);
    ASSERT_EQ(mFormatter.Format(mIndicators).Value(),
        std::string("[NonCompliant] Expected 'net.ipv4.ip_forward' value: '1' got '0' found in: '/etc/sysctl.d/foo.conf'\n"));
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSysctlTest, HappyPathSysctlValueEqualConfiruationOverrideLastOneWinsWithFilename)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    CreateSysctlFile(sysctlName, sysctlName + " = 1\n");
    AddMockCommand(systemdSysctlCat, true, sysctlIpForward1Then0Than1Than0WithFilenames, 0);
    std::map<std::string, std::string> args;
    args["sysctlName"] = sysctlName;
    args["value"] = "1";
    args["test_procfs"] = dir;

    auto result = AuditEnsureSysctl(args, mIndicators, mContext);

    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_EQ(mFormatter.Format(mIndicators).Value(),
        std::string("[NonCompliant] Expected 'net.ipv4.ip_forward' value: '1' got '0' found in: '/etc/sysctl.d/fwd_0_v2.conf'\n"));
}

TEST_F(EnsureSysctlTest, HappyPathValidateCisSysctls)
{
    for (size_t i = 0; i < cisSsysctlNames.size(); i++)
    {
        auto sysctlName = cisSsysctlNames[i].name;
        auto value = cisSsysctlNames[i].value;
        auto cfgOuput = cisSsysctlNames[i].CfgOutput();

        CreateSysctlFile(sysctlName, sysctlName + " = " + value + "\n");
        AddMockCommand(systemdSysctlCat, true, cfgOuput.c_str(), 0);
        std::map<std::string, std::string> args;
        args["sysctlName"] = sysctlName;
        args["value"] = value;
        args["test_procfs"] = dir;

        auto result = AuditEnsureSysctl(args, mIndicators, mContext);

        ASSERT_TRUE(result.HasValue()) << "HappyPathValidateSysctlNameAndValues FAILED: nr " << i << " name '" << sysctlName << "'";
        ASSERT_EQ(result.Value(), Status::Compliant) << "HappyPathValidateSysctlNameAndValues FAILED: nr " << i << " name '" << sysctlName << "'";
        ;
    }
}

TEST_F(EnsureSysctlTest, UnhappyPathSysctlMultilineOutput)
{
    SysctlNameValue sysctlNameValue(std::string("fs.binfmt_misc.python3/10"),
        std::string("enabled\ninterpreter /usr/bin/python3.10\nflags:\noffset 0\nmagic 6f0d0d0a\n"));

    auto sysctlName = sysctlNameValue.name;
    auto value = sysctlNameValue.value;
    auto cfgOuput = sysctlNameValue.CfgOutput();

    CreateSysctlFile(sysctlName, sysctlName + " = " + value + "\n");
    AddMockCommand(systemdSysctlCat, true, cfgOuput.c_str(), 0);
    std::map<std::string, std::string> args;
    args["sysctlName"] = sysctlName;
    args["value"] = value;
    args["test_procfs"] = dir;

    auto result = AuditEnsureSysctl(args, mIndicators, mContext);

    ASSERT_TRUE(result.HasValue()) << "HappyPathValidateSysctlNameAndValues FAILED: nr "
                                   << " name '" << sysctlName << "'";
    ASSERT_EQ(result.Value(), Status::NonCompliant) << "HappyPathValidateSysctlNameAndValues FAILED: nr "
                                                    << " name '" << sysctlName << "'";
    ;
}
