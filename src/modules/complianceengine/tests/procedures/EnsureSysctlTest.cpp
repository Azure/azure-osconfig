// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "MockContext.h"

#include <EnsureSysctl.h>
#include <algorithm>
#include <dirent.h>
#include <fstream>
#include <gtest/gtest.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <vector>

using ComplianceEngine::AuditEnsureSysctl;
using ComplianceEngine::CompactListFormatter;
using ComplianceEngine::EnsureSysctlParams;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Pattern;
using ComplianceEngine::Result;
using ComplianceEngine::Status;
using ::testing::Return;

static const std::string systemdSysctlCat = "/lib/systemd/systemd-sysctl --cat-config";
static const std::string systemdSysctlVersion = "/lib/systemd/systemd-sysctl --version";
static const std::string systemdUsrSysctlCat = "/usr/lib/systemd/systemd-sysctl --cat-config";
static const std::string systemdUsrSysctlVersion = "/usr/lib/systemd/systemd-sysctl --version";
static const std::string sysctlIpForward0 = "net.ipv4.ip_forward = 0";
static const std::string sysctlIpForward1 = "net.ipv4.ip_forward = 1";
static const std::string sysctlIpForward0Comment = "                          # net.ipv4.ip_forward = 0";

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
    MockContext mContext;
    IndicatorsTree mIndicators;
    CompactListFormatter mFormatter;

    void SetUp() override
    {
        mIndicators.Push("EnsureSysctl");
    }
};

TEST_F(EnsureSysctlTest, HappyPathSysctlValueEqualConfigurationNoOverride)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    auto sysctlSlashedName = sysctlName;
    std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
    EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>("0\n")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlCat)).WillRepeatedly(Return(Result<std::string>(sysctlIpForward0)));
    EnsureSysctlParams params;
    params.sysctlName = sysctlName;
    params.value = Pattern::Make("0").Value();

    auto result = AuditEnsureSysctl(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSysctlTest, HappyPathAlternativeSystemdSysctlLocation)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    auto sysctlSlashedName = sysctlName;
    std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
    EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>("0\n")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Error("Missing")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdUsrSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdUsrSysctlCat)).WillRepeatedly(Return(Result<std::string>(sysctlIpForward0)));
    EnsureSysctlParams params;
    params.sysctlName = sysctlName;
    params.value = Pattern::Make("0").Value();

    auto result = AuditEnsureSysctl(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSysctlTest, UnHappyPathSysctlValueConfigurationEqualEmptyOuput)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    auto sysctlSlashedName = sysctlName;
    std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
    EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>("0\n")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlCat)).WillRepeatedly(Return(Result<std::string>(emptyOutput)));
    EXPECT_CALL(mContext, GetFileContents("/etc/default/ufw")).WillRepeatedly(Return(Result<std::string>(Error("No such file or directory", -1))));
    EnsureSysctlParams params;
    params.sysctlName = sysctlName;
    params.value = Pattern::Make("0").Value();

    auto result = AuditEnsureSysctl(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSysctlTest, HappyPathSysctlValueEqualConfigurationOverrideLastOneWins)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    auto sysctlSlashedName = sysctlName;
    std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
    EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>("0\n")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlCat)).WillRepeatedly(Return(Result<std::string>(sysctlIpForward1Then0Than1Than0)));
    EnsureSysctlParams params;
    params.sysctlName = sysctlName;
    params.value = Pattern::Make("0").Value();

    auto result = AuditEnsureSysctl(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSysctlTest, UnHappyPathSysctlValueEqualConfigurationComment)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    auto sysctlSlashedName = sysctlName;
    std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
    EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>("0\n")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlCat)).WillRepeatedly(Return(Result<std::string>(sysctlIpForward0Comment)));
    EXPECT_CALL(mContext, GetFileContents("/etc/default/ufw")).WillRepeatedly(Return(Result<std::string>(Error("No such file or directory", -1))));
    EnsureSysctlParams params;
    params.sysctlName = sysctlName;
    params.value = Pattern::Make("0").Value();

    auto result = AuditEnsureSysctl(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSysctlTest, UnHappyPathSysctlValueNotEqual)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    auto sysctlSlashedName = sysctlName;
    std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
    EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>("0\n")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlCat)).WillRepeatedly(Return(Result<std::string>(sysctlIpForward1)));
    EnsureSysctlParams params;
    params.sysctlName = sysctlName;
    params.value = Pattern::Make("0").Value();

    auto result = AuditEnsureSysctl(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSysctlTest, UnHappyPathSysctlValueEqualConfiruationOverride)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    auto sysctlSlashedName = sysctlName;
    std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
    EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>("0\n")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlCat)).WillRepeatedly(Return(Result<std::string>(sysctlIpForward1)));
    EnsureSysctlParams params;
    params.sysctlName = sysctlName;
    params.value = Pattern::Make("0").Value();

    auto result = AuditEnsureSysctl(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

// Regexp value tests
TEST_F(EnsureSysctlTest, HappyPathSysctlValueRegexpDotEqualConfiruationNoOverride)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    auto sysctlSlashedName = sysctlName;
    std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
    EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>("0\n")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlCat)).WillRepeatedly(Return(Result<std::string>(sysctlIpForward0)));
    EnsureSysctlParams params;
    params.sysctlName = sysctlName;
    params.value = Pattern::Make(".").Value();

    auto result = AuditEnsureSysctl(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSysctlTest, HappyPathSysctlValueRegexpRangeEqualConfiruationNoOverride)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    auto sysctlSlashedName = sysctlName;
    std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
    EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>("0\n")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlCat)).WillRepeatedly(Return(Result<std::string>(sysctlIpForward0)));
    EnsureSysctlParams params;
    params.sysctlName = sysctlName;
    params.value = Pattern::Make("[0]").Value();

    auto result = AuditEnsureSysctl(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureSysctlTest, UnHappyPathSysctlValueRegexpRangeEqualConfiruationNoOverride)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    auto sysctlSlashedName = sysctlName;
    std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
    EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>("0\n")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlCat)).WillRepeatedly(Return(Result<std::string>(sysctlIpForward1)));
    EnsureSysctlParams params;
    params.sysctlName = sysctlName;
    params.value = Pattern::Make("[0]").Value();

    auto result = AuditEnsureSysctl(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSysctlTest, UnHappyPathSysctlValueRegexpRangeNotEqual)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    auto sysctlSlashedName = sysctlName;
    std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
    EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>("1\n")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlCat)).WillRepeatedly(Return(Result<std::string>(sysctlIpForward0)));
    EnsureSysctlParams params;
    params.sysctlName = sysctlName;
    params.value = Pattern::Make("[0]").Value();

    auto result = AuditEnsureSysctl(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSysctlTest, UnHappyPathSysctlValueEqualConfiruationNotEqualTabs)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    auto sysctlSlashedName = sysctlName;
    std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
    EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>("1\n")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlCat)).WillRepeatedly(Return(Result<std::string>(sysctlIpForward0FilenameTabs)));
    EnsureSysctlParams params;
    params.sysctlName = sysctlName;
    params.value = Pattern::Make("1").Value();

    auto result = AuditEnsureSysctl(params, mIndicators, mContext);
    ASSERT_EQ(mFormatter.Format(mIndicators).Value(),
        std::string("[Compliant] Correct value for 'net.ipv4.ip_forward' in runtime configuration\n[NonCompliant] Expected 'net.ipv4.ip_forward' got "
                    "'0' found in: '/etc/sysctl.d/foo.conf'\n"));
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSysctlTest, UnHappyPathSysctlValueEqualConfiruationNotEqualExtraSpacesFilenameReportCheck)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    auto sysctlSlashedName = sysctlName;
    std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
    EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>("1\n")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlCat)).WillRepeatedly(Return(Result<std::string>(sysctlIpForward0FilenameExtraSpaces)));
    EnsureSysctlParams params;
    params.sysctlName = sysctlName;
    params.value = Pattern::Make("1").Value();

    auto result = AuditEnsureSysctl(params, mIndicators, mContext);
    ASSERT_EQ(mFormatter.Format(mIndicators).Value(),
        std::string("[Compliant] Correct value for 'net.ipv4.ip_forward' in runtime configuration\n[NonCompliant] Expected 'net.ipv4.ip_forward' got "
                    "'0' found in: '/etc/sysctl.d/foo.conf'\n"));
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureSysctlTest, HappyPathSysctlValueEqualConfiruationOverrideLastOneWinsWithFilename)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    auto sysctlSlashedName = sysctlName;
    std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
    EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>("1\n")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlCat)).WillRepeatedly(Return(Result<std::string>(sysctlIpForward1Then0Than1Than0WithFilenames)));
    EnsureSysctlParams params;
    params.sysctlName = sysctlName;
    params.value = Pattern::Make("1").Value();

    auto result = AuditEnsureSysctl(params, mIndicators, mContext);

    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_EQ(mFormatter.Format(mIndicators).Value(),
        std::string("[Compliant] Correct value for 'net.ipv4.ip_forward' in runtime configuration\n[NonCompliant] Expected 'net.ipv4.ip_forward' got "
                    "'0' found in: '/etc/sysctl.d/fwd_0_v2.conf'\n"));
}

TEST_F(EnsureSysctlTest, HappyPathValidateCisSysctls)
{
    for (size_t i = 0; i < cisSsysctlNames.size(); i++)
    {
        auto sysctlName = cisSsysctlNames[i].name;
        auto value = cisSsysctlNames[i].value;
        auto cfgOuput = cisSsysctlNames[i].CfgOutput();

        auto sysctlSlashedName = sysctlName;
        std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
        EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>(value + "\n")));
        EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
        EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlCat)).WillRepeatedly(Return(Result<std::string>(cfgOuput.c_str())));
        EnsureSysctlParams params;
        params.sysctlName = sysctlName;
        params.value = Pattern::Make(value).Value();

        auto result = AuditEnsureSysctl(params, mIndicators, mContext);

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
    auto sysctlSlashedName = sysctlName;
    std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
    EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>(value + "\n")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlCat)).WillRepeatedly(Return(Result<std::string>(cfgOuput.c_str())));
    EnsureSysctlParams params;
    params.sysctlName = sysctlName;
    params.value = Pattern::Make(value).Value();

    auto result = AuditEnsureSysctl(params, mIndicators, mContext);

    ASSERT_TRUE(result.HasValue()) << "HappyPathValidateSysctlNameAndValues FAILED: nr "
                                   << " name '" << sysctlName << "'";
    ASSERT_EQ(result.Value(), Status::NonCompliant) << "HappyPathValidateSysctlNameAndValues FAILED: nr "
                                                    << " name '" << sysctlName << "'";
    ;
}

TEST_F(EnsureSysctlTest, UfwDefaultsFileMissing)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    auto sysctlSlashedName = sysctlName;
    std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
    EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>("1\n")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlCat)).WillRepeatedly(Return(Result<std::string>(emptyOutput)));
    EXPECT_CALL(mContext, GetFileContents("/etc/default/ufw")).WillRepeatedly(Return(Result<std::string>(Error("No such file or directory", -1))));

    EnsureSysctlParams params;
    params.sysctlName = sysctlName;
    params.value = Pattern::Make("1").Value();
    auto result = AuditEnsureSysctl(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("Failed to read /etc/default/ufw") != std::string::npos);
}

TEST_F(EnsureSysctlTest, UfwDefaultsFileNoIptSysctl)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    auto sysctlSlashedName = sysctlName;
    std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
    EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>("1\n")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlCat)).WillRepeatedly(Return(Result<std::string>(emptyOutput)));
    EXPECT_CALL(mContext, GetFileContents("/etc/default/ufw")).WillRepeatedly(Return(Result<std::string>("# No IPT_SYSCTL here\nFOO=bar\n")));

    EnsureSysctlParams params;
    params.sysctlName = sysctlName;
    params.value = Pattern::Make("1").Value();
    auto result = AuditEnsureSysctl(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("Failed to find IPT_SYSCTL") != std::string::npos);
}

TEST_F(EnsureSysctlTest, UfwSysctlFileMissing)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    auto sysctlSlashedName = sysctlName;
    std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
    EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>("1\n")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlCat)).WillRepeatedly(Return(Result<std::string>(emptyOutput)));
    EXPECT_CALL(mContext, GetFileContents("/etc/default/ufw")).WillRepeatedly(Return(Result<std::string>("IPT_SYSCTL=/tmp/ufw-sysctl.conf\n")));
    EXPECT_CALL(mContext, GetFileContents("/tmp/ufw-sysctl.conf")).WillRepeatedly(Return(Result<std::string>(Error("No such file or directory", -1))));

    EnsureSysctlParams params;
    params.sysctlName = sysctlName;
    params.value = Pattern::Make("1").Value();
    auto result = AuditEnsureSysctl(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("Failed to read ufw sysctl config file") != std::string::npos);
}

TEST_F(EnsureSysctlTest, UfwSysctlFileValueMatches)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    auto sysctlSlashedName = sysctlName;
    std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
    EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>("1\n")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlCat)).WillRepeatedly(Return(Result<std::string>(emptyOutput)));
    EXPECT_CALL(mContext, GetFileContents("/etc/default/ufw")).WillRepeatedly(Return(Result<std::string>("IPT_SYSCTL=/tmp/ufw-sysctl.conf\n")));
    EXPECT_CALL(mContext, GetFileContents("/tmp/ufw-sysctl.conf")).WillRepeatedly(Return(Result<std::string>("net/ipv4/ip_forward=1\n")));

    EnsureSysctlParams params;
    params.sysctlName = sysctlName;
    params.value = Pattern::Make("1").Value();
    auto result = AuditEnsureSysctl(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("in UFW configuration") != std::string::npos);
}

TEST_F(EnsureSysctlTest, UfwSysctlFileValueDoesNotMatch)
{
    auto sysctlName = std::string("net.ipv4.ip_forward");
    auto sysctlSlashedName = sysctlName;
    std::replace(sysctlSlashedName.begin(), sysctlSlashedName.end(), '.', '/');
    EXPECT_CALL(mContext, GetFileContents("/proc/sys/" + sysctlSlashedName)).WillRepeatedly(Return(Result<std::string>("1\n")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlVersion)).WillRepeatedly(Return(Result<std::string>("")));
    EXPECT_CALL(mContext, ExecuteCommand(systemdSysctlCat)).WillRepeatedly(Return(Result<std::string>(emptyOutput)));
    EXPECT_CALL(mContext, GetFileContents("/etc/default/ufw")).WillRepeatedly(Return(Result<std::string>("IPT_SYSCTL=/tmp/ufw-sysctl.conf\n")));
    EXPECT_CALL(mContext, GetFileContents("/tmp/ufw-sysctl.conf")).WillRepeatedly(Return(Result<std::string>("net/ipv4/ip_forward=0\n")));

    EnsureSysctlParams params;
    params.sysctlName = sysctlName;
    params.value = Pattern::Make("1").Value();
    auto result = AuditEnsureSysctl(params, mIndicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
    ASSERT_TRUE(mFormatter.Format(mIndicators).Value().find("got '0' in UFW configuration") != std::string::npos);
}
