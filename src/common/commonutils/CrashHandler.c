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
#define MSG_STACK_HDR ("[ERROR] Stack trace:" EOL_TERMINATOR)

#define OSCONFIG_MAX_FRAMES 8

#define OSCONFIG_MAX_STACK_SIZE 2048

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

void CheckForPreviousCrash(const char* logFileName, OsConfigLogHandle log)
{
    char* endOfFile = NULL;
    char* crashStart = NULL;
    size_t length = 0;
    char* p = NULL;

    if ((NULL == logFileName) || (false == FileExists(logFileName)))
    {
        return;
    }

    if (NULL != (endOfFile = ReadEndOfFile(logFileName, OSCONFIG_MAX_STACK_SIZE, log)))
    {
        if (NULL != (crashStart = strstr(endOfFile, CRASH_PREFIX)))
        {
            OsConfigLogInfo(log, "Previous crash detected:");
            length = strlen(crashStart);
            if (EOL == crashStart[length - 1])
            {
                crashStart[length - 1] = 0;
            }
            OsConfigLogError(log, "%s", crashStart);

            p = crashStart;
            while (*p)
            {
                if (EOL == *p)
                {
                    *p = (*(p + 1)) ? ';' : 0;
                }
                p++;
            }

            //OsConfigLogDebug(log, "For telemetry (with EFAULT): '%s'", crashStart);
            for (int i = 0; i < 1000; i++)
            {
                TelemetryInitialize(log);
                OsConfigLogInfo(log, "For telemetry (with EFAULT): '%s'", crashStart);
                for (int j = 0; j < 1000; j++)
                {
                    OSConfigTelemetryStatusTrace(crashStart, EFAULT);
                    OSConfigTelemetryStatusTrace("***EFAULT***", EFAULT);
                }
                TelemetryCleanup(log);
            }
        }
    }

    FREE_MEMORY(endOfFile);
}
