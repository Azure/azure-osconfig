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

static char* GetOsDistroInfoEntry(const char* name, void* log)
{
    const char* commandTemplate = "cat /etc/*-release | grep %s=";
    
    char* command = NULL;
    char* result = NULL;
    size_t commandLength = 0;
    int status = 0;

    if ((NULL == name) || (0 == strlen(name)))
    {
        OsConfigLogError(log, "GetOsDistroInfoEntry: invalid arguments");
        return NULL;
    }

    commandLength = strlen(commandTemplate) + strlen(name) + 1;

    if (NULL == (command = malloc(commandLength)))
    {
        OsConfigLogError(log, "GetOsDistroInfoEntry: out of memory");
    }
    else
    {
        memset(command, 0, commandLength);
        snprintf(command, commandLength, commandTemplate, name);

        if (0 == (status = ExecuteCommand(NULL, command, true, false, 0, 0, &result, NULL, log)))
        {
            RemovePrefixBlanks(result);
            RemoveTrailingBlanks(result);
            RemovePrefixUpTo(result, '=');
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
    }

    OsConfigLogInfo(log, "'%s': '%s'", name, result);
    
    return result;
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

static int GetDistroInfo(OS_DISTRO_INFO* info, void* log)
{
    if (NULL == info)
    {
        OsConfigLogError(log, "GetDistroInfo: invalid arguments");
        return EINVAL;
    }
    
    ClearOsDistroInfo(info);

    info->id = GetOsDistroInfoEntry("DISTRIB_ID", log);
    info->release = GetOsDistroInfoEntry("DISTRIB_RELEASE", log);
    info->codename = GetOsDistroInfoEntry("DISTRIB_CODENAME", log);
    info->description = GetOsDistroInfoEntry("DISTRIB_DESCRIPTION", log);

    return 0;
}

static int GetOsInfo(OS_DISTRO_INFO* info, void* log)
{
    if (NULL == info)
    {
        OsConfigLogError(log, "GetOsInfo: invalid arguments");
        return EINVAL;
    }

    ClearOsDistroInfo(info);

    info->id = GetOsDistroInfoEntry("ID", log);
    info->release = GetOsDistroInfoEntry("VERSION_ID", log);
    info->codename = GetOsDistroInfoEntry("VERSION_CODENAME", log);
    info->description = GetOsDistroInfoEntry("PRETTY_NAME", log);

    return 0;
}

bool CheckOsAndKernelMatchDistro(void* log)
{
    const char* linuxName = "Linux";

    OS_DISTRO_INFO distro = {0}, os = {0};
    char* kernelName = GetOsKernelName(log);
    int status = 0;
    bool match = false;

    if (0 == (status = GetDistroInfo(&distro, log)))
    {
        if (0 == (status = GetOsInfo(&os, log)))
        {
            if ((0 == strncmp(distro.id, os.id, strlen(distro.id))) &&
                (0 == strncmp(distro.release, os.release, strlen(distro.release))) &&
                (0 == strncmp(distro.codename, os.codename, strlen(distro.codename))) &&
                (0 == strncmp(distro.description, os.description, strlen(distro.description))) &&
                (0 == strncmp(kernelName, linuxName, strlen(linuxName))))
            {
                OsConfigLogInfo(log, "CheckOsAndKernelMatchDistro: installed OS and kernel match distro ('%s %s %s %s %s')", 
                    kernelName, distro.id, distro.release, distro.codename, distro.description);
                match = true;
            }
            else
            {
                OsConfigLogError(log, "CheckOsAndKernelMatchDistro: distro ('%s %s %s %s %s') does not match installed OS and kernel ('%s %s %s %s %s')",
                    linuxName, distro.id, distro.release, distro.codename, distro.description,
                    kernelName, os.id, os.release, os.codename, os.description);
            }
        }
        else
        {
            OsConfigLogError(log, "CheckOsAndKernelMatchDistro: GetOsInfo failed with %d", status);
        }
    }
    else
    {
        OsConfigLogError(log, "CheckOsAndKernelMatchDistro: GetDistroInfo failed with %d", status);
    }

    FREE_MEMORY(kernelName);

    ClearOsDistroInfo(&distro);
    ClearOsDistroInfo(&os);

    return match;
}

char* GetLoginUmask(void* log)
{
    const char* command = "cat /etc/login.defs | UMASK";
    char* result = NULL;

    if (0 == ExecuteCommand(NULL, command, true, true, 0, 0, &result, NULL, log))
    {
        RemovePrefixBlanks(result);
        RemoveTrailingBlanks(result);
    }
    else
    {
        FREE_MEMORY(result);
    }

    OsConfigLogInfo(log, "UMASK: '%s'", result);

    return result;
}

int CheckLoginUmask(const char* desired, void* log)
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
        }
    }

    return status;
}