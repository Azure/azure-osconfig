// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "inc/AgentCommon.h"
#include "inc/PnpUtils.h"
#include "inc/PnpAgent.h"
#include "inc/AisUtils.h"
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

static int g_iotHubProtocol = PROTOCOL_AUTO;

static REPORTED_PROPERTY* g_reportedProperties = NULL;
static int g_numReportedProperties = 0;

static unsigned int g_lastTime = 0;

extern IOTHUB_DEVICE_CLIENT_LL_HANDLE g_moduleHandle;

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
    IotHubInitializationFailure = 2,
    PlatformInitializationFailure = 3
};
typedef enum AgentExitState AgentExitState;
static AgentExitState g_exitState = NoError;

enum ConnectionStringSource
{
    FromAis = 0,
    FromFile = 1,
    FromCommandline = 2
};
typedef enum ConnectionStringSource ConnectionStringSource;
static ConnectionStringSource g_connectionStringSource = FromAis;

static int g_stopSignal = 0;
static int g_refreshSignal = 0;

static bool g_isIotHubEnabled = false;
static char* g_iotHubConnectionString = NULL;
const char* g_iotHubConnectionStringPrefix = "HostName=";

// Obtained from AIS alongside the connection string in case of X.509 authentication
static char* g_x509Certificate = NULL;
static char* g_x509PrivateKeyHandle = NULL;

// HTTP proxy options read from environment variables
static HTTP_PROXY_OPTIONS g_proxyOptions = {0};

MPI_HANDLE g_mpiHandle = NULL;
static unsigned int g_maxPayloadSizeBytes = OSCONFIG_MAX_PAYLOAD;

static OSCONFIG_LOG_HANDLE g_agentLog = NULL;

static int g_modelVersion = DEFAULT_DEVICE_MODEL_ID;
static int g_reportingInterval = DEFAULT_REPORTING_INTERVAL;

static const char g_modelIdTemplate[] = "dtmi:osconfig:deviceosconfiguration;%d";
static char g_modelId[DEVICE_MODEL_ID_SIZE] = {0};

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

OSCONFIG_LOG_HANDLE GetLog()
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
        OsConfigLogInfo(g_agentLog, "Interrupt signal (%d)", signal);
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

static IOTHUB_DEVICE_CLIENT_LL_HANDLE CallIotHubInitialize(void)
{
    IOTHUB_DEVICE_CLIENT_LL_HANDLE moduleHandle = NULL;

    if (g_isIotHubEnabled)
    {
        if (NULL == (moduleHandle = IotHubInitialize(g_modelId, g_productInfo, g_iotHubConnectionString, false, g_x509Certificate, g_x509PrivateKeyHandle,
            &g_proxyOptions, (PROTOCOL_MQTT_WS == g_iotHubProtocol) ? MQTT_WebSocket_Protocol : MQTT_Protocol)))
        {
            OsConfigLogError(GetLog(), "IotHubInitialize failed, failed to initialize connection to IoT Hub");
            IotHubDeInitialize();
        }
    }

    return moduleHandle;
}

static void RefreshConnection()
{
    char* connectionString = NULL;

    FREE_MEMORY(g_x509Certificate);
    FREE_MEMORY(g_x509PrivateKeyHandle);

    if (g_isIotHubEnabled)
    {
        // If initialized with AIS, try to get a new connection string same way:
        if ((FromAis == g_connectionStringSource) && (NULL != (connectionString = RequestConnectionStringFromAis(&g_x509Certificate, &g_x509PrivateKeyHandle))))
        {
            FREE_MEMORY(g_iotHubConnectionString);
            if (0 != mallocAndStrcpy_s(&g_iotHubConnectionString, connectionString))
            {
                OsConfigLogError(GetLog(), "RefreshConnection: out of memory making copy of the connection string");
                FREE_MEMORY(connectionString);
            }
        }
        else
        {
            if (FromAis == g_connectionStringSource)
            {
                // No new connection string from AIS, try to refresh using the existing connection string before bailing out:
                OsConfigLogError(GetLog(), "RefreshConnection: failed to obtain a new connection string from AIS, trying refresh with existing connection string");
            }
        }

        IotHubDeInitialize();
        g_moduleHandle = NULL;

        if ((NULL == g_moduleHandle) && (NULL != g_iotHubConnectionString))
        {
            if (NULL == (g_moduleHandle = CallIotHubInitialize()))
            {
                if (FromAis == g_connectionStringSource)
                {
                    FREE_MEMORY(g_iotHubConnectionString);
                }
                else if (!IsWatcherActive())
                {
                    g_exitState = IotHubInitializationFailure;
                    SignalInterrupt(SIGQUIT);
                }
            }
        }
    }
}

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

static void SignalProcessDesired(int incomingSignal)
{
    UNUSED(incomingSignal);
    if (g_isIotHubEnabled)
    {
        OsConfigLogInfo(GetLog(), "Processing desired twin updates");
        ProcessDesiredTwinUpdates();

        // Reset the signal handler for the next use otherwise the default handler will be invoked instead
        signal(SIGUSR1, SignalProcessDesired);
    }
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
        if (g_isIotHubEnabled && g_iotHubConnectionString)
        {
            if (NULL == (g_moduleHandle = CallIotHubInitialize()))
            {
                if (FromAis == g_connectionStringSource)
                {
                    // We will try to get a new connnection string from AIS and try to connect with that
                    FREE_MEMORY(g_iotHubConnectionString);
                }
                else if (false == IsWatcherActive())
                {
                    g_exitState = IotHubInitializationFailure;
                    status = false;
                }
            }
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
    if (g_isIotHubEnabled)
    {
        IotHubDeInitialize();
    }

    if (NULL != g_mpiHandle)
    {
        CallMpiClose(g_mpiHandle, GetLog());
        g_mpiHandle = NULL;
    }

    FREE_MEMORY(g_reportedProperties);

    OsConfigLogInfo(GetLog(), "The OSConfig Agent session is closed");
}

static void ReportProperties()
{
    if ((g_numReportedProperties <= 0) || (NULL == g_reportedProperties))
    {
        // No properties to report
        return;
    }

    for (int i = 0; i < g_numReportedProperties; i++)
    {
        if ((strlen(g_reportedProperties[i].componentName) > 0) && (strlen(g_reportedProperties[i].propertyName) > 0))
        {
            ReportPropertyToIotHub(g_reportedProperties[i].componentName, g_reportedProperties[i].propertyName, &(g_reportedProperties[i].lastPayloadHash));
        }
    }
}

static void AgentDoWork(void)
{
    char* connectionString = NULL;

    unsigned int currentTime = time(NULL);
    unsigned int timeInterval = g_reportingInterval;

    if (timeInterval <= (currentTime - g_lastTime))
    {
        if (g_isIotHubEnabled && (NULL == g_iotHubConnectionString) && (FromAis == g_connectionStringSource))
        {
            IotHubDeInitialize();

            if (NULL != (connectionString = RequestConnectionStringFromAis(&g_x509Certificate, &g_x509PrivateKeyHandle)))
            {
                if (0 == mallocAndStrcpy_s(&g_iotHubConnectionString, connectionString))
                {
                    if (NULL == (g_moduleHandle = CallIotHubInitialize()))
                    {
                        FREE_MEMORY(g_iotHubConnectionString);
                    }
                }
                else
                {
                    OsConfigLogError(GetLog(), "AgentDoWork: out of memory making copy of the connection string");
                    g_exitState = IotHubInitializationFailure;
                    SignalInterrupt(SIGQUIT);
                }
            }
            else
            {
                OsConfigLogError(GetLog(), "AgentDoWork: failed to obtain a connection string from AIS, to retry");
            }
        }

        // Process RCD/DC and/or Git clones DC files (for Iot Hub this is signaled to be done with SIGUSR1)
        WatcherDoWork(GetLog());

        // Process reported updates to the IoT Hub
        if (g_isIotHubEnabled && g_moduleHandle)
        {
            ReportProperties();
        }

        g_lastTime = (unsigned int)time(NULL);
    }
    else if (g_isIotHubEnabled)
    {
        IotHubDoWork();
    }
}

int main(int argc, char *argv[])
{
    char* connectionString = NULL;
    char* jsonConfiguration = NULL;
    char* proxyData = NULL;
    char* proxyHostAddress = NULL;
    int proxyPort = 0;
    char* proxyUsername = NULL;
    char* proxyPassword = NULL;
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
        SetCommandLogging(IsCommandLoggingEnabledInJsonConfig(jsonConfiguration));
        SetFullLogging(IsFullLoggingEnabledInJsonConfig(jsonConfiguration));
        FREE_MEMORY(jsonConfiguration);
    }

    g_agentLog = OpenLog(LOG_FILE, ROLLED_LOG_FILE);

    if (forkDaemon)
    {
        ForkDaemon();
    }

    g_connectionStringSource = FromAis;

    // Re-open the log
    CloseLog(&g_agentLog);
    g_agentLog = OpenLog(LOG_FILE, ROLLED_LOG_FILE);

    OsConfigLogInfo(GetLog(), "OSConfig Agent starting (PID: %d, PPID: %d)", pid = getpid(), getppid());
    OsConfigLogInfo(GetLog(), "OSConfig version: %s", OSCONFIG_VERSION);

    if (IsCommandLoggingEnabled() || IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetLog(), "WARNING: verbose logging (command and/or full) is enabled. To disable verbose logging edit %s and restart OSConfig", CONFIG_FILE);
    }

    // Load remaining configuration
    jsonConfiguration = LoadStringFromFile(CONFIG_FILE, false, GetLog());
    if (NULL != jsonConfiguration)
    {
        g_modelVersion = GetModelVersionFromJsonConfig(jsonConfiguration, GetLog());
        g_numReportedProperties = LoadReportedFromJsonConfig(jsonConfiguration, &g_reportedProperties, GetLog());
        g_reportingInterval = GetReportingIntervalFromJsonConfig(jsonConfiguration, GetLog());
        g_isIotHubEnabled = IsIotHubManagementEnabledInJsonConfig(jsonConfiguration);
        g_iotHubProtocol = GetIotHubProtocolFromJsonConfig(jsonConfiguration, GetLog());
    }

    RestrictFileAccessToCurrentAccountOnly(CONFIG_FILE);

    snprintf(g_productName, sizeof(g_productName), g_productNameTemplate, g_modelVersion, OSCONFIG_VERSION);
    OsConfigLogInfo(GetLog(), "Product name: %s", g_productName);

    snprintf(g_modelId, sizeof(g_modelId), g_modelIdTemplate, g_modelVersion);
    OsConfigLogInfo(GetLog(), "Model id: %s", g_modelId);

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

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetLog(), "Product info: '%s' (%d bytes)", g_productInfo, (int)strlen(g_productInfo));
    }

    FREE_MEMORY(osName);
    FREE_MEMORY(osVersion);
    FREE_MEMORY(cpuType);
    FREE_MEMORY(cpuVendor);
    FREE_MEMORY(cpuModel);
    FREE_MEMORY(productName);
    FREE_MEMORY(productVendor);
    FREE_MEMORY(encodedProductInfo);

    if (g_isIotHubEnabled)
    {
        OsConfigLogInfo(GetLog(), "Protocol: %s", (PROTOCOL_MQTT_WS == g_iotHubProtocol) ? "MQTT over Web Socket" : "MQTT");

        if (PROTOCOL_MQTT_WS == g_iotHubProtocol)
        {
            // Read the proxy options from environment variables, parse and fill the HTTP_PROXY_OPTIONS structure to pass to the SDK:
            if (NULL != (proxyData = GetHttpProxyData(GetLog())))
            {
                if (ParseHttpProxyData((const char*)proxyData, &proxyHostAddress, &proxyPort, &proxyUsername, &proxyPassword, GetLog()))
                {
                    // Assign the string pointers and trasfer ownership to the SDK
                    g_proxyOptions.host_address = proxyHostAddress;
                    g_proxyOptions.port = proxyPort;
                    g_proxyOptions.username = proxyUsername;
                    g_proxyOptions.password = proxyPassword;

                    FREE_MEMORY(proxyData);
                }
            }
        }

        if ((argc < 2) || ((2 == argc) && forkDaemon))
        {
            g_connectionStringSource = FromAis;
            if (NULL != (connectionString = RequestConnectionStringFromAis(&g_x509Certificate, &g_x509PrivateKeyHandle)))
            {
                if (0 != mallocAndStrcpy_s(&g_iotHubConnectionString, connectionString))
                {
                    OsConfigLogError(GetLog(), "Out of memory making copy of the connection string from AIS");
                    g_exitState = NoConnectionString;
                    goto done;
                }
            }
            else
            {
                OsConfigLogError(GetLog(), "Failed to obtain a connection string from AIS, to retry");
            }
        }
        else
        {
            if (0 == strncmp(argv[1], g_iotHubConnectionStringPrefix, strlen(g_iotHubConnectionStringPrefix)))
            {
                g_connectionStringSource = FromCommandline;
                if (0 != mallocAndStrcpy_s(&connectionString, argv[1]))
                {
                    OsConfigLogError(GetLog(), "Out of memory making copy of the connection string from the command line");
                    g_exitState = NoConnectionString;
                    goto done;
                }
            }
            else
            {
                g_connectionStringSource = FromFile;
                connectionString = LoadStringFromFile(argv[1], true, GetLog());
                if (NULL == connectionString)
                {
                    OsConfigLogError(GetLog(), "Failed to load a connection string from %s", argv[1]);

                    if (!IsWatcherActive())
                    {
                        g_exitState = NoConnectionString;
                        goto done;
                    }
                }
            }
        }

        if (connectionString && (0 != mallocAndStrcpy_s(&g_iotHubConnectionString, connectionString)))
        {
            OsConfigLogError(GetLog(), "Out of memory making copy of the connection string");
            goto done;
        }
    }

    for (int i = 0; i < stopSignalsCount; i++)
    {
        signal(g_stopSignals[i], SignalInterrupt);
    }
    signal(SIGHUP, SignalReloadConfiguration);
    signal(SIGUSR1, SignalProcessDesired);

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
            RefreshConnection();
            g_refreshSignal = 0;
        }
    }

done:
    OsConfigLogInfo(GetLog(), "OSConfig Agent (PID: %d) exiting with %d", pid, g_stopSignal);

    FREE_MEMORY(g_x509Certificate);
    FREE_MEMORY(g_x509PrivateKeyHandle);
    FREE_MEMORY(connectionString);
    FREE_MEMORY(g_iotHubConnectionString);

    WatcherCleanup(GetLog());

    CloseAgent();

    StopAndDisableDaemon(OSCONFIG_PLATFORM, GetLog());

    CloseLog(&g_agentLog);

    // Once the SDK is done, we can free these

    if (g_proxyOptions.host_address)
    {
        free((void *)g_proxyOptions.host_address);
    }

    if (g_proxyOptions.username)
    {
        free((void *)g_proxyOptions.username);
    }

    if (g_proxyOptions.password)
    {
        free((void *)g_proxyOptions.password);
    }

    return 0;
}
