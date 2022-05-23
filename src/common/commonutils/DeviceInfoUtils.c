// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Internal.h"

#define OS_NAME_COMMAND "cat /etc/os-release | grep ID="
#define OS_PRETTY_NAME_COMMAND "cat /etc/os-release | grep PRETTY_NAME="
#define OS_VERSION_COMMAND "cat /etc/os-release | grep VERSION="
#define OS_KERNEL_NAME_COMMAND "uname -s"
#define OS_KERNEL_RELEASE_COMMAND "uname -r"
#define OS_KERNEL_VERSION_COMMAND "uname -v"
#define OS_CPU_COMMAND "lscpu | grep Architecture:"
#define OS_CPU_VENDOR_COMMAND "lscpu | grep \"Vendor ID:\""
#define OS_CPU_MODEL_COMMAND "lscpu | grep \"Model name:\""
#define OS_TOTAL_MEMORY_COMMAND "grep MemTotal /proc/meminfo"
#define OS_FREE_MEMORY_COMMAND "grep MemFree /proc/meminfo"
#define OS_PRODUCT_NAME_COMMAND "cat /sys/devices/virtual/dmi/id/product_name"
#define OS_PRODUCT_VENDOR_COMMAND "cat /sys/devices/virtual/dmi/id/sys_vendor"
#define OS_PRODUCT_NAME_ALTERNATE_COMMAND "lshw -c system | grep -m 1 \"product:\""
#define OS_PRODUCT_VENDOR_ALTERNATE_COMMAND "lshw -c system | grep -m 1 \"vendor:\""
#define OS_PRODUCT_VERSION_COMMAND "lshw -c system | grep -m 1 \"version:\""
#define OS_SYSTEM_CONFIGURATION_COMMAND "lshw -c system | grep -m 1 \"configuration:\""
#define OS_SYSTEM_CAPABILITIES_COMMAND "lshw -c system | grep -m 1 \"capabilities:\""

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

    int targetLength =(int)strlen(target);
    char* found = strchr(target, marker);

    if (found)
    {
        found[0] = 0;
    }
}

char* GetOsName(void* log)
{
    char* textResult = NULL;

    if (0 == ExecuteCommand(NULL, OS_PRETTY_NAME_COMMAND, true, true, 0, 0, &textResult, NULL, log))
    {
        RemovePrefixBlanks(textResult);
        RemoveTrailingBlanks(textResult);
        RemovePrefixUpTo(textResult, '=');
        RemovePrefixBlanks(textResult);
        
        // Comment next line to capture the full pretty name including version (example: 'Ubuntu 20.04.3 LTS')
        TruncateAtFirst(textResult, ' ');
    }
    else if (0 == ExecuteCommand(NULL, OS_NAME_COMMAND, true, true, 0, 0, &textResult, NULL, log))
    {
        // PRETTY_NAME did not work, try ID
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
        OsConfigLogInfo(log, "OS name: '%s'", textResult);
    }
    
    return textResult;
}

char* GetOsVersion(void* log)
{
    char* textResult = NULL;

    if (0 == ExecuteCommand(NULL, OS_VERSION_COMMAND, true, true, 0, 0, &textResult, NULL, log))
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
    char* textResult = GetAnotherOsProperty(OS_KERNEL_NAME_COMMAND, log);
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "Kernel name: '%s'", textResult);
    }

    return textResult;
}

char* GetOsKernelRelease(void* log)
{
    char* textResult = GetAnotherOsProperty(OS_KERNEL_RELEASE_COMMAND, log);
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "Kernel release: '%s'", textResult);
    }

    return textResult;
}

char* GetOsKernelVersion(void* log)
{
    char* textResult = GetAnotherOsProperty(OS_KERNEL_VERSION_COMMAND, log);
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "Kernel version: '%s'", textResult);
    }
    
    return textResult;
}

char* GetCpuType(void* log)
{
    char* textResult = GetHardwareProperty(OS_CPU_COMMAND, false, log);
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "CPU type: '%s'", textResult);
    }

    return textResult;
}

char* GetCpuVendor(void* log)
{
    char* textResult = GetHardwareProperty(OS_CPU_VENDOR_COMMAND, false, log);

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "CPU vendor id: '%s'", textResult);
    }

    return textResult;
}

char* GetCpuModel(void* log)
{
    char* textResult = GetHardwareProperty(OS_CPU_MODEL_COMMAND, false, log);

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "CPU model: '%s'", textResult);
    }

    return textResult;
}

long GetTotalMemory(void* log)
{
    long totalMemory = 0;
    char* textResult = GetHardwareProperty(OS_TOTAL_MEMORY_COMMAND, true, log);
    
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
    long freeMemory = 0;
    char* textResult = GetHardwareProperty(OS_FREE_MEMORY_COMMAND, true, log);

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
    char* textResult = GetAnotherOsProperty(OS_PRODUCT_NAME_COMMAND, log);
    if ((NULL == textResult) || (0 == strlen(textResult)))
    {
        textResult = GetHardwareProperty(OS_PRODUCT_NAME_ALTERNATE_COMMAND, false, log);
    }
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "Product name: '%s'", textResult);
    }

    return textResult;
}

char* GetProductVendor(void* log)
{
    char* textResult = GetAnotherOsProperty(OS_PRODUCT_VENDOR_COMMAND, log);

    if ((NULL == textResult) || (0 == strlen(textResult)))
    {
        textResult = GetHardwareProperty(OS_PRODUCT_VENDOR_ALTERNATE_COMMAND, false, log);
    }
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "Product vendor: '%s'", textResult);
    }

    return textResult;
}

char* GetProductVersion(void* log)
{
    char* textResult = GetHardwareProperty(OS_PRODUCT_VERSION_COMMAND, false, log);

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "Product version: '%s'", textResult);
    }

    return textResult;
}

char* GetSystemCapabilities(void* log)
{
    char* textResult = GetHardwareProperty(OS_SYSTEM_CAPABILITIES_COMMAND, false, log);

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "Product capabilities: '%s'", textResult);
    }

    return textResult;
}

char* GetSystemConfiguration(void* log)
{
    char* textResult = GetHardwareProperty(OS_SYSTEM_CONFIGURATION_COMMAND, false, log);

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(log, "Product configuration: '%s'", textResult);
    }

    return textResult;
}

