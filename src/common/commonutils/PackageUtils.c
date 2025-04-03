// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

#if ((defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 9)))) || defined(__clang__))
#include <stdatomic.h>
#endif

static const char* g_aptGet = "apt-get";
static const char* g_dpkg = "dpkg";
static const char* g_tdnf = "tdnf";
static const char* g_dnf = "dnf";
static const char* g_yum = "yum";
static const char* g_zypper = "zypper";
static const char* g_rpm = "rpm";

static bool g_checkedPackageManagersPresence = false;
static bool g_aptGetIsPresent = false;
static bool g_dpkgIsPresent = false;
static bool g_tdnfIsPresent = false;
static bool g_dnfIsPresent = false;
static bool g_yumIsPresent = false;
static bool g_zypperIsPresent = false;
static bool g_rpmIsPresent = false;
static bool g_aptGetUpdateExecuted = false;
static bool g_zypperRefreshExecuted = false;
static bool g_tdnfCheckUpdateExecuted = false;
static bool g_dnfCheckUpdateExecuted = false;
static bool g_yumCheckUpdateExecuted = false;

#if ((defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 9)))) || defined(__clang__))
static atomic_int g_updateInstalledPackagesCache = 0;
#else
static int g_updateInstalledPackagesCache = 0;
#endif
static char* g_installedPackagesCache = NULL;

void PackageUtilsCleanup(void)
{
    FREE_MEMORY(g_installedPackagesCache);
}

int IsPresent(const char* what, void* log)
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

static void CheckPackageManagersPresence(void* log)
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
        g_rpmIsPresent = (0 == IsPresent(g_rpm, log)) ? true : false;
    }
}

static int CheckOrInstallPackage(const char* commandTemplate, const char* packageManager, const char* packageName, void* log)
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

    status = ExecuteCommand(NULL, command, false, false, 0, g_packageManagerTimeoutSeconds, NULL, NULL, log);

    OsConfigLogInfo(log, "Package manager '%s' command '%s' returning %d", packageManager, command, status);

    FREE_MEMORY(command);

    // Refresh the cache holding the list of installed packages next time we check
    g_updateInstalledPackagesCache = 1;

    return status;
}

static int CheckAllPackages(const char* commandTemplate, const char* packageManager, char** results, void* log)
{
    char* command = NULL;
    int status = ENOENT;

    if ((NULL == commandTemplate) || (NULL == packageManager) || (NULL == results))
    {
        OsConfigLogError(log, "CheckAllPackages called with invalid arguments");
        return EINVAL;
    }

    if (NULL == (command = FormatAllocateString(commandTemplate, packageManager)))
    {
        OsConfigLogError(log, "CheckAllPackages: FormatAllocateString failed");
        return ENOMEM;
    }

    status = ExecuteCommand(NULL, command, false, false, 0, g_packageManagerTimeoutSeconds, results, NULL, log);

    OsConfigLogInfo(log, "Package manager '%s' command '%s' returning  %d", packageManager, command, status);

    FREE_MEMORY(command);

    return status;
}

static int UpdateInstalledPackagesCache(void* log)
{
    const char* commandTemplateDpkg = "%s-query -W -f='${binary:Package}\n'";
    const char* commandTemplateRpm = "%s -qa --queryformat \"%{NAME}\n\"";
    const char* commandTemplateYumDnf = "%s list installed  --cacheonly | awk '{print $1}'";
    const char* commandTmeplateZypper = "%s search -i";

    char* results = NULL;
    char* buffer = NULL;
    int status = ENOENT;

    CheckPackageManagersPresence(log);

    if (g_aptGetIsPresent || g_dpkgIsPresent)
    {
        status = CheckAllPackages(commandTemplateDpkg, g_dpkg, &results, log);
    }
    else if (g_rpmIsPresent)
    {
        status = CheckAllPackages(commandTemplateRpm, g_rpm, &results, log);
    }
    else if (g_tdnfIsPresent)
    {
        status = CheckAllPackages(commandTemplateYumDnf, g_tdnf, &results, log);
    }
    else if (g_dnfIsPresent)
    {
        status = CheckAllPackages(commandTemplateYumDnf, g_dnf, &results, log);
    }
    else if (g_yumIsPresent)
    {
        status = CheckAllPackages(commandTemplateYumDnf, g_yum, &results, log);
    }
    else if (g_zypperIsPresent)
    {
        status = CheckAllPackages(commandTmeplateZypper, g_zypper, &results, log);
    }

    if ((0 == status) && (NULL != results))
    {
        if (NULL != (buffer = DuplicateString(results)))
        {
            FREE_MEMORY(g_installedPackagesCache);
            g_installedPackagesCache = buffer;
        }
        else
        {
            // Leave the cache as-is, just log the error
            OsConfigLogError(log, "UpdateInstalledPackagesCache: out of memory");
            status = ENOMEM;
        }
    }
    else
    {
        // Leave the cache as-is, we can still use it even if it's stale
        status = status ? status : ENOENT;
        OsConfigLogInfo(log, "UpdateInstalledPackagesCache: enumerating all packages failed with %d", status);
    }

    FREE_MEMORY(results);

    return status;
}

int IsPackageInstalled(const char* packageName, void* log)
{
    const char* searchTemplateDpkgRpm = "\n%s\n";
    const char* searchTemplateYumDnf = "\n%s.x86_64\n";
    const char* searchTemplateZypper = "| %s ";

    char* searchTarget = NULL;
    int status = 0;

    if ((NULL == packageName) || (0 == strlen(packageName)))
    {
        OsConfigLogError(log, "IsPackageInstalled called with an invalid argument");
        return EINVAL;
    }

    CheckPackageManagersPresence(log);

    if ((0 != g_updateInstalledPackagesCache) || (NULL == g_installedPackagesCache))
    {
        g_updateInstalledPackagesCache = 0;

        if (0 != (status = UpdateInstalledPackagesCache(log)))
        {
            OsConfigLogInfo(log, "IsPackageInstalled(%s) failed (UpdateInstalledPackagesCache failed)", packageName);
        }
    }

    if (NULL == g_installedPackagesCache)
    {
        OsConfigLogError(log, "IsPackageInstalled: cannot check for '%s' presence without cache", packageName);
        status = ENOENT;
    }
    else if (0 == status)
    {
        if (g_aptGetIsPresent || g_dpkgIsPresent || g_rpmIsPresent)
        {
            searchTarget = FormatAllocateString(searchTemplateDpkgRpm, packageName);
        }
        else if (g_tdnfIsPresent || g_dnfIsPresent || g_yumIsPresent)
        {
            searchTarget = FormatAllocateString(searchTemplateYumDnf, packageName);
        }
        else //if (g_zypperIsPresent)
        {
            searchTarget = FormatAllocateString(searchTemplateZypper, packageName);
        }

        if (NULL == searchTarget)
        {
            OsConfigLogError(log, "IsPackageInstalled: out of memory");
            status = ENOMEM;
        }
        else
        {
            if (NULL != strstr(g_installedPackagesCache, searchTarget))
            {
                OsConfigLogInfo(log, "IsPackageInstalled: '%s' is installed", packageName);
            }
            else
            {
                OsConfigLogInfo(log, "IsPackageInstalled: '%s' is not installed", packageName);
                status = ENOENT;
            }
        }

        FREE_MEMORY(searchTarget);
    }

    return status;
}

static bool WildcardsPresent(const char* packageName)
{
    return (packageName ? (strstr(packageName, "*") || strstr(packageName, "^")) : false);
}

int CheckPackageInstalled(const char* packageName, char** reason, void* log)
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

int CheckPackageNotInstalled(const char* packageName, char** reason, void* log)
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

static int ExecuteSimplePackageCommand(const char* command, bool* executed, void* log)
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

        // Refresh the cache holding the list of the installed packages next time we check
        g_updateInstalledPackagesCache = 1;
    }
    else
    {
        OsConfigLogInfo(log, "ExecuteSimplePackageCommand: '%s' returned %d", command, status);
        *executed = false;
    }

    return status;
}

static int ExecuteAptGetUpdate(void* log)
{
    return ExecuteSimplePackageCommand("apt-get update", &g_aptGetUpdateExecuted, log);
}

static int ExecuteZypperRefresh(void* log)
{
    const char* zypperClean = "zypper clean";
    const char* zypperRefresh = "zypper refresh";
    const char* zypperRefreshServices = "zypper refresh --services";

static int ExecuteZypperRefreshServices(void* log)
{
    int status = 0;

    if (true == g_zypperRefreshExecuted)
    {
        return status;
    }

    if (0 != (status = ExecuteCommand(NULL, zypperClean, false, false, 0, g_packageManagerTimeoutSeconds, NULL, NULL, log)))
    {
        OsConfigLogInfo(log, "ExecuteZypperRefresh: '%s' returned %d", zypperClean, status);
    }
    else if (0 != (status = ExecuteCommand(NULL, zypperRefresh, false, false, 0, g_packageManagerTimeoutSeconds, NULL, NULL, log)))
    {
        OsConfigLogInfo(log, "ExecuteZypperRefresh: '%s' returned %d", zypperRefresh, status);
    }
    else if (0 != (status = ExecuteCommand(NULL, zypperRefreshServices, false, false, 0, g_packageManagerTimeoutSeconds, NULL, NULL, log)))
    {
        OsConfigLogInfo(log, "ExecuteZypperRefresh: '%s' returned %d", zypperRefreshServices, status);
    }

    if (0 == status)
    {
        g_zypperRefreshExecuted = true;
    }

    // Regardless of result, we need to refresh the cache holding the list of installed packages next time we check
    g_updateInstalledPackagesCache = 1;

    return status;
}

static int ExecuteTdnfCheckUpdate(void* log)
{
    return ExecuteSimplePackageCommand("tdnf check-update", &g_tdnfCheckUpdateExecuted, log);
}

static int ExecuteDnfCheckUpdate(void* log)
{
    return ExecuteSimplePackageCommand("dnf check-update", &g_dnfCheckUpdateExecuted, log);
}

static int ExecuteYumCheckUpdate(void* log)
{
    return ExecuteSimplePackageCommand("yum check-update", &g_yumCheckUpdateExecuted, log);
}

int InstallOrUpdatePackage(const char* packageName, void* log)
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
        OsConfigLogInfo(log, "InstallOrUpdatePackage: installation or update of package '%s' returned %d", packageName, status);
    }

    return status;
}

int InstallPackage(const char* packageName, void* log)
{
    int status = ENOENT;

    if (0 != (status = IsPackageInstalled(packageName, log)))
    {
        if (0 == (status = InstallOrUpdatePackage(packageName, log)))
        {
            OsConfigLogInfo(log, "InstallPackage: package '%s' was successfully installed", packageName);
        }
    }
    else
    {
        OsConfigLogInfo(log, "InstallPackage: package '%s' is already installed", packageName);
        status = 0;
    }

    return status;
}

int UninstallPackage(const char* packageName, void* log)
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
            OsConfigLogInfo(log, "UninstallPackage: uninstallation of package '%s' returned %d", packageName, status);
        }
    }
    else if (EINVAL != status)
    {
        OsConfigLogInfo(log, "InstallPackage: package '%s' is not found", packageName);
        status = 0;
    }

    return status;
}