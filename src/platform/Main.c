// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <PlatformCommon.h>
#include <MpiServer.h>

// 100 milliseconds
#define DOWORK_SLEEP 100

// 30 seconds
#define DOWORK_INTERVAL 30

// The configuration file for OSConfig
#define CONFIG_FILE "/etc/osconfig/osconfig.json"

// The log file for the platform
#define LOG_FILE "/var/log/osconfig_platform.log"
#define ROLLED_LOG_FILE "/var/log/osconfig_platform.bak"

#define DEBUG_LOGGING "DebugLogging"

static unsigned int g_lastTime = 0;

extern OsConfigLogHandle g_platformLog;

// Signals on which the platform performs a graceful cleanup before terminating.
// Crash signals (SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL) are handled separately by
// the common crash handler (see InstallCrashHandler). SIGKILL is omitted to allow a
// clean and immediate process kill if needed.
static int g_stopSignals[] = {
    0,
    SIGINT,  // 2
    SIGQUIT, // 3
    SIGTERM, //15
    SIGSTOP, //19
    SIGTSTP  //20
};

static int g_stopSignal = 0;
static int g_refreshSignal = 0;

static void SignalInterrupt(int signal)
{
    OsConfigLogInfo(g_platformLog, "Interrupt signal (%d)", signal);
    g_stopSignal = signal;
}

static void SignalReloadConfiguration(int incomingSignal)
{
    g_refreshSignal = incomingSignal;

    // Reset the handler
    signal(SIGHUP, SignalReloadConfiguration);
}

static void Refresh()
{
    MpiShutdown();
    MpiInitialize();

    OsConfigLogInfo(GetPlatformLog(), "OSConfig Platform reintialized");
}

void ScheduleRefresh(void)
{
    OsConfigLogInfo(GetPlatformLog(), "Scheduling refresh");
    g_refreshSignal = SIGHUP;
}

static void InitializePlatform(void)
{
    g_lastTime = (unsigned int)time(NULL);

    MpiInitialize();

    OsConfigLogInfo(GetPlatformLog(), "OSConfig Platform initialized");
}

void TerminatePlatform(void)
{
    MpiShutdown();

    OsConfigLogInfo(GetPlatformLog(), "OSConfig Platform terminated");
}

static void PlatformDoWork(void)
{
    unsigned int currentTime = time(NULL);
    unsigned int timeInterval = DOWORK_INTERVAL;

    if (timeInterval <= (currentTime - g_lastTime))
    {
        MpiDoWork();
        g_lastTime = (unsigned int)time(NULL);
    }
}

int main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    pid_t pid = 0;
    int stopSignalsCount = ARRAY_SIZE(g_stopSignals);

    char* jsonConfiguration = LoadStringFromFile(CONFIG_FILE, false, GetPlatformLog());
    if (NULL != jsonConfiguration)
    {
        SetLoggingLevel(GetLoggingLevelFromJsonConfig(jsonConfiguration, GetPlatformLog()));
        SetMaxLogSize(GetMaxLogSizeFromJsonConfig(jsonConfiguration, GetPlatformLog()));
        SetMaxLogSizeDebugMultiplier(GetMaxLogSizeDebugMultiplierFromJsonConfig(jsonConfiguration, GetPlatformLog()));
        FREE_MEMORY(jsonConfiguration);
    }

    RestrictFileAccessToCurrentAccountOnly(CONFIG_FILE);

    g_platformLog = OpenLog(LOG_FILE, ROLLED_LOG_FILE);

    CheckForPreviousCrash(LOG_FILE, GetPlatformLog());

    OsConfigLogInfo(GetPlatformLog(), "OSConfig Platform starting (PID: %d, PPID: %d)", pid = getpid(), getppid());
    OsConfigLogInfo(GetPlatformLog(), "OSConfig version: %s", OSCONFIG_VERSION);

    if (IsDebugLoggingEnabled())
    {
        OsConfigLogWarning(GetPlatformLog(), "Debug logging is enabled. To disable debug logging, set 'LoggingLevel' to 6 in '%s' and restart OSConfig", CONFIG_FILE);
    }

    for (int i = 0; i < stopSignalsCount; i++)
    {
        signal(g_stopSignals[i], SignalInterrupt);
    }
    signal(SIGHUP, SignalReloadConfiguration);

    InitializePlatform();

    // Install the crash handler after all modules are loaded, in case any install their own:
    InstallCrashHandler(LOG_FILE);

    while (0 == g_stopSignal)
    {
        PlatformDoWork();

        sleep(DOWORK_SLEEP);

        if (0 != g_refreshSignal)
        {
            g_refreshSignal = 0;
            Refresh();
        }
    }

    OsConfigLogInfo(GetPlatformLog(), "OSConfig Platform (PID: %d) exiting with %d", pid, g_stopSignal);

    TerminatePlatform();
    CloseLog(&g_platformLog);

    return 0;
}
