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
//      signal(sig, SIG_DFL) + raise(sig) -- preserves core dump, never suppresses crash

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

// Saved previous handlers per signal -- populated at registration time.
// Allows chaining: after writing diagnostics, this handler invokes the
// previously registered handler (if any) before falling through to SIG_DFL.
static struct sigaction g_previousHandlers[NSIG];

static const char* g_logFileName = NULL;
static const char* g_lastOperation = NULL;

void TraceOperation(const char* operation)
{
    // Plain assignment of string constant, no memory allocation for copy
    g_lastOperation = operation ? operation : DEFAULT_OPERATION;
}

static void CrashWrite(int fd, const char* s)
{
    if (s)
    {
        write(fd, s, strlen(s));
    }
}

// Signal handler
static void OsConfigCrashHandler(int sig, siginfo_t* info, void* ctx)
{
    void* frames[OSCONFIG_MAX_FRAMES] = { 0 };
    int nFrames = 0;
    int fd = -1;
    const char* errorMessage = NULL;

    // Step 1: select compile-time crash message for this signal (code simplified here for legibility)
    if (SIGSEGV == sig) errorMessage = MSG_SIGSEGV;
    else if (SIGFPE == sig) errorMessage = MSG_SIGFPE;
    else if (SIGILL == sig) errorMessage = MSG_SIGILL;
    else if (SIGABRT == sig) errorMessage = MSG_SIGABRT;
    else if (SIGBUS == sig) errorMessage = MSG_SIGBUS;

    // Step 2: open log by known path (O_NONBLOCK): never block in signal handler
    fd = open(g_logFileName ? g_logFileName : DEFAULT_LOG_FILE, O_APPEND | O_WRONLY | O_NONBLOCK);

    // Step 3: write diagnostics if open succeeded
    if (fd >= 0)
    {
        // 3a. Human-readable crash line
        CrashWrite(fd, errorMessage);

        // 3b. Last operation marker
        CrashWrite(fd, MSG_LAST_OP);
        CrashWrite(fd, g_lastOperation ? g_lastOperation : DEFAULT_OPERATION);
        CrashWrite(fd, EOL_TERMINATOR);

        // 3c. Stack trace header
        CrashWrite(fd, MSG_STACK_HDR);

        // 3d+3e. Capture and write symbolic stack trace: backtrace_symbols_fd uses no malloc
        nFrames = backtrace(frames, OSCONFIG_MAX_FRAMES);
        backtrace_symbols_fd(frames, nFrames, fd);

        // 3f. Close log fd
        close(fd);
    }

    // Step 4: chain to previously registered handler if one exists
    if (sig < NSIG)
    {
        struct sigaction* prev = &g_previousHandlers[sig];

        if (prev->sa_flags & SA_SIGINFO)
        {
            // Previous handler used the SA_SIGINFO three-argument form
            if (prev->sa_sigaction && (prev->sa_sigaction != (void*)SIG_DFL) && (prev->sa_sigaction != (void*)SIG_IGN))
            {
                prev->sa_sigaction(sig, info, ctx);
                return;
            }
        }
        else
        {
            // Previous handler used the simple one-argument form
            if (prev->sa_handler && (prev->sa_handler != SIG_DFL) && (prev->sa_handler != SIG_IGN))
            {
                prev->sa_handler(sig);
                return;
            }
        }
    }

    // No previous handler -- restore default disposition and re-raise.
    // raise() delivers the signal with SIG_DFL: process terminates and core dump is generated, exactly as if this handler never existed.
    signal(sig, SIG_DFL);
    raise(sig);
}

// Each component that registers its own handler passes its own known log path, enabling the chain,
// each handler writes to its own log before passing to the next.
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
