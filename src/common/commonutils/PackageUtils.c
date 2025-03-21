// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

static const char* g_aptGet = "apt-get";
static const char* g_dpkg = "dpkg";
static const char* g_tdnf = "tdnf";
static const char* g_dnf = "dnf";
static const char* g_yum = "yum";
static const char* g_zypper = "zypper";

static bool g_checkedPackageManagersPresence = false;
static bool g_aptGetIsPresent = false;
static bool g_dpkgIsPresent = false;
static bool g_tdnfIsPresent = false;
static bool g_dnfIsPresent = false;
static bool g_yumIsPresent = false;
static bool g_zypperIsPresent = false;
static bool g_aptGetUpdateExecuted = false;
static bool g_zypperRefreshExecuted = false;
static bool g_zypperRefreshServicesExecuted = false;
static bool g_tdnfCheckUpdateExecuted = false;
static bool g_dnfCheckUpdateExecuted = false;
static bool g_yumCheckUpdateExecuted = false;

int IsPresent(const char* what, OsConfigLogHandle log)
{
    const char* commandTemplate = "command -v %s";
    char* command = NULL;
    int status = ENOENT;

    if (NULL == what)
    {
        OsConfigLogError(log, "IsPresent called with invalid argument");
        return EINVAL;
    }

    if (NULL != (command = FormatAllocateString(commandTemplate, what)))
    {
        if (0 == (status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
        {
            OsConfigLogInfo(log, "'%s' is locally present", what);
        }
    }
    else
    {
        OsConfigLogError(log, "IsPresent: FormatAllocateString failed");
        status = ENOMEM;
    }

    FREE_MEMORY(command);

    return status;
}

static void CheckPackageManagersPresence(OsConfigLogHandle log)
{
    if (false == g_checkedPackageManagersPresence)
    {
        g_checkedPackageManagersPresence = true;
        g_aptGetIsPresent = (0 == IsPresent(g_aptGet, log)) ? true : false;
        g_dpkgIsPresent = (0 == IsPresent(g_dpkg, log)) ? true : false;
        g_tdnfIsPresent = (0 == IsPresent(g_tdnf, log)) ? true : false;
        g_dnfIsPresent = (0 == IsPresent(g_dnf, log)) ? true : false;
        g_yumIsPresent = (0 == IsPresent(g_yum, log)) ? true : false;
        g_zypperIsPresent = (0 == IsPresent(g_zypper, log)) ? true : false;
    }
}

static int CheckOrInstallPackage(const char* commandTemplate, const char* packageManager, const char* packageName, OsConfigLogHandle log)
{
    char* command = NULL;
    int status = ENOENT;

    if ((NULL == commandTemplate) || (NULL == packageManager) || (NULL == packageName) || ((0 == strlen(packageName))))
    {
        OsConfigLogError(log, "CheckOrInstallPackage called with invalid arguments");
        return EINVAL;
    }

    if (NULL == (command = FormatAllocateString(commandTemplate, packageManager, packageName)))
    {
        OsConfigLogError(log, "CheckOrInstallPackage: FormatAllocateString failed");
        return ENOMEM;
    }

    status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log);

    OsConfigLogInfo(log, "Package manager '%s' command '%s' returning %d (errno: %d)", packageManager, command, status, errno);

    FREE_MEMORY(command);

    return status;
}

int IsPackageInstalled(const char* packageName, OsConfigLogHandle log)
{
    const char* commandTemplateDpkg = "%s -l %s | grep ^ii";
    const char* commandTemplateYumDnf = "%s list installed  --cacheonly %s";
    const char* commandTemplateRedHat = "%s list installed %s --disableplugin subscription-manager";
    const char* commandTemplateZypper = "%s se -x %s";
    int status = ENOENT;

    CheckPackageManagersPresence(log);

    if (g_dpkgIsPresent)
    {
        status = CheckOrInstallPackage(commandTemplateDpkg, g_dpkg, packageName, log);
    }
    else if (g_tdnfIsPresent)
    {
        status = CheckOrInstallPackage(commandTemplateYumDnf, g_tdnf, packageName, log);
    }
    else if (g_dnfIsPresent)
    {
        status = CheckOrInstallPackage(IsRedHatBased(log) ? commandTemplateRedHat : commandTemplateYumDnf, g_dnf, packageName, log);
    }
    else if (g_yumIsPresent)
    {
        status = CheckOrInstallPackage(IsRedHatBased(log) ? commandTemplateRedHat : commandTemplateYumDnf, g_yum, packageName, log);
    }
    else if (g_zypperIsPresent)
    {
        status = CheckOrInstallPackage(commandTemplateZypper, g_zypper, packageName, log);
    }

    if (0 == status)
    {
        OsConfigLogInfo(log, "IsPackageInstalled: package '%s' is installed", packageName);
    }
    else
    {
        OsConfigLogInfo(log, "IsPackageInstalled: package '%s' is not found (%d, errno: %d)", packageName, status, errno);
    }

    return status;
}

static bool WildcardsPresent(const char* packageName)
{
    return (packageName ? (strstr(packageName, "*") || strstr(packageName, "^")) : false);
}

int CheckPackageInstalled(const char* packageName, char** reason, OsConfigLogHandle log)
{
    int result = 0;

    if (0 == (result = IsPackageInstalled(packageName, log)))
    {
        OsConfigCaptureSuccessReason(reason, WildcardsPresent(packageName) ? "Some '%s' packages are installed" : "Package '%s' is installed", packageName);
    }
    else if ((EINVAL != result) && (ENOMEM != result))
    {
        OsConfigCaptureReason(reason, WildcardsPresent(packageName) ? "No '%s' packages are installed" : "Package '%s' is not installed", packageName);
    }
    else
    {
        OsConfigCaptureReason(reason, "Internal error: %d", result);
    }

    return result;
}

int CheckPackageNotInstalled(const char* packageName, char** reason, OsConfigLogHandle log)
{
    int result = 0;

    if (0 == (result = IsPackageInstalled(packageName, log)))
    {
        OsConfigCaptureReason(reason, WildcardsPresent(packageName) ? "Some '%s' packages are installed" : "Package '%s' is installed", packageName);
        result = ENOENT;
    }
    else if ((EINVAL != result) && (ENOMEM != result))
    {
        OsConfigCaptureSuccessReason(reason, WildcardsPresent(packageName) ? "No '%s' packages are installed" : "Package '%s' is not installed", packageName);
        result = 0;
    }
    else
    {
        OsConfigCaptureReason(reason, "Internal error: %d", result);
    }

    return result;
}

static int ExecuteSimplePackageCommand(const char* command, bool* executed, OsConfigLogHandle log)
{
    int status = 0;

    if ((NULL == command) || (NULL == executed))
    {
        OsConfigLogError(log, "ExecuteSimplePackageCommand called with invalid arguments");
        return EINVAL;
    }

    if (true == *executed)
    {
        return status;
    }

    if (0 == (status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log)))
    {
        OsConfigLogInfo(log, "ExecuteSimplePackageCommand: '%s' was successful", command);
        *executed = true;
    }
    else
    {
        OsConfigLogInfo(log, "ExecuteSimplePackageCommand: '%s' returned %d (errno: %d)", command, status, errno);
        *executed = false;
    }

    return status;
}

static int ExecuteAptGetUpdate(OsConfigLogHandle log)
{
    return ExecuteSimplePackageCommand("apt-get update", &g_aptGetUpdateExecuted, log);
}

static int ExecuteZypperRefresh(OsConfigLogHandle log)
{
    return ExecuteSimplePackageCommand("zypper refresh", &g_zypperRefreshExecuted, log);
}

static int ExecuteZypperRefreshServices(OsConfigLogHandle log)
{
    return ExecuteSimplePackageCommand("zypper refresh --services", &g_zypperRefreshServicesExecuted, log);
}

static int ExecuteTdnfCheckUpdate(OsConfigLogHandle log)
{
    return ExecuteSimplePackageCommand("tdnf check-update", &g_tdnfCheckUpdateExecuted, log);
}

static int ExecuteDnfCheckUpdate(OsConfigLogHandle log)
{
    return ExecuteSimplePackageCommand("dnf check-update", &g_dnfCheckUpdateExecuted, log);
}

static int ExecuteYumCheckUpdate(OsConfigLogHandle log)
{
    return ExecuteSimplePackageCommand("yum check-update", &g_yumCheckUpdateExecuted, log);
}

int InstallOrUpdatePackage(const char* packageName, OsConfigLogHandle log)
{
    const char* commandTemplate = "%s install -y %s";
    const char* commandTemplateTdnfDnfYum = "%s install -y --cacheonly %s";
    int status = ENOENT;

    CheckPackageManagersPresence(log);

    if (g_aptGetIsPresent)
    {
        ExecuteAptGetUpdate(log);
        status = CheckOrInstallPackage(commandTemplate, g_aptGet, packageName, log);
    }
    else if (g_tdnfIsPresent)
    {
        ExecuteTdnfCheckUpdate(log);
        status = CheckOrInstallPackage(commandTemplateTdnfDnfYum, g_tdnf, packageName, log);
    }
    else if (g_dnfIsPresent)
    {
        ExecuteDnfCheckUpdate(log);
        status = CheckOrInstallPackage(commandTemplateTdnfDnfYum, g_dnf, packageName, log);
    }
    else if (g_yumIsPresent)
    {
        ExecuteYumCheckUpdate(log);
        status = CheckOrInstallPackage(commandTemplateTdnfDnfYum, g_yum, packageName, log);
    }
    else if (g_zypperIsPresent)
    {
        ExecuteZypperRefresh(log);
        ExecuteZypperRefreshServices(log);
        status = CheckOrInstallPackage(commandTemplate, g_zypper, packageName, log);
    }

    if (0 == status)
    {
        status = IsPackageInstalled(packageName, log);
    }

    if (0 == status)
    {
        OsConfigLogInfo(log, "InstallOrUpdatePackage: package '%s' was successfully installed or updated", packageName);
    }
    else
    {
        OsConfigLogInfo(log, "InstallOrUpdatePackage: installation or update of package '%s' returned %d (errno: %d)", packageName, status, errno);
    }

    return status;
}

int InstallPackage(const char* packageName, OsConfigLogHandle log)
{
    int status = ENOENT;

    if (0 != (status = IsPackageInstalled(packageName, log)))
    {
        status = InstallOrUpdatePackage(packageName, log);
    }
    else
    {
        OsConfigLogInfo(log, "InstallPackage: package '%s' is already installed", packageName);
        status = 0;
    }

    return status;
}

int UninstallPackage(const char* packageName, OsConfigLogHandle log)
{
    const char* commandTemplateAptGet = "%s remove -y --purge %s";
    const char* commandTemplateZypper = "%s remove -y --force %s";
    const char* commandTemplateTdnfDnfYum = "%s remove -y --force --cacheonly %s";

    int status = ENOENT;

    CheckPackageManagersPresence(log);

    if (0 == (status = IsPackageInstalled(packageName, log)))
    {
        if (g_aptGetIsPresent)
        {
            ExecuteAptGetUpdate(log);
            status = CheckOrInstallPackage(commandTemplateAptGet, g_aptGet, packageName, log);
        }
        else if (g_tdnfIsPresent)
        {
            ExecuteTdnfCheckUpdate(log);
            status = CheckOrInstallPackage(commandTemplateTdnfDnfYum, g_tdnf, packageName, log);
        }
        else if (g_dnfIsPresent)
        {
            ExecuteDnfCheckUpdate(log);
            status = CheckOrInstallPackage(commandTemplateTdnfDnfYum, g_dnf, packageName, log);
        }
        else if (g_yumIsPresent)
        {
            ExecuteYumCheckUpdate(log);
            status = CheckOrInstallPackage(commandTemplateTdnfDnfYum, g_yum, packageName, log);
        }
        else if (g_zypperIsPresent)
        {
            ExecuteZypperRefresh(log);
            ExecuteZypperRefreshServices(log);
            status = CheckOrInstallPackage(commandTemplateZypper, g_zypper, packageName, log);
        }

        if ((0 == status) && (0 == IsPackageInstalled(packageName, log)))
        {
            status = ENOENT;
        }

        if (0 == status)
        {
            OsConfigLogInfo(log, "UninstallPackage: package '%s' was successfully uninstalled", packageName);
        }
        else
        {
            OsConfigLogInfo(log, "UninstallPackage: uninstallation of package '%s' returned %d (errno: %d)", packageName, status, errno);
        }
    }
    else if (EINVAL != status)
    {
        OsConfigLogInfo(log, "InstallPackage: package '%s' is not found", packageName);
        status = 0;
    }

    return status;
}
