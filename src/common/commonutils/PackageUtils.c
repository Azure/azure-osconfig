// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

/*
List from most important to find to least:

apt-get + dpkg
tdnf
dnf
yum
zypper

Check with:

command -v %s

For each, commands would be (there may be more):

yum
yum install -q %s
yum remove -q %s
yum list installed %s

dnf
dnf install -q %s
dnf remove -q %s
dnf list installed %s

W/o python (better):
tdnf
tdnf install -q %s
tdnf remove -q %s
tdnf list installed %s


apt-get + dpkg
apt-get install %s
apt-get remove -y --purge %s
dpkg -l %s | grep ^ii

zypper
zypper install %s
zypper remove %s
zypper se -x %s
*/

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
        if (0 == (status = ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log))
        {
            OsConfigLogInfo(log, "IsPresent: '%s' is present", what);
        }
        else
        {
            OsConfigLogInfo(log, "IsPresent: '%s' is not present", what);
        }
    }
    else
    {
        OsConfigLogError(log, "IsPresent: FormatAllocateString failed");
        status = ENOMEM;
    }

    return status;
}

static int CheckOrInstallPackage(const char* commandTemplate, const char* packageManager, const char* packageName, void* log)
{
    char* command = NULL;
    int status = ENOENT;

    if ((NULL == commandTemplate) || (NULL == packageManager) || (NULL == packageName) || ((0 == (packageNameLength = strlen(packageName)))))
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
        status = CheckOrInstallPackage(commandTemplateAllElse, packageName, log);
    }
    else if (0 == (status = IsPresent(g_dnf, log)))
    {
        status = CheckOrInstallPackage(commandTemplateAllElse, packageName, log);
    }
    else if (0 == (status = IsPresent(g_yum, log)))
    {
        status = CheckOrInstallPackage(commandTemplateAllElse, packageName, log);
    }
    else if (0 == (status = IsPresent(g_zypper, log)))
    {
        status = CheckOrInstallPackage(commandTemplateZypper, packageName, log);
    }

    if (0 == status)
    {
        OsConfigLogInfo(log, "CheckPackageInstalled: '%s' is installed", packageName);
    }
    else if (EINVAL != status)
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
    }

    return status;
}

int UninstallPackage(const char* packageName, void* log)
{
    const char* commandTemplateAptGet = "%s remove -y --purge %s";
    const char* commandTemplateAllElse = "% remove %s";
    int status = 0;

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

        if ((0 == status) && (0 == (status = CheckPackageInstalled(packageName, log))))
        {
            OsConfigLogInfo(log, "InstallPackage: '%s' was successfully installed", packageName);
        }
        else
        {
            OsConfigLogError(log, "InstallPackage: uninstallation of '%s' failed with %d", packageName, status);
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
    else if (EINVAL != status)
    {
        // Nothing to uninstall
        OsConfigLogInfo(log, "UninstallPackage: '%s' is not installed, nothing to uninstall", packageName);
        status = 0;
    }

    return status;
}