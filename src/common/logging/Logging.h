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


// The logging level values in this enumeration correspond to the severity values defined in RFC 5424.
// LoggingLevelInformational (6) is the most useful and is enabled by default.
// LoggingLevelDebug (7) is optional and disabled by default.
// LoggingLevelCritical (2) is used for telemetry events.
// The minimum recommended level is LoggingLevelInformational (6),
// as lower levels do not currently produce useful logging.
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
bool IsLoggingLevelSupported(LoggingLevel level);
void SetLoggingLevel(LoggingLevel level);
LoggingLevel GetLoggingLevel(void);
bool IsDebugLoggingEnabled(void);
const char* GetLoggingLevelName(LoggingLevel level);
unsigned int GetMaxLogSize(void);
void SetMaxLogSize(unsigned int value);
unsigned int GetMaxLogSizeDebugMultiplier(void);
void SetMaxLogSizeDebugMultiplier(unsigned int value);
bool IsConsoleLoggingEnabled(void);
void SetConsoleLoggingEnabled(bool enabledOrDisabled);
FILE* GetLogFile(OsConfigLogHandle log);
char* GetFormattedTime(void);
void TrimLog(OsConfigLogHandle log);
bool IsDaemon(void);

#define __PREFIX_TEMPLATE__ "[%s][%s][%s:%d] "
#define __SHORT_FILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define __LOG__(log, label, format, ...) printf(__PREFIX_TEMPLATE__ format "\n", GetFormattedTime(), label, __SHORT_FILE__, __LINE__, ## __VA_ARGS__)
#define __LOG_TO_FILE__(log, label, format, ...) {\
    TrimLog(log);\
    fprintf(GetLogFile(log), __PREFIX_TEMPLATE__ format "\n", GetFormattedTime(), label, __SHORT_FILE__, __LINE__, ## __VA_ARGS__);\
}\

#define OSCONFIG_LOG(log, level, format, ...) __LOG__(log, GetLoggingLevelName(level), format, ## __VA_ARGS__)
#define OSCONFIG_LOG_TO_FILE(log, level, format, ...) __LOG_TO_FILE__(log, GetLoggingLevelName(level), format, ## __VA_ARGS__)

// Universal macro that can log at any of the 7 levels:
#define OsConfigLog(log, level, FORMAT, ...) {\
    if (level <= GetLoggingLevel()) {\
        if (NULL != GetLogFile(log)) {\
            OSCONFIG_LOG_TO_FILE(log, level, FORMAT, ##__VA_ARGS__);\
            fflush(GetLogFile(log));\
        }\
        if (IsConsoleLoggingEnabled()) {\
            OSCONFIG_LOG(log, level, FORMAT, ##__VA_ARGS__);\
        }\
    }\
}\

// Shortcuts that directly log at the respective level:
#define OsConfigLogEmergency(log, FORMAT, ...) OsConfigLog(log, LoggingLevelEmergency, FORMAT, ## __VA_ARGS__)
#define OsConfigLogAlert(log, FORMAT, ...) OsConfigLog(log, LoggingLevelAlert, FORMAT, ## __VA_ARGS__)
#define OsConfigLogCritical(log, FORMAT, ...) OsConfigLog(log, LoggingLevelCritical, FORMAT, ## __VA_ARGS__)
#define OsConfigLogError(log, FORMAT, ...)  OsConfigLog(log, LoggingLevelError, FORMAT, ## __VA_ARGS__)
#define OsConfigLogWarning(log, FORMAT, ...) OsConfigLog(log, LoggingLevelWarning, FORMAT, ## __VA_ARGS__)
#define OsConfigLogNotice(log, FORMAT, ...) OsConfigLog(log, LoggingLevelNotice, FORMAT, ## __VA_ARGS__)
#define OsConfigLogInfo(log, FORMAT, ...) OsConfigLog(log, LoggingLevelInformational, FORMAT, ## __VA_ARGS__)
#define OsConfigLogDebug(log, FORMAT, ...) OsConfigLog(log, LoggingLevelDebug, FORMAT, ## __VA_ARGS__)

// For debug builds
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
