// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef LOGGING_H
#define LOGGING_H

#include <string.h>
#include <stdbool.h>
#include <stdio.h>

// Maximum log size (1,024,000 is 1MB aka 1024 * 1000), increase or decrease as needed
#define MAX_LOG_SIZE 1024000

#define TIME_FORMAT_STRING_LENGTH 20

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

#define SESSIONS_TELEMETRY_MARKER -9999

typedef struct OsConfigLog OsConfigLog;
typedef OsConfigLog* OsConfigLogHandle;

#ifdef __cplusplus
extern "C"
{
#endif

// Matching the severity values in RFC 5424. Currently we only use 2 values from this enumeration:
// - LoggingLevelInformational (6) uses [INFO] and [ERROR] labels and is always enabled by default
// - LoggingLevelDebug (7) is optional, uses the [DEBUG] label, and is managed via the 'DebugLogging' entry in the osconfig.json configuration
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
    SessionsTelemetry = 1,
    FailuresTelemetry = 2,
    RulesTelemetry = 3
};
typedef enum TelemetryLevel TelemetryLevel;

OsConfigLogHandle OpenLog(const char* logFileName, const char* bakLogFileName);
void CloseLog(OsConfigLogHandle* log);

bool IsDebugLoggingEnabled(void);
void SetDebugLogging(bool fullLogging);

TelemetryLevel GetTelemetryLevel(void);
void SetTelemetryLevel(TelemetryLevel level);

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

#define __PREFIX_TELEMETRY_TEMPLATE__ "{\"DateTime\":\"%s\""
#define __LOG_TELEMETRY__(log, format, ...) printf(__PREFIX_TELEMETRY_TEMPLATE__ format "}\n", GetFormattedTime(), ## __VA_ARGS__)
#define __LOG_TELEMETRY_TO_FILE__(log, format, ...) {\
    TrimLog(log); \
    fprintf(GetLogFile(log), __PREFIX_TELEMETRY_TEMPLATE__ format "}\n", GetFormattedTime(), ## __VA_ARGS__); \
}\



#define __INFO__ "INFO"
#define __ERROR__ "ERROR"
#define __DEBUG__ "DEBUG"

#define OSCONFIG_LOG_INFO(log, format, ...) __LOG__(log, __INFO__, format, ## __VA_ARGS__)
#define OSCONFIG_LOG_ERROR(log, format, ...) __LOG__(log, __ERROR__, format, ## __VA_ARGS__)
#define OSCONFIG_LOG_DEBUG(log, format, ...) __LOG__(log, __DEBUG__, format, ## __VA_ARGS__)
#define OSCONFIG_LOG_TELEMETRY(log, format, ...) __LOG_TELEMETRY__(log, format, ## __VA_ARGS__)

#define OSCONFIG_FILE_LOG_INFO(log, format, ...) __LOG_TO_FILE__(log, __INFO__, format, ## __VA_ARGS__)
#define OSCONFIG_FILE_LOG_ERROR(log, format, ...) __LOG_TO_FILE__(log, __ERROR__, format, ## __VA_ARGS__)
#define OSCONFIG_FILE_LOG_DEBUG(log, format, ...) __LOG_TO_FILE__(log, __DEBUG__, format, ## __VA_ARGS__)
#define OSCONFIG_FILE_LOG_TELEMETRY(log, format, ...) __LOG_TELEMETRY_TO_FILE__(log, format, ## __VA_ARGS__)

#define OsConfigLogInfo(log, FORMAT, ...) {\
    if (NULL != GetLogFile(log)) {\
        OSCONFIG_FILE_LOG_INFO(log, FORMAT, ##__VA_ARGS__);\
        fflush(GetLogFile(log));\
    }\
    if ((false == IsDaemon()) || (false == IsDebugLoggingEnabled())) {\
        OSCONFIG_LOG_INFO(log, FORMAT, ##__VA_ARGS__);\
    }\
}\

#define OsConfigLogError(log, FORMAT, ...) {\
    if (NULL != GetLogFile(log)) {\
        OSCONFIG_FILE_LOG_ERROR(log, FORMAT, ##__VA_ARGS__);\
        fflush(GetLogFile(log));\
    }\
    if ((false == IsDaemon()) || (false == IsDebugLoggingEnabled())) {\
        OSCONFIG_LOG_ERROR(log, FORMAT, ##__VA_ARGS__);\
    }\
}\

#define OsConfigLogDebug(log, FORMAT, ...) {\
    if (true == IsDebugLoggingEnabled()) {\
        if (NULL != GetLogFile(log)) {\
            OSCONFIG_FILE_LOG_DEBUG(log, FORMAT, ##__VA_ARGS__);\
            fflush(GetLogFile(log));\
        }\
        if (false == IsDaemon()) {\
            OSCONFIG_LOG_DEBUG(log, FORMAT, ##__VA_ARGS__);\
        }\
    }\
}\

#define OsConfigLogTelemetry(log, FORMAT, ...) {\
    if (NoTelemetry < GetTelemetryLevel()) {\
        if (NULL != GetLogFile(log)) {\
            OSCONFIG_FILE_LOG_TELEMETRY(log, FORMAT, ##__VA_ARGS__);\
            fflush(GetLogFile(log));\
        }\
        if (false == IsDaemon()) {\
            OSCONFIG_LOG_TELEMETRY(log, FORMAT, ##__VA_ARGS__);\
        }\
    }\
}\

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
