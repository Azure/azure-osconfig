// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef LOG_H
#define LOG_H

#include <errno.h>

#include <CommonUtils.h>
#include <Logging.h>
#include <version.h>

#define LOG_INFO(format, ...) OsConfigLogInfo(GetPlatformLog(), format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) OsConfigLogError(GetPlatformLog(), format, ##__VA_ARGS__)
#define LOG_TRACE(format, ...) if (IsFullLoggingEnabled()) { \
    OsConfigLogInfo(GetPlatformLog(), format, ##__VA_ARGS__); \
}
#define LOG_WARN(format, ...) if (IsFullLoggingEnabled()) { \
    OsConfigLogError(GetPlatformLog(), format, ##__VA_ARGS__); \
}

#ifdef __cplusplus
extern "C"
{
#endif

OSCONFIG_LOG_HANDLE GetPlatformLog();

#ifdef __cplusplus
}
#endif

#endif // LOG_H