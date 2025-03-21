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

#define SESSIONS_TELEMETRY_MARKER -9999

#ifdef __cplusplus
extern "C"
{
#endif

// The logging level values in this enumeration match the severity values in RFC 5424.
// LoggingLevelInformational (6) is by default and always enabled.
// LoggingLevelDebug (7) is optional and disabled by default.
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

enum TelemetryLevel
{
    NoTelemetry = 0,
    BasicTelemetry = 1,
    FailuresTelemetry = 2,
    RulesTelemetry = 3,
    AssertsTelemetry = 4,
    AllTelemetry = 5
};
typedef enum TelemetryLevel TelemetryLevel;

typedef struct OsConfigLog OsConfigLog;
typedef OsConfigLog* OsConfigLogHandle;

OsConfigLogHandle OpenLog(const char* logFileName, const char* bakLogFileName);
void CloseLog(OsConfigLogHandle* log);
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
TelemetryLevel GetTelemetryLevel(void);
void SetTelemetryLevel(TelemetryLevel level);
void LogTelemetry(const char* message, OsConfigLogHandle log);

// Telemetry macros:

#define __PREFIX_TELEMETRY_TEMPLATE__ "{\"DateTime\":\"%s\""
#define __LOG_TELEMETRY__(log, format, ...) printf(__PREFIX_TELEMETRY_TEMPLATE__ format "}\n", GetFormattedTime(), ## __VA_ARGS__)
#define __LOG_TELEMETRY_TO_FILE__(log, format, ...) {\
    TrimLog(log); \
    fprintf(GetLogFile(log), __PREFIX_TELEMETRY_TEMPLATE__ format "}\n", GetFormattedTime(), ## __VA_ARGS__); \
}\

#define OSCONFIG_LOG_TELEMETRY(log, format, ...) __LOG_TELEMETRY__(log, format, ## __VA_ARGS__)
#define OSCONFIG_FILE_LOG_TELEMETRY(log, format, ...) __LOG_TELEMETRY_TO_FILE__(log, format, ## __VA_ARGS__)

// Universal telemetry macro that can log telemetry at any level
#define OsConfigLogTelemetry(log, level, FORMAT, ...) {\
    if (level <= GetTelemetryLevel()) {\
        if (NULL != GetLogFile(log)) {\
            OSCONFIG_FILE_LOG_TELEMETRY(log, FORMAT, ##__VA_ARGS__);\
            fflush(GetLogFile(log));\
        }\
        if (IsConsoleLoggingEnabled()) {\
            OSCONFIG_LOG_TELEMETRY(log, FORMAT, ##__VA_ARGS__);\
        }\
    }\
}\

// Shortcuts that directly log telemetry at the respective level:
#define OsConfigLogBasicTelemetry(log, FORMAT, ...) OsConfigLog(log, BasicTelemetry, FORMAT, ## __VA_ARGS__)
#define OsConfigLogFailuresTelemetry(log, FORMAT, ...) OsConfigLog(log, FailuresTelemetry, FORMAT, ## __VA_ARGS__)
#define OsConfigLogRulesTelemetry(log, FORMAT, ...) OsConfigLog(log, RulesTelemetry, FORMAT, ## __VA_ARGS__)
#define OsConfigLogAssertsTelemetry(log, FORMAT, ...)  OsConfigLog(log, AssertsTelemetry, FORMAT, ## __VA_ARGS__)
#define OsConfigLogAllTelemetry(log, FORMAT, ...) OsConfigLog(log, AllTelemetry, FORMAT, ## __VA_ARGS__)

// Logging macros:

#define __PREFIX_TEMPLATE__ "[%s][%s][%s:%d] "
#define __SHORT_FILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define __LOG__(log, label, format, ...) printf(__PREFIX_TEMPLATE__ format "\n", GetFormattedTime(), label, __SHORT_FILE__, __LINE__, ## __VA_ARGS__)
#define __LOG_TO_FILE__(log, label, format, ...) {\
    TrimLog(log); \
    fprintf(GetLogFile(log), __PREFIX_TEMPLATE__ format "\n", GetFormattedTime(), label, __SHORT_FILE__, __LINE__, ## __VA_ARGS__); \
}\

#define OSCONFIG_LOG(log, level, format, ...) __LOG__(log, GetLoggingLevelName(level), format, ## __VA_ARGS__)
#define OSCONFIG_LOG_TO_FILE(log, level, format, ...) __LOG_TO_FILE__(log, GetLoggingLevelName(level), format, ## __VA_ARGS__)

// Universal macro that can log at any of the 7 levels:
#define OsConfigLog(log, level, FORMAT, ...) {\
    OsConfigLogAllTelemetry(FORMAT, ##__VA_ARGS__);\
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
        OsConfigLogAssertsTelemetry("Assert in %s", __func__);\
        OsConfigLogError(log, "Assert in %s", __func__);\
        assert(CONDITION);\
    }\
}\

#ifdef __cplusplus
}
#endif

#endif // LOGGING_H
