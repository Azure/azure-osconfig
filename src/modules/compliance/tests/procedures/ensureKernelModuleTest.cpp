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

using compliance::Audit_ensureKernelModuleUnavailable;
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

static const char lsmodCommand[] = "lsmod";
static const char lsmodPositiveOutput[] =
    "Module                  Size  Used by\npsmouse         13123 1 foo\nhator        512234 512 libchacha\npppoe          41233 0\n";
static const char lsmodNegativeOutput[] =
    "Module                  Size  Used by\npsmouse         13123 1 foo\nrotah        512234 512 libchacha\npppoe          41233 0\n";

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
    AddMockCommand(lsmodCommand, true, lsmodPositiveOutput, 0);
    AddMockCommand(modprobeCommand, true, modprobeNothingOutput, 0);
    std::map<std::string, std::string> args;

    std::ostringstream logstream;
    Result<bool> result = Audit_ensureKernelModuleUnavailable(args, logstream);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "No module name provided");
}

TEST_F(EnsureKernelModuleTest, FailedFindExecution)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, NULL, -1);
    AddMockCommand(lsmodCommand, true, lsmodPositiveOutput, 0);
    AddMockCommand(modprobeCommand, true, modprobeNothingOutput, 0);
    std::map<std::string, std::string> args;
    args["moduleName"] = "hator";

    std::ostringstream logstream;
    Result<bool> result = Audit_ensureKernelModuleUnavailable(args, logstream);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Failed to execute find command");
}

TEST_F(EnsureKernelModuleTest, FailedLsmodExecution)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, findPositiveOutput, 0);
    AddMockCommand(lsmodCommand, true, NULL, -1);
    AddMockCommand(modprobeCommand, true, modprobeNothingOutput, 0);
    std::map<std::string, std::string> args;
    args["moduleName"] = "hator";

    std::ostringstream logstream;
    Result<bool> result = Audit_ensureKernelModuleUnavailable(args, logstream);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Failed to execute lsmod");
}

TEST_F(EnsureKernelModuleTest, FailedModprobeExecution)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, findPositiveOutput, 0);
    AddMockCommand(lsmodCommand, true, lsmodPositiveOutput, 0);
    AddMockCommand(modprobeCommand, true, NULL, -1);
    std::map<std::string, std::string> args;
    args["moduleName"] = "hator";

    std::ostringstream logstream;
    Result<bool> result = Audit_ensureKernelModuleUnavailable(args, logstream);
    ASSERT_FALSE(result.HasValue());
    ASSERT_EQ(result.Error().message, "Failed to execute modprobe");
}

TEST_F(EnsureKernelModuleTest, ModuleNotFoundInFind)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, findNegativeOutput, 0);
    AddMockCommand(lsmodCommand, true, lsmodPositiveOutput, 0);
    AddMockCommand(modprobeCommand, true, modprobeNothingOutput, 0);
    std::map<std::string, std::string> args;
    args["moduleName"] = "hator";

    std::ostringstream logstream;
    Result<bool> result = Audit_ensureKernelModuleUnavailable(args, logstream);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), true);
}

TEST_F(EnsureKernelModuleTest, ModuleFoundInLsmod)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, findPositiveOutput, 0);
    AddMockCommand(lsmodCommand, true, lsmodPositiveOutput, 0);
    AddMockCommand(modprobeCommand, true, modprobeNothingOutput, 0);
    std::map<std::string, std::string> args;
    args["moduleName"] = "hator";

    std::ostringstream logstream;
    Result<bool> result = Audit_ensureKernelModuleUnavailable(args, logstream);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), false);
}

TEST_F(EnsureKernelModuleTest, NoAlias)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, findPositiveOutput, 0);
    AddMockCommand(lsmodCommand, true, lsmodNegativeOutput, 0);
    AddMockCommand(modprobeCommand, true, modprobeBlacklistOutput, 0);
    std::map<std::string, std::string> args;
    args["moduleName"] = "hator";

    std::ostringstream logstream;
    Result<bool> result = Audit_ensureKernelModuleUnavailable(args, logstream);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), false);
}

TEST_F(EnsureKernelModuleTest, NoBlacklist)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, findPositiveOutput, 0);
    AddMockCommand(lsmodCommand, true, lsmodNegativeOutput, 0);
    AddMockCommand(modprobeCommand, true, modprobeAliasOutput, 0);
    std::map<std::string, std::string> args;
    args["moduleName"] = "hator";

    std::ostringstream logstream;
    Result<bool> result = Audit_ensureKernelModuleUnavailable(args, logstream);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), false);
}

TEST_F(EnsureKernelModuleTest, ModuleBlocked)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, findPositiveOutput, 0);
    AddMockCommand(lsmodCommand, true, lsmodNegativeOutput, 0);
    AddMockCommand(modprobeCommand, true, modprobeBlockedOutput, 0);
    std::map<std::string, std::string> args;
    args["moduleName"] = "hator";

    std::ostringstream logstream;
    Result<bool> result = Audit_ensureKernelModuleUnavailable(args, logstream);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), true);
}

TEST_F(EnsureKernelModuleTest, OverlayedModuleNotBlocked)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, findOverlayedOutput, 0);
    AddMockCommand(lsmodCommand, true, lsmodNegativeOutput, 0);
    AddMockCommand(modprobeCommand, true, modprobeBlockedOutput, 0);
    std::map<std::string, std::string> args;
    args["moduleName"] = "hator";

    std::ostringstream logstream;
    Result<bool> result = Audit_ensureKernelModuleUnavailable(args, logstream);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), false);
}

TEST_F(EnsureKernelModuleTest, OverlayedModuleBlocked)
{
    CleanupMockCommands();
    AddMockCommand(findCommand, true, findOverlayedOutput, 0);
    AddMockCommand(lsmodCommand, true, lsmodNegativeOutput, 0);
    AddMockCommand(modprobeCommand, true, modprobeBlockedOverlayOutput, 0);
    std::map<std::string, std::string> args;
    args["moduleName"] = "hator";

    std::ostringstream logstream;
    Result<bool> result = Audit_ensureKernelModuleUnavailable(args, logstream);
    ASSERT_TRUE(result.HasValue());
    ASSERT_EQ(result.Value(), true);
}
