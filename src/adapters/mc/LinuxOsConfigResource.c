// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Common.h"

// The log file for the NRP
#define LOG_FILE "/var/log/osconfig_mc_nrp.log"
#define ROLLED_LOG_FILE "/var/log/osconfig_mc_nrp.bak"

// Retail we'll change these to dynamically allocated strings TODO
#define MAX_STRING_LENGTH 256

// Retail we'll change this to 0 meaning no limit
#define MAX_PAYLOAD_LENGTH MAX_STRING_LENGTH

// OSConfig's MPI server
#define MPI_SERVER "osconfig-platform"

#define LogWithMiContext(context, miResult, log, FORMAT, ...) {\
    {\
        char message[512] = {0};\
        if (0 < snprintf(message, ARRAY_SIZE(message), FORMAT, ##__VA_ARGS__)) {\
            if (MI_RESULT_OK == miResult) {\
                MI_Context_WriteVerbose(context, message);\
            } else{\
                MI_Context_PostError(context, miResult, MI_RESULT_TYPE_MI, message);\
            }\
        }\
    }\
}\

#define LogInfo(context, log, FORMAT, ...) {\
    OsConfigLogInfo(log, FORMAT, ##__VA_ARGS__);\
    LogWithMiContext(context, MI_RESULT_OK, log, FORMAT, ##__VA_ARGS__);\
}\

#define LogError(context, miResult, log, FORMAT, ...) {\
    OsConfigLogError(log, FORMAT, ##__VA_ARGS__);\
    LogWithMiContext(context, miResult, log, FORMAT, ##__VA_ARGS__);\
}\

// Desired (write; also reported together with read group)
static char* g_prototypeClassKey = NULL;
static char* g_componentName = NULL;
static char* g_reportedObjectName = NULL;
static char* g_desiredObjectName = NULL;
static char* g_desiredObjectValue = NULL;

//Reported (read)
static char* g_reportedObjectValue = NULL;
static uint32 g_reportedMpiResult = 0;

MPI_HANDLE g_mpiHandle = NULL;

static OSCONFIG_LOG_HANDLE g_log = NULL;

OSCONFIG_LOG_HANDLE GetLog()
{
    return g_log;
}

bool RefreshMpiClientSession(void)
{
    bool status = true;

    if (g_mpiHandle && IsDaemonActive(MPI_SERVER, GetLog()))
    {
        return status;
    }

    if (true == (status = EnableAndStartDaemon(MPI_SERVER, GetLog())))
    {
        sleep(1);

        if (NULL == (g_mpiHandle = CallMpiOpen("MC OSConfig NRP", MAX_PAYLOAD_LENGTH, GetLog())))
        {
            OsConfigLogError(GetLog(), "[LinuxOsConfigResource] MpiOpen failed");
            status = false;
        }
    }
    else
    {
        OsConfigLogError(GetLog(), "[LinuxOsConfigResource] MPI server could not be started");
    }

    return status;
}

void __attribute__((constructor)) Initialize()
{
    g_log = OpenLog(LOG_FILE, ROLLED_LOG_FILE);

    RefreshMpiClientSession();

    OsConfigLogInfo(GetLog(), "[LinuxOsConfigResource] Initialized (PID: %d, MPI handle: %p)", getpid(), g_mpiHandle);
}

void __attribute__((destructor)) Destroy()
{
    OsConfigLogInfo(GetLog(), "[LinuxOsConfigResource] Terminated (PID: %d, MPI handle: %p)", getpid(), g_mpiHandle);
    
    if (NULL != g_mpiHandle)
    {
        CallMpiClose(g_mpiHandle, GetLog());
        g_mpiHandle = NULL;
    }

    CloseLog(&g_log);

    FREE_MEMORY(g_prototypeClassKey);
    FREE_MEMORY(g_componentName);
    FREE_MEMORY(g_reportedObjectName);
    FREE_MEMORY(g_desiredObjectName);
    FREE_MEMORY(g_desiredObjectValue);
    FREE_MEMORY(g_reportedObjectValue);
}

/* @migen@ */

void MI_CALL LinuxOsConfigResource_Load(
    _Outptr_result_maybenull_ LinuxOsConfigResource_Self** self,
    _In_opt_ MI_Module_Self* selfModule,
    _In_ MI_Context* context)
{
    MI_UNREFERENCED_PARAMETER(selfModule);

    LogInfo(context, GetLog(), "[LinuxOsConfigResource] Load");

    *self = NULL;

    MI_Context_PostResult(context, MI_RESULT_OK);
}

void MI_CALL LinuxOsConfigResource_Unload(
    _In_opt_ LinuxOsConfigResource_Self* self,
    _In_ MI_Context* context)
{
    MI_UNREFERENCED_PARAMETER(self);

    LogInfo(context, GetLog(), "[LinuxOsConfigResource] Unload");

    MI_Context_PostResult(context, MI_RESULT_OK);
}

void MI_CALL LinuxOsConfigResource_EnumerateInstances(
    _In_opt_ LinuxOsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_opt_ const MI_PropertySet* propertySet,
    _In_ MI_Boolean keysOnly,
    _In_opt_ const MI_Filter* filter)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(nameSpace);
    MI_UNREFERENCED_PARAMETER(className);
    MI_UNREFERENCED_PARAMETER(propertySet);
    MI_UNREFERENCED_PARAMETER(keysOnly);
    MI_UNREFERENCED_PARAMETER(filter);

    LogInfo(context, GetLog(), "[LinuxOsConfigResource] EnumerateInstances");

    MI_Context_PostResult(context, MI_RESULT_NOT_SUPPORTED);
}

void MI_CALL LinuxOsConfigResource_GetInstance(
    _In_opt_ LinuxOsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_ const LinuxOsConfigResource* instanceName,
    _In_opt_ const MI_PropertySet* propertySet)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(nameSpace);
    MI_UNREFERENCED_PARAMETER(className);
    MI_UNREFERENCED_PARAMETER(resourceClass);
    MI_UNREFERENCED_PARAMETER(propertySet);

    LogInfo(context, GetLog(), "[LinuxOsConfigResource] GetInstance");

    MI_Context_PostResult(context, MI_RESULT_NOT_SUPPORTED);
}

void MI_CALL LinuxOsConfigResource_CreateInstance(
    _In_opt_ LinuxOsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_ const LinuxOsConfigResource* newInstance)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(nameSpace);
    MI_UNREFERENCED_PARAMETER(className);
    MI_UNREFERENCED_PARAMETER(newInstance);

    LogInfo(context, GetLog(), "[LinuxOsConfigResource] CreateInstance");

    MI_Context_PostResult(context, MI_RESULT_NOT_SUPPORTED);
}

void MI_CALL LinuxOsConfigResource_ModifyInstance(
    _In_opt_ LinuxOsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_ const LinuxOsConfigResource* modifiedInstance,
    _In_opt_ const MI_PropertySet* propertySet)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(nameSpace);
    MI_UNREFERENCED_PARAMETER(className);
    MI_UNREFERENCED_PARAMETER(modifiedInstance);
    MI_UNREFERENCED_PARAMETER(propertySet);

    LogInfo(context, GetLog(), "[LinuxOsConfigResource] ModifyInstance");

    MI_Context_PostResult(context, MI_RESULT_NOT_SUPPORTED);
}

void MI_CALL LinuxOsConfigResource_DeleteInstance(
    _In_opt_ LinuxOsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_ const LinuxOsConfigResource* instanceName)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(nameSpace);
    MI_UNREFERENCED_PARAMETER(className);
    MI_UNREFERENCED_PARAMETER(resourceClass);

    LogInfo(context, GetLog(), "[LinuxOsConfigResource] DeleteInstance");

    MI_Context_PostResult(context, MI_RESULT_NOT_SUPPORTED);
}

static MI_Result GetCurrentParameterValuesFromDevice(const char* who, const char* componentName, const char* objectName, MI_Context* context)
{
    JSON_Value* jsonValue = NULL;
    const char* jsonString = NULL;
    char* objectValue = NULL;
    int objectValueLength = 0;
    char* payloadString = NULL;
    int mpiResult = MPI_OK;
    MI_Result miResult = MI_RESULT_OK;

    if (NULL == g_mpiHandle)
    {
        if (!RefreshMpiClientSession())
        {
            mpiResult = ESRCH;
            miResult = MI_RESULT_FAILED;
            LogError(context, miResult, GetLog(), "[%s] Failed to start the MPI server (%d)", who, mpiResult);
        }
    }

    if (g_mpiHandle)
    {
        if (MPI_OK == (mpiResult = CallMpiGet(componentName, objectName, &objectValue, &objectValueLength, GetLog())))
        {
            if (NULL == objectValue)
            {
                mpiResult = ENODATA;
                miResult = MI_RESULT_FAILED;
                LogError(context, miResult, GetLog(), "[%s] CallMpiGet for '%s' and '%s' returned no payload ('%s', %d) (%d)", 
                    who, componentName, objectName, objectValue, objectValueLength, mpiResult);
            }
            else
            {
                if (NULL != (payloadString = malloc(objectValueLength + 1)))
                {
                    memset(payloadString, 0, objectValueLength + 1);
                    memcpy(payloadString, objectValue, objectValueLength);

                    if (NULL != (jsonValue = json_parse_string(payloadString)))
                    {
                        jsonString = json_value_get_string(jsonValue);
                        if (jsonString)
                        {
                            FREE_MEMORY(g_reportedObjectValue);
                            g_reportedObjectValue = DuplicateString(jsonString);
                            if (NULL == g_reportedObjectValue)
                            {
                                mpiResult = ENOMEM;
                                miResult = MI_RESULT_FAILED;
                                LogError(context, miResult, GetLog(), "[%s] DuplicateString(%s) failed", who, jsonString);
                            }
                        }
                        else
                        {
                            mpiResult = EINVAL;
                            miResult = MI_RESULT_FAILED;
                            LogError(context, miResult, GetLog(), "[%s] json_value_get_string(%s) failed", who, payloadString);
                        }

                        json_value_free(jsonValue);
                    }
                    else
                    {
                        mpiResult = EINVAL;
                        miResult = MI_RESULT_FAILED;
                        LogError(context, miResult, GetLog(), "[%s] json_parse_string(%s) failed", who, payloadString);
                    }

                    FREE_MEMORY(payloadString);
                }
                else
                {
                    mpiResult = ENOMEM;
                    miResult = MI_RESULT_FAILED;
                    LogError(context, miResult, GetLog(), "[%s] Failed to allocate %d bytes", who, objectValueLength + 1);
                }

                LogInfo(context, GetLog(), "[%s] ReportedString value: '%s'", who, g_reportedString);
                CallMpiFree(objectValue);
            }
        }
    }

    g_reportedMpiResult = mpiResult;

    FREE_MEMORY(payloadString);

    return miResult;
}

struct LinuxOsConfigResourceParameters
{
    const char* name;
    int miType;
    const char* stringValue;
    const int integerValue;
};

void MI_CALL LinuxOsConfigResource_Invoke_GetTargetResource(
    _In_opt_ LinuxOsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_opt_z_ const MI_Char* methodName,
    _In_ const LinuxOsConfigResource* instanceName,
    _In_opt_ const LinuxOsConfigResource_GetTargetResource* in)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(resourceClass);

    MI_Result miResult = MI_RESULT_OK;
    MI_Instance *resultResourceObject = NULL;
    MI_Value miValue = {0};
    MI_Value miValueResult = {0};
    MI_Value miValueResource = {0};

    // Reported values
    struct LinuxOsConfigResource_Parameters allParameters[] = {
        { "LinuxOsConfigClassKey", MI_STRING, in->InputResource.value->LinuxOsConfigClassKey.value, 0 },
        { "ComponentName", MI_STRING, g_componentName, 0 },
        { "ReportedObjectName", MI_STRING, g_reportedObjectName, 0 },
        { "ReportedObjectValue", MI_STRING, g_reportedObjectValue, 0 },
        { "DesiredObjectName", MI_STRING, g_desiredObjectName, 0 },
        { "DesiredObjectValue", MI_STRING, g_desiredObjectValue, 0 },
        { "ReportedMpiResult", MI_UINT32, NULL, g_reportedMpiResult }
    };

    int allParametersSize = ARRAY_SIZE(allParameters);

    /*
    string LinuxOsConfigClassKey;
    string ComponentName;
    string ReportedObjectName;
    string ReportedObjectValue;
    string DesiredObjectName;
    string DesiredObjectValue;
    uint32 ReportedMpiResult;
    */
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    LinuxOsConfigResource_GetTargetResource get_result_object;

    LogInfo(context, GetLog(), "[LinuxOsConfigResource.Get] Starting Get");

    if ((NULL == in) || (MI_FALSE == in->InputResource.exists) || (NULL == in->InputResource.value))
    {
        miResult = MI_RESULT_FAILED;
        LogError(context, miResult, GetLog(), "[LinuxOsConfigResource.Get] Invalid Get argument");
        goto Exit;
    }

    if ((MI_FALSE == in->InputResource.value->PrototypeClassKey.exists) && (NULL != in->InputResource.value->PrototypeClassKey.value))
    {
        LogError(context, miResult, GetLog(), "[LinuxOsConfigResource.Get] No PrototypeClassKey");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    if (strncmp(g_prototypeClassKey, in->InputResource.value->PrototypeClassKey.value, ARRAY_SIZE(g_prototypeClassKey) - 1))
    {
        // Refresh our stored key
        memset(g_prototypeClassKey, 0, sizeof(g_prototypeClassKey));
        strncpy(g_prototypeClassKey, in->InputResource.value->PrototypeClassKey.value, ARRAY_SIZE(g_prototypeClassKey) - 1);
    }

    if (MI_RESULT_OK != (miResult = LinuxOsConfigResource_GetTargetResource_Construct(&get_result_object, context)))
    {
        LogError(context, miResult, GetLog(), "[LinuxOsConfigResource.Get] GetTargetResource_Construct failed with %d", miResult);
        goto Exit;
    }

    if (MI_RESULT_OK != (miResult = LinuxOsConfigResource_GetTargetResource_Set_MIReturn(&get_result_object, 0)))
    {
        LogError(context, miResult, GetLog(), "[LinuxOsConfigResource.Get] GetTargetResource_Set_MIReturn failed with %d", miResult);
        goto Exit;
    }

    if (MI_RESULT_OK != (miResult = MI_Context_NewInstance(context, &LinuxOsConfigResource_rtti, &resultResourceObject)))
    {
        LogError(context, miResult, GetLog(), "[LinuxOsConfigResource.Get] MI_Context_NewInstance failed with %d", miResult);
        goto Exit;
    }

    if (MI_RESULT_OK != (miResult = GetCurrentParameterValuesFromDevice("LinuxOsConfigResource.Get", context)))
    {
        goto Exit;
    }

    miValueResource.instance = resultResourceObject;

    for (int i = 0; i < allParametersSize; i++)
    {
        switch (allParameters[i].miType)
        {
            case MI_STRING:
                if (NULL != allParameters[i].stringValue)
                {
                    miValue.string = (MI_Char*)(allParameters[i].stringValue);
                    miResult = MI_Instance_SetElement(resultResourceObject, MI_T(allParameters[i].name), &miValue, MI_STRING, 0);
                    LogInfo(context, GetLog(), "[LinuxOsConfigResource.Get] MI_Instance_SetElement('%s') to string value '%s' complete with miResult %d", 
                        allParameters[i].name, miValue.string, miResult);
                }
                else
                {
                    LogError(context, miResult, GetLog(), "[LinuxOsConfigResource.Get] No string value for '%s'", allParameters[i].name);
                    miResult = MI_RESULT_FAILED;
                }
                break;

            case MI_BOOLEAN:
                miValue.boolean = (MI_Boolean)(allParameters[i].booleanValue);
                miResult = MI_Instance_SetElement(resultResourceObject, MI_T(allParameters[i].name), &miValue, MI_BOOLEAN, 0);
                LogInfo(context, GetLog(), "[LinuxOsConfigResource.Get] MI_Instance_SetElement('%s') to boolean value '%s' complete with miResult %d", 
                    allParameters[i].name, miValue.boolean ? "true" : "false", miResult);
                break;

            case MI_UINT32:
            default:
                miValue.uint32 = (MI_Uint32)(allParameters[i].integerValue);
                miResult = MI_Instance_SetElement(resultResourceObject, MI_T(allParameters[i].name), &miValue, MI_UINT32, 0);
                LogInfo(context, GetLog(), "[LinuxOsConfigResource.Get] MI_Instance_SetElement('%s') to integer value '%d' complete with miResult %d", 
                    allParameters[i].name, miValue.uint32, miResult);
        }

        if (MI_RESULT_OK != miResult)
        {
            LogError(context, miResult, GetLog(), "[LinuxOsConfigResource.Get] MI_Instance_SetElement('%s') failed with %d", allParameters[i].name, miResult);
        }

        memset(&miValue, 0, sizeof(miValue));
    }
    
    // Set the created output resource instance as the output resource in the GetTargetResource instance
    if (MI_RESULT_OK != (miResult = MI_Instance_SetElement(&get_result_object.__instance, MI_T("OutputResource"), &miValueResource, MI_INSTANCE, 0)))
    {
        LogError(context, miResult, GetLog(), "[LinuxOsConfigResource.Get] MI_Instance_SetElement(OutputResource) failed with %d", miResult);
        goto Exit;
    }

    // Post the GetTargetResource instance
    if (MI_RESULT_OK != (miResult = LinuxOsConfigResource_GetTargetResource_Post(&get_result_object, context)))
    {
        LogError(context, miResult, GetLog(), "[LinuxOsConfigResource.Get] LinuxOsConfigResource_GetTargetResource_Post failed with %d", miResult);
        goto Exit;
    }
        
Exit:
    // Clean up the Result MI value instance if needed
    if ((NULL != miValueResult.instance) && (MI_RESULT_OK != (miResult = MI_Instance_Delete(miValueResult.instance))))
    {
        LogError(context, miResult, GetLog(), "[LinuxOsConfigResource.Get] MI_Instance_Delete(miValueResult) failed");
    }

    // Clean up the output resource instance
    if ((NULL != resultResourceObject) && (MI_RESULT_OK != (miResult = MI_Instance_Delete(resultResourceObject))))
    {
        LogError(context, miResult, GetLog(), "[LinuxOsConfigResource.Get] MI_Instance_Delete(resultResourceObject) failed");
    }

    // Clean up the GetTargetResource instance
    if (MI_RESULT_OK != (miResult = LinuxOsConfigResource_GetTargetResource_Destruct(&get_result_object)))
    {
        LogError(context, miResult, GetLog(), "[LinuxOsConfigResource.Get] GetTargetResource_Destruct failed with %d", miResult);
    }

    // Post MI result back to MI to finish
    LogInfo(context, GetLog(), "[LinuxOsConfigResource.Get] Get complete with miResult %d", miResult);
    MI_Context_PostResult(context, miResult);
   ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

void MI_CALL LinuxOsConfigResource_Invoke_TestTargetResource(
    _In_opt_ LinuxOsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_opt_z_ const MI_Char* methodName,
    _In_ const LinuxOsConfigResource* instanceName,
    _In_opt_ const LinuxOsConfigResource_TestTargetResource* in)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(nameSpace);
    MI_UNREFERENCED_PARAMETER(className);
    MI_UNREFERENCED_PARAMETER(methodName);
    MI_UNREFERENCED_PARAMETER(instanceName);
    MI_UNREFERENCED_PARAMETER(in);

    MI_Context_PostResult(context, MI_RESULT_NOT_SUPPORTED);
}

void MI_CALL LinuxOsConfigResource_Invoke_SetTargetResource(
    _In_opt_ LinuxOsConfigResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_opt_z_ const MI_Char* methodName,
    _In_ const LinuxOsConfigResource* instanceName,
    _In_opt_ const LinuxOsConfigResource_SetTargetResource* in)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(nameSpace);
    MI_UNREFERENCED_PARAMETER(className);
    MI_UNREFERENCED_PARAMETER(methodName);
    MI_UNREFERENCED_PARAMETER(instanceName);
    MI_UNREFERENCED_PARAMETER(in);

    MI_Context_PostResult(context, MI_RESULT_NOT_SUPPORTED);
}

