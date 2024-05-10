// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

const char* g_aptGet = "apt-get";
const char* g_dpkg = "dpkg";
const char* g_tdnf = "tdnf";
const char* g_dnf = "dnf";
const char* g_yum = "yum";
const char* g_zypper = "zypper";

static int IsPresent(const char* what, void* log)
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
        status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log);
    }
    else
    {
        OsConfigLogError(log, "IsPresent: FormatAllocateString failed");
        status = ENOMEM;
    }

    FREE_MEMORY(command);

    return status;
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

    status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log);

    FREE_MEMORY(command);

    return status;
}

int IsPackageInstalled(const char* packageName, void* log)
{
    const char* commandTemplateDpkg = "%s -l %s | grep ^ii";
    const char* commandTemplateAllElse = "%s list installed %s";
    const char* commandTemplateZypper = "%s se -x %s";
    int status = ENOENT;

    if (0 == (status = IsPresent(g_dpkg, log)))
    {
        status = CheckOrInstallPackage(commandTemplateDpkg, g_dpkg, packageName, log);
    }
    else if (0 == (status = IsPresent(g_tdnf, log)))
    {
        status = CheckOrInstallPackage(commandTemplateAllElse, g_tdnf, packageName, log);
    }
    else if (0 == (status = IsPresent(g_dnf, log)))
    {
        status = CheckOrInstallPackage(commandTemplateAllElse, g_dnf, packageName, log);
    }
    else if (0 == (status = IsPresent(g_yum, log)))
    {
        status = CheckOrInstallPackage(commandTemplateAllElse, g_yum, packageName, log);
    }
    else if (0 == (status = IsPresent(g_zypper, log)))
    {
        status = CheckOrInstallPackage(commandTemplateZypper, g_zypper, packageName, log);
    }

    if (0 == status)
    {
        OsConfigLogInfo(log, "IsPackageInstalled: package '%s' is installed", packageName);
    }
    else
    {
        OsConfigLogInfo(log, "IsPackageInstalled: package '%s' is not found", packageName);
    }

    return status;
}

int CheckPackageInstalled(const char* packageName, char** reason, void* log)
{
    int result = 0; 
    bool wildcards = packageName ? (strstr(packageName, "*") || strstr(packageName, "^")) : false;
    
    OsConfigLogInfo(log, "CheckPackageInstalled: package '%s' has wildcards: %s, '*': %s, '^': %s", packageName, 
        (wildcards ? "yes" : "no"), (strstr(packageName, "*") ? "yes" : "no"), (strstr(packageName, "^") : "yes" : "no"));

    if (0 == (result = IsPackageInstalled(packageName, log)))
    {
        OsConfigCaptureSuccessReason(reason, wildcards ? "Some '%s' packages are installed" : "Package '%s' is installed", packageName);
    }
    else if ((EINVAL != result) && (ENOMEM != result))
    {
        OsConfigCaptureReason(reason, wildcards ? "No '%s' packages are installed" : "Package '%s' is not installed", packageName);
    }

    return result;
}

int CheckPackageNotInstalled(const char* packageName, char** reason, void* log)
{
    int result = 0;

    if (0 == (result = IsPackageInstalled(packageName, log)))
    {
        OsConfigCaptureReason(reason, "Package '%s' is installed", packageName);
        result = ENOENT;
    }
    else if ((EINVAL != result) && (ENOMEM != result))
    {
        OsConfigCaptureSuccessReason(reason, "Package '%s' is not installed", packageName);
        result = 0;
    }

    return result;
}

int InstallOrUpdatePackage(const char* packageName, void* log)
{
    const char* commandTemplate = "%s install -y %s";
    int status = ENOENT;

    if (0 == (status = IsPresent(g_aptGet, log)))
    {
        status = CheckOrInstallPackage(commandTemplate, g_aptGet, packageName, log);
    }
    else if (0 == (status = IsPresent(g_tdnf, log)))
    {
        status = CheckOrInstallPackage(commandTemplate, g_tdnf, packageName, log);
    }
    else if (0 == (status = IsPresent(g_dnf, log)))
    {
        status = CheckOrInstallPackage(commandTemplate, g_dnf, packageName, log);
    }
    else if (0 == (status = IsPresent(g_yum, log)))
    {
        status = CheckOrInstallPackage(commandTemplate, g_yum, packageName, log);
    }
    else if (0 == (status = IsPresent(g_zypper, log)))
    {
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
        OsConfigLogError(log, "InstallOrUpdatePackage: installation or update of package '%s' failed with %d", packageName, status);
    }

    return status;
}

int InstallPackage(const char* packageName, void* log)
{
    int status = ENOENT;

    if (0 != (status = IsPackageInstalled(packageName, log)))
    {
        InstallOrUpdatePackage(packageName, log);
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
    const char* commandTemplateAllElse = "% remove -y %s";
    int status = ENOENT;

    if (0 == (status = IsPackageInstalled(packageName, log)))
    {
        if (0 == (status = IsPresent(g_aptGet, log)))
        {
            status = CheckOrInstallPackage(commandTemplateAptGet, g_aptGet, packageName, log);
        }
        else if (0 == (status = IsPresent(g_tdnf, log)))
        {
            status = CheckOrInstallPackage(commandTemplateAllElse, g_tdnf, packageName, log);
        }
        else if (0 == (status = IsPresent(g_dnf, log)))
        {
            status = CheckOrInstallPackage(commandTemplateAllElse, g_dnf, packageName, log);
        }
        else if (0 == (status = IsPresent(g_yum, log)))
        {
            status = CheckOrInstallPackage(commandTemplateAllElse, g_yum, packageName, log);
        }
        else if (0 == (status = IsPresent(g_zypper, log)))
        {
            status = CheckOrInstallPackage(commandTemplateAllElse, g_zypper, packageName, log);
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
            OsConfigLogError(log, "UninstallPackage: uninstallation of package '%s' failed with %d", packageName, status);
        }
    }
    else if (EINVAL != status)
    {
        OsConfigLogInfo(log, "InstallPackage: package '%s' is not found", packageName);
        status = 0;
    }

    return status;
}