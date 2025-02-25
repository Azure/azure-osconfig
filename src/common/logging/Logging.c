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
#include "Logging.h"

#define MAX_LOG_TRIM 1000

static LoggingLevel g_loggingLevel = LoggingLevelInformational;

struct OSCONFIG_LOG
{
    FILE* log;
    const char* logFileName;
    const char* backLogFileName;
    unsigned int trimLogCount;
};

void SetDebugLogging(bool fullLogging)
{
    g_loggingLevel = fullLogging ? LoggingLevelDebug : LoggingLevelInformational;
}

bool IsDebugLoggingEnabled(void)
{
    return (LoggingLevelDebug == g_loggingLevel) ? true : false;
}

static int RestrictFileAccessToCurrentAccountOnly(const char* fileName)
{
    return chmod(fileName, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
}

OSCONFIG_LOG_HANDLE OpenLog(const char* logFileName, const char* bakLogFileName)
{
    OSCONFIG_LOG* newLog = (OSCONFIG_LOG*)malloc(sizeof(OSCONFIG_LOG));
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

void CloseLog(OSCONFIG_LOG_HANDLE* log)
{
    if ((NULL == log) || (NULL == (*log)))
    {
        return;
    }

    OSCONFIG_LOG* logToClose = *log;

    if (NULL != logToClose->log)
    {
        fclose(logToClose->log);
    }

    memset(logToClose, 0, sizeof(OSCONFIG_LOG));

    free(logToClose);
    *log = NULL;
}

FILE* GetLogFile(OSCONFIG_LOG_HANDLE log)
{
    return log ? log->log : NULL;
}

static char g_logTime[TIME_FORMAT_STRING_LENGTH] = {0};

// Returns the local date/time formatted as YYYY-MM-DD HH:MM:SS (for example: 2014-03-19 11:11:52)
char* GetFormattedTime()
{
    time_t rawTime = {0};
    struct tm* timeInfo = NULL;
    time(&rawTime);
    timeInfo = localtime(&rawTime);
    strftime(g_logTime, ARRAY_SIZE(g_logTime), "%Y-%m-%d %H:%M:%S", timeInfo);
    return g_logTime;
}

// Checks and rolls the log over if larger than MAX_LOG_SIZE
void TrimLog(OSCONFIG_LOG_HANDLE log)
{
    int fileSize = 0;
    int savedErrno = errno;

    if (NULL == log)
    {
        return;
    }

    // Loop incrementing the trim log counter from 0 to MAX_LOG_TRIM
    log->trimLogCount = (log->trimLogCount < MAX_LOG_TRIM) ? (log->trimLogCount + 1) : 1;

    // Check every 10 calls:
    if (0 == (log->trimLogCount % 10))
    {
        // In append mode the file pointer will always be at end of file:
        fileSize = ftell(log->log);

        if ((fileSize >= MAX_LOG_SIZE) || (-1 == fileSize))
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
