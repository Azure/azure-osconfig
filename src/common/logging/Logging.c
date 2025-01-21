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

static bool g_fullLoggingEnabled = false;

typedef struct OSCONFIG_LOG
{
    FILE* log;
    const char* logFileName;
    const char* backLogFileName;
    unsigned int trimLogCount;
} OSCONFIG_LOG;

void SetFullLogging(bool fullLogging)
{
    g_fullLoggingEnabled = fullLogging;
}

bool IsFullLoggingEnabled()
{
    return g_fullLoggingEnabled;
}

static int RestrictFileAccessToCurrentAccountOnly(const char* fileName)
{
    return chmod(fileName, S_ISUID | S_ISGID | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IXUSR | S_IXGRP);
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

    return (OSCONFIG_LOG_HANDLE)newLog;
}

void CloseLog(OSCONFIG_LOG_HANDLE* log)
{
    if ((NULL == log) || (NULL == (*log)))
    {
        return;
    }

    OSCONFIG_LOG* logToClose = (OSCONFIG_LOG*)(*log);

    if (NULL != logToClose->log)
    {
        fclose(logToClose->log);
    }

    memset(logToClose, 0, sizeof(OSCONFIG_LOG));

    free(logToClose);
    log = NULL;
}

FILE* GetLogFile(OSCONFIG_LOG_HANDLE log)
{
    return log ? ((OSCONFIG_LOG*)log)->log : NULL;
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
    OSCONFIG_LOG* whatLog = NULL;
    int fileSize = 0;

    if ((NULL == log) || (NULL == (whatLog = (OSCONFIG_LOG*)log)))
    {
        return;
    }

    // Loop incrementing the trim log counter from 0 to MAX_LOG_TRIM
    whatLog->trimLogCount = (whatLog->trimLogCount < MAX_LOG_TRIM) ? (whatLog->trimLogCount + 1) : 1;

    // Check every 10 calls:
    if (0 == (whatLog->trimLogCount % 10))
    {
        // In append mode the file pointer will always be at end of file:
        fileSize = ftell(whatLog->log);
        
        if ((fileSize >= MAX_LOG_SIZE) || (-1 == fileSize))
        {
            fclose(whatLog->log);

            // Rename the log in place to make a backup copy, overwriting previous copy if any:
            if ((NULL == whatLog->backLogFileName) || (0 != rename(whatLog->logFileName, whatLog->backLogFileName)))
            {
                // If the log could not be renamed, empty it:
                whatLog->log = fopen(whatLog->logFileName, "w");
                fclose(whatLog->log);
            }

            // Reopen the log in append mode:
            whatLog->log = fopen(whatLog->logFileName, "a");
            
            // Reapply restrictions once the file is recreated (also for backup, if any):
            RestrictFileAccessToCurrentAccountOnly(whatLog->logFileName);
            RestrictFileAccessToCurrentAccountOnly(whatLog->backLogFileName);
        }
    }
}

bool IsDaemon()
{
    return (1 == getppid());
}