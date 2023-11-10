// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

typedef struct OS_DISTRO_INFO
{
    char* id;
    char* release;
    char* codename;
    char* description;
} OS_DISTRO_INFO;

void RemovePrefixBlanks(char* target)
{
    if (NULL == target)
    {
        return;
    }

    int targetLength =(int)strlen(target);
    int i = 0;
    
    while ((i < targetLength) && (' ' == target[i]))
    {
        i += 1;
    }

    memcpy(target, target + i, targetLength - i);
    target[targetLength - i] = 0;
}

void RemovePrefixUpTo(char* target, char marker)
{
    if (NULL == target)
    {
        return;
    }

    int targetLength =(int)strlen(target);
    char* equalSign = strchr(target, marker);

    if (equalSign)
    {
        targetLength = strlen(equalSign + 1);
        memcpy(target, equalSign + 1, targetLength);
        target[targetLength] = 0;
    }
}

void RemovePrefixUpToString(char* target, const char* marker)
{
    if ((NULL == target) || (NULL == marker))
    {
        return;
    }

    int targetLength = 0;
    char* equalSign = strstr(target, marker);

    if (equalSign)
    {
        targetLength = (int)strlen(equalSign);
        memcpy(target, equalSign, targetLength);
        target[targetLength] = 0;
    }
}

void RemoveTrailingBlanks(char* target)
{
    if (NULL == target)
    {
        return;
    }

    int targetLength = (int)strlen(target);
    int i = targetLength;

    while ((i > 0) && (' ' == target[i - 1]))
    {
        target[i - 1] = 0;
        i -= 1;
    }
}

void TruncateAtFirst(char* target, char marker)
{
    if (NULL == target)
    {
        return;
    }

    char* found = strchr(target, marker);

    if (found)
    {
        found[0] = 0;
    }
}

char* GetOsName(void* log)
{
    const char* osNameCommand = "cat /etc/os-release | grep ID=";
    const char* osPrettyNameCommand = "cat /etc/os-release | grep PRETTY_NAME=";
    char* textResult = NULL;

    if (0 == ExecuteCommand(NULL, osPrettyNameCommand, true, true, 0, 0, &textResult, NULL, log))
    {
        RemovePrefixBlanks(textResult);
        RemoveTrailingBlanks(textResult);
        RemovePrefixUpTo(textResult, '=');
        RemovePrefixBlanks(textResult);
        
        // Comment next line to capture the full pretty name including version (example: 'Ubuntu 20.04.3 LTS')
        TruncateAtFirst(textResult, ' ');
    }
    else
    {
        FREE_MEMORY(textResult);

        // PRETTY_NAME did not work, try ID
        if (0 == ExecuteCommand(NULL, osNameCommand, true, true, 0, 0, &textResult, NULL, log))
        {
            RemovePrefixBlanks(textResult);
            RemoveTrailingBlanks(textResult);
            RemovePrefixUpTo(textResult, '=');
            RemovePrefixBlanks(textResult);
            TruncateAtFirst(textResult, ' ');
        }
        else
        {
            FREE_MEMORY(textResult);
        }
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "OS name: '%s'", textResult);
    }
    
    return textResult;
}

char* GetOsVersion(void* log)
{
    const char* osVersionCommand = "cat /etc/os-release | grep VERSION=";
    char* textResult = NULL;

    if (0 == ExecuteCommand(NULL, osVersionCommand, true, true, 0, 0, &textResult, NULL, log))
    {
        RemovePrefixBlanks(textResult);
        RemoveTrailingBlanks(textResult);
        RemovePrefixUpTo(textResult, '=');
        RemovePrefixBlanks(textResult);
        TruncateAtFirst(textResult, ' ');
    }
    else
    {
        FREE_MEMORY(textResult);
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "OS version: '%s'", textResult);
    }

    return textResult;
}

static char* GetHardwareProperty(const char* command, bool truncateAtFirstSpace, void* log)
{
    char* textResult = NULL;

    if (0 == ExecuteCommand(NULL, command, true, true, 0, 0, &textResult, NULL, log))
    {
        RemovePrefixUpTo(textResult, ':');
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

static char* GetAnotherOsProperty(const char* command, void* log)
{
    char* textResult = NULL;
    
    if (NULL == command)
    {
        return NULL;
    }

    if (0 == ExecuteCommand(NULL, command, true, true, 0, 0, &textResult, NULL, log))
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

char* GetOsKernelName(void* log)
{
    static char* osKernelNameCommand = "uname -s";
    char* textResult = GetAnotherOsProperty(osKernelNameCommand, log);
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "Kernel name: '%s'", textResult);
    }
    
    return textResult;
}

char* GetOsKernelRelease(void* log)
{
    static char* osKernelReleaseCommand = "uname -r";
    char* textResult = GetAnotherOsProperty(osKernelReleaseCommand, log);
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "Kernel release: '%s'", textResult);
    }
    
    return textResult;
}

char* GetOsKernelVersion(void* log)
{
    static char* osKernelVersionCommand = "uname -v";
    char* textResult = GetAnotherOsProperty(osKernelVersionCommand, log);
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "Kernel version: '%s'", textResult);
    }
    
    return textResult;
}

char* GetCpuType(void* log)
{
    const char* osCpuTypeCommand = "lscpu | grep Architecture:";
    char* textResult = GetHardwareProperty(osCpuTypeCommand, false, log);
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "CPU type: '%s'", textResult);
    }
    
    return textResult;
}

char* GetCpuVendor(void* log)
{
    const char* osCpuVendorCommand = "lscpu | grep \"Vendor ID:\"";
    char* textResult = GetHardwareProperty(osCpuVendorCommand, false, log);
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "CPU vendor id: '%s'", textResult);
    }
    
    return textResult;
}

char* GetCpuModel(void* log)
{
    const char* osCpuModelCommand = "lscpu | grep \"Model name:\"";
    char* textResult = GetHardwareProperty(osCpuModelCommand, false, log);
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "CPU model: '%s'", textResult);
    }
    
    return textResult;
}

char* GetCpuFlags(void* log)
{
    const char* osCpuFlagsCommand = "lscpu | grep \"Flags:\"";
    char* textResult = GetHardwareProperty(osCpuFlagsCommand, false, log);

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "CPU flags: '%s'", textResult);
    }

    return textResult;
}

bool IsCpuFlagSupported(const char* cpuFlag, void* log)
{
    bool result = false;
    char* cpuFlags = GetCpuFlags(log);

    if ((NULL != cpuFlag) && (NULL != strstr(cpuFlags, cpuFlag)))
    {
        OsConfigLogInfo(log, "CPU flag '%s' is supported", cpuFlag);
        result = true;
    }
    else
    {
        OsConfigLogInfo(log, "CPU flag '%s' is not supported", cpuFlag);
    }

    FREE_MEMORY(cpuFlags);

    return result;
}

long GetTotalMemory(void* log)
{
    const char* osTotalMemoryCommand = "grep MemTotal /proc/meminfo";
    char* textResult = GetHardwareProperty(osTotalMemoryCommand, true, log);
    long totalMemory = 0;
    
    if (NULL != textResult)
    {
        totalMemory = atol(textResult);
    }
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "Total memory: %lu kB", totalMemory);
    }
    
    return totalMemory;
}

long GetFreeMemory(void* log)
{
    const char* osFreeMemoryCommand = "grep MemFree /proc/meminfo";
    char* textResult = GetHardwareProperty(osFreeMemoryCommand, true, log);
    long freeMemory = 0;
    
    if (NULL != textResult)
    {
        freeMemory = atol(textResult);
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "Free memory: %lu kB", freeMemory);
    }

    return freeMemory;
}

char* GetProductName(void* log)
{
    const char* osProductNameCommand = "cat /sys/devices/virtual/dmi/id/product_name";
    const char* osProductNameAlternateCommand = "lshw -c system | grep -m 1 \"product:\"";
    char* textResult = GetAnotherOsProperty(osProductNameCommand, log);
    
    if ((NULL == textResult) || (0 == strlen(textResult)))
    {
        textResult = GetHardwareProperty(osProductNameAlternateCommand, false, log);
    }
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "Product name: '%s'", textResult);
    }

    return textResult;
}

char* GetProductVendor(void* log)
{
    const char* osProductVendorCommand = "cat /sys/devices/virtual/dmi/id/sys_vendor";
    const char* osProductVendorAlternateCommand = "lshw -c system | grep -m 1 \"vendor:\"";
    char* textResult = GetAnotherOsProperty(osProductVendorCommand, log);

    if ((NULL == textResult) || (0 == strlen(textResult)))
    {
        textResult = GetHardwareProperty(osProductVendorAlternateCommand, false, log);
    }
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "Product vendor: '%s'", textResult);
    }

    return textResult;
}

char* GetProductVersion(void* log)
{
    const char* osProductVersionCommand = "cat /sys/devices/virtual/dmi/id/product_version";
    const char* osProductVersionAlternateCommand = "lshw -c system | grep -m 1 \"version:\"";
    char* textResult = GetHardwareProperty(osProductVersionCommand, false, log);

    if ((NULL == textResult) || (0 == strlen(textResult)))
    {
        textResult = GetHardwareProperty(osProductVersionAlternateCommand, false, log);
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "Product version: '%s'", textResult);
    }

    return textResult;
}

char* GetSystemCapabilities(void* log)
{
    const char* osSystemCapabilitiesCommand = "lshw -c system | grep -m 1 \"capabilities:\"";
    char* textResult = GetHardwareProperty(osSystemCapabilitiesCommand, false, log);

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "Product capabilities: '%s'", textResult);
    }

    return textResult;
}

char* GetSystemConfiguration(void* log)
{
    const char* osSystemConfigurationCommand = "lshw -c system | grep -m 1 \"configuration:\"";
    char* textResult = GetHardwareProperty(osSystemConfigurationCommand, false, log);

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "Product configuration: '%s'", textResult);
    }

    return textResult;
}

static char* GetOsReleaseEntry(const char* commandTemplate, const char* name, char separator, void* log)
{
    char* command = NULL;
    char* result = NULL;
    size_t commandLength = 0;
    int status = 0;

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

            if (0 == (status = ExecuteCommand(NULL, command, true, false, 0, 0, &result, NULL, log)))
            {
                RemovePrefixBlanks(result);
                RemoveTrailingBlanks(result);
                RemovePrefixUpTo(result, separator);
                RemovePrefixBlanks(result);

                if ('"' == result[0])
                {
                    RemovePrefixUpTo(result, '"');
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

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "'%s': '%s'", name, result);
    }

    return result;
}

static char* GetEtcReleaseEntry(const char* name, void* log)
{
    return GetOsReleaseEntry("cat /etc/*-release | grep %s=", name, '=', log);
}

static char* GetLsbReleaseEntry(const char* name, void* log)
{
    return GetOsReleaseEntry("lsb_release -a | grep \"%s:\"", name, ':', log);
}

static void ClearOsDistroInfo(OS_DISTRO_INFO* info)
{
    if (info)
    {
        FREE_MEMORY(info->id);
        FREE_MEMORY(info->release);
        FREE_MEMORY(info->codename);
        FREE_MEMORY(info->description);
    }
}

bool CheckOsAndKernelMatchDistro(char** reason, void* log)
{
    const char* linuxName = "Linux";
    const char* none = "<null>";

    OS_DISTRO_INFO distro = {0}, os = {0};
    char* kernelName = GetOsKernelName(log);
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

    if ((0 == strncmp(distro.id, os.id, strlen(distro.id))) &&
        (0 == strcmp(distro.release, os.release)) &&
        (0 == strcmp(distro.codename, os.codename)) &&
        (0 == strcmp(distro.description, os.description)) &&
        (0 == strcmp(kernelName, linuxName)))
    {
        OsConfigLogInfo(log, "CheckOsAndKernelMatchDistro: distro and installed image match ('%s', '%s', '%s', '%s', '%s')",
            distro.id, distro.release, distro.codename, distro.description, kernelName);
        match = true;
    }
    else
    {
        OsConfigLogError(log, "CheckOsAndKernelMatchDistro: distro ('%s', '%s', '%s', '%s', '%s') and installed image ('%s', '%s', '%s', '%s', '%s') do not match",
            distro.id, distro.release, distro.codename, distro.description, linuxName, os.id, os.release, os.codename, os.description, kernelName);

        if (reason)
        {
            *reason = FormatAllocateString("Distro ('%s', '%s', '%s', '%s', '%s') and installed image ('%s', '%s', '%s', '%s', '%s') do not match",
                distro.id, distro.release, distro.codename, distro.description, linuxName, os.id, os.release, os.codename, os.description, kernelName);
        }
    }

    FREE_MEMORY(kernelName);

    ClearOsDistroInfo(&distro);
    ClearOsDistroInfo(&os);

    return match;
}

char* GetLoginUmask(void* log)
{
    const char* command = "grep -v '^#' /etc/login.defs | grep UMASK";
    char* result = NULL;

    if (0 == ExecuteCommand(NULL, command, true, true, 0, 0, &result, NULL, log))
    {
        RemovePrefixUpTo(result, ' ');
        RemovePrefixBlanks(result);
        RemoveTrailingBlanks(result);
    }
    else
    {
        FREE_MEMORY(result);
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "UMASK: '%s'", result);
    }

    return result;
}

int CheckLoginUmask(const char* desired, char** reason, void* log)
{
    char* current = NULL;
    size_t length = 0;
    int status = 0;
        
    if ((NULL == desired) || (0 == (length = strlen(desired))))
    {
        OsConfigLogError(log, "CheckLoginUmask: invalid argument");
        return EINVAL;
    }

    if (NULL == (current = GetLoginUmask(log)))
    {
        OsConfigLogError(log, "CheckLoginUmask: GetLoginUmask failed");
        status = ENOENT;
    }
    else
    {
        if (0 == strncmp(desired, current, length))
        {
            OsConfigLogInfo(log, "CheckLoginUmask: current login UMASK '%s' matches desired '%s'", current, desired);
        }
        else
        {
            OsConfigLogError(log, "CheckLoginUmask: current login UMASK '%s' does not match desired '%s'", current, desired);
            status = ENOENT;

            if (reason)
            {
                *reason = FormatAllocateString("Current login UMASK '%s' does not match desired '%s'", current, desired);
            }
        }
    }

    return status;
}

static long GetPasswordDays(const char* name, void* log)
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

        if (0 == ExecuteCommand(NULL, command, true, false, 0, 0, &result, NULL, log))
        {
            RemovePrefixUpTo(result, ' ');
            RemovePrefixBlanks(result);
            RemoveTrailingBlanks(result);

            days = atol(result);
        }

        FREE_MEMORY(result);
        FREE_MEMORY(command);
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "%s: %ld", name, days);
    }

    return days;
}

long GetPassMinDays(void* log)
{
    return GetPasswordDays("PASS_MIN_DAYS", log);
}

long GetPassMaxDays(void* log)
{
    return GetPasswordDays("PASS_MAX_DAYS", log);
}

long GetPassWarnAge(void* log)
{
    return GetPasswordDays("PASS_WARN_AGE", log);
}