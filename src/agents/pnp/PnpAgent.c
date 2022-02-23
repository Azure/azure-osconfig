// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "inc/AgentCommon.h"
#include "inc/PnpUtils.h"
#include "inc/MpiProxy.h"
#include "inc/PnpAgent.h"
#include "inc/AisUtils.h"

// TraceLogging Provider UUID: CF452C24-662B-4CC5-9726-5EFE827DB281
TRACELOGGING_DEFINE_PROVIDER(g_providerHandle, "Microsoft.Azure.OsConfigAgent",
    (0xcf452c24, 0x662b, 0x4cc5, 0x97, 0x26, 0x5e, 0xfe, 0x82, 0x7d, 0xb2, 0x81));

// 30 seconds
#define DEFAULT_REPORTING_INTERVAL 30

// 1 second
#define MIN_REPORTING_INTERVAL 1

// 24 hours
#define MAX_REPORTING_INTERVAL 86400

// 100 milliseconds
#define DOWORK_SLEEP 100

// The log file for the agent
#define LOG_FILE "/var/log/osconfig_pnp_agent.log"
#define ROLLED_LOG_FILE "/var/log/osconfig_pnp_agent.bak"

// The local Desired Configuration (DC) and Reported Configuration (RC) files
#define DC_FILE "/etc/osconfig/osconfig_desired.json"
#define RC_FILE "/etc/osconfig/osconfig_reported.json"

// The optional second command line argument that when present instructs the agent to run as a traditional daemon
#define FORK_ARG "fork"

#define MODEL_VERSION_NAME "ModelVersion"
#define REPORTED_NAME "Reported"
#define REPORTED_COMPONENT_NAME "ComponentName"
#define REPORTED_SETTING_NAME "ObjectName"
#define REPORTING_INTERVAL_SECONDS "ReportingIntervalSeconds"
#define LOCAL_PRIORITY "LocalPriority"
#define LOCAL_REPORTING "LocalReporting"

#define PROTOCOL "Protocol"
#define PROTOCOL_AUTO 0
#define PROTOCOL_MQTT 1
#define PROTOCOL_MQTT_WS 2
static int g_protocol = PROTOCOL_AUTO;

#define DEFAULT_DEVICE_MODEL_ID 4
#define MIN_DEVICE_MODEL_ID 3
#define MAX_DEVICE_MODEL_ID 999
#define DEVICE_MODEL_ID_SIZE 40
#define DEVICE_PRODUCT_INFO_SIZE 256

#define MAX_COMPONENT_NAME 256
typedef struct REPORTED_PROPERTY
{
    char componentName[MAX_COMPONENT_NAME];
    char propertyName[MAX_COMPONENT_NAME];
    size_t lastPayloadHash;
} REPORTED_PROPERTY;

static REPORTED_PROPERTY* g_reportedProperties = NULL;
static int g_numReportedProperties = 0;

static TICK_COUNTER_HANDLE g_tickCounter = NULL;
static tickcounter_ms_t g_lastTick = 0;

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
    MpiInitializationFailure = 3
};
typedef enum AgentExitState AgentExitState;
static AgentExitState g_exitState = NoError;

static int g_stopSignal = 0;
static int g_refreshSignal = 0;

static char* g_iotHubConnectionString = NULL;
const char* g_iotHubConnectionStringPrefix = "HostName=";
static bool g_iotHubConnectionStringFromAis = false;

// Obtained from AIS alongside the connection string in case of X.509 authentication
static char* g_x509Certificate = NULL;
static char* g_x509PrivateKeyHandle = NULL;

// HTTP proxy options read from environment variables
static HTTP_PROXY_OPTIONS g_proxyOptions = {0};

MPI_HANDLE g_mpiHandle = NULL;
static unsigned int g_maxPayloadSizeBytes = OSCONFIG_MAX_PAYLOAD;

static OSCONFIG_LOG_HANDLE g_agentLog = NULL;

extern char g_mpiCall[MPI_CALL_MESSAGE_LENGTH];

static const char g_configFile[] = "/etc/osconfig/osconfig.json";
static const char g_fullLoggingValue[] = "FullLogging";

static int g_modelVersion = DEFAULT_DEVICE_MODEL_ID;
static int g_reportingInterval = DEFAULT_REPORTING_INTERVAL;

static const char g_modelIdTemplate[] = "dtmi:osconfig:deviceosconfiguration;%d";
static char g_modelId[DEVICE_MODEL_ID_SIZE] = {0};

static const char g_productInfoTemplate[] = "Azure OSConfig %d;%s";
static char g_productInfo[DEVICE_PRODUCT_INFO_SIZE] = {0};

static size_t g_reportedHash = 0;
static size_t g_desiredHash = 0;

static int g_localPriority = 0;
static int g_localReporting = 0;

OSCONFIG_LOG_HANDLE GetLog()
{
    return g_agentLog;
}

void InitTraceLogging(void)
{
    TraceLoggingRegister(g_providerHandle);
}

void CloseTraceLogging(void)
{
    TraceLoggingUnregister(g_providerHandle);
}

#define ERROR_MESSAGE_CRASH "[ERROR] OSConfig crash due to "
#define ERROR_MESSAGE_SIGSEGV ERROR_MESSAGE_CRASH "segmentation fault (SIGSEGV)"
#define ERROR_MESSAGE_SIGFPE ERROR_MESSAGE_CRASH "fatal arithmetic error (SIGFPE)"
#define ERROR_MESSAGE_SIGILL ERROR_MESSAGE_CRASH "illegal instruction (SIGILL)"
#define ERROR_MESSAGE_SIGABRT ERROR_MESSAGE_CRASH "abnormal termination (SIGABRT)"
#define ERROR_MESSAGE_SIGBUS ERROR_MESSAGE_CRASH "illegal memory access (SIGBUS)"
#define EOL_TERMINATOR "\n"

static void SignalInterrupt(int signal)
{
    int logDescriptor = -1;
    char* errorMessage = NULL;
    size_t errorMessageSize = 0;
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
        OsConfigLogInfo(g_agentLog, "Interrupt signal (%d)", signal);
        g_stopSignal = signal;
    }

    if (NULL != errorMessage)
    {
        errorMessageSize = strlen(errorMessage);

        if (0 < (logDescriptor = open(LOG_FILE, O_APPEND | O_WRONLY | O_NONBLOCK)))
        {
            if (0 < (writeResult = write(logDescriptor, (const void*)errorMessage, errorMessageSize)))
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
            }
            close(logDescriptor);
        }

        _exit(signal);
    }
}

static void SignalReloadConfiguration(int signal)
{
    g_refreshSignal = signal;
}

static void RefreshConnection()
{
    char* connectionString = NULL;

    FREE_MEMORY(g_x509Certificate);
    FREE_MEMORY(g_x509PrivateKeyHandle);

    // If initialized with AIS, try to get a new connection string same way:
    if (g_iotHubConnectionStringFromAis && (NULL != (connectionString = RequestConnectionStringFromAis(&g_x509Certificate, &g_x509PrivateKeyHandle))))
    {
        FREE_MEMORY(g_iotHubConnectionString);
        if (0 != mallocAndStrcpy_s(&g_iotHubConnectionString, connectionString))
        {
            LogErrorWithTelemetry(GetLog(), "RefreshConnection: out of memory making copy of the connection string");
            FREE_MEMORY(connectionString);
        }
    }
    else
    {
        if (g_iotHubConnectionStringFromAis)
        {
            // No new connection string from AIS, try to refresh using the existing connection string before bailing out:
            OsConfigLogError(GetLog(), "RefreshConnection: failed to obtain a new connection string from AIS, trying refresh with existing connection string");
        }
        connectionString = g_iotHubConnectionString;
    }

    if (NULL != connectionString)
    {
        // Reinitialize communication with the IoT Hub:
        IotHubDeInitialize();
        if (NULL == (g_moduleHandle = IotHubInitialize(g_modelId, g_productInfo, connectionString, false, 
            g_x509Certificate, g_x509PrivateKeyHandle, &g_proxyOptions, (PROTOCOL_MQTT_WS == g_protocol) ? MQTT_WebSocket_Protocol : MQTT_Protocol)))
        {
            LogErrorWithTelemetry(GetLog(), "RefreshConnection: IotHubInitialize failed");
            g_exitState = IotHubInitializationFailure;
            SignalInterrupt(SIGQUIT);
        }
    }
    else
    {
        LogErrorWithTelemetry(GetLog(), "RefreshConnection: no connection string");
        g_exitState = NoConnectionString;
        SignalInterrupt(SIGQUIT);
    }
}

void ScheduleRefreshConnection(void)
{
    OsConfigLogInfo(GetLog(), "Scheduling refresh connection");
    SignalReloadConfiguration(SIGHUP);
}

static void SignalChild(int signal)
{
    // No-op for this version of the agent
    UNUSED(signal);
}

static void SignalProcessDesired(int signal)
{
    OsConfigLogInfo(GetLog(), "Processing desired twin updates");
    ProcessDesiredTwinUpdates();
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
        LogErrorWithTelemetry(GetLog(), "fork() failed, could not fork daemon process");
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
        LogErrorWithTelemetry(GetLog(), "setsid() failed, could not fork daemon process");
        exit(EXIT_FAILURE);
    }

    signal(SIGCHLD, SignalChild);
    signal(SIGHUP, SignalReloadConfiguration);

    // Fork off for the second time
    pidDaemon = fork();
    if (pidDaemon < 0)
    {
        LogErrorWithTelemetry(GetLog(), "Second fork() failed, could not fork daemon process");
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

static bool IsFullLoggingInJsonConfig(const char* jsonString)
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
                result = (0 == (int)json_object_get_number(rootObject, g_fullLoggingValue)) ? false : true;
            }
            json_value_free(rootValue);
        }
    }
    return result;
}

static int GetIntegerFromJsonConfig(const char* valueName, const char* jsonString, int defaultValue, int minValue, int maxValue)
{
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;
    int valueToReturn = defaultValue;

    if (NULL == valueName)
    {
        LogErrorWithTelemetry(GetLog(), "GetIntegerFromJsonConfig: no %s value, using default (%d)", valueName, defaultValue);
        return valueToReturn;
    }

    if (minValue >= maxValue)
    {
        LogErrorWithTelemetry(GetLog(), "GetIntegerFromJsonConfig: bad min (%d) and/or max (%d) values for %s, using default (%d)",
            minValue, maxValue, valueName, defaultValue);
        return valueToReturn;
    }

    if (NULL != jsonString)
    {
        if (NULL != (rootValue = json_parse_string(jsonString)))
        {
            if (NULL != (rootObject = json_value_get_object(rootValue)))
            {
                valueToReturn = (int)json_object_get_number(rootObject, valueName);
                if (0 == valueToReturn)
                {
                    valueToReturn = defaultValue;
                    OsConfigLogInfo(GetLog(), "GetIntegerFromJsonConfig: %s value not found or 0, using default (%d)", valueName, defaultValue);
                }
                else if (valueToReturn < minValue)
                {
                    LogErrorWithTelemetry(GetLog(), "GetIntegerFromJsonConfig: %s value %d too small, using minimum (%d)", valueName, valueToReturn, minValue);
                    valueToReturn = minValue;
                }
                else if (valueToReturn > maxValue)
                {
                    LogErrorWithTelemetry(GetLog(), "GetIntegerFromJsonConfig: %s value %d too big, using maximum (%d)", valueName, valueToReturn, maxValue);
                    valueToReturn = maxValue;
                }
                else
                {
                    OsConfigLogInfo(GetLog(), "GetIntegerFromJsonConfig: %s: %d", valueName, valueToReturn);
                }
            }
            else
            {
                LogErrorWithTelemetry(GetLog(), "GetIntegerFromJsonConfig: json_value_get_object(root) failed, using default (%d) for %s", defaultValue, valueName);
            }
            json_value_free(rootValue);
        }
        else
        {
            LogErrorWithTelemetry(GetLog(), "GetIntegerFromJsonConfig: json_parse_string failed, using default (%d) for %s", defaultValue, valueName);
        }
    }
    else
    {
        LogErrorWithTelemetry(GetLog(), "GetIntegerFromJsonConfig: no configuration data, using default (%d) for %s", defaultValue, valueName);
    }

    return valueToReturn;
}

static int GetReportingIntervalFromJsonConfig(const char* jsonString)
{
    return g_reportingInterval = GetIntegerFromJsonConfig(REPORTING_INTERVAL_SECONDS, jsonString, DEFAULT_REPORTING_INTERVAL, MIN_REPORTING_INTERVAL, MAX_REPORTING_INTERVAL);
}

static int GetModelVersionFromJsonConfig(const char* jsonString)
{
    return g_modelVersion = GetIntegerFromJsonConfig(MODEL_VERSION_NAME, jsonString, DEFAULT_DEVICE_MODEL_ID, MIN_DEVICE_MODEL_ID, MAX_DEVICE_MODEL_ID);
}

static int GetLocalPriorityFromJsonConfig(const char* jsonString)
{
    return g_localPriority = GetIntegerFromJsonConfig(LOCAL_PRIORITY, jsonString, 0, 0, 1);
}

static int GetLocalReportingFromJsonConfig(const char* jsonString)
{
    return g_localReporting = GetIntegerFromJsonConfig(LOCAL_REPORTING, jsonString, 0, 0, 1);
}

static int GetProtocolFromJsonConfig(const char* jsonString)
{
    return g_protocol = GetIntegerFromJsonConfig(PROTOCOL, jsonString, PROTOCOL_AUTO, PROTOCOL_AUTO, PROTOCOL_MQTT_WS);
}

static int LoadReportedFromJsonConfig(const char* jsonString)
{
    JSON_Value* rootValue = NULL;
    JSON_Object* rootObject = NULL;
    JSON_Object* itemObject = NULL;
    JSON_Array* reportedArray = NULL;
    const char* componentName = NULL;
    const char* propertyName = NULL;
    size_t numReported = 0;
    size_t bufferSize = 0;
    size_t i = 0;

    g_numReportedProperties = 0;
    FREE_MEMORY(g_reportedProperties);

    if (NULL != jsonString)
    {
        if (NULL != (rootValue = json_parse_string(jsonString)))
        {
            if (NULL != (rootObject = json_value_get_object(rootValue)))
            {
                reportedArray = json_object_get_array(rootObject, REPORTED_NAME);
                if (NULL != reportedArray)
                {
                    numReported = json_array_get_count(reportedArray);
                    OsConfigLogInfo(GetLog(), "LoadReportedFromJsonConfig: found %d %s entries in configuration", (int)numReported, REPORTED_NAME);

                    if (numReported > 0)
                    {
                        bufferSize = numReported * sizeof(REPORTED_PROPERTY);
                        g_reportedProperties = (REPORTED_PROPERTY*)malloc(bufferSize);
                        if (NULL != g_reportedProperties)
                        {
                            memset(g_reportedProperties, 0, bufferSize);
                            g_numReportedProperties = (int)numReported;

                            for (i = 0; i < numReported; i++)
                            {
                                itemObject = json_array_get_object(reportedArray, i);
                                if (NULL != itemObject)
                                {
                                    componentName = json_object_get_string(itemObject, REPORTED_COMPONENT_NAME);
                                    propertyName = json_object_get_string(itemObject, REPORTED_SETTING_NAME);

                                    if ((NULL != componentName) && (NULL != propertyName))
                                    {
                                        strncpy(g_reportedProperties[i].componentName, componentName, ARRAY_SIZE(g_reportedProperties[i].componentName) - 1);
                                        strncpy(g_reportedProperties[i].propertyName, propertyName, ARRAY_SIZE(g_reportedProperties[i].propertyName) - 1);

                                        OsConfigLogInfo(GetLog(), "LoadReportedFromJsonConfig: found report property candidate at position %d of %d: %s.%s", (int)(i + 1),
                                            g_numReportedProperties, g_reportedProperties[i].componentName, g_reportedProperties[i].propertyName);
                                    }
                                    else
                                    {
                                        LogErrorWithTelemetry(GetLog(), "LoadReportedFromJsonConfig: %s or %s missing at position %d of %d, no property to report",
                                            REPORTED_COMPONENT_NAME, REPORTED_SETTING_NAME, (int)(i + 1), (int)numReported);
                                    }
                                }
                                else
                                {
                                    LogErrorWithTelemetry(GetLog(), "LoadReportedFromJsonConfig: json_array_get_object failed at position %d of %d, no reported property",
                                        (int)(i + 1), (int)numReported);
                                }
                            }
                        }
                        else
                        {
                            LogErrorWithTelemetry(GetLog(), "LoadReportedFromJsonConfig: out of memory, cannot allocate %d bytes for %d reported properties",
                                (int)bufferSize, (int)numReported);
                        }
                    }
                }
                else
                {
                    LogErrorWithTelemetry(GetLog(), "LoadReportedFromJsonConfig: no valid %s array in configuration, no properties to report", REPORTED_NAME);
                }
            }
            else
            {
                LogErrorWithTelemetry(GetLog(), "LoadReportedFromJsonConfig: json_value_get_object(root) failed, no properties to report");
            }

            json_value_free(rootValue);
        }
        else
        {
            LogErrorWithTelemetry(GetLog(), "LoadReportedFromJsonConfig: json_parse_string failed, no properties to report");
        }
    }
    else
    {
        LogErrorWithTelemetry(GetLog(), "LoadReportedFromJsonConfig: no configuration data, no properties to report");
    }

    return g_numReportedProperties;
}

static char* GetHttpProxyData()
{
    const char* proxyVariables[] = {
        "http_proxy",
        "https_proxy",
        "HTTP_PROXY",
        "HTTPS_PROXY"
    };
    int proxyVariablesSize = ARRAY_SIZE(proxyVariables);

    char* proxyData = NULL;
    char* environmentVariable = NULL;
    int i = 0;

    for (i = 0; i < proxyVariablesSize; i++)
    {
        environmentVariable = getenv(proxyVariables[i]);
        if (NULL != environmentVariable)
        {
            // The environment variable string must be treated as read-only, make a copy for our use:
            proxyData = strdup(environmentVariable);
            if (NULL == proxyData)
            {
                LogErrorWithTelemetry(GetLog(), "Cannot make a copy of the %s variable: %d", proxyVariables[i], errno);
            }
            else
            {
                OsConfigLogInfo(GetLog(), "Proxy data from %s: %s", proxyVariables[i], proxyData);
            }
            break;
        }
    }

    return proxyData;
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
    bool freeConnectionString = false;
    int stopSignalsCount = ARRAY_SIZE(g_stopSignals);
    bool forkDaemon = false;
    pid_t pid = 0;

    forkDaemon = (bool)(((3 == argc) && (NULL != argv[2]) && (0 == strcmp(argv[2], FORK_ARG))) ||
        ((2 == argc) && (NULL != argv[1]) && (0 == strcmp(argv[1], FORK_ARG))));

    jsonConfiguration = LoadStringFromFile(g_configFile, false);
    if (NULL != jsonConfiguration)
    {
        SetFullLogging(IsFullLoggingInJsonConfig(jsonConfiguration));
        FREE_MEMORY(jsonConfiguration);
    }

    g_agentLog = OpenLog(LOG_FILE, ROLLED_LOG_FILE);
    InitTraceLogging();

    if (forkDaemon)
    {
        ForkDaemon();
    }

    g_iotHubConnectionStringFromAis = false;

    // Re-open the log
    CloseLog(&g_agentLog);
    g_agentLog = OpenLog(LOG_FILE, ROLLED_LOG_FILE);

    OsConfigLogInfo(GetLog(), "OSConfig PnP Agent starting (PID: %d, PPID: %d)", pid = getpid(), getppid());
    OsConfigLogInfo(GetLog(), "OSConfig version: %s", OSCONFIG_VERSION);

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetLog(), "WARNING: full logging is enabled. To disable full logging edit %s and restart OSConfig", g_configFile);
    }

    TraceLoggingWrite(g_providerHandle, "AgentStart", TraceLoggingInt32((int32_t)pid, "Pid"), TraceLoggingString(OSCONFIG_VERSION, "Version"));

    // Load remaining configuration
    jsonConfiguration = LoadStringFromFile(g_configFile, false);
    if (NULL != jsonConfiguration)
    {
        GetModelVersionFromJsonConfig(jsonConfiguration);
        LoadReportedFromJsonConfig(jsonConfiguration);
        GetReportingIntervalFromJsonConfig(jsonConfiguration);
        GetLocalPriorityFromJsonConfig(jsonConfiguration);
        GetLocalReportingFromJsonConfig(jsonConfiguration);
        GetProtocolFromJsonConfig(jsonConfiguration);
        FREE_MEMORY(jsonConfiguration);
    }

    snprintf(g_modelId, sizeof(g_modelId), g_modelIdTemplate, g_modelVersion);
    OsConfigLogInfo(GetLog(), "Model id: %s", g_modelId);

    snprintf(g_productInfo, sizeof(g_productInfo), g_productInfoTemplate, g_modelVersion, OSCONFIG_VERSION);
    OsConfigLogInfo(GetLog(), "Product info: %s", g_productInfo);

    OsConfigLogInfo(GetLog(), "Protocol: %s", (PROTOCOL_MQTT_WS == g_protocol) ? "MQTT over Web Socket" : "MQTT");

    if (PROTOCOL_MQTT_WS == g_protocol)
    {
        // Read the proxy options from environment variables, parse and fill the HTTP_PROXY_OPTIONS structure to pass to the SDK:
        if (NULL != (proxyData = GetHttpProxyData()))
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
        connectionString = RequestConnectionStringFromAis(&g_x509Certificate, &g_x509PrivateKeyHandle);
        if (NULL != connectionString)
        {
            freeConnectionString = true;
            g_iotHubConnectionStringFromAis = true;
        }
        else
        {
            OsConfigLogError(GetLog(), "Failed to obtain a connection string from AIS");
            g_exitState = NoConnectionString;
            goto done;
        }
    }
    else
    {
        if (0 == strncmp(argv[1], g_iotHubConnectionStringPrefix, strlen(g_iotHubConnectionStringPrefix)))
        {
            connectionString = argv[1];
        }
        else
        {
            connectionString = LoadStringFromFile(argv[1], true);
            if (NULL != connectionString)
            {
                freeConnectionString = true;
            }
            else
            {
                LogErrorWithTelemetry(GetLog(), "Failed to load a connection string from %s", argv[1]);
                g_exitState = NoConnectionString;
                goto done;
            }
        }
    }

    if (0 != mallocAndStrcpy_s(&g_iotHubConnectionString, connectionString))
    {
        LogErrorWithTelemetry(GetLog(), "Out of memory making copy of the connection string");
        goto done;
    }

    for (int i = 0; i < stopSignalsCount; i++)
    {
        signal(g_stopSignals[i], SignalInterrupt);
    }
    signal(SIGHUP, SignalReloadConfiguration);
    signal(SIGUSR1, SignalProcessDesired);

    if (0 != InitializeAgent(connectionString, (PROTOCOL_MQTT_WS == g_protocol) ? MQTT_WebSocket_Protocol : MQTT_Protocol))
    {
        LogErrorWithTelemetry(GetLog(), "Failed to initialize the OSConfig PnP Agent");
        goto done;
    }

    while (0 == g_stopSignal)
    {
        AgentDoWork();
        ThreadAPI_Sleep(DOWORK_SLEEP);

        if (0 != g_refreshSignal)
        {
            RefreshConnection();
            g_refreshSignal = 0;
        }
    }

done:
    OsConfigLogInfo(GetLog(), "OSConfig PnP Agent (PID: %d) exiting with %d", pid, g_stopSignal);

    TraceLoggingWrite(g_providerHandle, "AgentShutdown",
        TraceLoggingInt32((int32_t)pid, "Pid"),
        TraceLoggingString(OSCONFIG_VERSION, "Version"),
        TraceLoggingInt32((int32_t)g_stopSignal, "ExitCode"),
        TraceLoggingInt32((int32_t)g_exitState, "ExitState"));

    FREE_MEMORY(g_x509Certificate);
    FREE_MEMORY(g_x509PrivateKeyHandle);
    FREE_MEMORY(g_iotHubConnectionString);
    if (freeConnectionString)
    {
        FREE_MEMORY(connectionString);
    }

    CloseAgent();
    CloseTraceLogging();
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

int InitializeAgent(const char* connectionString, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    if (NULL == (g_tickCounter = tickcounter_create()))
    {
        LogErrorWithTelemetry(GetLog(), "tickcounter_create failed");
        return -1;
    }

    // Open the MPI session for this PnP Module instance:
    if (NULL == (g_mpiHandle = CallMpiOpen(g_productInfo, g_maxPayloadSizeBytes)))
    {
        LogErrorWithTelemetry(GetLog(), "MpiOpen failed");
        g_exitState = MpiInitializationFailure;
        return -1;
    }

    // Initialize communication with the IoT Hub:
    if (NULL == (g_moduleHandle = IotHubInitialize(g_modelId, g_productInfo, connectionString, false, g_x509Certificate, g_x509PrivateKeyHandle, &g_proxyOptions, protocol)))
    {
        LogErrorWithTelemetry(GetLog(), "IotHubInitialize failed");
        g_exitState = IotHubInitializationFailure;
        return -1;
    }

    tickcounter_get_current_ms(g_tickCounter, &g_lastTick);

    OsConfigLogInfo(GetLog(), "OSConfig PnP Agent initialized");

    return 0;
}

static void LoadDesiredConfigurationFromFile()
{
    size_t payloadHash = 0;
    int payloadSizeBytes = 0;
    char* payload = LoadStringFromFile(DC_FILE, false);
    if (payload && (0 != (payloadSizeBytes = strlen(payload))))
    {
        // Do not call MpiSetDesired unless we need to overwrite (when LocalPriority is non-zero) or this desired is different from previous
        if (g_localPriority || (g_desiredHash != (payloadHash = HashString(payload))))
        {
            if (MPI_OK == CallMpiSetDesired(g_productInfo, (MPI_JSON_STRING)payload, payloadSizeBytes))
            {
                g_desiredHash = payloadHash;
            }
        }
        RestrictFileAccessToCurrentAccountOnly(DC_FILE);
    }
    FREE_MEMORY(payload);
}

static void SaveReportedConfigurationToFile()
{
    char* payload = NULL;
    int payloadSizeBytes = 0;
    size_t payloadHash = 0;
    int mpiResult = MPI_OK;
    if (g_localReporting)
    {
        mpiResult = CallMpiGetReported(g_productInfo, 0/*no limit for payload size*/, (MPI_JSON_STRING*)&payload, &payloadSizeBytes);
        if ((MPI_OK == mpiResult) && (NULL != payload) && (0 < payloadSizeBytes))
        {
            if (g_reportedHash != (payloadHash = HashString(payload)))
            {
                if (SavePayloadToFile(RC_FILE, payload, payloadSizeBytes))
                {
                    RestrictFileAccessToCurrentAccountOnly(RC_FILE);
                    g_reportedHash = payloadHash;
                }
            }
        }
        CallMpiFree(payload);
    }
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

void AgentDoWork(void)
{
    tickcounter_ms_t nowTick = 0;
    tickcounter_ms_t intervalTick = g_reportingInterval * 1000;
    tickcounter_get_current_ms(g_tickCounter, &nowTick);

    if (intervalTick <= (nowTick - g_lastTick))
    {
        // Process desired updates from local DC file (for Iot Hub this is signaled to be done with SIGUSR1)
        LoadDesiredConfigurationFromFile();

        // Send reported to both Iot Hub and local RC file
        ReportProperties();
        SaveReportedConfigurationToFile();

        // Allow the inproc (for now) platform to unload unused modules
        CallMpiDoWork();

        tickcounter_get_current_ms(g_tickCounter, &g_lastTick);
    }
    else
    {
        IotHubDoWork();
    }
}

void CloseAgent(void)
{
    IotHubDeInitialize();

    if (NULL != g_mpiHandle)
    {
        CallMpiClose(g_mpiHandle);
        g_mpiHandle = NULL;
    }

    FREE_MEMORY(g_reportedProperties);

    OsConfigLogInfo(GetLog(), "OSConfig PnP Agent terminated");
}