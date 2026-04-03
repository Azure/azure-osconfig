// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "inc/AgentCommon.h"
#include "inc/Agent.h"
#include "inc/Watcher.h"

// 100 milliseconds
#define DOWORK_SLEEP 100

// The log file for the agent
#define LOG_FILE "/var/log/osconfig_pnp_agent.log"
#define ROLLED_LOG_FILE "/var/log/osconfig_pnp_agent.bak"

// The configuration file for OSConfig
#define CONFIG_FILE "/etc/osconfig/osconfig.json"

// The optional second command line argument that when present instructs the agent to run as a traditional daemon
#define FORK_ARG "fork"

#define DEVICE_MODEL_ID_SIZE 40
#define DEVICE_PRODUCT_NAME_SIZE 128
#define DEVICE_PRODUCT_INFO_SIZE 1024

static unsigned int g_lastTime = 0;

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

enum AgentExitState
{
    NoError = 0,
    NoConnectionString = 1,
    initializationFailure = 2,
    PlatformInitializationFailure = 3
};
typedef enum AgentExitState AgentExitState;
static AgentExitState g_exitState = NoError;

static int g_stopSignal = 0;
static int g_refreshSignal = 0;

MPI_HANDLE g_mpiHandle = NULL;
static unsigned int g_maxPayloadSizeBytes = OSCONFIG_MAX_PAYLOAD;

static OsConfigLogHandle g_agentLog = NULL;

static int g_modelVersion = DEFAULT_DEVICE_MODEL_ID;
static int g_reportingInterval = DEFAULT_REPORTING_INTERVAL;

static const char g_productNameTemplate[] = "Azure OSConfig %d;%s";
static char g_productName[DEVICE_PRODUCT_NAME_SIZE] = {0};

// Alternate OSConfig own format for product info: "Azure OSConfig %d;%s;%s %s %s %s %s %lu %lu;%s %s %s;%s %s;"
static const char g_productInfoTemplate[] = "Azure OSConfig %d;%s "
    "(\"os_name\"=\"%s\"&os_version\"=\"%s\"&"
    "\"cpu_type\"=\"%s\"&\"cpu_vendor\"=\"%s\"&\"cpu_model\"=\"%s\"&"
    "\"total_memory\"=\"%lu\"&\"free_memory\"=\"%lu\"&"
    "\"kernel_name\"=\"%s\"&\"kernel_release\"=\"%s\"&\"kernel_version\"=\"%s\"&"
    "\"product_vendor\"=\"%s\"&\"product_name\"=\"%s\")";
static char g_productInfo[DEVICE_PRODUCT_INFO_SIZE] = {0};

OsConfigLogHandle GetLog()
{
    return g_agentLog;
}

#define EOL_TERMINATOR "\n"
#define ERROR_MESSAGE_CRASH "[ERROR] OSConfig crash due to "
#define ERROR_MESSAGE_SIGSEGV ERROR_MESSAGE_CRASH "segmentation fault (SIGSEGV)" EOL_TERMINATOR
#define ERROR_MESSAGE_SIGFPE ERROR_MESSAGE_CRASH "fatal arithmetic error (SIGFPE)" EOL_TERMINATOR
#define ERROR_MESSAGE_SIGILL ERROR_MESSAGE_CRASH "illegal instruction (SIGILL)" EOL_TERMINATOR
#define ERROR_MESSAGE_SIGABRT ERROR_MESSAGE_CRASH "abnormal termination (SIGABRT)" EOL_TERMINATOR
#define ERROR_MESSAGE_SIGBUS ERROR_MESSAGE_CRASH "illegal memory access (SIGBUS)" EOL_TERMINATOR

static void SignalInterrupt(int signal)
{
    int logDescriptor = -1;
    char* errorMessage = NULL;

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
        OsConfigLogInfo(g_agentLog, "Interrupt signal (%d)", signal);
        g_stopSignal = signal;
    }

    if (NULL != errorMessage)
    {
        if (0 < (logDescriptor = open(LOG_FILE, O_APPEND | O_WRONLY | O_NONBLOCK)))
        {
            ssize_t writeResult = -1;
            writeResult = write(logDescriptor, (const void*)errorMessage, strlen(errorMessage));
            UNUSED(writeResult);
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

/*static void RefreshConnection()
{
}*/

void ScheduleRefreshConnection(void)
{
    OsConfigLogInfo(GetLog(), "Scheduling refresh connection");
    g_refreshSignal = SIGHUP;
}

static void SignalChild(int signal)
{
    // No-op for this version of the agent
    UNUSED(signal);
}

static void ForkDaemon()
{
    OsConfigLogInfo(GetLog(), "Attempting to fork daemon process");

    int status = 0;

    UNUSED(status);

    pid_t pidDaemon = fork();
    if (pidDaemon < 0)
    {
        OsConfigLogError(GetLog(), "fork() failed, could not fork daemon process");
        exit(EXIT_FAILURE);
    }

    if (pidDaemon > 0)
    {
        // This is in the parent process, terminate it
        OsConfigLogInfo(GetLog(), "fork() succeeded, terminating parent");
        exit(EXIT_SUCCESS);
    }

    // The forked daemon process becomes session leader
    if (setsid() < 0)
    {
        OsConfigLogError(GetLog(), "setsid() failed, could not fork daemon process");
        exit(EXIT_FAILURE);
    }

    signal(SIGCHLD, SignalChild);
    signal(SIGHUP, SignalReloadConfiguration);

    // Fork off for the second time
    pidDaemon = fork();
    if (pidDaemon < 0)
    {
        OsConfigLogError(GetLog(), "Second fork() failed, could not fork daemon process");
        exit(EXIT_FAILURE);
    }

    if (pidDaemon > 0)
    {
        OsConfigLogInfo(GetLog(), "Second fork() succeeded, terminating parent");
        exit(EXIT_SUCCESS);
    }

    // Set new file permissions
    umask(0);

    // Change the working directory to the root directory
    status = chdir("/");
    LogAssert(GetLog(), 0 == status);

    // Close all open file descriptors
    for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--)
    {
        close(x);
    }
}

bool RefreshMpiClientSession(bool* platformAlreadyRunning)
{
    bool status = true;

    if (g_mpiHandle && IsDaemonActive(OSCONFIG_PLATFORM, GetLog()))
    {
        // Platform is already running

        if (NULL != platformAlreadyRunning)
        {
            *platformAlreadyRunning = true;
        }

        return status;
    }

    if (NULL != platformAlreadyRunning)
    {
        *platformAlreadyRunning = false;
    }

    if (true == (status = EnableAndStartDaemon(OSCONFIG_PLATFORM, GetLog())))
    {
        sleep(1);

        if (NULL == (g_mpiHandle = CallMpiOpen(g_productName, g_maxPayloadSizeBytes, GetLog())))
        {
            OsConfigLogError(GetLog(), "MpiOpen failed");
            g_exitState = PlatformInitializationFailure;
            status = false;
        }
    }
    else
    {
        OsConfigLogError(GetLog(), "The OSConfig Platform cannot be started");
        g_exitState = PlatformInitializationFailure;
    }

    return status;
}

static bool InitializeAgent(void)
{
    bool status = true;

    g_lastTime = (unsigned int)time(NULL);

    if (0 == (status = RefreshMpiClientSession(NULL)))
    {
        if (false == IsWatcherActive())
        {
            g_exitState = initializationFailure;
            status = false;
        }
    }

    if (status)
    {
        OsConfigLogInfo(GetLog(), "The OSConfig Agent session is now intialized");
    }

    return status;
}

void CloseAgent(void)
{
    if (NULL != g_mpiHandle)
    {
        CallMpiClose(g_mpiHandle, GetLog());
        g_mpiHandle = NULL;
    }

    OsConfigLogInfo(GetLog(), "The OSConfig Agent session is closed");
}

static void AgentDoWork(void)
{
    unsigned int currentTime = time(NULL);
    unsigned int timeInterval = g_reportingInterval;

    if (timeInterval <= (currentTime - g_lastTime))
    {
        // Process RCD/DC and/or Git clones DC files
        WatcherDoWork(GetLog());

        g_lastTime = (unsigned int)time(NULL);
    }
}

int main(int argc, char *argv[])
{
    char* jsonConfiguration = NULL;
    int stopSignalsCount = ARRAY_SIZE(g_stopSignals);
    bool forkDaemon = false;
    pid_t pid = 0;
    char* osName = NULL;
    char* osVersion = NULL;
    char* cpuType = NULL;
    char* cpuVendor = NULL;
    char* cpuModel = NULL;
    long totalMemory = 0;
    long freeMemory = 0;
    char* kernelName = NULL;
    char* kernelRelease = NULL;
    char* kernelVersion = NULL;
    char* productName = NULL;
    char* productVendor = NULL;
    char* encodedProductInfo = NULL;

    forkDaemon = (bool)(((3 == argc) && (NULL != argv[2]) && (0 == strcmp(argv[2], FORK_ARG))) ||
        ((2 == argc) && (NULL != argv[1]) && (0 == strcmp(argv[1], FORK_ARG))));

    jsonConfiguration = LoadStringFromFile(CONFIG_FILE, false, GetLog());
    if (NULL != jsonConfiguration)
    {
        SetLoggingLevel(GetLoggingLevelFromJsonConfig(jsonConfiguration, GetLog()));
        SetMaxLogSize(GetMaxLogSizeFromJsonConfig(jsonConfiguration, GetLog()));
        SetMaxLogSizeDebugMultiplier(GetMaxLogSizeDebugMultiplierFromJsonConfig(jsonConfiguration, GetLog()));
        FREE_MEMORY(jsonConfiguration);
    }

    g_agentLog = OpenLog(LOG_FILE, ROLLED_LOG_FILE);

    if (forkDaemon)
    {
        ForkDaemon();
    }

    // Re-open the log
    CloseLog(&g_agentLog);
    g_agentLog = OpenLog(LOG_FILE, ROLLED_LOG_FILE);

    OsConfigLogInfo(GetLog(), "OSConfig Agent starting (PID: %d, PPID: %d)", pid = getpid(), getppid());
    OsConfigLogInfo(GetLog(), "OSConfig version: %s", OSCONFIG_VERSION);

    if (IsDebugLoggingEnabled())
    {
        OsConfigLogWarning(GetLog(), "Debug logging is enabled. To disable debug logging, set 'LoggingLevel' to 6 in '%s' and restart OSConfig", CONFIG_FILE);
    }

    // Load remaining configuration
    jsonConfiguration = LoadStringFromFile(CONFIG_FILE, false, GetLog());
    if (NULL != jsonConfiguration)
    {
        g_reportingInterval = GetReportingIntervalFromJsonConfig(jsonConfiguration, GetLog());
    }

    RestrictFileAccessToCurrentAccountOnly(CONFIG_FILE);

    snprintf(g_productName, sizeof(g_productName), g_productNameTemplate, g_modelVersion, OSCONFIG_VERSION);
    OsConfigLogInfo(GetLog(), "Product name: %s", g_productName);

    osName = GetOsName(GetLog());
    osVersion = GetOsVersion(GetLog());
    cpuType = GetCpuType(GetLog());
    cpuVendor = GetCpuVendor(GetLog());
    cpuModel = GetCpuModel(GetLog());
    totalMemory = GetTotalMemory(GetLog());
    freeMemory = GetFreeMemory(GetLog());
    kernelName = GetOsKernelName(GetLog());
    kernelRelease = GetOsKernelRelease(GetLog());
    kernelVersion = GetOsKernelVersion(GetLog());
    productVendor = GetProductVendor(GetLog());
    productName = GetProductName(GetLog());

    snprintf(g_productInfo, sizeof(g_productInfo), g_productInfoTemplate, g_modelVersion, OSCONFIG_VERSION, osName, osVersion,
        cpuType, cpuVendor, cpuModel, totalMemory, freeMemory, kernelName, kernelRelease, kernelVersion, productVendor, productName);

    if (NULL != (encodedProductInfo = UrlEncode(g_productInfo)))
    {
        if (strlen(encodedProductInfo) >= sizeof(g_productInfo))
        {
            OsConfigLogError(GetLog(), "Encoded product info string is too long (%d bytes, over maximum of %d bytes) and will be truncated",
                (int)strlen(encodedProductInfo), (int)sizeof(g_productInfo));
        }

        memset(g_productInfo, 0, sizeof(g_productInfo));
        memcpy(g_productInfo, encodedProductInfo, sizeof(g_productInfo) - 1);
    }

    OsConfigLogDebug(GetLog(), "Product info: '%s' (%d bytes)", g_productInfo, (int)strlen(g_productInfo));

    FREE_MEMORY(osName);
    FREE_MEMORY(osVersion);
    FREE_MEMORY(cpuType);
    FREE_MEMORY(cpuVendor);
    FREE_MEMORY(cpuModel);
    FREE_MEMORY(kernelName);
    FREE_MEMORY(kernelRelease);
    FREE_MEMORY(kernelVersion);
    FREE_MEMORY(productName);
    FREE_MEMORY(productVendor);
    FREE_MEMORY(encodedProductInfo);

    for (int i = 0; i < stopSignalsCount; i++)
    {
        signal(g_stopSignals[i], SignalInterrupt);
    }
    signal(SIGHUP, SignalReloadConfiguration);

    if (false == InitializeAgent())
    {
        OsConfigLogError(GetLog(), "Failed to initialize the OSConfig Agent");
        goto done;
    }

    // Call the Watcher to initialize itself
    InitializeWatcher(jsonConfiguration, GetLog());
    FREE_MEMORY(jsonConfiguration);

    while (0 == g_stopSignal)
    {
        AgentDoWork();

        SleepMilliseconds(DOWORK_SLEEP);

        if (0 != g_refreshSignal)
        {
            //RefreshConnection();
            g_refreshSignal = 0;
        }
    }

done:
    OsConfigLogInfo(GetLog(), "OSConfig Agent (PID: %d) exiting with %d", pid, g_stopSignal);

    FREE_MEMORY(jsonConfiguration);

    WatcherCleanup(GetLog());

    CloseAgent();

    StopAndDisableDaemon(OSCONFIG_PLATFORM, GetLog());

    CloseLog(&g_agentLog);

    return 0;
}
