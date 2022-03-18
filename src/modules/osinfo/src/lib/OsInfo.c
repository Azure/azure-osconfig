// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>
#include "OsInfo.h"

#define OSINFO_MODULE_INFO "({"
    "\"Name\": \"OsInfo\","
    "\"Description\": \"Provides functionality to observe OS and device information\","
    "\"Manufacturer\": \"Microsoft\","
    "\"VersionMajor\": 1,"
    "\"VersionMinor\": 0,"
    "\"VersionInfo\": \"Copper\","
    "\"Components\": [\"OsInfo\"],"
    "\"Lifetime\": 2,"
    "\"UserAccount\": 0})"

#define OSINFO_LOG_FILE "/var/log/osconfig_osinfo.log"
#define OSINFO_ROLLED_LOG_FILE "/var/log/osconfig_osinfo.bak"

OSCONFIG_LOG_HANDLE g_log;

OSCONFIG_LOG_HANDLE Get(void)
{
    return g_log;
}

void OpenLog(void)
{
    g_log = OpenLog(OSINFO_LOG_FILE, OSINFO_ROLLED_LOG_FILE);
}

void CloseLog(void)
{
    CloseLog(&g_log);
}