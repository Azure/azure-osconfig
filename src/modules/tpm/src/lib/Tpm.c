// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>
#include <parson.h>

#include "Tpm.h"

#define TPM_RESPONSE_SIZE 4096

static const char* g_component = "Tpm";
static const char* g_tpmStatus = "tpmStatus";
static const char* g_tpmVersion = "tpmVersion";
static const char* g_tpmManufacturer = "tpmManufacturer";

static const char* g_tpmLogFile = "/var/log/osconfig_tpm.log";
static const char* g_tpmRolledLogFile = "/var/log/osconfig_tpm.bak";

static const char* g_tpmModuleInfo = "{\"Name\": \"Tpm\","
    "\"Description\": \"Provides functionality to remotely query the TPM on device\","
    "\"Manufacturer\": \"Microsoft\","
    "\"VersionMajor\": 1,"
    "\"VersionMinor\": 0,"
    "\"VersionInfo\": \"Nickel\","
    "\"Components\": [\"Tpm\"],"
    "\"Lifetime\": 1,"
    "\"UserAccount\": 0}";

typedef enum TPM_STATUS
{
    UNKNOWN = 0,
    DETECTED,
    NOT_DETECTED
} TPM_STATUS;

typedef struct TPM_PROPERTIES
{
    char* version;
    char* manufacturer;
} TPM_PROPERTIES;

typedef struct HANDLE
{
    unsigned int maxPayloadSizeBytes;
} HANDLE;

static TPM_STATUS g_status = UNKNOWN;
static TPM_PROPERTIES* g_properties = NULL;

static OSCONFIG_LOG_HANDLE g_log = NULL;

static OSCONFIG_LOG_HANDLE GetTpmLog(void)
{
    return g_log;
}

TPM_STATUS GetTpmStatus(void)
{
    const char* command = "ls -d ./dev/tpm* | grep -E \"tpm(rm)?[0-9]\"";
    return (0 == ExecuteCommand(NULL, command, false, false, 0, 0, NULL, NULL, GetTpmLog())) ? DETECTED : NOT_DETECTED;
}

static TPM_PROPERTIES* GetTpmProperties(void)
{
    TPM_PROPERTIES* properties = NULL;
    const char* tpm = "/dev/tpm0";
    int fd = -1;
    uint8_t buffer[TPM_RESPONSE_SIZE] = {0};
    ssize_t bytes = 0;
    uint64_t key = 0;
    unsigned char value[5] = {0};
    char* property = NULL;
    int n = 0;
    int i = 0;

    const uint8_t request[] = {
        0x80, 0x01,                 // TPM_ST_NO_SESSIONS
        0x00, 0x00, 0x00, 0x16,     // commandSize
        0x00, 0x00, 0x01, 0x7A,     // TPM_CC_GetCapability
        0x00, 0x00, 0x00, 0x06,     // TPM_CAP_TPM_PROPERTIES
        0x00, 0x00, 0x01, 0x00,     // Property: TPM_PT_FAMILY_INDICATOR
        0x00, 0x00, 0x00, 0x66      // propertyCount (102)
    };

    if (-1 == (fd = open(tpm, O_RDWR)))
    {
        OsConfigLogError(GetTpmLog(), "Tpm: failed to open device file '%s'", tpm);
    }
    else if (-1 == (bytes = write(fd, request, sizeof(request))))
    {
        OsConfigLogError(GetTpmLog(), "Tpm: error writing request to the device");
    }
    else if (bytes != sizeof(request))
    {
        OsConfigLogError(GetTpmLog(), "Tpm: error writing request to the device, wrote %ld bytes instead of %ld", bytes, sizeof(request));
    }
    else if (-1 == (bytes = read(fd, buffer, TPM_RESPONSE_SIZE)))
    {
        OsConfigLogError(GetTpmLog(), "Tpm: error reading response from the device");
    }
    else if (NULL == (properties = (TPM_PROPERTIES*)malloc(sizeof(TPM_PROPERTIES))))
    {
        OsConfigLogError(GetTpmLog(), "Tpm: Failed to allocate memory for properties");
    }
    else
    {
        OsConfigLogInfo(GetTpmLog(), "Tpm: extracting properties from device response");

        memset(properties, 0, sizeof(TPM_PROPERTIES));

        for (n = 0x13; n < (TPM_RESPONSE_SIZE - 8); n += 8)
        {
            key = 0;

            for (i = 0; i < 4; i++)
            {
                key = key << 8;
                key += buffer[n + i];
            }

            value[0] = buffer[n + 4];
            value[1] = buffer[n + 5];
            value[2] = buffer[n + 6];
            value[3] = buffer[n + 7];
            value[4] = '\0';

            property = (char*)value;

            switch (key)
            {
                case 0x100:
                    // TODO: trim the string
                    if (NULL == (properties->version = strdup(property)))
                    {
                        OsConfigLogError(GetTpmLog(), "Tpm: Failed to allocate memory for version string");
                    }
                    break;

                case 0x100 + 5:
                    // TODO: trim the string
                    if (NULL == (properties->manufacturer = strdup(property)))
                    {
                        OsConfigLogError(GetTpmLog(), "Tpm: Failed to allocate memory for manufacturer string");
                    }
            }
        }
    }

    if (fd != -1)
    {
        close(fd);
    }

    return properties;
}

void TpmInitialize(void)
{
    g_log = OpenLog(g_tpmLogFile, g_tpmRolledLogFile);

    g_status = GetTpmStatus();
    g_properties = GetTpmProperties();

    OsConfigLogInfo(GetTpmLog(), "%s initialized", g_component);
}

void TpmShutdown(void)
{
    OsConfigLogInfo(GetTpmLog(), "%s shutting down", g_component);

    if (g_properties)
    {
        FREE_MEMORY(g_properties->version);
        FREE_MEMORY(g_properties->manufacturer);
        FREE_MEMORY(g_properties);
    }

    CloseLog(&g_log);
}

MMI_HANDLE TpmMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    HANDLE* handle = NULL;

    if (NULL == clientName)
    {
        OsConfigLogError(GetTpmLog(), "MmiOpen() called with NULL clientName");
        return NULL;
    }

    if (NULL == (handle = (HANDLE*)malloc(sizeof(HANDLE))))
    {
        OsConfigLogError(GetTpmLog(), "MmiOpen() failed to allocate memory for handle");
    }
    else
    {
        handle->maxPayloadSizeBytes = maxPayloadSizeBytes;
        OsConfigLogInfo(GetTpmLog(), "MmiOpen(%s, %u) = %p", clientName, maxPayloadSizeBytes, handle);
    }

    return (MMI_HANDLE)handle;
}

void TpmMmiClose(MMI_HANDLE clientSession)
{
    HANDLE* handle = (HANDLE*)clientSession;

    if (NULL == handle)
    {
        OsConfigLogError(GetTpmLog(), "MmiClose() called with NULL handle");
        return;
    }

    OsConfigLogInfo(GetTpmLog(), "MmiClose(%p)", handle);

    FREE_MEMORY(handle);
}

int TpmMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if ((NULL == payload) || (NULL == payloadSizeBytes))
    {
        return EINVAL;
    }

    *payload = NULL;
    *payloadSizeBytes = (int)strlen(g_tpmModuleInfo);

    if (NULL != (*payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes)))
    {
        memcpy(*payload, g_tpmModuleInfo, *payloadSizeBytes);
    }
    else
    {
        OsConfigLogError(GetTpmLog(), "MmiGetInfo: failed to allocate %d bytes", *payloadSizeBytes);
        *payloadSizeBytes = 0;
        status = ENOMEM;
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetTpmLog(), "MmiGetInfo(%s, %.*s, %d) returning %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

int TpmMmiGet(MMI_HANDLE clientSession, const char* component, const char* object, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;
    HANDLE* handle = (HANDLE*)clientSession;
    JSON_Value* value = NULL;
    char* json = NULL;

    if ((NULL == handle) || (NULL == component) || (NULL == object) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(GetTpmLog(), "MmiGet(%p, %s, %s, %p, %p) called with invalid arguments", handle, component, object, payload, payloadSizeBytes);
        return EINVAL;
    }

    *payload = NULL;
    *payloadSizeBytes = 0;

    if (0 == strcmp(component, g_component))
    {
        if (0 == strcmp(object, g_tpmStatus))
        {
            value = json_value_init_number(g_status);
        }
        else if (0 == strcmp(object, g_tpmVersion))
        {
            if (g_properties)
            {
                value = json_value_init_string(g_properties->version);
            }
            else
            {
                OsConfigLogError(GetTpmLog(), "Tpm: Failed to get version");
                status = EINVAL;
            }
        }
        else if (0 == strcmp(object, g_tpmManufacturer))
        {
            if (g_properties)
            {
                value = json_value_init_string(g_properties->manufacturer);
            }
            else
            {
                OsConfigLogError(GetTpmLog(), "Tpm: Failed to get manufacturer");
                status = EINVAL;
            }
        }
        else
        {
            OsConfigLogError(GetTpmLog(), "MmiGet called for an invalid object name '%s'", object);
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(GetTpmLog(), "MmiGet called for an invalid component name '%s'", component);
        status = EINVAL;
    }

    if ((MMI_OK == status) && (NULL != value))
    {
        json = json_serialize_to_string(value);

        if (NULL == json)
        {
            OsConfigLogError(GetTpmLog(), "Failed to serialize JSON object");
            status = ENOMEM;
        }
        else if ((0 < handle->maxPayloadSizeBytes) && (handle->maxPayloadSizeBytes < strlen(json)))
        {
            OsConfigLogError(GetTpmLog(), "Payload size exceeds maximum size");
            status = E2BIG;
        }
        else
        {
            *payloadSizeBytes = strlen(json);
            *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);

            if (NULL == *payload)
            {
                OsConfigLogError(GetTpmLog(), "Failed to allocate memory for payload");
                status = ENOMEM;
            }
            else
            {
                memcpy(*payload, json, *payloadSizeBytes);
            }
        }

        json_value_free(value);

        FREE_MEMORY(json);
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(GetTpmLog(), "MmiGet(%p, %s, %s, %.*s, %d) returning %d", clientSession, component, object, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

int TpmMmiSet(MMI_HANDLE clientSession, const char* component, const char* object, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    OsConfigLogError(GetTpmLog(), "No desired objects, MmiSet not implemented");

    UNUSED(clientSession);
    UNUSED(component);
    UNUSED(object);
    UNUSED(payload);
    UNUSED(payloadSizeBytes);

    return ENOSYS;
}

void TpmMmiFree(MMI_JSON_STRING payload)
{
    FREE_MEMORY(payload);
}
