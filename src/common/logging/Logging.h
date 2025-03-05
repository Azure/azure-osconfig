// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef LOGGING_H
#define LOGGING_H

#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

// Matching the severity values in RFC 5424. Currently we only use 2 values from this enumeration:
// - LoggingLevelInformational (6) uses [INFO] and [ERROR] labels and is always enabled by default
// - LoggingLevelDebug (7) is optional, uses the [DEBUG] label, and can be configured via osconfig.json
enum LoggingLevel
{
    LoggingLevelEmergency = 0,
    LoggingLevelAlert = 1,
    LoggingLevelCritical = 2,
    LoggingLevelError = 3,
    LoggingLevelWarning = 4,
    LoggingLevelNotice = 5,
    LoggingLevelInformational = 6,
    LoggingLevelDebug = 7
};
typedef enum LoggingLevel LoggingLevel;

typedef struct OsConfigLog OsConfigLog;
typedef OsConfigLog* OsConfigLogHandle;

OsConfigLogHandle OpenLog(const char* logFileName, const char* bakLogFileName);
void CloseLog(OsConfigLogHandle* log);

void SetLoggingLevel(LoggingLevel level);
LoggingLevel GetLoggingLevel(void);
bool IsDebugLoggingEnabled(void);

unsigned int GetMaxLogSize(void);
void SetMaxLogSize(unsigned int value);
unsigned int GetMaxLogSizeDebugMultiplier(void);
void SetMaxLogSizeDebugMultiplier(unsigned int value);

FILE* GetLogFile(OsConfigLogHandle log);
char* GetFormattedTime(void);
void TrimLog(OsConfigLogHandle log);
bool IsDaemon(void);

#define __PREFIX_TEMPLATE__ "[%s][%s][%s:%d] "
#define __SHORT_FILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define __LOG__(log, label, format, ...) printf(__PREFIX_TEMPLATE__ format "\n", GetFormattedTime(), label, __SHORT_FILE__, __LINE__, ## __VA_ARGS__)
#define __LOG_TO_FILE__(log, label, format, ...) {\
    TrimLog(log);\
    fprintf(GetLogFile(log), __PREFIX_TEMPLATE__ format "\n", GetFormattedTime(), label, __SHORT_FILE__, __LINE__, ## __VA_ARGS__); \
}\

#define __EMERGENCY__ "EMERGENCY"
#define __ALERT__ "ALERT"
#define __CRITICAL__ "CRITICAL"
#define __ERROR__ "ERROR"
#define __WARNING__ "WARNING"
#define __NOTICE__ "NOTICE"
#define __INFO__ "INFO"
#define __DEBUG__ "DEBUG"

#define GetLoggingLevelName(level) {\
    if (LoggingLevelEmergency == level)\
        return __EMERGENCY__;\
    else if (LoggingLevelAlert == level)\
        return __ALERT__;\
    else if (LoggingLevelCritical == level)\
        return __CRITICAL__;\
    else if (LoggingLevelError == level)\
        return __ERROR__;\
    else if LoggingLevelWarning == level)\
        return __WARNING__;\
    else if LoggingLevelNotice == level)\
        return __NOTICE__;\
    else if LoggingLevelInformational == level)\
        return __INFO__;\
    else
        return __DEBUG__;\
}\

#define OSCONFIG_LOG(log, level, format, ...) __LOG__(log, GetLoggingLevelName(level), format, ## __VA_ARGS__)
#define OSCONFIG_LOG_INFO(log, format, ...) OSCONFIG_LOG(log, __INFO__, format,  ## __VA_ARGS__)
#define OSCONFIG_LOG_ERROR(log, format, ...) OSCONFIG_LOG(log, __ERROR__, format,  ## __VA_ARGS__)

#define OSCONFIG_FILE_LOG(log, level, format, ...) __LOG_TO_FILE__(log, GetLoggingLevelName(level), format, ## __VA_ARGS__)
#define OSCONFIG_FILE_LOG_INFO(log, format, ...) OSCONFIG_FILE_LOG(log, __INFO__, format,  ## __VA_ARGS__)
#define OSCONFIG_FILE_LOG_ERROR(log, format, ...) OSCONFIG_FILE_LOG(log, __ERROR__, format,  ## __VA_ARGS__)

#define OsConfigLog(log, level, FORMAT, ...) {\
    if (GetLoggingLevel() >= level) {\
        if (NULL != GetLogFile(log)) {\
            OSCONFIG_FILE_LOG(log, level, FORMAT, ##__VA_ARGS__);\
            fflush(GetLogFile(log));\
        }\
        if (false == IsDaemon()) {\
            OSCONFIG_LOG(log, level, FORMAT, ##__VA_ARGS__);\
        }\
    }\
}\

#define OsConfigLogInfo(log, FORMAT, ...) OsConfigLog(log, __INFO__, FORMAT, ## __VA_ARGS__)
#define OsConfigLogError(log, FORMAT, ...)  OsConfigLog(log, __ERROR__, FORMAT, ## __VA_ARGS__)

#define LogAssert(log, CONDITION) {\
    if (!(CONDITION)) {\
        OsConfigLogError(log, "Assert in %s", __func__);\
        assert(CONDITION);\
    }\
}\

#ifdef __cplusplus
}
#endif

#endif // LOGGING_H
