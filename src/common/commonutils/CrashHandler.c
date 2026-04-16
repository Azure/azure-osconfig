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

#define PERF_LOG_FILE "/var/log/osconfig_telemetry_perf.log"
#define ROLLED_PERF_LOG_FILE "/var/log/osconfig_telemetry_perf.bak"

void CheckForPreviousCrash(const char* logFileName, bool logTelemetry, OsConfigLogHandle log)
{
    char* endOfFile = NULL;
    char* crashStart = NULL;
    char* p = NULL;
    char* crashInfo = NULL;
    size_t length = 0;
    PerfClock perfClock = {{0, 0}, {0, 0}};
    OsConfigLogHandle perfLog = {0};

    if ((NULL == logFileName) || (false == FileExists(logFileName)))
    {
        return NULL;
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

            perfLog = OpenLog(PERF_LOG_FILE, ROLLED_PERF_LOG_FILE);

            crashInfo = DuplicateString(crashStart);

            OsConfigLogInfo(log, "For telemetry (with EFAULT): '%s'", crashInfo);

            for (int i = 0; i < 10000; i++)
            {
                if (logTelemetry)
                {
                    StartPerfClock(&perfClock, perfLog);
                    TelemetryInitialize(log);
                    for (int j = 0; j < 3; j++)
                    {
                        OSConfigTelemetryStatusTrace(crashInfo, EFAULT);
                    }
                    TelemetryCleanup(log);
                    StopPerfClock(&perfClock, perfLog);
                    LogPerfClock(&perfClock, "TelemetryCleanup", NULL, 0, 10000000, perfLog);
                }
            }
            CloseLog(&perfLog);
        }
    }

    FREE_MEMORY(endOfFile);
    FREE_MEMORY(crashInfo);
}
