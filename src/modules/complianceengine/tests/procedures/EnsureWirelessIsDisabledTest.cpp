// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "MockContext.h"

#include <EnsureWirelessIsDisabled.h>
#include <algorithm>
#include <dirent.h>
#include <fstream>
#include <gtest/gtest.h>
#include <linux/limits.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <vector>

using ComplianceEngine::AuditEnsureWirelessIsDisabled;
using ComplianceEngine::CompactListFormatter;
using ComplianceEngine::EnsureWirelessIsDisabledParams;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Result;
using ComplianceEngine::Status;

namespace
{
bool startsWith(const std::string& str, const std::string& prefix)
{
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}
} // namespace

class EnsureWirelessIsDisabledTest : public ::testing::Test
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
    char dirTemplate[PATH_MAX] = "/tmp/ensureWirelessSysfs.XXXXXX";
    std::string dir;    // " sysfs base (/sys)
    std::string sysDir; // " sysfs base (/sys)
    std::set<std::string, LengthComparator> sysDirs;
    std::set<std::string, LengthComparator> sysFiles;

    MockContext mContext;
    IndicatorsTree mIndicators;
    CompactListFormatter mFormatter;

    void SetUp() override
    {
        int ret = -1;
        dir = mkdtemp(dirTemplate);
        ASSERT_TRUE(dir != "");
        auto class_dir = dir + std::string("/class");
        ret = ::mkdir(class_dir.c_str(), 0777);
        ASSERT_TRUE((ret == 0) || (ret != 0 && (errno == EEXIST)));
        sysDirs.insert(class_dir);

        sysDir = class_dir + "/net";
        ret = ::mkdir(sysDir.c_str(), 0777);
        ASSERT_TRUE((ret == 0) || (ret != 0 && (errno == EEXIST)));
        sysDirs.insert(sysDir);

        mIndicators.Push("EnsureWirelessIsDisable");
    }

    void CreateSysFile(std::string fileName, std::string data)
    {
        int ret;
        std::string path = dir + std::string("/") + fileName;

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
            sysDirs.insert(pathPart);
        }

        std::ofstream file(path);
        file << data;
        file.close();
        sysFiles.insert(path);
    }

    void CreateSysDir(std::string dirname)
    {
        int ret;
        std::string path = dir + std::string("/") + dirname;

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
            sysDirs.insert(pathPart);
        }
        dirname = dir + "/" + dirname;
        ret = ::mkdir(dirname.c_str(), 0777);
        ASSERT_TRUE((ret == 0) || (ret != 0 && (errno == EEXIST))) << "ERROR creating directory " << dirname << " errno " << errno << ": " << strerror(errno);
        sysDirs.insert(dirname);
    }

    void CreateSysLinkDir(std::string from, std::string to)
    {
        from = dir + "/" + from;
        to = dir + "/" + to;
        // auto ret = ::symlink(from.c_str(), to.c_str());
        // ASSERT_TRUE((ret == 0) || (ret != 0 && (errno == EEXIST)));
        auto ret = ::symlink(to.c_str(), from.c_str());
        ASSERT_TRUE((ret == 0) || (ret != 0 && (errno == EEXIST)));
    }

    void CreateWirelessDevice(std::string name, std::string kernelModuleName)
    {
        auto driverName = "class/net/" + name;
        CreateSysDir(driverName);
        auto wireless = driverName + "/wireless";
        CreateSysDir(wireless);

        auto deviceDir = "class/net/" + name + "/device/driver";
        CreateSysDir(deviceDir);

        auto kernelModuleNameDir = "module/" + kernelModuleName;
        CreateSysDir(kernelModuleNameDir);

        auto module = deviceDir + "/module";
        CreateSysLinkDir(module, kernelModuleNameDir);
    }

    void TearDown() override
    {
        for (auto& file : sysFiles)
        {
            unlink(file.c_str());
        }
        for (auto& dir : sysDirs)
        {
            rmdir(dir.c_str());
        }
        rmdir(dir.c_str());
    }
};

static const char procModulesPath[] = "/proc/modules";
static const char procModulesPositiveOutput[] =
    "iwlwifi 290816 1 iwldvm, Live 0xffffffffc05ec000\n"
    "cfg80211 634880 3 iwldvm,mac80211,iwlwifi, Live 0xffffffffc04ab000\n"
    "parport_pc 32768 0 - Live 0xffffffffc0330000\n"
    "parport 49152 3 parport_pc,ppdev,lp, Live 0xffffffffc02f7000\n";
static const char procModulesNegativeOutput[] =
    "rotah 110592 0 - Live 0xffffffffc135d000\n"
    "curve25519_x86_64 36864 1 rotah, Live 0xffffffffc12f7000\n"
    "libcurve25519_generic 49152 2 rotah,curve25519_x86_64, Live 0xffffffffc12e6000\n";

static const char modprobeCommand[] = "modprobe";
static const char modprobeNothingOutput[] = "blacklist neofb\nalias net_pf_3 off\n";
static const char modprobeBlacklistOutput[] = "blacklist iwlwifi\nalias net_pf_3 off\n";
static const char modprobeBlockedOutput[] = "blacklist iwlwifi\ninstall iwlwifi /usr/bin/true\n";

TEST_F(EnsureWirelessIsDisabledTest, HappyPathTest)
{
    EnsureWirelessIsDisabledParams params;
    params.test_sysfs_class_net = sysDir;
    CreateWirelessDevice("wlp2s0", "iwlwifi");
    UNUSED(procModulesPositiveOutput);
    UNUSED(modprobeNothingOutput);

    // Prime IsModuleBlocked
    {
        // Setup the expectation for the proc modules read
        EXPECT_CALL(mContext, GetFileContents(::testing::StrEq(procModulesPath))).WillRepeatedly(::testing::Return(Result<std::string>(procModulesNegativeOutput)));

        // Setup the expectation for the modprobe command
        EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(modprobeCommand))).WillRepeatedly(::testing::Return(Result<std::string>(modprobeBlockedOutput)));
    }

    auto result = AuditEnsureWirelessIsDisabled(params, mIndicators, mContext);

    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureWirelessIsDisabledTest, UnhappyPathModuleLoaded)
{
    EnsureWirelessIsDisabledParams params;
    params.test_sysfs_class_net = sysDir;
    CreateWirelessDevice("wlp2s0", "iwlwifi");

    // Prime IsModuleBlocked
    {
        // Setup the expectation for the proc modules read
        EXPECT_CALL(mContext, GetFileContents(::testing::StrEq(procModulesPath))).WillRepeatedly(::testing::Return(Result<std::string>(procModulesPositiveOutput)));

        // Setup the expectation for the modprobe command
        EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(modprobeCommand))).WillRepeatedly(::testing::Return(Result<std::string>(modprobeNothingOutput)));
    }

    auto result = AuditEnsureWirelessIsDisabled(params, mIndicators, mContext);

    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureWirelessIsDisabledTest, UnhappyPathModuleNotLoadedNotBlocked)
{
    EnsureWirelessIsDisabledParams params;
    params.test_sysfs_class_net = sysDir;
    CreateWirelessDevice("wlp2s0", "iwlwifi");

    // Prime IsModuleBlocked
    {
        // Setup the expectation for the proc modules read
        EXPECT_CALL(mContext, GetFileContents(::testing::StrEq(procModulesPath))).WillRepeatedly(::testing::Return(Result<std::string>(procModulesNegativeOutput)));

        // Setup the expectation for the modprobe command
        EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(modprobeCommand))).WillRepeatedly(::testing::Return(Result<std::string>(modprobeNothingOutput)));
    }

    auto result = AuditEnsureWirelessIsDisabled(params, mIndicators, mContext);

    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureWirelessIsDisabledTest, UnhappyPathModuleNotLoadedNotBlockedonlyBlacklisted)
{
    EnsureWirelessIsDisabledParams params;
    params.test_sysfs_class_net = sysDir;
    CreateWirelessDevice("wlp2s0", "iwlwifi");

    // Prime IsModuleBlocked
    {
        // Setup the expectation for the proc modules read
        EXPECT_CALL(mContext, GetFileContents(::testing::StrEq(procModulesPath))).WillRepeatedly(::testing::Return(Result<std::string>(procModulesNegativeOutput)));

        // Setup the expectation for the modprobe command
        EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(modprobeCommand))).WillRepeatedly(::testing::Return(Result<std::string>(modprobeBlacklistOutput)));
    }

    auto result = AuditEnsureWirelessIsDisabled(params, mIndicators, mContext);

    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureWirelessIsDisabledTest, UnhappyPathOnlyOneDriverIsBlocked)
{
    EnsureWirelessIsDisabledParams params;
    params.test_sysfs_class_net = sysDir;
    CreateWirelessDevice("wlp2s0", "iwlwifi");
    CreateWirelessDevice("wlp3s1", "mwl8k");

    // Prime IsModuleBlocked
    // iwlwifi, mwl8k is not loaded  but only iwlwifi is blocked
    {
        // Setup the expectation for the proc modules read
        EXPECT_CALL(mContext, GetFileContents(::testing::StrEq(procModulesPath))).WillRepeatedly(::testing::Return(Result<std::string>(procModulesNegativeOutput)));

        // Setup the expectation for the modprobe command
        EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(modprobeCommand))).WillRepeatedly(::testing::Return(Result<std::string>(modprobeBlockedOutput)));
    }

    auto result = AuditEnsureWirelessIsDisabled(params, mIndicators, mContext);

    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}
