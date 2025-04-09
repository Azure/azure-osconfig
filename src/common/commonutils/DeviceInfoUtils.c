// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

typedef struct OsDistroInfo
{
    char* id;
    char* release;
    char* codename;
    char* description;
} OsDistroInfo;

static bool g_selinuxPresent = false;

void RemovePrefix(char* target, char marker)
{
    size_t targetLength = 0, i = 0;

    if ((NULL == target) || (0 == (targetLength = strlen(target))))
    {
        return;
    }

    while ((i < targetLength) && (marker == target[i]))
    {
        i += 1;
    }

    memmove(target, target + i, targetLength - i);
    target[targetLength - i] = 0;
}

void RemovePrefixBlanks(char* target)
{
    RemovePrefix(target, ' ');
}

void RemovePrefixUpTo(char* target, char marker)
{
    char markerString[2] = {marker, 0};
    RemovePrefixUpToString(target, markerString);
}

void RemovePrefixUpToString(char* target, const char* marker)
{
    size_t targetLength = 0, markerLength = 0;
    char* found = NULL;

    if ((NULL == target) || (0 == (targetLength = strlen(target))) ||
        (NULL == marker) || (0 == (markerLength = strlen(marker))) ||
        (markerLength >= targetLength))
    {
        return;
    }

    if (NULL != (found = strstr(target, marker)))
    {
        targetLength = strlen(found);
        memmove(target, found, targetLength);
        target[targetLength] = 0;
    }
}

void RemoveTrailingBlanks(char* target)
{
    int i = 0;

    if (NULL == target)
    {
        return;
    }

    i = (int)strlen(target);

    while ((i > 0) && (' ' == target[i - 1]))
    {
        target[i - 1] = 0;
        i -= 1;
    }
}

void TruncateAtFirst(char* target, char marker)
{
    char* found = NULL;

    if (NULL == target)
    {
        return;
    }

    if (NULL != (found = strchr(target, marker)))
    {
        found[0] = 0;
    }
}

char* GetOsPrettyName(OsConfigLogHandle log)
{
    const char* osPrettyNameCommand = "cat /etc/os-release | grep PRETTY_NAME=";
    char* textResult = NULL;

    if ((0 == ExecuteCommand(NULL, osPrettyNameCommand, true, true, 0, 0, &textResult, NULL, log)) && textResult)
    {
        RemovePrefixUpTo(textResult, '=');
        RemovePrefix(textResult, '=');
        RemovePrefixBlanks(textResult);
        RemoveTrailingBlanks(textResult);
    }
    else
    {
        FREE_MEMORY(textResult);
    }

    OsConfigLogDebug(log, "OS pretty name: '%s'", textResult);

    return textResult;
}

char* GetOsName(OsConfigLogHandle log)
{
    const char* osNameCommand = "cat /etc/os-release | grep ID=";
    char* textResult = NULL;

    if (NULL != (textResult = GetOsPrettyName(log)))
    {
        // Comment next line to capture the full pretty name including version (example: 'Ubuntu 20.04.3 LTS')
        TruncateAtFirst(textResult, ' ');
    }
    else
    {
        // PRETTY_NAME did not work, try ID
        if ((0 == ExecuteCommand(NULL, osNameCommand, true, true, 0, 0, &textResult, NULL, log)) && textResult)
        {
            RemovePrefixUpTo(textResult, '=');
            RemovePrefix(textResult, '=');
            TruncateAtFirst(textResult, ' ');
            RemovePrefixBlanks(textResult);
            RemoveTrailingBlanks(textResult);
        }
        else
        {
            FREE_MEMORY(textResult);
        }
    }

    OsConfigLogDebug(log, "OS name: '%s'", textResult);

    return textResult;
}

char* GetOsVersion(OsConfigLogHandle log)
{
    const char* osVersionCommand = "cat /etc/os-release | grep VERSION=";
    char* textResult = NULL;

    if ((0 == ExecuteCommand(NULL, osVersionCommand, true, true, 0, 0, &textResult, NULL, log)) && textResult)
    {
        RemovePrefixUpTo(textResult, '=');
        TruncateAtFirst(textResult, '=');
        TruncateAtFirst(textResult, ' ');
        RemovePrefixBlanks(textResult);
        RemoveTrailingBlanks(textResult);
    }
    else
    {
        FREE_MEMORY(textResult);
    }

    OsConfigLogDebug(log, "OS version: '%s'", textResult);

    return textResult;
}

static char* GetHardwareProperty(const char* command, bool truncateAtFirstSpace, OsConfigLogHandle log)
{
    char* textResult = NULL;

    if ((0 == ExecuteCommand(NULL, command, true, true, 0, 0, &textResult, NULL, log)) && textResult)
    {
        RemovePrefixUpTo(textResult, ':');
        RemovePrefix(textResult, ':');
        RemovePrefixBlanks(textResult);

        if (truncateAtFirstSpace)
        {
            TruncateAtFirst(textResult, ' ');
        }
        else
        {
            RemoveTrailingBlanks(textResult);
        }
    }
    else
    {
        FREE_MEMORY(textResult);
    }

    return textResult;
}

static char* GetAnotherOsProperty(const char* command, OsConfigLogHandle log)
{
    char* textResult = NULL;

    if (NULL == command)
    {
        return NULL;
    }

    if ((0 == ExecuteCommand(NULL, command, true, true, 0, 0, &textResult, NULL, log)) && textResult)
    {
        RemovePrefixBlanks(textResult);
        RemoveTrailingBlanks(textResult);
    }
    else
    {
        FREE_MEMORY(textResult);
    }

    return textResult;
}

char* GetOsKernelName(OsConfigLogHandle log)
{
    static char* osKernelNameCommand = "uname -s";
    char* textResult = GetAnotherOsProperty(osKernelNameCommand, log);
    OsConfigLogDebug(log, "Kernel name: '%s'", textResult);
    return textResult;
}

char* GetOsKernelRelease(OsConfigLogHandle log)
{
    static char* osKernelReleaseCommand = "uname -r";
    char* textResult = GetAnotherOsProperty(osKernelReleaseCommand, log);
    OsConfigLogDebug(log, "Kernel release: '%s'", textResult);
    return textResult;
}

char* GetOsKernelVersion(OsConfigLogHandle log)
{
    static char* osKernelVersionCommand = "uname -v";
    char* textResult = GetAnotherOsProperty(osKernelVersionCommand, log);
    OsConfigLogDebug(log, "Kernel version: '%s'", textResult);
    return textResult;
}

char* GetCpuType(OsConfigLogHandle log)
{
    const char* osCpuTypeCommand = "lscpu | grep Architecture:";
    char* textResult = GetHardwareProperty(osCpuTypeCommand, false, log);
    OsConfigLogDebug(log, "CPU type: '%s'", textResult);
    return textResult;
}

char* GetCpuVendor(OsConfigLogHandle log)
{
    const char* osCpuVendorCommand = "grep 'vendor_id' /proc/cpuinfo | uniq";
    char* textResult = GetHardwareProperty(osCpuVendorCommand, false, log);
    OsConfigLogDebug(log, "CPU vendor id: '%s'", textResult);
    return textResult;
}

char* GetCpuModel(OsConfigLogHandle log)
{
    const char* osCpuModelCommand = "grep 'model name' /proc/cpuinfo | uniq";
    char* textResult = GetHardwareProperty(osCpuModelCommand, false, log);
    OsConfigLogDebug(log, "CPU model: '%s'", textResult);
    return textResult;
}

unsigned int GetNumberOfCpuCores(OsConfigLogHandle log)
{
    const char* osCpuCoresCommand = "grep -c ^processor /proc/cpuinfo";
    unsigned int numberOfCores = 1;
    char* textResult = NULL;

    if (NULL != (textResult = GetHardwareProperty(osCpuCoresCommand, false, log)))
    {
        numberOfCores = atoi(textResult);
    }

    OsConfigLogDebug(log, "Number of CPU cores: %u ('%s')", numberOfCores, textResult);

    FREE_MEMORY(textResult);

    return numberOfCores;
}

char* GetCpuFlags(OsConfigLogHandle log)
{
    const char* osCpuFlagsCommand = "lscpu | grep \"Flags:\"";
    char* textResult = GetHardwareProperty(osCpuFlagsCommand, false, log);
    OsConfigLogDebug(log, "CPU flags: '%s'", textResult);
    return textResult;
}

bool CheckCpuFlagSupported(const char* cpuFlag, char** reason, OsConfigLogHandle log)
{
    bool result = false;
    char* cpuFlags = GetCpuFlags(log);

    if ((NULL != cpuFlag) && (NULL != cpuFlags) && (NULL != strstr(cpuFlags, cpuFlag)))
    {
        OsConfigLogInfo(log, "CPU flag '%s' is supported", cpuFlag);
        OsConfigCaptureSuccessReason(reason, "The device's CPU supports '%s'", cpuFlag);
        result = true;
    }
    else
    {
        OsConfigLogInfo(log, "CPU flag '%s' is not supported", cpuFlag);
        OsConfigCaptureReason(reason, "The device's CPU does not support '%s'", cpuFlag);
    }

    FREE_MEMORY(cpuFlags);

    return result;
}

long GetTotalMemory(OsConfigLogHandle log)
{
    const char* osTotalMemoryCommand = "grep MemTotal /proc/meminfo";
    char* textResult = GetHardwareProperty(osTotalMemoryCommand, true, log);
    long totalMemory = 0;

    if (NULL != textResult)
    {
        totalMemory = atol(textResult);
        FREE_MEMORY(textResult);
    }

    OsConfigLogDebug(log, "Total memory: %lu kB", totalMemory);

    return totalMemory;
}

long GetFreeMemory(OsConfigLogHandle log)
{
    const char* osFreeMemoryCommand = "grep MemFree /proc/meminfo";
    char* textResult = GetHardwareProperty(osFreeMemoryCommand, true, log);
    long freeMemory = 0;

    if (NULL != textResult)
    {
        freeMemory = atol(textResult);
        FREE_MEMORY(textResult);
    }

    OsConfigLogDebug(log, "Free memory: %lu kB", freeMemory);

    return freeMemory;
}

char* GetProductName(OsConfigLogHandle log)
{
    const char* osProductNameCommand = "cat /sys/devices/virtual/dmi/id/product_name";
    const char* osProductNameAlternateCommand = "lshw -c system | grep -m 1 \"product:\"";
    char* textResult = GetAnotherOsProperty(osProductNameCommand, log);

    if ((NULL == textResult) || (0 == strlen(textResult)))
    {
        FREE_MEMORY(textResult);
        textResult = GetHardwareProperty(osProductNameAlternateCommand, false, log);
    }

    OsConfigLogDebug(log, "Product name: '%s'", textResult);

    return textResult;
}

char* GetProductVendor(OsConfigLogHandle log)
{
    const char* osProductVendorCommand = "cat /sys/devices/virtual/dmi/id/sys_vendor";
    const char* osProductVendorAlternateCommand = "lshw -c system | grep -m 1 \"vendor:\"";
    char* textResult = GetAnotherOsProperty(osProductVendorCommand, log);

    if ((NULL == textResult) || (0 == strlen(textResult)))
    {
        FREE_MEMORY(textResult);
        textResult = GetHardwareProperty(osProductVendorAlternateCommand, false, log);
    }

    OsConfigLogDebug(log, "Product vendor: '%s'", textResult);

    return textResult;
}

char* GetProductVersion(OsConfigLogHandle log)
{
    const char* osProductVersionCommand = "cat /sys/devices/virtual/dmi/id/product_version";
    const char* osProductVersionAlternateCommand = "lshw -c system | grep -m 1 \"version:\"";
    char* textResult = GetHardwareProperty(osProductVersionCommand, false, log);

    if ((NULL == textResult) || (0 == strlen(textResult)))
    {
        FREE_MEMORY(textResult);
        textResult = GetHardwareProperty(osProductVersionAlternateCommand, false, log);
    }

    OsConfigLogDebug(log, "Product version: '%s'", textResult);

    return textResult;
}

char* GetSystemCapabilities(OsConfigLogHandle log)
{
    const char* osSystemCapabilitiesCommand = "lshw -c system | grep -m 1 \"capabilities:\"";
    char* textResult = GetHardwareProperty(osSystemCapabilitiesCommand, false, log);
    OsConfigLogDebug(log, "Product capabilities: '%s'", textResult);
    return textResult;
}

char* GetSystemConfiguration(OsConfigLogHandle log)
{
    const char* osSystemConfigurationCommand = "lshw -c system | grep -m 1 \"configuration:\"";
    char* textResult = GetHardwareProperty(osSystemConfigurationCommand, false, log);
    OsConfigLogDebug(log, "Product configuration: '%s'", textResult);
    return textResult;
}

static char* GetOsReleaseEntry(const char* commandTemplate, const char* name, char separator, OsConfigLogHandle log)
{
    char* command = NULL;
    char* result = NULL;
    size_t commandLength = 0;

    if ((NULL == commandTemplate) || (NULL == name) || (0 == strlen(name)))
    {
        OsConfigLogError(log, "GetOsReleaseEntry: invalid arguments");
        result = DuplicateString("<error>");
    }
    else
    {
        commandLength = strlen(commandTemplate) + strlen(name) + 1;

        if (NULL == (command = malloc(commandLength)))
        {
            OsConfigLogError(log, "GetOsReleaseEntry: out of memory");
        }
        else
        {
            memset(command, 0, commandLength);
            snprintf(command, commandLength, commandTemplate, name);

            if ((0 == ExecuteCommand(NULL, command, true, false, 0, 0, &result, NULL, log)) && result)
            {
                RemovePrefixBlanks(result);
                RemoveTrailingBlanks(result);
                RemovePrefixUpTo(result, separator);
                RemovePrefix(result, separator);
                RemovePrefixBlanks(result);

                if ('"' == result[0])
                {
                    RemovePrefixUpTo(result, '"');
                    RemovePrefix(result, '"');
                    TruncateAtFirst(result, '"');
                }
            }
            else
            {
                FREE_MEMORY(result);
            }

            FREE_MEMORY(command);
        }
    }

    if (NULL == result)
    {
        result = DuplicateString("<null>");
    }

    OsConfigLogDebug(log, "'%s': '%s'", name, result);

    return result;
}

static char* GetEtcReleaseEntry(const char* name, OsConfigLogHandle log)
{
    return GetOsReleaseEntry("cat /etc/*-release | grep %s=", name, '=', log);
}

static char* GetLsbReleaseEntry(const char* name, OsConfigLogHandle log)
{
    return GetOsReleaseEntry("lsb_release -a | grep \"%s:\"", name, ':', log);
}

static void ClearOsDistroInfo(OsDistroInfo* info)
{
    if (info)
    {
        FREE_MEMORY(info->id);
        FREE_MEMORY(info->release);
        FREE_MEMORY(info->codename);
        FREE_MEMORY(info->description);
    }
}

bool CheckOsAndKernelMatchDistro(char** reason, OsConfigLogHandle log)
{
    const char* linuxName = "Linux";
    const char* ubuntuName = "Ubuntu";
    const char* debianName = "Debian";
    const char* none = "<null>";

    OsDistroInfo distro = {0}, os = {0};
    char* kernelName = GetOsKernelName(log);
    char* kernelVersion = GetOsKernelVersion(log);
    bool match = false;

    // Distro

    distro.id = GetEtcReleaseEntry("DISTRIB_ID", log);
    if (0 == strcmp(distro.id, none))
    {
        FREE_MEMORY(distro.id);
        distro.id = GetLsbReleaseEntry("Distributor ID", log);
    }

    distro.release = GetEtcReleaseEntry("DISTRIB_RELEASE", log);
    if (0 == strcmp(distro.release, none))
    {
        FREE_MEMORY(distro.release);
        distro.release = GetLsbReleaseEntry("Release", log);
    }

    distro.codename = GetEtcReleaseEntry("DISTRIB_CODENAME", log);
    if (0 == strcmp(distro.codename, none))
    {
        FREE_MEMORY(distro.codename);
        distro.codename = GetLsbReleaseEntry("Codename", log);
    }

    distro.description = GetEtcReleaseEntry("DISTRIB_DESCRIPTION", log);
    if (0 == strcmp(distro.description, none))
    {
        FREE_MEMORY(distro.description);
        distro.description = GetLsbReleaseEntry("Description", log);
    }

    // installed OS

    os.id = GetEtcReleaseEntry("-w NAME", log);
    os.release = GetEtcReleaseEntry("VERSION_ID", log);
    os.codename = GetEtcReleaseEntry("VERSION_CODENAME", log);
    os.description = GetEtcReleaseEntry("PRETTY_NAME", log);

    if (IsCurrentOs(ubuntuName, log) || IsCurrentOs(debianName, log))
    {
        if ((0 == strncmp(distro.id, os.id, strlen(distro.id))) &&
            (0 == strcmp(distro.release, os.release)) &&
            (0 == strcmp(distro.codename, os.codename)) &&
            (0 == strcmp(distro.description, os.description)) &&
            (0 == strcmp(kernelName, linuxName)))
        {
            OsConfigLogInfo(log, "CheckOsAndKernelMatchDistro: distro and installed image match ('%s', '%s', '%s', '%s', '%s')",
                distro.id, distro.release, distro.codename, distro.description, kernelName);
            OsConfigCaptureSuccessReason(reason, "Distro and installed image match ('%s', '%s', '%s', '%s', '%s')",
                distro.id, distro.release, distro.codename, distro.description, kernelName);
            match = true;
        }
        else
        {
            OsConfigLogInfo(log, "CheckOsAndKernelMatchDistro: distro ('%s', '%s', '%s', '%s', '%s') and installed image ('%s', '%s', '%s', '%s', '%s') do not match",
                distro.id, distro.release, distro.codename, distro.description, linuxName, os.id, os.release, os.codename, os.description, kernelName);
            OsConfigCaptureReason(reason, "Distro ('%s', '%s', '%s', '%s', '%s') and installed image ('%s', '%s', '%s', '%s', '%s') do not match, automatic remediation is not possible",
                distro.id, distro.release, distro.codename, distro.description, linuxName, os.id, os.release, os.codename, os.description, kernelName);
        }
    }
    else
    {
        if (0 == strcmp(kernelName, linuxName))
        {
            OsConfigLogInfo(log, "CheckOsAndKernelMatchDistro: distro and installed image match ('%s', '%s')", kernelName, kernelVersion);
            OsConfigCaptureSuccessReason(reason, "Distro and installed image match ('%s', '%s')", kernelName, kernelVersion);
            match = true;
        }
        else
        {
            OsConfigLogInfo(log, "CheckOsAndKernelMatchDistro: distro ('%s') and installed image ('%s', '%s') do not match", linuxName, kernelName, kernelVersion);
            OsConfigCaptureReason(reason, "Distro ('%s') and installed image ('%s', '%s') do not match, automatic remediation is not possible", linuxName, kernelName, kernelVersion);
        }
    }

    FREE_MEMORY(kernelName);
    FREE_MEMORY(kernelVersion);

    ClearOsDistroInfo(&distro);
    ClearOsDistroInfo(&os);

    return match;
}

char* GetLoginUmask(char** reason, OsConfigLogHandle log)
{
    const char* command = "grep -v '^#' /etc/login.defs | grep UMASK";
    char* result = NULL;

    if ((0 == ExecuteCommand(NULL, command, true, true, 0, 0, &result, NULL, log)) && result)
    {
        RemovePrefixUpTo(result, ' ');
        RemovePrefixBlanks(result);
        RemoveTrailingBlanks(result);
    }
    else
    {
        OsConfigCaptureReason(reason, "'%s' failed, cannot check the current login UMASK", command);
        FREE_MEMORY(result);
    }

    OsConfigLogDebug(log, "UMASK: '%s'", result);

    return result;
}

int CheckLoginUmask(const char* desired, char** reason, OsConfigLogHandle log)
{
    char* current = NULL;
    size_t length = 0;
    int status = 0;

    if ((NULL == desired) || (0 == (length = strlen(desired))))
    {
        OsConfigLogError(log, "CheckLoginUmask: invalid argument");
        return EINVAL;
    }

    if (NULL == (current = GetLoginUmask(reason, log)))
    {
        OsConfigLogInfo(log, "CheckLoginUmask: GetLoginUmask failed");
        status = ENOENT;
    }
    else
    {
        if (0 == strncmp(desired, current, length))
        {
            OsConfigLogInfo(log, "CheckLoginUmask: current login UMASK '%s' matches desired '%s'", current, desired);
            OsConfigCaptureSuccessReason(reason, "'%s' (current login UMASK) matches desired '%s'", current, desired);
        }
        else
        {
            OsConfigLogInfo(log, "CheckLoginUmask: current login UMASK '%s' does not match desired '%s'", current, desired);
            OsConfigCaptureReason(reason, "Current login UMASK '%s' does not match desired '%s'", current, desired);
            status = ENOENT;
        }

        FREE_MEMORY(current);
    }

    return status;
}

static long GetPasswordDays(const char* name, OsConfigLogHandle log)
{
    const char* commandTemplate = "cat /etc/login.defs | grep %s | grep -v ^#";
    size_t commandLength = 0;
    char* command = NULL;
    char* result = NULL;
    long days = -1;

    if ((NULL == name) || (0 == strlen(name)))
    {
        OsConfigLogError(log, "GetPasswordDays: invalid argument");
        return -1;
    }

    commandLength = strlen(commandTemplate) + strlen(name) + 1;

    if (NULL == (command = malloc(commandLength)))
    {
        OsConfigLogError(log, "GetPasswordDays: out of memory");
    }
    else
    {
        memset(command, 0, commandLength);
        snprintf(command, commandLength, commandTemplate, name);

        if ((0 == ExecuteCommand(NULL, command, true, false, 0, 0, &result, NULL, log)) && result)
        {
            RemovePrefixBlanks(result);
            RemovePrefixUpTo(result, ' ');
            RemovePrefixBlanks(result);
            RemoveTrailingBlanks(result);

            days = atol(result);
        }

        FREE_MEMORY(result);
        FREE_MEMORY(command);
    }

    OsConfigLogDebug(log, "%s: %ld", name, days);

    return days;
}

long GetPassMinDays(OsConfigLogHandle log)
{
    return GetPasswordDays("PASS_MIN_DAYS", log);
}

long GetPassMaxDays(OsConfigLogHandle log)
{
    return GetPasswordDays("PASS_MAX_DAYS", log);
}

long GetPassWarnAge(OsConfigLogHandle log)
{
    return GetPasswordDays("PASS_WARN_AGE", log);
}

static int SetPasswordDays(const char* name, long days, OsConfigLogHandle log)
{
    const char* etcLoginDefs = "/etc/login.defs";
    char* value = NULL;
    long currentDays = -1;
    int status = 0;

    if ((NULL == name) || (0 == strlen(name)))
    {
        OsConfigLogError(log, "SetPasswordDays: invalid argument");
        return EINVAL;
    }
    else if (NULL == (value = FormatAllocateString("%ld", days)))
    {
        OsConfigLogError(log, "SetPasswordDays: out of memory");
        return ENOMEM;
    }

    if (days == (currentDays = GetPasswordDays(name, log)))
    {
        OsConfigLogInfo(log, "SetPasswordDays: '%s' already set to %ld days in '%s'", name, days, etcLoginDefs);
    }
    else
    {
        OsConfigLogInfo(log, "SetPasswordDays: '%s' is set to %ld days in '%s' instead of %ld days", name, currentDays, etcLoginDefs, days);
        if (0 == (status = SetEtcLoginDefValue(name, value, log)))
        {
            OsConfigLogInfo(log, "SetPasswordDays: '%s' is now set to %ld days in '%s'", name, days, etcLoginDefs);
        }
    }

    FREE_MEMORY(value);

    return status;
}

int SetPassMinDays(long days, OsConfigLogHandle log)
{
    return SetPasswordDays("PASS_MIN_DAYS", days, log);
}

int SetPassMaxDays(long days, OsConfigLogHandle log)
{
    return SetPasswordDays("PASS_MAX_DAYS", days, log);
}

int SetPassWarnAge(long days, OsConfigLogHandle log)
{
    return SetPasswordDays("PASS_WARN_AGE", days, log);
}

bool IsCurrentOs(const char* name, OsConfigLogHandle log)
{
    char* prettyName = NULL;
    size_t prettyNameLength = 0;
    size_t nameLength = 0;
    bool result = false;

    if ((NULL == name) || (0 == (nameLength = strlen(name))))
    {
        OsConfigLogError(log, "IsCurrentOs called with an invalid argument");
        return result;
    }

    if ((NULL == (prettyName = GetOsPrettyName(log))) || (0 == (prettyNameLength = strlen(prettyName))))
    {
        OsConfigLogInfo(log, "IsCurrentOs: no valid PRETTY_NAME found in /etc/os-release, assuming this is not the '%s' distro", name);
    }
    else
    {
        if (true == (result = (0 == strncmp(name, prettyName, ((nameLength <= prettyNameLength) ? nameLength : prettyNameLength) ? true : false))))
        {
            OsConfigLogInfo(log, "Running on '%s' ('%s')", name, prettyName);
        }
        else
        {
            OsConfigLogInfo(log, "Not running on '%s' ('%s')", name, prettyName);
        }
    }

    FREE_MEMORY(prettyName);

    return result;
}

static bool IsRedHatBasedInternal(OsConfigLogHandle log)
{
    const char* distros[] = {"Red Hat", "CentOS", "AlmaLinux", "Rocky Linux", "Oracle Linux"};
    int numDistros = ARRAY_SIZE(distros);
    char* prettyName = NULL;
    size_t prettyNameLength = 0;
    size_t nameLength = 0;
    int i = 0;
    bool result = false;

    if ((NULL == (prettyName = GetOsPrettyName(log))) || (0 == (prettyNameLength = strlen(prettyName))))
    {
        OsConfigLogInfo(log, "IsRedHatBased: no valid PRETTY_NAME found in /etc/os-release, cannot check if Red Hat based, assuming not");
    }
    else
    {
        for (i = 0; i < numDistros; i++)
        {
            nameLength = strlen(distros[i]);
            if (true == (result = (0 == strncmp(distros[i], prettyName, ((nameLength <= prettyNameLength) ? nameLength : prettyNameLength) ? true : false))))
            {
                if (0 == i)
                {
                    OsConfigLogInfo(log, "Running on '%s' which is Red Hat", prettyName);
                }
                else
                {
                    OsConfigLogInfo(log, "Running on '%s' which is Red Hat based", prettyName);
                }
                break;
            }
        }

        if (false == result)
        {
            OsConfigLogInfo(log, "Running on '%s' which is not Red Hat based", prettyName);
        }
    }

    FREE_MEMORY(prettyName);

    return result;
}

bool IsRedHatBased(OsConfigLogHandle log)
{
    static bool firstTime = true;
    static bool redHatBased = false;
    if (firstTime)
    {
        redHatBased = IsRedHatBasedInternal(log);
        firstTime = false;
    }
    return redHatBased;
}

int EnableVirtualMemoryRandomization(OsConfigLogHandle log)
{
    const char* procSysKernelRandomizeVaSpace = "/proc/sys/kernel/randomize_va_space";
    const char* fullRandomization = "2";
    int status = 0;

    if (0 == CheckSmallFileContainsText(procSysKernelRandomizeVaSpace, fullRandomization, NULL, log))
    {
        OsConfigLogInfo(log, "EnableVirtualMemoryRandomization: full virtual memory randomization '%s' is already enabled in '%s'", fullRandomization, procSysKernelRandomizeVaSpace);
    }
    else
    {
        if (SavePayloadToFile(procSysKernelRandomizeVaSpace, fullRandomization, strlen(fullRandomization), log))
        {
            OsConfigLogInfo(log, "EnableVirtualMemoryRandomization: '%s' was written to '%s'", fullRandomization, procSysKernelRandomizeVaSpace);
        }
        else
        {
            OsConfigLogInfo(log, "EnableVirtualMemoryRandomization: cannot write '%s' to '%s' (%d)", fullRandomization, procSysKernelRandomizeVaSpace, errno);
            status = ENOENT;
        }
    }

    return status;
}

bool IsCommodore(OsConfigLogHandle log)
{
    const char* productNameCommand = "cat /etc/os-subrelease | grep PRODUCT_NAME=";
    char* textResult = NULL;
    bool status = false;

    if ((0 == ExecuteCommand(NULL, productNameCommand, true, false, 0, 0, &textResult, NULL, log)) && textResult)
    {
        RemovePrefixBlanks(textResult);
        RemoveTrailingBlanks(textResult);
        RemovePrefixUpTo(textResult, '=');
        RemovePrefix(textResult, '=');
        RemovePrefixBlanks(textResult);

        if (0 == strcmp(textResult, PRODUCT_NAME_AZURE_COMMODORE))
        {
            status = true;
        }
    }

    FREE_MEMORY(textResult);

    return status;
}

bool IsSelinuxPresent(void)
{
    return g_selinuxPresent;
}

bool DetectSelinux(OsConfigLogHandle log)
{
    const char* command = "cat /sys/kernel/security/lsm | grep selinux";
    bool status = false;

    if (0 == ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, log))
    {
        status = true;
    }

    g_selinuxPresent = status;

    return status;
}
