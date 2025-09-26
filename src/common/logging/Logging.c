// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include "Logging.h"

#define TIME_FORMAT_STRING_LENGTH 64

struct OsConfigLog
{
    FILE* log;
    const char* logFileName;
    const char* backLogFileName;
    unsigned int trimLogCount;
};

static LoggingLevel g_loggingLevel = LoggingLevelInformational;

// Default maximum log size (1,048,576 is 1024 * 1024 aka 1MB)
static unsigned int g_maxLogSize = 1048576;
static unsigned int g_maxLogSizeDebugMultiplier = 5;

static const char* g_emergency = "EMERGENCY";
static const char* g_alert = "ALERT";
static const char* g_critical = "CRITICAL";
static const char* g_error = "ERROR";
static const char* g_warning = "WARNING";
static const char* g_notice = "NOTICE";
static const char* g_info = "INFO";
static const char* g_debug = "DEBUG";

static bool g_consoleLoggingEnabled = true;

bool IsConsoleLoggingEnabled(void)
{
    return IsDaemon() ? false : g_consoleLoggingEnabled;
}

void SetConsoleLoggingEnabled(bool enabledOrDisabled)
{
    g_consoleLoggingEnabled = enabledOrDisabled;
}

const char* GetLoggingLevelName(LoggingLevel level)
{
    const char* result = g_debug;
    (void)result;

    switch (level)
    {
        case LoggingLevelEmergency:
            result = g_emergency;
            break;

        case LoggingLevelAlert:
            result = g_alert;
            break;

        case LoggingLevelCritical:
            result = g_critical;
            break;

        case LoggingLevelError:
            result = g_error;
            break;

        case LoggingLevelWarning:
            result = g_warning;
            break;

        case LoggingLevelNotice:
            result = g_notice;
            break;

        case LoggingLevelInformational:
            result = g_info;
            break;

        case LoggingLevelDebug:
        default:
            result = g_debug;
    }

    return result;
}

bool IsLoggingLevelSupported(LoggingLevel level)
{
    return ((level >= LoggingLevelEmergency) && (level <= LoggingLevelDebug)) ? true : false;
}

void SetLoggingLevel(LoggingLevel level)
{
    g_loggingLevel = IsLoggingLevelSupported(level) ? level : LoggingLevelInformational;
}

LoggingLevel GetLoggingLevel(void)
{
    return g_loggingLevel;
}

bool IsDebugLoggingEnabled(void)
{
    return (LoggingLevelDebug == g_loggingLevel) ? true : false;
}

unsigned int GetMaxLogSize(void)
{
    return g_maxLogSize;
}

void SetMaxLogSize(unsigned int value)
{
    g_maxLogSize = value;
}

unsigned int GetMaxLogSizeDebugMultiplier(void)
{
    return g_maxLogSizeDebugMultiplier;
}

void SetMaxLogSizeDebugMultiplier(unsigned int value)
{
    g_maxLogSizeDebugMultiplier = value;
}

static int RestrictFileAccessToCurrentAccountOnly(const char* fileName)
{
    return chmod(fileName, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
}

OsConfigLogHandle OpenLog(const char* logFileName, const char* bakLogFileName)
{
    OsConfigLog* newLog = (OsConfigLog*)malloc(sizeof(OsConfigLog));
    if (NULL == newLog)
    {
        return NULL;
    }

    memset(newLog, 0, sizeof(*newLog));

    newLog->logFileName = logFileName;
    newLog->backLogFileName = newLog->logFileName ? bakLogFileName : NULL;

    if (NULL != newLog->logFileName)
    {
        newLog->log = fopen(newLog->logFileName, "a");
        RestrictFileAccessToCurrentAccountOnly(newLog->logFileName);
    }

    if (NULL != newLog->backLogFileName)
    {
        RestrictFileAccessToCurrentAccountOnly(newLog->backLogFileName);
    }

    return newLog;
}

void CloseLog(OsConfigLogHandle* log)
{
    if ((NULL == log) || (NULL == (*log)))
    {
        return;
    }

    OsConfigLog* logToClose = *log;

    if (NULL != logToClose->log)
    {
        fclose(logToClose->log);
    }

    memset(logToClose, 0, sizeof(OsConfigLog));

    free(logToClose);
    *log = NULL;
}

FILE* GetLogFile(OsConfigLogHandle log)
{
    return log ? log->log : NULL;
}

static char g_logTime[TIME_FORMAT_STRING_LENGTH] = {0};

// Returns the local date/time with GMT offset, formatted as YYYY-MM-DD HH:MM:SS-GGGG (for example: 2025-09-26 15:49:55-0700)
char* GetFormattedTime(void)
{
    time_t rawTime = {0};
    struct tm* timeInfo = NULL;
    struct tm result = {0};
    time(&rawTime);
    timeInfo = localtime_r(&rawTime, &result);
    strftime(g_logTime, ARRAY_SIZE(g_logTime), "%Y-%m-%d %H:%M:%S%z", timeInfo);
    return g_logTime;
}

// Checks and rolls the log over if larger than maximum size
void TrimLog(OsConfigLogHandle log)
{
    unsigned int maxLogSize = IsDebugLoggingEnabled() ? ((g_maxLogSize < (UINT_MAX / 5)) ? (g_maxLogSize * 5) : UINT_MAX) : g_maxLogSize;
    unsigned int maxLogTrim = 1000;
    long fileSize = 0;
    int savedErrno = errno;

    if (NULL == log)
    {
        return;
    }

    // Loop incrementing the trim log counter from 0 to maxLogTrim
    log->trimLogCount = (log->trimLogCount < maxLogTrim) ? (log->trimLogCount + 1) : 1;

    // Check every 10 calls:
    if (0 == (log->trimLogCount % 10))
    {
        // In append mode the file pointer will always be at end of file:
        fileSize = ftell(log->log);

        if ((fileSize >= (long)maxLogSize) || (-1 == fileSize))
        {
            fclose(log->log);

            // Rename the log in place to make a backup copy, overwriting previous copy if any:
            if ((NULL == log->backLogFileName) || (0 != rename(log->logFileName, log->backLogFileName)))
            {
                // If the log could not be renamed, empty it:
                log->log = fopen(log->logFileName, "w");
                fclose(log->log);
            }

            // Reopen the log in append mode:
            log->log = fopen(log->logFileName, "a");

            // Reapply restrictions once the file is recreated (also for backup, if any):
            RestrictFileAccessToCurrentAccountOnly(log->logFileName);
            if (NULL != log->backLogFileName)
            {
                // Reapply restrictions to the backup file:
                RestrictFileAccessToCurrentAccountOnly(log->backLogFileName);
            }
        }
    }

    errno = savedErrno;
}

bool IsDaemon()
{
    return (1 == getppid());
}
