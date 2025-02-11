// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef LOGGING_H
#define LOGGING_H

#include <string.h>

// Maximum log size (1,024,000 is 1MB aka 1024 * 1000), increase or decrease as needed
#define MAX_LOG_SIZE 1024000

#define TIME_FORMAT_STRING_LENGTH 20

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

typedef void* OSCONFIG_LOG_HANDLE;

#ifdef __cplusplus
extern "C"
{
#endif

OSCONFIG_LOG_HANDLE OpenLog(const char* logFileName, const char* bakLogFileName);
void CloseLog(OSCONFIG_LOG_HANDLE* log);

void SetFullLogging(bool fullLogging);
bool IsFullLoggingEnabled(void);

FILE* GetLogFile(OSCONFIG_LOG_HANDLE log);
char* GetFormattedTime();
void TrimLog(OSCONFIG_LOG_HANDLE log);
bool IsDaemon(void);

#define __SHORT_FILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define __LOG__(log, format, loglevel, ...) printf("[%s] [%s:%d]%s" format "\n", GetFormattedTime(), __SHORT_FILE__, __LINE__, loglevel, ## __VA_ARGS__)
#define __LOG_TO_FILE__(log, format, loglevel, ...) {\
    TrimLog(log);\
    fprintf(GetLogFile(log), "[%s] [%s:%d]%s" format "\n", GetFormattedTime(), __SHORT_FILE__, __LINE__, loglevel, ## __VA_ARGS__);\
}\

#define __INFO__ " "
#define __ERROR__ " [ERROR] "

#define OSCONFIG_LOG_INFO(log, format, ...) __LOG__(log, format, __INFO__, ## __VA_ARGS__)
#define OSCONFIG_LOG_ERROR(log, format, ...) __LOG__(log, format, __ERROR__, ## __VA_ARGS__)
#define OSCONFIG_FILE_LOG_INFO(log, format, ...) __LOG_TO_FILE__(log, format, __INFO__, ## __VA_ARGS__)
#define OSCONFIG_FILE_LOG_ERROR(log, format, ...) __LOG_TO_FILE__(log, format, __ERROR__, ## __VA_ARGS__)

#define OsConfigLogInfo(log, FORMAT, ...) {\
    if (NULL != GetLogFile(log)) {\
        OSCONFIG_FILE_LOG_INFO(log, FORMAT, ##__VA_ARGS__);\
        fflush(GetLogFile(log));\
    }\
    if ((false == IsDaemon()) || (false == IsFullLoggingEnabled())) {\
        OSCONFIG_LOG_INFO(log, FORMAT, ##__VA_ARGS__);\
    }\
}\

#define OsConfigLogError(log, FORMAT, ...) {\
    if (NULL != GetLogFile(log)) {\
        OSCONFIG_FILE_LOG_ERROR(log, FORMAT, ##__VA_ARGS__);\
        fflush(GetLogFile(log));\
    }\
    if ((false == IsDaemon()) || (false == IsFullLoggingEnabled())) {\
        OSCONFIG_LOG_ERROR(log, FORMAT, ##__VA_ARGS__);\
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
