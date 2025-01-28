// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "inc/AgentCommon.h"
#include "inc/PnpUtils.h"
#include "inc/PnpAgent.h"

#define PNP_STATUS_SUCCESS 200
#define PNP_STATUS_BAD_DATA 400

#define EXTRA_PROP_PAYLOAD_ESTIMATE 256

static const char g_componentMarker[] = "__t";
static const char g_desiredObjectName[] = "desired";
static const char g_desiredVersion[] = "$version";

// The openssl engine from the AIS aziot-identity-service package:
static const char g_azIotKeys[] = "aziot_keys";
static const OPTION_OPENSSL_KEY_TYPE g_keyTypeEngine = KEY_TYPE_ENGINE;

// The following values need to be filled via these templates, in order:
// 1. component name
// 2. property name
// 3. property value (simple or complex/object)
// 4. ackowledged code (HTTP result)
// 5. ackowledged description (optional and here fixed to '-')
// 6. ackowledged version
static const char g_propertyReadTemplate[] = "{\"""%s\":{\"__t\":\"c\",\"%s\":%.*s}}";
static const char g_propertyAckTemplate[] = "{\"""%s\":{\"__t\":\"c\",\"%s\":{\"value\":%.*s,\"ac\":%d,\"ad\":\"-\",\"av\":%d}}}";

IOTHUB_DEVICE_CLIENT_LL_HANDLE g_moduleHandle = NULL;

static bool g_lostNetworkConnection = false;

typedef IOTHUB_CLIENT_RESULT(*PROPERTY_UPDATE_CALLBACK)(const char* componentName, const char* propertyName, JSON_Value* propertyValue, int version);

static const char g_connectionAuthenticated[] = "IOTHUB_CLIENT_CONNECTION_AUTHENTICATED";
static const char g_connectionUnauthenticated[] = "IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED";

#define MAX_DESIRED_TWIN_QUEUE 10
typedef struct DESIRED_TWIN_UPDATE
{
    DEVICE_TWIN_UPDATE_STATE updateState;
    unsigned char* payload;
    size_t size;
    int processed;
} DESIRED_TWIN_UPDATE;

static DESIRED_TWIN_UPDATE g_desiredTwinUpdates[MAX_DESIRED_TWIN_QUEUE] = {0};
static int g_desiredTwinUpdatesIndex = 0;

static void IotHubConnectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback)
{
    bool authenticated = false;
    const char* connectionAuthentication = NULL;

    switch (result)
    {
        case IOTHUB_CLIENT_CONNECTION_AUTHENTICATED:
            connectionAuthentication = g_connectionAuthenticated;
            authenticated = true;
            break;

        case IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED:
            connectionAuthentication = g_connectionUnauthenticated;
            break;

        default:
            OsConfigLogInfo(GetLog(), "IotHubConnectionStatusCallback: unknown %d result received", (int)result);
    }

    switch (reason)
    {
        case IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN:
            OsConfigLogInfo(GetLog(), "IotHubConnectionStatusCallback: %s, reason: IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN", connectionAuthentication ? connectionAuthentication : "-");
            ScheduleRefreshConnection();
            break;

        case IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED:
            OsConfigLogInfo(GetLog(), "IotHubConnectionStatusCallback: %s, reason: IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED", connectionAuthentication ? connectionAuthentication : "-");
            ScheduleRefreshConnection();
            break;

        case IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR:
            OsConfigLogInfo(GetLog(), "IotHubConnectionStatusCallback: %s, reason: IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR", connectionAuthentication ? connectionAuthentication : "-");
            ScheduleRefreshConnection();
            break;

        case IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE:
            OsConfigLogInfo(GetLog(), "IotHubConnectionStatusCallback: %s, reason: IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE", connectionAuthentication ? connectionAuthentication : "-");
            if (!authenticated)
            {
                g_lostNetworkConnection = true;
                OsConfigLogError(GetLog(), "Lost network connection");
            }
            break;

        case IOTHUB_CLIENT_CONNECTION_NO_NETWORK:
            OsConfigLogInfo(GetLog(), "IotHubConnectionStatusCallback: %s, reason: IOTHUB_CLIENT_CONNECTION_NO_NETWORK", connectionAuthentication ? connectionAuthentication : "-");
            if (!authenticated)
            {
                g_lostNetworkConnection = true;
                OsConfigLogError(GetLog(), "Lost network connection");
            }
            break;

        case IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED:
            OsConfigLogInfo(GetLog(), "IotHubConnectionStatusCallback: %s, reason: IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED", connectionAuthentication ? connectionAuthentication : "-");
            break;

        case IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL:
            OsConfigLogInfo(GetLog(), "IotHubConnectionStatusCallback: %s, reason: IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL", connectionAuthentication ? connectionAuthentication : "-");
            break;

        case IOTHUB_CLIENT_CONNECTION_OK:
            OsConfigLogInfo(GetLog(), "IotHubConnectionStatusCallback: %s, reason: IOTHUB_CLIENT_CONNECTION_OK", connectionAuthentication ? connectionAuthentication : "-");
            if (g_lostNetworkConnection && authenticated)
            {
                g_lostNetworkConnection = false;
                OsConfigLogInfo(GetLog(), "Got network connection");
                ScheduleRefreshConnection();
            }
            break;

        default:
            OsConfigLogInfo(GetLog(), "IotHubConnectionStatusCallback: %s, unknown reason %d received", connectionAuthentication ? connectionAuthentication : "-", (int)reason);
    }

    UNUSED(userContextCallback);
}

static IOTHUB_CLIENT_RESULT PropertyUpdateFromIotHubCallback(const char* componentName, const char* propertyName, JSON_Value* propertyValue, int version)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_ERROR;

    if (NULL == componentName)
    {
        OsConfigLogError(GetLog(), "PropertyUpdateFromIotHubCallback: property %s arrived with a NULL component name, indicating root", propertyName);
        return result;
    }

    OsConfigLogInfo(GetLog(), "PropertyUpdateFromIotHubCallback: invoking %s for property %s, version %d", componentName, propertyName, version);
    result = UpdatePropertyFromIotHub(componentName, propertyName, propertyValue, version);

    return result;
}

static char* CopyPayloadToString(const unsigned char* payload, size_t size)
{
    char* jsonStr = NULL;
    size_t sizeToAllocate = 0;

    if ((NULL != payload) && (size > 0) && (size < SIZE_MAX))
    {
        sizeToAllocate = size + 1;
        if (NULL != (jsonStr = (char*)malloc(sizeToAllocate)))
        {
            memcpy(jsonStr, payload, size);
            jsonStr[size] = '\0';
        }
        else
        {
            OsConfigLogError(GetLog(), "CopyPayloadToString: out of memory allocating %d bytes", (int)sizeToAllocate);
        }
    }
    else
    {
        OsConfigLogError(GetLog(), "CopyPayloadToString: invalid payload or payload size");
    }

    return jsonStr;
}

static IOTHUB_CLIENT_RESULT ProcessJsonFromTwin(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char* payload, size_t size, PROPERTY_UPDATE_CALLBACK propertyCallback)
{
    JSON_Value* rootValue = NULL;
    JSON_Value* versionValue = NULL;
    JSON_Value* propertyValue = NULL;
    JSON_Value* childValue = NULL;
    JSON_Object* rootObject = NULL;
    JSON_Object* desiredObject = NULL;
    JSON_Object* childObject = NULL;
    size_t numChildren = 0;
    size_t numChildChildren = 0;
    int version = 0;
    char* jsonString = NULL;
    const char* componentName = NULL;
    const char* propertyName = NULL;
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_OK;

    LogAssert(GetLog(), NULL != payload);

    jsonString = CopyPayloadToString(payload, size);
    if (NULL == jsonString)
    {
        OsConfigLogError(GetLog(), "ProcessJsonFromTwin: CopyPayloadToString failed");
        result = IOTHUB_CLIENT_ERROR;
    }

    if (IOTHUB_CLIENT_OK == result)
    {
        rootValue = json_parse_string(jsonString);
        if (NULL == rootValue)
        {
            OsConfigLogError(GetLog(), "ProcessJsonFromTwin: json_parse_string(root) failed");
            result = IOTHUB_CLIENT_ERROR;
        }
    }

    if (IOTHUB_CLIENT_OK == result)
    {
        rootObject = json_value_get_object(rootValue);
        if (NULL == rootObject)
        {
            OsConfigLogError(GetLog(), "ProcessJsonFromTwin: json_value_get_object(root) failed, cannot get desired object");
            result = IOTHUB_CLIENT_ERROR;
        }
    }

    if (IOTHUB_CLIENT_OK == result)
    {
        if (DEVICE_TWIN_UPDATE_COMPLETE == updateState)
        {
            OsConfigLogInfo(GetLog(), "ProcessJsonFromTwin: DEVICE_TWIN_UPDATE_COMPLETE");

            // For a complete update the JSON from IoT Hub contains both "desired" and "reported" (the full twin):
            desiredObject = json_object_get_object(rootObject, g_desiredObjectName);
        }
        else
        {
            OsConfigLogInfo(GetLog(), "ProcessJsonFromTwin: DEVICE_TWIN_UPDATE_PARTIAL");

            // For a partial update the JSON from IoT Hub skips the "desired" envelope, we need to read from root:
            desiredObject = rootObject;
        }

        if (NULL == desiredObject)
        {
            OsConfigLogError(GetLog(), "ProcessJsonFromTwin: no desired object");
            result = IOTHUB_CLIENT_ERROR;
        }
    }

    if (IOTHUB_CLIENT_OK == result)
    {
        versionValue = json_object_get_value(desiredObject, g_desiredVersion);
        if (NULL != versionValue)
        {
            if (JSONNumber == json_value_get_type(versionValue))
            {
                version = (int)json_value_get_number(versionValue);
            }
            else
            {
                OsConfigLogError(GetLog(), "ProcessJsonFromTwin: field %s type is not JSONNumber, cannot read the desired version", g_desiredVersion);
            }
        }
        else
        {
            OsConfigLogError(GetLog(), "ProcessJsonFromTwin: json_object_get_value(%s) failed, cannot read the desired version", g_desiredVersion);
        }

        numChildren = json_object_get_count(desiredObject);

        for (size_t i = 0; i < numChildren; i++)
        {
            componentName = json_object_get_name(desiredObject, i);
            childValue = json_object_get_value_at(desiredObject, i);

            if (0 == strcmp(componentName, g_desiredVersion))
            {
                // Ignore, nothing to do here
                continue;
            }

            if (JSONObject == json_type(childValue))
            {
                childObject = json_value_get_object(childValue);
                numChildChildren = json_object_get_count(childObject);

                for (size_t i = 0; i < numChildChildren; i++)
                {
                    propertyName = json_object_get_name(childObject, i);
                    propertyValue = json_object_get_value_at(childObject, i);

                    if ((NULL == propertyName) || (NULL == propertyValue))
                    {
                        OsConfigLogError(GetLog(), "ProcessJsonFromTwin: error retrieving property name and/or value from %s (child[%d])", componentName, (int)i);
                        continue;
                    }

                    if (0 == strcmp(propertyName, g_componentMarker))
                    {
                        // Ignore the marker
                        continue;
                    }

                    result = propertyCallback(componentName, propertyName, propertyValue, version);
                }
            }
        }
    }

    if (NULL != rootValue)
    {
        json_value_free(rootValue);
    }

    FREE_MEMORY(jsonString);

    UNUSED(propertyCallback);

    OsConfigLogInfo(GetLog(), "ProcessJsonFromTwin completed with %d", result);

    return result;
}

static void QueueDesiredTwinUpdate(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char* payload, size_t size)
{
    // This code is currently running all on a single thread. If it will become multi-threaded,
    // add a mutex and lock within all functions that access this common desired twin queue data.

    int queueSize = (int)ARRAY_SIZE(g_desiredTwinUpdates);

    if ((NULL == payload) || (0 >= size) || (SIZE_MAX <= size))
    {
        OsConfigLogError(GetLog(), "QueueDesiredTwinUpdate failed, no payload to queue or invalid payload size (%p, %d)", payload,  (int)size);
        return;
    }

    // Clear existing slot
    FREE_MEMORY(g_desiredTwinUpdates[g_desiredTwinUpdatesIndex].payload);
    memset(&(g_desiredTwinUpdates[g_desiredTwinUpdatesIndex]), 0, sizeof(g_desiredTwinUpdates[g_desiredTwinUpdatesIndex]));

    // Allocate memory for new desired twin payload to queue
    g_desiredTwinUpdates[g_desiredTwinUpdatesIndex].payload = malloc(size);
    if (NULL != g_desiredTwinUpdates[g_desiredTwinUpdatesIndex].payload)
    {
        // Fill in the slot (the processed field is already cleared to 0)
        memcpy(g_desiredTwinUpdates[g_desiredTwinUpdatesIndex].payload, payload, size);
        g_desiredTwinUpdates[g_desiredTwinUpdatesIndex].updateState = updateState;
        g_desiredTwinUpdates[g_desiredTwinUpdatesIndex].size = size;
        OsConfigLogInfo(GetLog(), "Queued desired payload of %d bytes at slot %d", (int)size, g_desiredTwinUpdatesIndex + 1);
    }
    else
    {
        OsConfigLogError(GetLog(), "QueueDesiredTwinUpdate failed to allocate buffer for new payload (%d bytes)", (int)size);
    }

    // Circular buffer, when full, continue overwritting from beginning
    g_desiredTwinUpdatesIndex += 1;
    if (g_desiredTwinUpdatesIndex >= queueSize)
    {
        g_desiredTwinUpdatesIndex = 0;
    }
}

static void ClearDesiredTwinUpdates()
{
    int queueSize = (int)ARRAY_SIZE(g_desiredTwinUpdates);
    int i = 0;

    for (i = 0; i < queueSize; i++)
    {
        FREE_MEMORY(g_desiredTwinUpdates[i].payload);
    }

    memset(g_desiredTwinUpdates, 0, sizeof(g_desiredTwinUpdates));
}

void ProcessDesiredTwinUpdates()
{
    int queueSize = (int)ARRAY_SIZE(g_desiredTwinUpdates);
    int i = 0;

    for (i = 0; i < queueSize; i++)
    {
        if ((g_desiredTwinUpdates[i].size > 0) && (NULL != g_desiredTwinUpdates[i].payload) && (0 == g_desiredTwinUpdates[i].processed))
        {
            IOTHUB_CLIENT_RESULT result = ProcessJsonFromTwin(g_desiredTwinUpdates[i].updateState, g_desiredTwinUpdates[i].payload, g_desiredTwinUpdates[i].size, PropertyUpdateFromIotHubCallback);
            g_desiredTwinUpdates[i].processed = 1;
            OsConfigLogInfo(GetLog(), "ProcessDesiredTwinUpdates: processing desired twin update at slot %d completed with result %d", i + 1, (int)result);
        }
    }
}

static void ModuleTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char* payload, size_t size, void* userContextCallback)
{
    LogAssert(GetLog(), NULL != payload);
    LogAssert(GetLog(), 0 < size);

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetLog(), "ModuleTwinCallback: received %.*s (%d bytes)", (int)size, payload, (int)size);
    }
    else
    {
        OsConfigLogInfo(GetLog(), "ModuleTwinCallback: received %d bytes", (int)size);
    }

    QueueDesiredTwinUpdate(updateState, payload, size);

    UNUSED(userContextCallback);

    raise(SIGUSR1);

    OsConfigLogInfo(GetLog(), "ModuleTwinCallback: done");
}

static bool IotHubSetOption(const char* optionName, const void* value)
{
    if ((NULL == g_moduleHandle) || (NULL == optionName) || (NULL == value))
    {
        OsConfigLogError(GetLog(), "Invalid argument, IotHubSetOption failed");
        return false;
    }

    IOTHUB_CLIENT_RESULT iothubClientResult = IOTHUB_CLIENT_ERROR;
    if (IOTHUB_CLIENT_OK != (iothubClientResult = IoTHubDeviceClient_LL_SetOption(g_moduleHandle, optionName, value)))
    {
        OsConfigLogError(GetLog(), "Failed to set option %s, error %d", optionName, iothubClientResult);
        IoTHubDeviceClient_LL_Destroy(g_moduleHandle);
        g_moduleHandle = NULL;

        IoTHub_Deinit();
        return false;
    }
    else
    {
        return true;
    }
}

IOTHUB_DEVICE_CLIENT_LL_HANDLE IotHubInitialize(const char* modelId, const char* productInfo, const char* connectionString, bool traceOn,
    const char* x509Certificate, const char* x509PrivateKeyHandle, const HTTP_PROXY_OPTIONS* proxyOptions, IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
    IOTHUB_CLIENT_RESULT iothubResult = IOTHUB_CLIENT_OK;

    bool urlEncodeOn = true;

    g_desiredTwinUpdatesIndex = 0;

    if (NULL != g_moduleHandle)
    {
        OsConfigLogError(GetLog(), "IotHubInitialize called at the wrong time");
        return NULL;
    }

    if ((NULL == modelId) || (NULL == productInfo))
    {
        OsConfigLogError(GetLog(), "IotHubInitialize called without model id and/or product info");
        return NULL;
    }

    if (0 != IoTHub_Init())
    {
        OsConfigLogError(GetLog(), "IoTHub_Init failed");
    }
    else
    {
        if (NULL == (g_moduleHandle = IoTHubDeviceClient_LL_CreateFromConnectionString(connectionString, protocol)))
        {
            OsConfigLogError(GetLog(), "IoTHubDeviceClient_LL_CreateFromConnectionString failed");
        }
        else
        {
            IotHubSetOption(OPTION_LOG_TRACE, &traceOn);
            IotHubSetOption(OPTION_MODEL_ID, modelId);
            IotHubSetOption(OPTION_PRODUCT_INFO, productInfo);
            IotHubSetOption(OPTION_AUTO_URL_ENCODE_DECODE, &urlEncodeOn);

            if ((NULL != x509Certificate) && (NULL != x509PrivateKeyHandle))
            {
                IotHubSetOption(OPTION_OPENSSL_ENGINE, g_azIotKeys);
                IotHubSetOption(OPTION_OPENSSL_PRIVATE_KEY_TYPE, &g_keyTypeEngine);
                IotHubSetOption(OPTION_X509_CERT, x509Certificate);
                IotHubSetOption(OPTION_X509_PRIVATE_KEY, x509PrivateKeyHandle);
            }

            if ((NULL != proxyOptions) && (NULL != proxyOptions->host_address))
            {
                IotHubSetOption(OPTION_HTTP_PROXY, proxyOptions);
            }

            if (IOTHUB_CLIENT_OK != (iothubResult = IoTHubDeviceClient_LL_SetDeviceTwinCallback(g_moduleHandle, ModuleTwinCallback, (void*)g_moduleHandle)))
            {
                OsConfigLogError(GetLog(), "IoTHubDeviceClient_SetDeviceTwinCallback failed with %d", iothubResult);
            }
            else if (IOTHUB_CLIENT_OK != (iothubResult = IoTHubDeviceClient_LL_SetConnectionStatusCallback(g_moduleHandle, IotHubConnectionStatusCallback, (void*)g_moduleHandle)))
            {
                OsConfigLogError(GetLog(), "IoTHubDeviceClient_LL_SetConnectionStatusCallback failed with %d", iothubResult);
            }
        }

        if (NULL == g_moduleHandle)
        {
            OsConfigLogError(GetLog(), "IotHubInitialize failed");
            IoTHub_Deinit();
        }
    }

    return g_moduleHandle;
}

void IotHubDeInitialize(void)
{
    if (NULL != g_moduleHandle)
    {
        IoTHubDeviceClient_LL_Destroy(g_moduleHandle);
        IoTHub_Deinit();
        g_moduleHandle = NULL;
    }

    ClearDesiredTwinUpdates();
}

void IotHubDoWork(void)
{
    IoTHubDeviceClient_LL_DoWork(g_moduleHandle);
}

static void ReadReportedStateCallback(int statusCode, void* userContextCallback)
{
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetLog(), "Report for %s complete with status %u", userContextCallback ? (char*)userContextCallback : "all properties", statusCode);
    }
}

IOTHUB_CLIENT_RESULT ReportPropertyToIotHub(const char* componentName, const char* propertyName, size_t* lastPayloadHash)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_OK;
    char* valuePayload = NULL;
    int valueLength = 0;
    char* decoratedPayload = NULL;
    int decoratedLength = 0;
    size_t hashPayload = 0;
    bool reportProperty = true;
    bool platformAlreadyRunning = true;
    int mpiResult = MPI_OK;

    LogAssert(GetLog(), NULL != componentName);
    LogAssert(GetLog(), NULL != propertyName);

    if (NULL == g_moduleHandle)
    {
        OsConfigLogError(GetLog(), "%s: the component needs to be initialized before reporting properties", componentName);
        return IOTHUB_CLIENT_ERROR;
    }

    mpiResult = CallMpiGet(componentName, propertyName, &valuePayload, &valueLength, GetLog());
    if ((MPI_OK != mpiResult) && RefreshMpiClientSession(&platformAlreadyRunning) && (false == platformAlreadyRunning))
    {
        CallMpiFree(valuePayload);

        mpiResult = CallMpiGet(componentName, propertyName, &valuePayload, &valueLength, GetLog());
    }

    if ((MPI_OK == mpiResult) && (valueLength > 0) && (NULL != valuePayload))
    {
        decoratedLength = strlen(componentName) + strlen(propertyName) + valueLength + EXTRA_PROP_PAYLOAD_ESTIMATE;
        decoratedPayload = (char*)malloc(decoratedLength);
        if (NULL != decoratedPayload)
        {
            snprintf(decoratedPayload, decoratedLength, g_propertyReadTemplate, componentName, propertyName, valueLength, valuePayload);

            LogAssert(GetLog(), decoratedLength >= (int)strlen(decoratedPayload));
            decoratedLength = strlen(decoratedPayload);

            if (NULL != lastPayloadHash)
            {
                hashPayload = HashString(decoratedPayload);
                if (hashPayload == *lastPayloadHash)
                {
                    reportProperty = false;
                }
                else
                {
                    *lastPayloadHash = hashPayload;
                }
            }

            if (reportProperty)
            {
                result = IoTHubDeviceClient_LL_SendReportedState(g_moduleHandle, (const unsigned char*)decoratedPayload, decoratedLength, ReadReportedStateCallback, (void*)propertyName);

                if (IsFullLoggingEnabled())
                {
                    OsConfigLogInfo(GetLog(), "%s.%s: reported %.*s (%d bytes), result: %d", componentName, propertyName, decoratedLength, decoratedPayload, decoratedLength, result);
                }

                if (IOTHUB_CLIENT_OK != result)
                {
                    OsConfigLogError(GetLog(), "%s.%s: IoTHubDeviceClient_LL_SendReportedState failed with %d", componentName, propertyName, result);
                }
            }
        }
        else
        {
            OsConfigLogError(GetLog(), "%s: out of memory allocating %u bytes to report property %s", componentName, decoratedLength, propertyName);
        }
    }
    else
    {
        // Avoid log abuse when a component specified in configuration is not active
        if (IsFullLoggingEnabled())
        {
            if (MPI_OK == mpiResult)
            {
                OsConfigLogError(GetLog(), "%s.%s: MpiGet returned MMI_OK with no payload", componentName, propertyName);
            }
            else
            {
                OsConfigLogError(GetLog(), "%s.%s: MpiGet failed with %d", componentName, propertyName, mpiResult);
            }
        }
        result = IOTHUB_CLIENT_ERROR;
    }

    CallMpiFree(valuePayload);

    FREE_MEMORY(decoratedPayload);

    return result;
}

IOTHUB_CLIENT_RESULT UpdatePropertyFromIotHub(const char* componentName, const char* propertyName, const JSON_Value* propertyValue, int version)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_OK;
    int propertyUpdateResult = PNP_STATUS_SUCCESS;
    char* serializedValue = NULL;
    int valueLength = 0;
    bool platformAlreadyRunning = true;
    int mpiResult = MPI_OK;

    LogAssert(GetLog(), NULL != componentName);
    LogAssert(GetLog(), NULL != propertyName);
    LogAssert(GetLog(), NULL != propertyValue);

    serializedValue = json_serialize_to_string(propertyValue);
    if (NULL == serializedValue)
    {
        OsConfigLogInfo(GetLog(), "%s: %s property update requested with no data (nothing to do)", componentName, propertyName);
    }
    else
    {
        valueLength = strlen(serializedValue);

        if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(GetLog(), "%s.%s: received %.*s (%d bytes)", componentName, propertyName, valueLength, serializedValue, valueLength);
        }

        mpiResult = CallMpiSet(componentName, propertyName, serializedValue, valueLength, GetLog());
        if ((MPI_OK != mpiResult) && RefreshMpiClientSession(&platformAlreadyRunning) && (false == platformAlreadyRunning))
        {
            mpiResult = CallMpiSet(componentName, propertyName, serializedValue, valueLength, GetLog());
        }

        if (MPI_OK == mpiResult)
        {
            OsConfigLogInfo(GetLog(), "%s: property %s successfully updated via MPI", componentName, propertyName);
            result = IOTHUB_CLIENT_OK;
            propertyUpdateResult = PNP_STATUS_SUCCESS;
        }
        else
        {
            OsConfigLogError(GetLog(), "%s.%s: MpiSet failed with %d", componentName, propertyName, mpiResult);
            result = IOTHUB_CLIENT_INVALID_ARG;
            propertyUpdateResult = PNP_STATUS_BAD_DATA;
        }

        result = AckPropertyUpdateToIotHub(componentName, propertyName, serializedValue, valueLength, version, propertyUpdateResult);

        json_free_serialized_string(serializedValue);
    }

    return result;
}

static void AckReportedStateCallback(int statusCode, void* userContextCallback)
{
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetLog(), "Property update acknowledgement complete with status %u", statusCode);
    }
    UNUSED(userContextCallback);
}

IOTHUB_CLIENT_RESULT AckPropertyUpdateToIotHub(const char* componentName, const char* propertyName, char* propertyValue, int valueLength, int version, int propertyUpdateResult)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_OK;
    int ackCode = propertyUpdateResult;
    char* ackBuffer = NULL;
    int ackValueLength = 0;

    LogAssert(GetLog(), NULL != componentName);
    LogAssert(GetLog(), NULL != propertyName);
    LogAssert(GetLog(), NULL != propertyValue);
    LogAssert(GetLog(), 0 != valueLength);

    OsConfigLogInfo(GetLog(), "%s: acknowledging received new desired payload for property %s, version %d, ack. code %d",  componentName, propertyName, version, ackCode);

    ackValueLength = strlen(componentName) + strlen(propertyName) + valueLength + EXTRA_PROP_PAYLOAD_ESTIMATE;
    ackBuffer = (char*)malloc(ackValueLength);
    if (NULL != ackBuffer)
    {
        snprintf(ackBuffer, ackValueLength, g_propertyAckTemplate, componentName, propertyName, valueLength, propertyValue, ackCode, version);

        LogAssert(GetLog(), ackValueLength >= (int)strlen(ackBuffer));
        ackValueLength = strlen(ackBuffer);

        result = IoTHubDeviceClient_LL_SendReportedState(g_moduleHandle, (const unsigned char*)ackBuffer, ackValueLength, AckReportedStateCallback, NULL);

        if (IsFullLoggingEnabled())
        {
            OsConfigLogInfo(GetLog(), "%s.%s: acknowledged %.*s (%d bytes), result: %d", componentName, propertyName, ackValueLength, ackBuffer, ackValueLength, result);
        }

        if (IOTHUB_CLIENT_OK != result)
        {
            OsConfigLogError(GetLog(), "%s.%s: IoTHubDeviceClient_LL_SendReportedState failed with %d", componentName, propertyName, result);
        }
    }
    else
    {
        OsConfigLogError(GetLog(), "%s: out of memory allocating %u bytes to acknowledge property %s", componentName, ackValueLength, propertyName);
    }

    FREE_MEMORY(ackBuffer);

    return result;
}
