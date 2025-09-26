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

extern char g_mpiCall[MPI_CALL_MESSAGE_LENGTH];

// All signals on which we want the agent to cleanup before terminating process.
// SIGKILL is omitted to allow a clean and immediate process kill if needed.
static int g_stopSignals[] = {
    0,
    SIGINT,  // 2
    SIGQUIT, // 3
    SIGILL,  // 4
    SIGABRT, // 6
    SIGBUS,  // 7
    SIGFPE,  // 8
    SIGSEGV, //11
    SIGTERM, //15
    SIGSTOP, //19
    SIGTSTP  //20
};

static int g_stopSignal = 0;
static int g_refreshSignal = 0;

#define EOL_TERMINATOR "\n"
#define ERROR_MESSAGE_CRASH "[ERROR] OSConfig Platform crash due to "
#define ERROR_MESSAGE_SIGSEGV ERROR_MESSAGE_CRASH "segmentation fault (SIGSEGV)"
#define ERROR_MESSAGE_SIGFPE ERROR_MESSAGE_CRASH "fatal arithmetic error (SIGFPE)"
#define ERROR_MESSAGE_SIGILL ERROR_MESSAGE_CRASH "illegal instruction (SIGILL)"
#define ERROR_MESSAGE_SIGABRT ERROR_MESSAGE_CRASH "abnormal termination (SIGABRT)"
#define ERROR_MESSAGE_SIGBUS ERROR_MESSAGE_CRASH "illegal memory access (SIGBUS)"

static void SignalInterrupt(int signal)
{
    int logDescriptor = -1;
    char* errorMessage = NULL;
    size_t sizeOfMpiMessage = 0;
    ssize_t writeResult = -1;

    if (SIGSEGV == signal)
    {
        errorMessage = ERROR_MESSAGE_SIGSEGV;
    }
    else if (SIGFPE == signal)
    {
        errorMessage = ERROR_MESSAGE_SIGFPE;
    }
    else if (SIGILL == signal)
    {
        errorMessage = ERROR_MESSAGE_SIGILL;
    }
    else if (SIGABRT == signal)
    {
        errorMessage = ERROR_MESSAGE_SIGABRT;
    }
    else if (SIGBUS == signal)
    {
        errorMessage = ERROR_MESSAGE_SIGBUS;
    }
    else
    {
        OsConfigLogInfo(g_platformLog, "Interrupt signal (%d)", signal);
        g_stopSignal = signal;
    }

    if (NULL != errorMessage)
    {
        if (0 < (logDescriptor = open(LOG_FILE, O_APPEND | O_WRONLY | O_NONBLOCK)))
        {
            if (0 < write(logDescriptor, (const void*)errorMessage, strlen(errorMessage)))
            {
                sizeOfMpiMessage = strlen(g_mpiCall);
                if (sizeOfMpiMessage > 0)
                {
                    writeResult = write(logDescriptor, (const void*)(&g_mpiCall[0]), sizeOfMpiMessage);
                }
                else
                {
                    writeResult = write(logDescriptor, (const void*)EOL_TERMINATOR, sizeof(char));
                }
                UNUSED(writeResult);
            }
            close(logDescriptor);
        }
        _exit(signal);
    }
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

    OsConfigLogInfo(GetPlatformLog(), "OSConfig Platform intialized");
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
    OSConfigTelemetryOpen(g_platformLog);

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
    OSConfigTelemetryClose();
    CloseLog(&g_platformLog);

    return 0;
}
