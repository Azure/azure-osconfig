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

struct OsConfigLog
{
    FILE* log;
    const char* logFileName;
    const char* backLogFileName;
    unsigned int trimLogCount;
};

static LoggingLevel g_loggingLevel = LoggingLevelInformational;

void SetLoggingLevel(LoggingLevel level)
{
    g_loggingLevel = (level > LoggingLevelNotice) ? level : LoggingLevelInformational;
}

LoggingLevel GetLoggingLevel(void)
{
    return g_loggingLevel;
}

bool IsDebugLoggingEnabled(void)
{
    return (LoggingLevelDebug == g_loggingLevel) ? true : false;
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

// Returns the local date/time formatted as YYYY-MM-DD HH:MM:SS (for example: 2014-03-19 11:11:52)
char* GetFormattedTime(void)
{
    time_t rawTime = {0};
    struct tm* timeInfo = NULL;
    time(&rawTime);
    timeInfo = localtime(&rawTime);
    strftime(g_logTime, ARRAY_SIZE(g_logTime), "%Y-%m-%d %H:%M:%S", timeInfo);
    return g_logTime;
}

// Checks and rolls the log over if larger than maximum size
void TrimLog(OsConfigLogHandle log)
{
    // Try to increase the maximum debug log size 5 times
    int maxLogSize = IsDebugLoggingEnabled() ? ((MAX_LOG_SIZE < (INT_MAX / 5)) ? (MAX_LOG_SIZE * 5) : INT_MAX) : MAX_LOG_SIZE;
    int maxLogTrim = 1000;

    int fileSize = 0;
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

        if ((fileSize >= maxLogSize) || (-1 == fileSize))
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
            RestrictFileAccessToCurrentAccountOnly(log->backLogFileName);
        }
    }

    errno = savedErrno;
}

bool IsDaemon()
{
    return (1 == getppid());
}
