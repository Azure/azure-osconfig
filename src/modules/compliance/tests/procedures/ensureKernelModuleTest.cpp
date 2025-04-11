// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "CommonUtils.h"
#include "Evaluator.h"
#include "ProcedureMap.h"

#include <dirent.h>
#include <fstream>
#include <gtest/gtest.h>
#include <linux/limits.h>
#include <string>
#include <unistd.h>

using compliance::AuditEnsureKernelModuleUnavailable;
using compliance::Error;
using compliance::Result;

static const char findCommand[] = "find";
static const char findPositiveOutput[] =
    "/lib/modules/5.15.167.4-microsoft-standard-WSL2/kernel/drivers/block/nbd.ko\n/lib/modules/5.15.167.4-microsoft-standard-WSL2/kernel/drivers/usb/"
    "serial/hator.ko\n/lib/modules/5.15.167.4-microsoft-standard-WSL2/kernel/net/netfilter/xt_CT.ko\n/lib/modules/5.15.167.4-microsoft-standard-WSL2/"
    "kernel/net/netfilter/xt_u32.ko\n";
static const char findNegativeOutput[] =
    "/lib/modules/5.15.167.4-microsoft-standard-WSL2/kernel/drivers/block/nbd.ko\n/lib/modules/5.15.167.4-microsoft-standard-WSL2/kernel/drivers/usb/"
    "serial/usbserial.ko\n/lib/modules/5.15.167.4-microsoft-standard-WSL2/kernel/net/netfilter/xt_CT.ko\n/lib/modules/"
    "5.15.167.4-microsoft-standard-WSL2/kernel/net/netfilter/xt_u32.ko\n";
static const char findOverlayedOutput[] =
    "/lib/modules/5.15.167.4-microsoft-standard-WSL2/kernel/drivers/block/nbd.ko\n/lib/modules/5.15.167.4-microsoft-standard-WSL2/kernel/drivers/usb/"
    "serial/hator_overlay.ko\n/lib/modules/5.15.167.4-microsoft-standard-WSL2/kernel/net/netfilter/xt_CT.ko\n/lib/modules/"
    "5.15.167.4-microsoft-standard-WSL2/kernel/net/netfilter/xt_u32.ko\n";

static const char catModulesCommand[] = "cat";
static const char catModulesPositiveOutput[] =
    "hator 110592 0 - Live 0xffffffffc135d000\n"
    "libchacha20poly1305 16384 1 hator, Live 0xffffffffc1316000\n"
    "chacha_x86_64 28672 1 libchacha20poly1305, Live 0xffffffffc132b000\n";
static const char catModulesNegativeOutput[] =
    "rotah 110592 0 - Live 0xffffffffc135d000\n"
    "libchacha20poly1305 16384 1 rotah, Live 0xffffffffc1316000\n"
    "chacha_x86_64 28672 1 libchacha20poly1305, Live 0xffffffffc132b000\n";

static const char modprobeCommand[] = "modprobe";
static const char modprobeNothingOutput[] = "blacklist neofb\nalias net_pf_3 off\n";
static const char modprobeBlacklistOutput[] = "blacklist hator\nalias net_pf_3 off\n";
static const char modprobeAliasOutput[] = "blacklist neofb\ninstall hator /usr/bin/true\n";
static const char modprobeBlockedOutput[] = "blacklist hator\ninstall hator /usr/bin/true\n";
static const char modprobeBlockedOverlayOutput[] = "blacklist hator_overlay\ninstall hator_overlay /usr/bin/true\n";

class EnsureKernelModuleTest : public ::testing::Test
{
protected:
    char dirTemplate[PATH_MAX] = "/tmp/fsoptionTest.XXXXXX";
    std::string dir;
    std::string fstabFile;
    std::string mtabFile;

    void SetUp() override
    {
    }

    void TearDown() override
    {
        CleanupMockCommands();
    }
};

TEST_F(EnsureKernelModuleTest, AuditNoArgument)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, NULL, -1);
    AddMockCommand(catModulesCommand, true, catModulesPositiveOutput, 0);
    AddMockCommand(modprobeCommand, true, modprobeNothingOutput, 0);
    std::map<std::string, std::string> args;

    std::ostringstream logstream;
    Result<bool> result = AuditEnsureKernelModuleUnavailable(args, logstream, nullptr);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "No module name provided");
}

TEST_F(EnsureKernelModuleTest, FailedFindExecution)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, NULL, -1);
    AddMockCommand(catModulesCommand, true, catModulesPositiveOutput, 0);
    AddMockCommand(modprobeCommand, true, modprobeNothingOutput, 0);
    std::map<std::string, std::string> args;
    args["moduleName"] = "hator";

    std::ostringstream logstream;
    Result<bool> result = AuditEnsureKernelModuleUnavailable(args, logstream, nullptr);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Failed to execute find command");
}

TEST_F(EnsureKernelModuleTest, FailedLsmodExecution)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, findPositiveOutput, 0);
    AddMockCommand(catModulesCommand, true, NULL, -1);
    AddMockCommand(modprobeCommand, true, modprobeNothingOutput, 0);
    std::map<std::string, std::string> args;
    args["moduleName"] = "hator";

    std::ostringstream logstream;
    Result<bool> result = AuditEnsureKernelModuleUnavailable(args, logstream, nullptr);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Failed to execute cat");
}

TEST_F(EnsureKernelModuleTest, FailedModprobeExecution)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, findPositiveOutput, 0);
    AddMockCommand(catModulesCommand, true, catModulesPositiveOutput, 0);
    AddMockCommand(modprobeCommand, true, NULL, -1);
    std::map<std::string, std::string> args;
    args["moduleName"] = "hator";

    std::ostringstream logstream;
    Result<bool> result = AuditEnsureKernelModuleUnavailable(args, logstream, nullptr);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Failed to execute modprobe");
}

TEST_F(EnsureKernelModuleTest, ModuleNotFoundInFind)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, findNegativeOutput, 0);
    AddMockCommand(catModulesCommand, true, catModulesPositiveOutput, 0);
    AddMockCommand(modprobeCommand, true, modprobeNothingOutput, 0);
    std::map<std::string, std::string> args;
    args["moduleName"] = "hator";

    std::ostringstream logstream;
    Result<bool> result = AuditEnsureKernelModuleUnavailable(args, logstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), true);
}

TEST_F(EnsureKernelModuleTest, ModuleFoundInLsmod)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, findPositiveOutput, 0);
    AddMockCommand(catModulesCommand, true, catModulesPositiveOutput, 0);
    AddMockCommand(modprobeCommand, true, modprobeNothingOutput, 0);
    std::map<std::string, std::string> args;
    args["moduleName"] = "hator";

    std::ostringstream logstream;
    Result<bool> result = AuditEnsureKernelModuleUnavailable(args, logstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), false);
}

TEST_F(EnsureKernelModuleTest, NoAlias)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, findPositiveOutput, 0);
    AddMockCommand(catModulesCommand, true, catModulesNegativeOutput, 0);
    AddMockCommand(modprobeCommand, true, modprobeBlacklistOutput, 0);
    std::map<std::string, std::string> args;
    args["moduleName"] = "hator";

    std::ostringstream logstream;
    Result<bool> result = AuditEnsureKernelModuleUnavailable(args, logstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), false);
}

TEST_F(EnsureKernelModuleTest, NoBlacklist)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, findPositiveOutput, 0);
    AddMockCommand(catModulesCommand, true, catModulesNegativeOutput, 0);
    AddMockCommand(modprobeCommand, true, modprobeAliasOutput, 0);
    std::map<std::string, std::string> args;
    args["moduleName"] = "hator";

    std::ostringstream logstream;
    Result<bool> result = AuditEnsureKernelModuleUnavailable(args, logstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), false);
}

TEST_F(EnsureKernelModuleTest, ModuleBlocked)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, findPositiveOutput, 0);
    AddMockCommand(catModulesCommand, true, catModulesNegativeOutput, 0);
    AddMockCommand(modprobeCommand, true, modprobeBlockedOutput, 0);
    std::map<std::string, std::string> args;
    args["moduleName"] = "hator";

    std::ostringstream logstream;
    Result<bool> result = AuditEnsureKernelModuleUnavailable(args, logstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), true);
}

TEST_F(EnsureKernelModuleTest, OverlayedModuleNotBlocked)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, findOverlayedOutput, 0);
    AddMockCommand(catModulesCommand, true, catModulesNegativeOutput, 0);
    AddMockCommand(modprobeCommand, true, modprobeBlockedOutput, 0);
    std::map<std::string, std::string> args;
    args["moduleName"] = "hator";

    std::ostringstream logstream;
    Result<bool> result = AuditEnsureKernelModuleUnavailable(args, logstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), false);
}

TEST_F(EnsureKernelModuleTest, OverlayedModuleBlocked)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, findOverlayedOutput, 0);
    AddMockCommand(catModulesCommand, true, catModulesNegativeOutput, 0);
    AddMockCommand(modprobeCommand, true, modprobeBlockedOverlayOutput, 0);
    std::map<std::string, std::string> args;
    args["moduleName"] = "hator";

    std::ostringstream logstream;
    Result<bool> result = AuditEnsureKernelModuleUnavailable(args, logstream, nullptr);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), true);
}
