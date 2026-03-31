// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "Internal.h"

#define EOL_TERMINATOR "\n"
#define CRASH_PREFIX "[ERROR] Crash due to "
#define MSG_SIGSEGV CRASH_PREFIX "segmentation fault (SIGSEGV)"  EOL_TERMINATOR
#define MSG_SIGFPE CRASH_PREFIX "fatal arithmetic error (SIGFPE)" EOL_TERMINATOR
#define MSG_SIGILL CRASH_PREFIX "illegal instruction (SIGILL)" EOL_TERMINATOR
#define MSG_SIGABRT CRASH_PREFIX "abnormal termination (SIGABRT)" EOL_TERMINATOR
#define MSG_SIGBUS CRASH_PREFIX "illegal memory access (SIGBUS)" EOL_TERMINATOR
#define MSG_DEFAULT "<unknown>"
#define DEFAULT_LOG_FILE  "/var/log/osconfig_nrp.log"
#define MSG_STACK_HDR "[ERROR] Stack trace:" EOL_TERMINATOR

#define OSCONFIG_MAX_FRAMES 10

static const char* g_logFileName = DEFAULT_LOG_FILE;

static void OsConfigCrashHandler(int sig, siginfo_t* info, void* ctx)
{
    void* frames[OSCONFIG_MAX_FRAMES] = {NULL};
    int nFrames = 0;
    int logDescriptor = -1;
    const char* errorMessage = NULL;

    UNUSED(info);
    UNUSED(ctx);

    if (SIGSEGV == sig)
    {
        errorMessage = MSG_SIGSEGV;
    }
    else if (SIGFPE == sig)
    {
        errorMessage = MSG_SIGFPE;
    }
    else if (SIGILL == sig)
    {
        errorMessage = MSG_SIGILL;
    }
    else if (SIGABRT == sig)
    {
        errorMessage = MSG_SIGABRT;
    }
    else if (SIGBUS == sig)
    {
        errorMessage = MSG_SIGBUS;
    }
    else
    {
        errorMessage = MSG_DEFAULT;
    }

    if (0 < (logDescriptor = open(g_logFileName, O_APPEND | O_WRONLY | O_NONBLOCK)))
    {
        ssize_t writeResult = write(logDescriptor, (const void*)errorMessage, strlen(errorMessage));
        writeResult = write(logDescriptor, (const void*)MSG_STACK_HDR, strlen(MSG_STACK_HDR));
        UNUSED(writeResult);
        nFrames = backtrace(frames, OSCONFIG_MAX_FRAMES);
        backtrace_symbols_fd(frames, nFrames, logDescriptor);
        close(logDescriptor);
    }

    signal(sig, SIG_DFL);
    raise(sig);
}

void InstallCrashHandler(const char* logFileName)
{
    g_logFileName = logFileName ? logFileName : DEFAULT_LOG_FILE;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = OsConfigCrashHandler;
    // SA_SIGINFO provides siginfo_t (fault address) to handler
    // SA_RESETHAND intentionally omitted as it would collapse the handler chain
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
}

static char* LoadEndOfFile(const char* logFileName, OsConfigLogHandle log)
{
    const int maxSize = 2048;
    int size = 0;
    long offset = 0;
    size_t sizeRead = 0;
    FILE* file = NULL;
    char* string = NULL;

    if (false == FileExists(logFileName))
    {
        return string;
    }

    if (NULL != (file = fopen(logFileName, "r")))
    {
        if (LockFile(file, log))
        {
            fseek(file, 0, SEEK_END);
            size = (int)ftell(file);

            if (size > maxSize)
            {
                size = maxSize;
            }

            offset = (long)(ftell(file) - size);
            fseek(file, offset, SEEK_SET);

            if (NULL != (string = (char*)malloc(size + 1)))
            {
                memset(string, 0, size + 1);
                sizeRead = fread(string, sizeof(char), size, file);
                UNUSED(sizeRead);
            }
            else
            {
                OsConfigLogError(log, "LoadEndOfFile: unable to allocate memory");
            }

            UnlockFile(file, log);
        }

        fclose(file);
    }

    OsConfigLogDebug(log, "LoadEndOfFile: '%s' ends in '%s'", logFileName, string);

    return string;
}

void ParseLogForPreviousCrashIfAny(const char* logFileName, OsConfigLogHandle log)
{
    const char* crashDueToMarker = "[ERROR] Crash due to";
    const char* stackTraceMarker = "[ERROR] Stack trace:";
    char* endOfFile = NULL;
    char* crashStart = NULL;
    char* stackStart = NULL;
    char* endOfLine = NULL;

    if (NULL != (endOfFile = LoadEndOfFile(logFileName, log)))
    {
        if (NULL != (crashStart = strstr(endOfFile, crashDueToMarker)))
        {
            // Search for stack trace before mutating crashStart
            stackStart = strstr(endOfFile, stackTraceMarker);

            // Null-terminate the crash header line
            if (NULL != (endOfLine = strchr(crashStart, '\n')))
            {
                endOfLine[0] = 0;
            }

            OsConfigLogInfo(log, "### Crash start: '%s'", crashStart);
            OsConfigLogInfo(log, "### Stack start: '%s'", stackStart);
        }
        else
        {
            OsConfigLogError(log, "ParseLogForPreviousCrashIfAny: '%s' not found in '%s'", crashDueToMarker, logFileName);
        }
    }
    else
    {
        OsConfigLogError(log, "ParseLogForPreviousCrashIfAny: could not open '%s' (%d, %s)", logFileName, errno, strerror(errno));
    }

    FREE_MEMORY(endOfFile);
}
