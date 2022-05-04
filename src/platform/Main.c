// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <PlatformCommon.h>

// 100 milliseconds
#define DOWORK_SLEEP 100

// 30 seconds
#define DOWORK_INTERVAL 30

// The log file for the platform
#define LOG_FILE "/var/log/osconfig_platform.log"
#define ROLLED_LOG_FILE "/var/log/osconfig_platform.bak"

//TBD: replace the log objects in MM and Orchestrator with the one here and export: OSCONFIG_LOG_HANDLE GetLog();

// The local Desired Configuration (DC) and Reported Configuration (RC) files
#define DC_FILE "/etc/osconfig/osconfig_desired.json"
#define RC_FILE "/etc/osconfig/osconfig_reported.json"

// The configuration file for OSConfig
#define CONFIG_FILE "/etc/osconfig/osconfig.json"

#define FULL_LOGGING "FullLogging"

static unsigned int g_lastTime = 0;

OSCONFIG_LOG_HANDLE g_platformLog = NULL;
OSCONFIG_LOG_HANDLE GetLog()
{
    return g_platformLog;
}

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
    //TBD: add a MpiRefresh to MM?
    MpiShutdown();
    MpiInitialize();
}

void ScheduleRefresh(void)
{
    OsConfigLogInfo(GetLog(), "Scheduling refresh");
    g_refreshSignal = SIGHUP;
}

static bool InitializePlatform(void)
{
    g_lastTime = (unsigned int)time(NULL);
    
    MpiInitialize();
    
    OsConfigLogInfo(GetLog(), "OSConfig Platform intialized");
    
    return true;
}

void TerminatePlatform(void)
{
    MpiShutdown();
    
    OsConfigLogInfo(GetLog(), "OSConfig Platform terminated");
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

static bool IsFullLoggingEnabledInJsonConfig(const char* jsonString)
{
    bool result = false;
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;

    if (NULL != jsonString)
    {
        if (NULL != (rootValue = json_parse_string(jsonString)))
        {
            if (NULL != (rootObject = json_value_get_object(rootValue)))
            {
                result = (0 == (int)json_object_get_number(rootObject, FULL_LOGGING)) ? false : true;
            }
            json_value_free(rootValue);
        }
    }

    return result;
}

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);
    
    pid_t pid = 0;
    int stopSignalsCount = ARRAY_SIZE(g_stopSignals);

    char* jsonConfiguration = LoadStringFromFile(CONFIG_FILE, false, GetLog());
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

    for (int i = 0; i < stopSignalsCount; i++)
    {
        signal(g_stopSignals[i], SignalInterrupt);
    }
    signal(SIGHUP, SignalReloadConfiguration);

    if (!InitializePlatform())
    {
        OsConfigLogError(GetLog(), "Failed to initialize the OSConfig Platform");
        goto done;
    }

    while (0 == g_stopSignal)
    {
        PlatformDoWork();
        
        sleep(DOWORK_SLEEP);

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