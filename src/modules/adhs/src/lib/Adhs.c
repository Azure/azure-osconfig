// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <errno.h>
#include <stdatomic.h>
#include <version.h>
#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>
#include <parson.h>
#include <regex.h>

#include "Adhs.h"

static const char* g_adhsModuleInfo = "{\"Name\": \"Adhs\","
    "\"Description\": \"Provides functionality to observe and configure ADHS\","
    "\"Manufacturer\": \"Microsoft\","
    "\"VersionMajor\": 1,"
    "\"VersionMinor\": 0,"
    "\"VersionInfo\": \"Copper\","
    "\"Components\": [\"Adhs\"],"
    "\"Lifetime\": 2,"
    "\"UserAccount\": 0}";

static const char* g_adhsModuleName = "Adhs module";
static const char* g_adhsComponentName = "Adhs";

static const char* g_reportedOptInObjectName = "optIn";
static const char* g_desiredOptInObjectName = "desiredOptIn";

static const char* g_permissionConfigName = "Permission";
// static const char* g_permissionNoneConfigValue = "None";
// static const char* g_permissionRequiredConfigValue = "Required";
// static const char* g_permissionOptionalConfigValue = "Optional";

static const char* g_permissionPattern = "\\bPermission\\s*=\\s*([\\\"'])([A-Za-z0-9]*)\\1";

static atomic_int g_referenceCount = 0;
static unsigned int g_maxPayloadSizeBytes = 0;

static const char* g_adhsConfigFile = NULL;
static const char* g_adhsLogFile = "/var/log/osconfig_adhs.log";
static const char* g_adhsRolledLogFile = "/var/log/osconfig_adhs.bak";

static OSCONFIG_LOG_HANDLE g_log = NULL;

static OSCONFIG_LOG_HANDLE AdhsGetLog()
{
    return g_log;
}

void AdhsInitialize(const char* configFile)
{
    g_adhsConfigFile = configFile;
    g_log = OpenLog(g_adhsLogFile, g_adhsRolledLogFile);
        
    OsConfigLogInfo(AdhsGetLog(), "%s initialized", g_adhsModuleName);
}

void AdhsShutdown(void)
{
    OsConfigLogInfo(AdhsGetLog(), "%s shutting down", g_adhsModuleName);

    g_adhsConfigFile = NULL;
    CloseLog(&g_log);
}

MMI_HANDLE AdhsMmiOpen(const char* clientName, const unsigned int maxPayloadSizeBytes)
{
    MMI_HANDLE handle = (MMI_HANDLE)g_adhsModuleName;
    g_maxPayloadSizeBytes = maxPayloadSizeBytes;
    ++g_referenceCount;
    OsConfigLogInfo(AdhsGetLog(), "MmiOpen(%s, %d) returning %p", clientName, maxPayloadSizeBytes, handle);
    return handle;
}

static bool IsValidSession(MMI_HANDLE clientSession)
{
    return ((NULL == clientSession) || (0 != strcmp(g_adhsModuleName, (char*)clientSession)) || (g_referenceCount <= 0)) ? false : true;
}

void AdhsMmiClose(MMI_HANDLE clientSession)
{
    if (IsValidSession(clientSession))
    {
        --g_referenceCount;
        OsConfigLogInfo(AdhsGetLog(), "MmiClose(%p)", clientSession);
    }
    else 
    {
        OsConfigLogError(AdhsGetLog(), "MmiClose() called outside of a valid session");
    }
}

int AdhsMmiGetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = EINVAL;

    if ((NULL == payload) || (NULL == payloadSizeBytes))
    {
        return status;
    }
    
    *payloadSizeBytes = (int)strlen(g_adhsModuleInfo);
    *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes);
    if (*payload)
    {
        memcpy(*payload, g_adhsModuleInfo, *payloadSizeBytes);
        status = MMI_OK;
    }
    else
    {
        OsConfigLogError(AdhsGetLog(), "MmiGetInfo: failed to allocate %d bytes", *payloadSizeBytes);
        *payloadSizeBytes = 0;
        status = ENOMEM;
    }
    
    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(AdhsGetLog(), "MmiGetInfo(%s, %.*s, %d) returning %d", clientName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    return status;
}

int AdhsMmiGet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if ((NULL == componentName) || (NULL == objectName) || (NULL == payload) || (NULL == payloadSizeBytes))
    {
        OsConfigLogError(AdhsGetLog(), "MmiGet(%s, %s, %p, %p) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        status = EINVAL;
        return status;
    }

    *payload = NULL;
    *payloadSizeBytes = 0;

    if (!IsValidSession(clientSession))
    {
        OsConfigLogError(AdhsGetLog(), "MmiGet(%s, %s) called outside of a valid session", componentName, objectName);
        status = EINVAL;
    }
        
    if ((MMI_OK == status) && (strcmp(componentName, g_adhsComponentName)))
    {
        OsConfigLogError(AdhsGetLog(), "MmiGet called for an unsupported component name (%s)", componentName);
        status = EINVAL;
    }

    if ((MMI_OK == status) && (strcmp(objectName, g_reportedOptInObjectName)))
    {
        OsConfigLogError(AdhsGetLog(), "MmiGet called for an unsupported object name (%s)", componentName);
        status = EINVAL;
    }

    char* value = NULL;

    if (MMI_OK == status)
    {
        char* fileContent = LoadStringFromFile(g_adhsConfigFile, false, AdhsGetLog());
        if (NULL != fileContent)
        {
            const unsigned int matchGroupsSize = 3;
            regmatch_t matchGroups[matchGroupsSize];
            regex_t permissionRegex;

            if (0 == regcomp(&permissionRegex, g_permissionPattern, REG_EXTENDED))
            {
                if (0 == regexec(&permissionRegex, fileContent, matchGroupsSize, matchGroups, 0))
                {
                    // Property value is located in the third match group.
                    const unsigned int fileContentSizeBytes = strlen(fileContent);
                    if ((matchGroups[0].rm_so != -1) && (matchGroups[0].rm_eo != -1) &&
                        (matchGroups[1].rm_so != -1) && (matchGroups[1].rm_eo != -1) &&
                        (matchGroups[2].rm_so > -1) && ((unsigned int)matchGroups[2].rm_so < fileContentSizeBytes) &&
                        (matchGroups[2].rm_eo > -1) && ((unsigned int)matchGroups[2].rm_eo < fileContentSizeBytes) && 
                        (matchGroups[2].rm_so < matchGroups[2].rm_eo))
                    {
                        const unsigned int valueSizeBytes = matchGroups[2].rm_eo - matchGroups[2].rm_so;
                        value = malloc(valueSizeBytes + 1);
                        if (value != *payload)
                        {
                            value[valueSizeBytes] = '\0';
                            strncpy(value, fileContent + matchGroups[2].rm_so, valueSizeBytes);
                        }
                        else
                        {
                            OsConfigLogError(AdhsGetLog(), "MmiGet: failed to allocate %d bytes", *payloadSizeBytes + 1);
                            status = ENOMEM;
                        }
                    }
                    else 
                    {
                        if (IsFullLoggingEnabled())
                        {
                            OsConfigLogError(AdhsGetLog(), "MmiGet failed to find TOML property (%s)", g_permissionConfigName);
                        }
                        status = EINVAL;
                    }
                }
                else 
                {
                    if (IsFullLoggingEnabled())
                    {
                        OsConfigLogError(AdhsGetLog(), "MmiGet failed to find TOML property (%s)", g_permissionConfigName);
                    }
                    status = EINVAL;
                }

                regfree(&permissionRegex);
            } 
            else 
            {
                OsConfigLogError(AdhsGetLog(), "MmiGet failed to compile regular expression (%s)", g_permissionPattern);
                status = EINVAL;
            }
            
            FREE_MEMORY(fileContent);
        }
        else 
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(AdhsGetLog(), "MmiGet failed to read TOML file (%s)", g_adhsConfigFile);
            }
            status = EINVAL;
        }

        // Reset status and payload if TOML file could not be parsed or property not found, as it may yet have to be configured.
        if (MMI_OK != status)
        {
            status = MMI_OK;
        }
    }

    if (MMI_OK == status)
    {
        *payloadSizeBytes = ((NULL != value) ? strlen(value) : 0) + 2;

        if ((g_maxPayloadSizeBytes > 0) && ((unsigned)*payloadSizeBytes > g_maxPayloadSizeBytes))
        {
            OsConfigLogError(AdhsGetLog(), "MmiGet(%s, %s) insufficient maxmimum size (%d bytes) versus data size (%d bytes), reported value will be truncated", componentName, objectName, g_maxPayloadSizeBytes, *payloadSizeBytes);
            *payloadSizeBytes = g_maxPayloadSizeBytes;
        }

        *payload = (MMI_JSON_STRING)malloc(*payloadSizeBytes + 1);
        if (NULL != *payload)
        {
            snprintf(*payload, *payloadSizeBytes + 1, "\"%s\"", (NULL != value) ? value : "");
        }
        else
        {
            OsConfigLogError(AdhsGetLog(), "MmiGet: failed to allocate %d bytes", *payloadSizeBytes + 1);
            *payloadSizeBytes = 0;
            status = ENOMEM;
        }
    }

    if (IsFullLoggingEnabled())
    {
        OsConfigLogInfo(AdhsGetLog(), "MmiGet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, *payloadSizeBytes, *payload, *payloadSizeBytes, status);
    }

    FREE_MEMORY(value);

    return status;
}

int AdhsMmiSet(MMI_HANDLE clientSession, const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{
    int status = MMI_OK;

    if ((NULL == componentName) || (NULL == objectName) || (NULL == payload) || (payloadSizeBytes < 0))
    {
        OsConfigLogError(AdhsGetLog(), "MmiSet(%s, %s, %p, %d) called with invalid arguments", componentName, objectName, payload, payloadSizeBytes);
        status = EINVAL;
        return status;
    }

    if (!IsValidSession(clientSession))
    {
        OsConfigLogError(AdhsGetLog(), "MmiSet(%s, %s) called outside of a valid session", componentName, objectName);
        status = EINVAL;
    }
        
    if ((MMI_OK == status) && (strcmp(componentName, g_adhsComponentName)))
    {
        OsConfigLogError(AdhsGetLog(), "MmiSet called for an unsupported component name (%s)", componentName);
        status = EINVAL;
    }
        
    if ((MMI_OK == status) && (strcmp(objectName, g_desiredOptInObjectName)))
    {
        OsConfigLogError(AdhsGetLog(), "MmiSet called for an unsupported object name (%s)", objectName);
        status = EINVAL;
    }

    // if (MMI_OK == status)
    // {
    //     // Make sure that payload is null-terminated for json_parse_string.
    //     char* buffer = malloc(payloadSizeBytes + 1);
    //     if (NULL != buffer)
    //     {
    //         buffer[payloadSizeBytes] = '\0';
    //         strncpy(buffer, payload, payloadSizeBytes);

    //         JSON_Value* rootValue = json_parse_string(buffer);
    //         if (json_value_get_type(rootValue) == JSONObject)
    //         {
    //             JSON_Object* rootObject = json_value_get_object(rootValue);

    //             // Create new JSON_Value with validated output.
    //             JSON_Value* newValue = json_value_init_object();
    //             JSON_Object* newObject = json_value_get_object(newValue);

    //             const unsigned int count = json_object_get_count(rootObject);
    //             for (unsigned int i = 0; i < count; i++)
    //             {
    //                 const char* name = json_object_get_name(rootObject, i);
    //                 JSON_Value* currentValue = json_object_get_value(rootObject, name);

    //                 if ((0 == strcmp(name, g_desiredCacheHostSettingName)) && (JSONString == json_value_get_type(currentValue)))
    //                 {
    //                     const char* cacheHost = json_value_get_string(currentValue);
    //                     json_object_set_string(newObject, g_cacheHostConfigName, cacheHost);
    //                 }
    //                 else if ((0 == strcmp(name, g_desiredCacheHostSourceSettingName)) && (JSONNumber == json_value_get_type(currentValue)))
    //                 {
    //                     const int cacheHostSource = (int)json_value_get_number(currentValue);
    //                     if ((cacheHostSource >= 0) && (cacheHostSource <= 3))
    //                     {
    //                         json_object_set_number(newObject, g_cacheHostSourceConfigName, cacheHostSource);
    //                     }
    //                     else 
    //                     {
    //                         OsConfigLogError(AdhsGetLog(), "MmiSet called with invalid cacheHostSource (%d)", cacheHostSource);
    //                         status = EINVAL;
    //                     }
    //                 }
    //                 else if ((0 == strcmp(name, g_desiredCacheHostFallbackSettingName)) && (JSONNumber == json_value_get_type(currentValue)))
    //                 {
    //                     const int cacheHostFallback = (int)json_value_get_number(currentValue);
    //                     json_object_set_number(newObject, g_cacheHostFallbackConfigName, cacheHostFallback);
    //                 }
    //                 else if ((0 == strcmp(name, g_desiredPercentageDownloadThrottleSettingName)) && (JSONNumber == json_value_get_type(currentValue)))
    //                 {
    //                     const int percentageDownloadThrottle = (int)json_value_get_number(currentValue);
    //                     if ((percentageDownloadThrottle >= 0) && (percentageDownloadThrottle <= 100))
    //                     {
    //                         json_object_set_number(newObject, g_percentageDownloadThrottleConfigName, percentageDownloadThrottle);
    //                     }
    //                     else 
    //                     {
    //                         OsConfigLogError(AdhsGetLog(), "MmiSet called with invalid percentageDownloadThrottle (%d)", percentageDownloadThrottle);
    //                         status = EINVAL;
    //                     }
    //                 }
    //             }

    //             if (JSONSuccess != json_serialize_to_file_pretty(newValue, g_adhsConfigFile))
    //             {
    //                 OsConfigLogError(AdhsGetLog(), "MmiSet failed to write JSON file (%s)", g_adhsConfigFile);
    //                 status = EIO;
    //             }

    //             json_value_free(newValue);
    //         }
    //         else
    //         {
    //             OsConfigLogError(AdhsGetLog(), "MmiSet failed to parse JSON (%.*s)", payloadSizeBytes, payload);
    //             status = EINVAL;
    //         }

    //         json_value_free(rootValue);
    //         FREE_MEMORY(buffer);
    //     }
    //     else 
    //     {
    //         OsConfigLogError(AdhsGetLog(), "MmiSet failed to allocate %d bytes", payloadSizeBytes + 1);
    //         status = ENOMEM;
    //     }
    // }

    OsConfigLogInfo(AdhsGetLog(), "MmiSet(%p, %s, %s, %.*s, %d) returning %d", clientSession, componentName, objectName, payloadSizeBytes, payload, payloadSizeBytes, status);
    
    return status;
}

void AdhsMmiFree(MMI_JSON_STRING payload)
{
    FREE_MEMORY(payload);
}
