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

int CheckPackageInstalled(const char* packageName, void* log)
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
        OsConfigLogInfo(log, "CheckPackageInstalled: '%s' is installed", packageName);
    }
    else
    {
        OsConfigLogInfo(log, "CheckPackageInstalled: '%s' is not installed", packageName);
    }

    return status;
}

int InstallPackage(const char* packageName, void* log)
{
    const char* commandTemplateAptGet = "%s install -y %s";
    const char* commandTemplateAllElse = "%s install %s";
    int status = ENOENT;

    if (0 != (status = CheckPackageInstalled(packageName, log)))
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

        if ((0 == status) && (0 == (status = CheckPackageInstalled(packageName, log))))
        {
            OsConfigLogInfo(log, "InstallPackage: '%s' was successfully installed", packageName);
        }
        else
        {
            OsConfigLogError(log, "InstallPackage: installation of '%s' failed with %d", packageName, status);
        }
    }
    else
    {
        OsConfigLogInfo(log, "InstallPackage: '%s' is already installed", packageName);
        status = 0;
    }

    return status;
}

int UninstallPackage(const char* packageName, void* log)
{
    const char* commandTemplateAptGet = "%s remove -y --purge %s";
    const char* commandTemplateAllElse = "% remove %s";
    int status = ENOENT;

    if (0 == (status = CheckPackageInstalled(packageName, log)))
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

        if ((0 == status) && (0 == CheckPackageInstalled(packageName, log)))
        {
            status = ENOENT;
        }
        
        if (0 == status)
        {
            OsConfigLogInfo(log, "UninstallPackage: '%s' was successfully uninstalled", packageName);
        }
        else
        {
            OsConfigLogError(log, "UninstallPackage: uninstallation of '%s' failed with %d", packageName, status);
        }
    }
    else
    {
        OsConfigLogInfo(log, "InstallPackage: '%s' is not found", packageName);
    }

    return status;
}