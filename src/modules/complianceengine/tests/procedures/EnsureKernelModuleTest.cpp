// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "Evaluator.h"
#include "MockContext.h"

#include <EnsureKernelModule.h>
#include <dirent.h>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <unistd.h>
#include <vector>

using ComplianceEngine::AuditEnsureKernelModuleUnavailable;
using ComplianceEngine::EnsureKernelModuleUnavailableParams;
using ComplianceEngine::Error;
using ComplianceEngine::IndicatorsTree;
using ComplianceEngine::Result;
using ComplianceEngine::Status;

// Test module directory layouts now created in a temporary directory using MockContext::SetSpecialFilePath
// find* outputs removed due to refactor away from executing find command.

static const char procModulesPath[] = "/proc/modules";
static const char procModulesPositiveOutput[] =
    "hator 110592 0 - Live 0xffffffffc135d000\n"
    "curve25519_x86_64 36864 1 hator, Live 0xffffffffc12f7000\n"
    "libcurve25519_generic 49152 2 hator,curve25519_x86_64, Live 0xffffffffc12e6000\n";
static const char procModulesNegativeOutput[] =
    "rotah 110592 0 - Live 0xffffffffc135d000\n"
    "curve25519_x86_64 36864 1 rotah, Live 0xffffffffc12f7000\n"
    "libcurve25519_generic 49152 2 rotah,curve25519_x86_64, Live 0xffffffffc12e6000\n";

static const char modprobeCommand[] = "modprobe";
static const char modprobeNothingOutput[] = "blacklist neofb\nalias net_pf_3 off\n";
static const char modprobeBlacklistOutput[] = "blacklist hator\nalias net_pf_3 off\n";
static const char modprobeAliasOutput[] = "blacklist neofb\ninstall hator /usr/bin/true\n";
static const char modprobeBlockedOutput[] = "blacklist hator\ninstall hator /usr/bin/true\n";
static const char modprobeBlockedOverlayOutput[] = "blacklist hator_overlay\ninstall hator_overlay /usr/bin/true\n";

class EnsureKernelModuleTest : public ::testing::Test
{
protected:
    MockContext mContext;
    IndicatorsTree indicators;

    void SetUp() override
    {
        indicators.Push("EnsureKernelModule");
    }

    void TearDown() override
    {
    }
};

// TODO(kkanas) remove
// Helper to create a fake /lib/modules tree
static std::string CreateModulesTree(MockContext& ctx, const std::vector<std::string>& files)
{
    std::string root = ctx.GetTempdirPath() + "/modulesRoot";
    if (::mkdir(root.c_str(), 0755) != 0)
    {
        ADD_FAILURE() << "Failed to create root dir: " << strerror(errno);
        return "";
    }
    std::string versionDir = root + "/5.15.test";
    if (::mkdir(versionDir.c_str(), 0755) != 0)
    {
        ADD_FAILURE() << "Failed to create version dir: " << strerror(errno);
        return "";
    }
    std::string kernelDir = versionDir + "/kernel";
    if (::mkdir(kernelDir.c_str(), 0755) != 0)
    {
        ADD_FAILURE() << "Failed to create kernel dir: " << strerror(errno);
        return "";
    }
    for (const auto& f : files)
    {
        std::string full = kernelDir + "/" + f;
        std::ofstream ofs(full);
        ofs << "placeholder";
        ofs.close();
    }
    ctx.SetSpecialFilePath("/lib/modules", root);
    return root;
}

TEST_F(EnsureKernelModuleTest, FailedLsmodExecution)
{
    CreateModulesTree(mContext, {"hator.ko", "nbd.ko"});

    // Set up the expectation for the proc modules read to fail
    EXPECT_CALL(mContext, GetFileContents(::testing::StrEq(procModulesPath)))
        .WillRepeatedly(::testing::Return(Result<std::string>(Error("Failed to read /proc/modules", -1))));

    // Set up the expectation for the modprobe command
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(modprobeCommand))).WillRepeatedly(::testing::Return(Result<std::string>(modprobeNothingOutput)));

    EnsureKernelModuleUnavailableParams params;
    params.moduleName = "hator";

    auto result = AuditEnsureKernelModuleUnavailable(params, indicators, mContext);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Failed to read /proc/modules");
}

TEST_F(EnsureKernelModuleTest, FailedModprobeExecution)
{
    CreateModulesTree(mContext, {"hator.ko"});

    // Set up the expectation for the /proc/modules read to succeed
    EXPECT_CALL(mContext, GetFileContents(::testing::StrEq(procModulesPath))).WillRepeatedly(::testing::Return(Result<std::string>(procModulesNegativeOutput)));

    // Set up the expectation for the modprobe command to fail
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(modprobeCommand)))
        .WillRepeatedly(::testing::Return(Result<std::string>(Error("Failed to execute modprobe", -1))));

    EnsureKernelModuleUnavailableParams params;
    params.moduleName = "hator";

    auto result = AuditEnsureKernelModuleUnavailable(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureKernelModuleTest, ModuleNotFoundInFilesystem)
{
    CreateModulesTree(mContext, {"usbserial.ko", "nbd.ko"});

    // Set up the expectation for the proc modules read
    EXPECT_CALL(mContext, GetFileContents(::testing::StrEq(procModulesPath))).WillRepeatedly(::testing::Return(Result<std::string>(procModulesPositiveOutput)));

    // Set up the expectation for the modprobe command
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(modprobeCommand))).WillRepeatedly(::testing::Return(Result<std::string>(modprobeNothingOutput)));

    EnsureKernelModuleUnavailableParams params;
    params.moduleName = "hator";

    auto result = AuditEnsureKernelModuleUnavailable(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureKernelModuleTest, ModuleFoundInProcModules)
{
    CreateModulesTree(mContext, {"hator.ko"});

    // Set up the expectation for the proc modules read showing the module is loaded
    EXPECT_CALL(mContext, GetFileContents(::testing::StrEq(procModulesPath))).WillRepeatedly(::testing::Return(Result<std::string>(procModulesPositiveOutput)));

    // Set up the expectation for the modprobe command
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(modprobeCommand))).WillRepeatedly(::testing::Return(Result<std::string>(modprobeNothingOutput)));

    EnsureKernelModuleUnavailableParams params;
    params.moduleName = "hator";

    auto result = AuditEnsureKernelModuleUnavailable(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureKernelModuleTest, NoAlias)
{
    CreateModulesTree(mContext, {"hator.ko"});

    // Set up the expectation for the proc modules read
    EXPECT_CALL(mContext, GetFileContents(::testing::StrEq(procModulesPath))).WillRepeatedly(::testing::Return(Result<std::string>(procModulesPositiveOutput)));

    // Set up the expectation for the modprobe command with blacklist output
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(modprobeCommand))).WillRepeatedly(::testing::Return(Result<std::string>(modprobeBlacklistOutput)));

    EnsureKernelModuleUnavailableParams params;
    params.moduleName = "hator";

    auto result = AuditEnsureKernelModuleUnavailable(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureKernelModuleTest, NoBlacklist)
{
    CreateModulesTree(mContext, {"hator.ko"});

    // Set up the expectation for the proc modules read
    EXPECT_CALL(mContext, GetFileContents(::testing::StrEq(procModulesPath))).WillRepeatedly(::testing::Return(Result<std::string>(procModulesNegativeOutput)));

    // Set up the expectation for the modprobe command with alias output
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(modprobeCommand))).WillRepeatedly(::testing::Return(Result<std::string>(modprobeAliasOutput)));

    EnsureKernelModuleUnavailableParams params;
    params.moduleName = "hator";

    auto result = AuditEnsureKernelModuleUnavailable(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureKernelModuleTest, ModuleBlocked)
{
    CreateModulesTree(mContext, {"hator.ko"});

    // Set up the expectation for the proc modules read
    EXPECT_CALL(mContext, GetFileContents(::testing::StrEq(procModulesPath))).WillRepeatedly(::testing::Return(Result<std::string>(procModulesNegativeOutput)));

    // Set up the expectation for the modprobe command with blocked output
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(modprobeCommand))).WillRepeatedly(::testing::Return(Result<std::string>(modprobeBlockedOutput)));

    EnsureKernelModuleUnavailableParams params;
    params.moduleName = "hator";

    auto result = AuditEnsureKernelModuleUnavailable(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}

TEST_F(EnsureKernelModuleTest, OverlayedModuleNotBlocked)
{
    CreateModulesTree(mContext, {"hator_overlay.ko"});

    // Set up the expectation for the proc modules read
    EXPECT_CALL(mContext, GetFileContents(::testing::StrEq(procModulesPath))).WillRepeatedly(::testing::Return(Result<std::string>(procModulesNegativeOutput)));

    // Set up the expectation for the modprobe command with blocked output
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(modprobeCommand))).WillRepeatedly(::testing::Return(Result<std::string>(modprobeBlockedOutput)));

    EnsureKernelModuleUnavailableParams params;
    params.moduleName = "hator";

    auto result = AuditEnsureKernelModuleUnavailable(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::NonCompliant);
}

TEST_F(EnsureKernelModuleTest, OverlayedModuleBlocked)
{
    CreateModulesTree(mContext, {"hator_overlay.ko"});

    // Set up the expectation for the proc modules read
    EXPECT_CALL(mContext, GetFileContents(::testing::StrEq(procModulesPath))).WillRepeatedly(::testing::Return(Result<std::string>(procModulesNegativeOutput)));

    // Set up the expectation for the modprobe command with blocked overlay output
    EXPECT_CALL(mContext, ExecuteCommand(::testing::HasSubstr(modprobeCommand))).WillRepeatedly(::testing::Return(Result<std::string>(modprobeBlockedOverlayOutput)));

    EnsureKernelModuleUnavailableParams params;
    params.moduleName = "hator";

    auto result = AuditEnsureKernelModuleUnavailable(params, indicators, mContext);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), Status::Compliant);
}
