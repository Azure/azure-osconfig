// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// 100 milliseconds
#define DOWORK_SLEEP 100

// The log file for the platform
#define LOG_FILE "/var/log/osconfig_platform.log"
#define ROLLED_LOG_FILE "/var/log/osconfig_platform.bak"

// The local Desired Configuration (DC) and Reported Configuration (RC) files
#define DC_FILE "/etc/osconfig/osconfig_desired.json"
#define RC_FILE "/etc/osconfig/osconfig_reported.json"

// The configuration file for OSConfig
#define CONFIG_FILE "/etc/osconfig/osconfig.json"

/////////////////////////////////////////////////////////
#include <stdint.h>
#include <time.h>
#include <pthread.h>

#ifndef __MACH__
extern clockid_t time_basis;
#endif

extern void set_time_basis(void);
extern int get_time_ns(struct timespec* ts);
extern int64_t get_time_ms(void);

#define INVALID_TIME_VALUE (int64_t)(-1)

#define NANOSECONDS_IN_1_SECOND 1000000000L
#define MILLISECONDS_IN_1_SECOND 1000
#define NANOSECONDS_IN_1_MILLISECOND 1000000L

typedef struct TICK_COUNTER_INSTANCE_TAG
{
    int64_t init_time_value;
    tickcounter_ms_t current_ms;
} TICK_COUNTER_INSTANCE;

TICK_COUNTER_HANDLE tickcounter_create(void)
{
    TICK_COUNTER_INSTANCE* result = (TICK_COUNTER_INSTANCE*)malloc(sizeof(TICK_COUNTER_INSTANCE));
    if (result != NULL)
    {
        set_time_basis();

        result->init_time_value = get_time_ms();
        if (result->init_time_value == INVALID_TIME_VALUE)
        {
            free(result);
            result = NULL;
        }
        else
        {
            result->current_ms = 0;
        }
    }
    return result;
}

void tickcounter_destroy(TICK_COUNTER_HANDLE tick_counter)
{
    if (tick_counter != NULL)
    {
        free(tick_counter);
    }
}

int tickcounter_get_current_ms(TICK_COUNTER_HANDLE tick_counter, tickcounter_ms_t * current_ms)
{
    int result;

    if (tick_counter == NULL || current_ms == NULL)
    {
        result = MU_FAILURE;
    }
    else
    {
        int64_t time_value = get_time_ms();
        if (time_value == INVALID_TIME_VALUE)
        {
            result = MU_FAILURE;
        }
        else
        {
            TICK_COUNTER_INSTANCE* tick_counter_instance = (TICK_COUNTER_INSTANCE*)tick_counter;
            tick_counter_instance->current_ms = (tickcounter_ms_t)time_value - tick_counter_instance->init_time_value;
            *current_ms = tick_counter_instance->current_ms;
            result = 0;
        }
    }

    return result;
}

/////////////////////////////////////////////////////////

static TICK_COUNTER_HANDLE g_tickCounter = NULL;
static tickcounter_ms_t g_lastTick = 0;

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
static int g_localManagement = 0;

static int g_reportingInterval = DEFAULT_REPORTING_INTERVAL;

static OSCONFIG_LOG_HANDLE g_platformLog = NULL;
OSCONFIG_LOG_HANDLE GetLog()
{
    return g_platformLog;
}

#define EOL_TERMINATOR "\n"
#define ERROR_MESSAGE_CRASH "[ERROR] OSConfig Platform crash due to "
#define ERROR_MESSAGE_SIGSEGV ERROR_MESSAGE_CRASH "segmentation fault (SIGSEGV)" EOL_TERMINATOR
#define ERROR_MESSAGE_SIGFPE ERROR_MESSAGE_CRASH "fatal arithmetic error (SIGFPE)" EOL_TERMINATOR
#define ERROR_MESSAGE_SIGILL ERROR_MESSAGE_CRASH "illegal instruction (SIGILL)" EOL_TERMINATOR
#define ERROR_MESSAGE_SIGABRT ERROR_MESSAGE_CRASH "abnormal termination (SIGABRT)" EOL_TERMINATOR
#define ERROR_MESSAGE_SIGBUS ERROR_MESSAGE_CRASH "illegal memory access (SIGBUS)" EOL_TERMINATOR

static void SignalInterrupt(int signal)
{
    int logDescriptor = -1;
    char* errorMessage = NULL;
    ssize_t writeResult = -1;

    UNUSED(writeResult);

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
            writeResult = write(logDescriptor, (const void*)errorMessage, strlen(errorMessage));
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
    //TBD refresh the platform, reload modules, etc.
}

void ScheduleRefresh(void)
{
    OsConfigLogInfo(GetLog(), "Scheduling refresh connection");
    g_refreshSignal = SIGHUP;
}

static void SignalChild(int signal)
{
    // No-op
    UNUSED(signal);
}

static bool InitializePlatform(void)
{
    bool status = true;

    if (g_tickCounter = tickcounter_create())
    {
        tickcounter_get_current_ms(g_tickCounter, &g_lastTick);
    }
    else
    {
        LogErrorWithTelemetry(GetLog(), "tickcounter_create failed");
        status = false;
    }

    if (status)
    {
        //MpiInitialize()
        OsConfigLogInfo(GetLog(), "OSConfig Platform intialized");
    }

    return status;
}

void TerminatePlatform(void)
{
    //MpiShutdown()
    OsConfigLogInfo(GetLog(), "OSConfig PnP Agent terminated");
}

static void PlatformDoWork(void)
{
    tickcounter_ms_t nowTick = 0;
    tickcounter_ms_t intervalTick = g_reportingInterval * 1000;
    tickcounter_get_current_ms(g_tickCounter, &nowTick);

    if (intervalTick <= (nowTick - g_lastTick))
    {
        //MpiDoWork
        tickcounter_get_current_ms(g_tickCounter, &g_lastTick);
    }
}

int main(int argc, char *argv[])
{
    pid_t pid = 0;
    int stopSignalsCount = ARRAY_SIZE(g_stopSignals);

    char* jsonConfiguration LoadStringFromFile(CONFIG_FILE, false, GetLog());
    if (NULL != jsonConfiguration)
    {
        SetFullLogging(IsFullLoggingEnabledInJsonConfig(jsonConfiguration));
        FREE_MEMORY(jsonConfiguration);
    }

    g_platformLog = OpenLog(LOG_FILE, ROLLED_LOG_FILE);

    OsConfigLogInfo(GetLog(), "OSConfig Platform starting (PID: %d, PPID: %d)", pid = getpid(), getppid());
    OsConfigLogInfo(GetLog(), "OSConfig version: %s", OSCONFIG_VERSION);

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetLog(), "WARNING: full logging is enabled. To disable full logging edit %s and restart OSConfig", CONFIG_FILE);
    }

    // Load remaining configuration
    jsonConfiguration = LoadStringFromFile(CONFIG_FILE, false, GetLog());
    if (NULL != jsonConfiguration)
    {
        g_reportingInterval = GetReportingIntervalFromJsonConfig(jsonConfiguration);
        g_localManagement = GetLocalManagementFromJsonConfig(jsonConfiguration);
        FREE_MEMORY(jsonConfiguration);
    }

    for (int i = 0; i < stopSignalsCount; i++)
    {
        signal(g_stopSignals[i], SignalInterrupt);
    }
    signal(SIGHUP, SignalReloadConfiguration);

    if (!InitializePlatform())
    {
        LogErrorWithTelemetry(GetLog(), "Failed to initialize the OSConfig Platform");
        goto done;
    }

    while (0 == g_stopSignal)
    {
        PlatformDoWork();
        ThreadAPI_Sleep(DOWORK_SLEEP);

        if (0 != g_refreshSignal)
        {
            Refresh();
            g_refreshSignal = 0;
        }
    }

done:
    OsConfigLogInfo(GetLog(), "OSConfig Platform (PID: %d) exiting with %d", pid, g_stopSignal);

    TerminatePlatform();
    CloseLog(&g_platformLog);

    return 0;
}