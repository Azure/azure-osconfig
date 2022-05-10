// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <PlatformCommon.h>

OSCONFIG_LOG_HANDLE g_platformLog = NULL;

OSCONFIG_LOG_HANDLE GetPlatformLog()
{
    return g_platformLog;
}