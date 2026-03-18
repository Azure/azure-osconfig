// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// Chaining: if a previous signal handler was registered before this one, it will be invoked after this handler writes its diagnostics.
// This allows multiple OSConfig components (each with their own log path) to all receive the crash signal and each write to their own
// log (in chain order) before the process dies.
//
// On fatal signals the handler executes these steps:
//   1. Select human-readable crash message string for the signal (compile-time literal)
//   2. Open the component log by known path: O_APPEND | O_WRONLY | O_NONBLOCK
//   3. If open succeeded:
//      a. Write crash message line  e.g. "[ERROR] OSConfig NRP crash due to segmentation fault (SIGSEGV)"
//      b. Write last operation line "[ERROR] OSConfig NRP last operation: <g_lastOperation>"
//      c. Write stack trace header  "[ERROR] OSConfig NRP stack trace:"
//      d. backtrace() into stack-allocated buffer
//      e. backtrace_symbols_fd() -- writes frames to log fd, no malloc
//      f. close(fd)
//   4. Chain to previously registered handler if one exists, otherwise:
//      signal(sig, SIG_DFL) + raise(sig) preserves core dump, never suppresses crash

#include "Internal.h"

#define DEFAULT_LOG_FILE  "/var/log/osconfig_nrp.log"
#define DEFAULT_OPERATION  "<unknown>"

#define EOL_TERMINATOR "\n"
#define CRASH_PREFIX "[ERROR] OSConfig NRP crash due to "
#define MSG_SIGSEGV CRASH_PREFIX "segmentation fault (SIGSEGV)"  EOL_TERMINATOR
#define MSG_SIGFPE CRASH_PREFIX "fatal arithmetic error (SIGFPE)" EOL_TERMINATOR
#define MSG_SIGILL CRASH_PREFIX "illegal instruction (SIGILL)" EOL_TERMINATOR
#define MSG_SIGABRT CRASH_PREFIX "abnormal termination (SIGABRT)" EOL_TERMINATOR
#define MSG_SIGBUS CRASH_PREFIX "illegal memory access (SIGBUS)" EOL_TERMINATOR

#define MSG_LAST_OP "[ERROR] OSConfig NRP last operation: "
#define MSG_STACK_HDR "[ERROR] OSConfig NRP stack trace:" EOL_TERMINATOR

#define OSCONFIG_MAX_FRAMES  32
static struct sigaction g_previousHandlers[NSIG];

static const char* g_logFileName = NULL;
static const char* g_lastOperation = NULL;

void TraceOperation(const char* operation)
{
    // Plain assignment of string constant, no memory allocation for copy
    g_lastOperation = operation ? operation : DEFAULT_OPERATION;
}

static int CrashWrite(int fd, const char* s)
{
    return (s ? write(fd, s, strlen(s)) : write(fd, DEFAULT_OPERATION, strlen(DEFAULT_OPERATION)));
}

static void OsConfigCrashHandler(int sig, siginfo_t* info, void* ctx)
{
    void* frames[OSCONFIG_MAX_FRAMES] = { 0 };
    int nFrames = 0;
    int fd = -1;
    const char* errorMessage = NULL;

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
        errorMessage = DEFAULT_OPERATION;
    }

    fd = open(g_logFileName ? g_logFileName : DEFAULT_LOG_FILE, O_APPEND | O_WRONLY | O_NONBLOCK);

    if (fd >= 0)
    {
        CrashWrite(fd, errorMessage);
        CrashWrite(fd, MSG_LAST_OP);
        CrashWrite(fd, g_lastOperation ? g_lastOperation : DEFAULT_OPERATION);
        CrashWrite(fd, EOL_TERMINATOR);
        CrashWrite(fd, MSG_STACK_HDR);
        nFrames = backtrace(frames, OSCONFIG_MAX_FRAMES);
        backtrace_symbols_fd(frames, nFrames, fd);
        close(fd);
    }

    if (sig < NSIG)
    {
        struct sigaction* prev = &g_previousHandlers[sig];

        if (prev->sa_flags & SA_SIGINFO)
        {
            if (prev->sa_sigaction && (prev->sa_sigaction != (void*)SIG_DFL) && (prev->sa_sigaction != (void*)SIG_IGN))
            {
                prev->sa_sigaction(sig, info, ctx);
                return;
            }
        }
        else
        {
            if (prev->sa_handler && (prev->sa_handler != SIG_DFL) && (prev->sa_handler != SIG_IGN))
            {
                prev->sa_handler(sig);
                return;
            }
        }
    }

    signal(sig, SIG_DFL);
    raise(sig);
}

void InstallCrashHandler(const char* logFileName)
{
    memset(g_previousHandlers, 0, sizeof(g_previousHandlers));

    // Plain assignment of string constant, no memory allocation for copy
    g_logFileName = logFileName ? logFileName : DEFAULT_LOG_FILE;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = OsConfigCrashHandler;
    // SA_SIGINFO provides siginfo_t (fault address) to handler
    // SA_RESETHAND is intentionally omitted as it would collapse the handler chain after the first link
    // Re-entry is prevented naturally: the chain always terminates at signal(SIG_DFL) + raise()
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGSEGV, &sa, &g_previousHandlers[SIGSEGV]);
    sigaction(SIGABRT, &sa, &g_previousHandlers[SIGABRT]);
    sigaction(SIGBUS, &sa, &g_previousHandlers[SIGBUS]);
    sigaction(SIGFPE, &sa, &g_previousHandlers[SIGFPE]);
    sigaction(SIGILL, &sa, &g_previousHandlers[SIGILL]);
}
