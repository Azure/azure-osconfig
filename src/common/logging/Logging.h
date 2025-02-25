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

typedef struct OSCONFIG_LOG OSCONFIG_LOG;
typedef OSCONFIG_LOG* OSCONFIG_LOG_HANDLE;

#ifdef __cplusplus
extern "C"
{
#endif

// Matching the severity values in RFC 5424 
enum LoggingLevel
{
    LoggingLevelEmergency = 0,
    LoggingLevelAlert = 1,
    LoggingLevelCritical = 2,
    LoggingLevelError = 3, //used for default error logging
    LoggingLevelWarning = 4,
    LoggingLevelNotice = 5,
    LoggingLevelInformational = 6, //used for default informational logging
    LoggingLevelDebug = 7 //used for CommandLogging and DebugLogging
};
typedef enum LoggingLevel LoggingLevel;

OSCONFIG_LOG_HANDLE OpenLog(const char* logFileName, const char* bakLogFileName);
void CloseLog(OSCONFIG_LOG_HANDLE* log);

bool IsDebugLoggingEnabled(void);
void SetDebugLogging(bool fullLogging);

FILE* GetLogFile(OSCONFIG_LOG_HANDLE log);
char* GetFormattedTime();
void TrimLog(OSCONFIG_LOG_HANDLE log);
bool IsDaemon(void);

#define __PREFIX_TEMPLATE__ "[%s][%s][%s:%d] "
#define __SHORT_FILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define __LOG__(log, loglevel, format, ...) printf(__PREFIX_TEMPLATE__ format "\n", GetFormattedTime(), loglevel, __SHORT_FILE__, __LINE__, ## __VA_ARGS__)
#define __LOG_TO_FILE__(log, loglevel, format, ...) {\
    TrimLog(log);\
    fprintf(GetLogFile(log), __PREFIX_TEMPLATE__ format "\n", GetFormattedTime(), loglevel, __SHORT_FILE__, __LINE__, ## __VA_ARGS__); \
}\

#define __INFO__ "INFO"
#define __ERROR__ "ERROR"
#define __DEBUG__ "DEBUG"

#define OSCONFIG_LOG_INFO(log, format, ...) __LOG__(log, __INFO__, format, ## __VA_ARGS__)
#define OSCONFIG_LOG_ERROR(log, format, ...) __LOG__(log, __ERROR__, format, ## __VA_ARGS__)
#define OSCONFIG_LOG_DEBUG(log, format, ...) __LOG__(log, __DEBUG__, format, ## __VA_ARGS__)

#define OSCONFIG_FILE_LOG_INFO(log, format, ...) __LOG_TO_FILE__(log, __INFO__, format, ## __VA_ARGS__)
#define OSCONFIG_FILE_LOG_ERROR(log, format, ...) __LOG_TO_FILE__(log, __ERROR__, format, ## __VA_ARGS__)
#define OSCONFIG_FILE_LOG_DEBUG(log, format, ...) __LOG_TO_FILE__(log, __DEBUG__, format, ## __VA_ARGS__)

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
        if ((false == IsDaemon()) || (false == IsDebugLoggingEnabled())) {\
            OSCONFIG_LOG_DEBUG(log, FORMAT, ##__VA_ARGS__);\
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
