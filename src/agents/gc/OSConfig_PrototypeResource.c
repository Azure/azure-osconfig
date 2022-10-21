// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Common.h"

// The log file for the NRP
#define LOG_FILE "/var/log/osconfig_gc_nrp.log"
#define ROLLED_LOG_FILE "/var/log/osconfig_gc_nrp.bak"

// Retail we'll change these to dynamically allocated strings
#define MAX_PROTO_STRING_LENGTH 256

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
static char g_prototypeClassKey[MAX_PROTO_STRING_LENGTH] = {0};
static char g_ensure[MAX_PROTO_STRING_LENGTH] = {0}; //"Present","Absent"
static char g_desiredString[MAX_PROTO_STRING_LENGTH] = {0};
static bool g_desiredBoolean = false;
static unsigned int g_desiredInteger = 0;

//Reported (read)
static char g_reportedString[MAX_PROTO_STRING_LENGTH] = {0};
static bool g_reportedBoolean = false;
//unused for now static unsigned int g_reportedInteger = 0;
static unsigned int g_reportedIntegerStatus = 0;
static char g_reportedStringResult[MAX_PROTO_STRING_LENGTH] = {0}; //"PASS", "FAIL", "ERROR", "WARNING", "SKIP"

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

        if (NULL == (g_mpiHandle = CallMpiOpen("GC OSConfig NRP Prototype", 0, GetLog())))
        {
            OsConfigLogError(GetLog(), "MpiOpen failed");
            status = false;
        }
    }
    else
    {
        OsConfigLogError(GetLog(), "[OSConfig_PrototypeResource] MPI server could not be started");
    }

    return status;
}

void __attribute__((constructor)) Initialize()
{
    strncpy(g_prototypeClassKey, "Prototype class key", ARRAY_SIZE(g_prototypeClassKey) - 1);
    strncpy(g_ensure, "Present", ARRAY_SIZE(g_ensure) - 1);
    strncpy(g_desiredString, "Desired string value", ARRAY_SIZE(g_desiredString) - 1);
    strncpy(g_reportedString, "Reported string value", ARRAY_SIZE(g_reportedString) - 1);
    strncpy(g_reportedStringResult, "PASS", ARRAY_SIZE(g_reportedStringResult) - 1);
    g_desiredBoolean = false;
    g_reportedBoolean = false;
    g_desiredInteger = 0;
    //g_reportedInteger = 0;
    g_reportedIntegerStatus = 0;

    g_log = OpenLog(LOG_FILE, ROLLED_LOG_FILE);

    RefreshMpiClientSession();

    OsConfigLogInfo(GetLog(), "[OSConfig_PrototypeResource] Initialized (PID: %d, MPI handle: %p)", getpid(), g_mpiHandle);
}

void __attribute__((destructor)) Destroy()
{
    OsConfigLogInfo(GetLog(), "[OSConfig_PrototypeResource] Terminated (PID: %d, MPI handle: %p)", getpid(), g_mpiHandle);
    
    if (NULL != g_mpiHandle)
    {
        CallMpiClose(g_mpiHandle, GetLog());
        g_mpiHandle = NULL;
    }

    CloseLog(&g_log);
}

/* @migen@ */

void MI_CALL OSConfig_PrototypeResource_Load(
    _Outptr_result_maybenull_ OSConfig_PrototypeResource_Self** self,
    _In_opt_ MI_Module_Self* selfModule,
    _In_ MI_Context* context)
{
    MI_UNREFERENCED_PARAMETER(selfModule);

    LogInfo(context, GetLog(), "[OSConfig_PrototypeResource] Load");

    *self = NULL;

    MI_Context_PostResult(context, MI_RESULT_OK);
}

void MI_CALL OSConfig_PrototypeResource_Unload(
    _In_opt_ OSConfig_PrototypeResource_Self* self,
    _In_ MI_Context* context)
{
    MI_UNREFERENCED_PARAMETER(self);

    LogInfo(context, GetLog(), "[OSConfig_PrototypeResource] Unload");

    MI_Context_PostResult(context, MI_RESULT_OK);
}

void MI_CALL OSConfig_PrototypeResource_EnumerateInstances(
    _In_opt_ OSConfig_PrototypeResource_Self* self,
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

    LogInfo(context, GetLog(), "[OSConfig_PrototypeResource] EnumerateInstances");

    MI_Context_PostResult(context, MI_RESULT_NOT_SUPPORTED);
}

void MI_CALL OSConfig_PrototypeResource_GetInstance(
    _In_opt_ OSConfig_PrototypeResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_ const OSConfig_PrototypeResource* resourceClass,
    _In_opt_ const MI_PropertySet* propertySet)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(nameSpace);
    MI_UNREFERENCED_PARAMETER(className);
    MI_UNREFERENCED_PARAMETER(resourceClass);
    MI_UNREFERENCED_PARAMETER(propertySet);

    LogInfo(context, GetLog(), "[OSConfig_PrototypeResource] GetInstance");

    MI_Context_PostResult(context, MI_RESULT_NOT_SUPPORTED);
}

void MI_CALL OSConfig_PrototypeResource_CreateInstance(
    _In_opt_ OSConfig_PrototypeResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_ const OSConfig_PrototypeResource* newInstance)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(nameSpace);
    MI_UNREFERENCED_PARAMETER(className);
    MI_UNREFERENCED_PARAMETER(newInstance);

    LogInfo(context, GetLog(), "[OSConfig_PrototypeResource] CreateInstance");

    MI_Context_PostResult(context, MI_RESULT_NOT_SUPPORTED);
}

void MI_CALL OSConfig_PrototypeResource_ModifyInstance(
    _In_opt_ OSConfig_PrototypeResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_ const OSConfig_PrototypeResource* modifiedInstance,
    _In_opt_ const MI_PropertySet* propertySet)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(nameSpace);
    MI_UNREFERENCED_PARAMETER(className);
    MI_UNREFERENCED_PARAMETER(modifiedInstance);
    MI_UNREFERENCED_PARAMETER(propertySet);

    LogInfo(context, GetLog(), "[OSConfig_PrototypeResource] ModifyInstance");

    MI_Context_PostResult(context, MI_RESULT_NOT_SUPPORTED);
}

void MI_CALL OSConfig_PrototypeResource_DeleteInstance(
    _In_opt_ OSConfig_PrototypeResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_ const OSConfig_PrototypeResource* resourceClass)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(nameSpace);
    MI_UNREFERENCED_PARAMETER(className);
    MI_UNREFERENCED_PARAMETER(resourceClass);

    LogInfo(context, GetLog(), "[OSConfig_PrototypeResource] DeleteInstance");

    MI_Context_PostResult(context, MI_RESULT_NOT_SUPPORTED);
}

struct OSConfig_PrototypeResource_Parameters
{
    const char* name;
    int miType;
    const char* stringValue;
    const bool booleanValue;
    const int integerValue;
};

void MI_CALL OSConfig_PrototypeResource_Invoke_GetTargetResource(
    _In_opt_ OSConfig_PrototypeResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_opt_z_ const MI_Char* methodName,
    _In_ const OSConfig_PrototypeResource* resourceClass,
    _In_opt_ const OSConfig_PrototypeResource_GetTargetResource* in)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(resourceClass);

    MI_Result miResult = MI_RESULT_OK;
    int mpiResult = MPI_OK;
    MI_Instance *resultResourceObject = NULL;
    MI_Value miValue = {0};
    MI_Value miValueResult = {0};
    MI_Value miValueResource = {0};
    /* Do we need a separate MI_Value for each?
    MI_Value miValuePrototypeClassKey = {0};
    MI_Value miValueEnsure = {0};
    MI_Value miValueDesiredString = {0};
    MI_Value miValueDesiredBoolean = {0};
    MI_Value miValueDesiredInteger = {0};
    MI_Value miValueReportedString = {0};
    MI_Value miValueReportedBoolean = {0};
    MI_Value miValueReportedInteger = {0};
    MI_Value miValueReportedIntegerStatus = {0};
    MI_Value miValueReportedStringResult = {0};*/

    const char* componentName = "HostName";
    const char* objectName = "name";
    char* hostName = NULL;
    int hostNameLength = 0;
    JSON_Value* jsonValue = NULL;
    const char* jsonString = NULL;
    char* payloadString = NULL;


    // Reported values
    struct OSConfig_PrototypeResource_Parameters allParameters[] = {
        { "PrototypeClassKey", MI_STRING, in->InputResource.value->PrototypeClassKey.value, false, 0 },
        { "Ensure", MI_STRING, g_ensure, false, 0 },
        { "DesiredString", MI_STRING, g_desiredString, false, 0 },
        { "DesiredBoolean", MI_BOOLEAN, NULL, g_desiredBoolean, 0 },
        { "DesiredInteger", MI_UINT32, NULL, false, g_desiredInteger },
        { "ReportedString", MI_STRING, g_reportedString, false, 0 },
        { "ReportedBoolean", MI_BOOLEAN, NULL, g_reportedBoolean, 0 },
        // Set as Boolean in MOF, will keep as such for now { "ReportedInteger", MI_UINT32, NULL, false, g_reportedInteger },
        { "ReportedInteger", MI_BOOLEAN, NULL, false, g_reportedBoolean },
        { "ReportedIntegerStatus", MI_UINT32, NULL, false, g_reportedIntegerStatus },
        { "ReportedStringResult", MI_STRING, g_reportedStringResult, false, 0 }
    };

    int allParametersSize = ARRAY_SIZE(allParameters);

    OSConfig_PrototypeResource_GetTargetResource get_result_object;

    LogInfo(context, GetLog(), "[OSConfig_PrototypeResource.Get] Starting Get");

    if ((NULL == in) || (MI_FALSE == in->InputResource.exists) || (NULL == in->InputResource.value))
    {
        miResult = MI_RESULT_FAILED;
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Get] Invalid Get argument");
        goto Exit;
    }

    if ((MI_FALSE == in->InputResource.value->PrototypeClassKey.exists) && (NULL != in->InputResource.value->PrototypeClassKey.value))
    {
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Get] No PrototypeClassKey");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    if (strncmp(g_prototypeClassKey, in->InputResource.value->PrototypeClassKey.value, ARRAY_SIZE(g_prototypeClassKey) - 1))
    {
        // Refresh our stored key
        memset(g_prototypeClassKey, 0, sizeof(g_prototypeClassKey));
        strncpy(g_prototypeClassKey, in->InputResource.value->PrototypeClassKey.value, ARRAY_SIZE(g_prototypeClassKey) - 1);
    }

    if (MI_RESULT_OK != (miResult = OSConfig_PrototypeResource_GetTargetResource_Construct(&get_result_object, context)))
    {
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Get] GetTargetResource_Construct failed with %d", miResult);
        goto Exit;
    }

    if (MI_RESULT_OK != (miResult = OSConfig_PrototypeResource_GetTargetResource_Set_MIReturn(&get_result_object, 0)))
    {
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Get] GetTargetResource_Set_MIReturn failed with %d", miResult);
        goto Exit;
    }

    if (MI_RESULT_OK != (miResult = MI_Context_NewInstance(context, &OSConfig_PrototypeResource_rtti, &resultResourceObject)))
    {
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Get] MI_Context_NewInstance failed with %d", miResult);
        goto Exit;
    }

    miValueResource.instance = resultResourceObject;

    if (NULL == g_mpiHandle)
    {
        if (!RefreshMpiClientSession())
        {
            mpiResult = ESRCH;
            miResult = MI_RESULT_FAILED;
            LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Get] Failed to start the MPI server (%d)", mpiResult);
        }
    }

    if (g_mpiHandle)
    {
        // Read a simple reported value from OSConfig, such as HostName.name (name of the local host, aka device name)
        if (MPI_OK == (mpiResult = CallMpiGet(componentName, objectName, &hostName, &hostNameLength, GetLog())))
        {
            if (NULL == hostName)
            {
                mpiResult = ENODATA;
                miResult = MI_RESULT_FAILED;
                LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Get] CallMpiGet for '%s' and '%s' returned no payload ('%s', %d) (%d)", 
                    componentName, objectName, hostName, hostNameLength, mpiResult);
            }
            else
            {
                if (NULL != (payloadString = malloc(hostNameLength + 1)))
                {
                    memset(payloadString, 0, hostNameLength + 1);
                    memcpy(payloadString, hostName, hostNameLength);

                    if (NULL != (jsonValue = json_parse_string(payloadString)))
                    {
                        jsonString = json_value_get_string(jsonValue);
                        if (jsonString)
                        {
                            memset(g_reportedString, 0, sizeof(g_reportedString));
                            strncpy(g_reportedString, jsonString, ARRAY_SIZE(g_reportedString) - 1);
                        }
                        else
                        {
                            mpiResult = EINVAL;
                            miResult = MI_RESULT_FAILED;
                            LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Get] json_value_get_string(%s) failed", payloadString);
                        }
                        
                        json_value_free(jsonValue);
                    }
                    else
                    {
                        mpiResult = EINVAL;
                        miResult = MI_RESULT_FAILED;
                        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Get] json_parse_string(%s) failed", payloadString);
                    }

                    FREE_MEMORY(payloadString);
                }
                else
                {
                    mpiResult = ENOMEM;
                    miResult = MI_RESULT_FAILED;
                    LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Get] Failed to allocate %d bytes", hostNameLength + 1);
                }

                LogInfo(context, GetLog(), "GetTargetResource: ReportedString value: '%s'", g_reportedString);
                CallMpiFree(hostName);
            }
        }
    }
    
    if (MPI_OK != mpiResult)
    {
        miResult = MI_RESULT_FAILED;
        
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Get] CallMpiGet for '%s' and '%s' failed with %d", componentName, objectName, mpiResult);

        if ((0 == g_reportedIntegerStatus) || (0 == strcmp(g_reportedStringResult, "PASS")))
        {
            g_reportedIntegerStatus = mpiResult;
            memset(g_reportedStringResult, 0, sizeof(g_reportedStringResult));
            strncpy(g_reportedStringResult, "FAIL", ARRAY_SIZE(g_reportedStringResult) - 1);
        }
    }

    for (int i = 0; i < allParametersSize; i++)
    {
        switch (allParameters[i].miType)
        {
            case MI_STRING:
                if (NULL != allParameters[i].stringValue)
                {
                    miValue.string = (MI_Char*)(allParameters[i].stringValue);
                    miResult = MI_Instance_SetElement(resultResourceObject, MI_T(allParameters[i].name), &miValue, MI_STRING, 0);
                }
                else
                {
                    LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Get] No string value for '%s'", allParameters[i].name);
                    miResult = MI_RESULT_FAILED;
                }
                break;

            case MI_BOOLEAN:
                miValue.boolean = (MI_Boolean)(allParameters[i].booleanValue);
                miResult = MI_Instance_SetElement(resultResourceObject, MI_T(allParameters[i].name), &miValue, MI_BOOLEAN, 0);
                break;

            case MI_UINT32:
            default:
                miValue.uint32 = (MI_Uint32)(allParameters[i].integerValue);
                miResult = MI_Instance_SetElement(resultResourceObject, MI_T(allParameters[i].name), &miValue, MI_UINT32, 0);
        }

        if (MI_RESULT_OK != miResult)
        {
            LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Get] MI_Instance_SetElement('%s') failed with %d", allParameters[i].name, miResult);
        }

        memset(&miValue, 0, sizeof(miValue));
    }
    
    // Set the created output resource instance as the output resource in the GetTargetResource instance
    if (MI_RESULT_OK != (miResult = MI_Instance_SetElement(&get_result_object.__instance, MI_T("OutputResource"), &miValueResource, MI_INSTANCE, 0)))
    {
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Get] MI_Instance_SetElement(OutputResource) failed with %d", miResult);
        goto Exit;
    }
        
Exit:
    // Clean up the Result MI value instance if needed
    if ((NULL != miValueResult.instance) && (MI_RESULT_OK != (miResult = MI_Instance_Delete(miValueResult.instance))))
    {
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Get] MI_Instance_Delete(miValueResult) failed");
    }

    // Clean up the output resource instance
    if ((NULL != resultResourceObject) && (MI_RESULT_OK != (miResult = MI_Instance_Delete(resultResourceObject))))
    {
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Get] MI_Instance_Delete(resultResourceObject) failed");
    }

    // Clean up the GetTargetResource instance
    if (MI_RESULT_OK != (miResult = OSConfig_PrototypeResource_GetTargetResource_Destruct(&get_result_object)))
    {
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Get] GetTargetResource_Destruct failed with %d", miResult);
    }

    // Post MI result back to MI to finish
    LogInfo(context, GetLog(), "[OSConfig_PrototypeResource.Get] Get complete with miResult %d", miResult);
    MI_Context_PostResult(context, miResult);
}

void MI_CALL OSConfig_PrototypeResource_Invoke_TestTargetResource(
    _In_opt_ OSConfig_PrototypeResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_opt_z_ const MI_Char* methodName,
    _In_ const OSConfig_PrototypeResource* resourceClass,
    _In_opt_ const OSConfig_PrototypeResource_TestTargetResource* in)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(resourceClass);

    OSConfig_PrototypeResource_TestTargetResource test_result_object;

    MI_Result miResult = MI_RESULT_OK;
    MI_Boolean is_compliant = MI_FALSE;

    MI_Value miValueResult;
    memset(&miValueResult, 0, sizeof(MI_Value));

    LogInfo(context, GetLog(), "[OSConfig_PrototypeResource.Test] Starting Test");

    if ((in == NULL) || (in->InputResource.exists == MI_FALSE) || (in->InputResource.value == NULL))
    {
        miResult = MI_RESULT_FAILED;
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Test] Invalid Test argument");
        goto Exit;
    }

    if ((in->InputResource.value->PrototypeClassKey.exists == MI_FALSE) &&  (in->InputResource.value->PrototypeClassKey.value != NULL))
    {
        miResult = MI_RESULT_FAILED;
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Test] No PrototypeClassKey");
        goto Exit;
    }

    if (strncmp(g_prototypeClassKey, in->InputResource.value->PrototypeClassKey.value, ARRAY_SIZE(g_prototypeClassKey) - 1))
    {
        // Refresh our stored key
        memset(g_prototypeClassKey, 0, sizeof(g_prototypeClassKey));
        strncpy(g_prototypeClassKey, in->InputResource.value->PrototypeClassKey.value, ARRAY_SIZE(g_prototypeClassKey) - 1);
    }

    if ((in->InputResource.value->Ensure.exists == MI_TRUE) && (in->InputResource.value->Ensure.value != NULL))
    {
        if ((!strcmp(in->InputResource.value->Ensure.value, "Present")) || (!strcmp(in->InputResource.value->Ensure.value, "Absent")))
        {
            memset(g_ensure, 0, sizeof(g_ensure));
            memcpy(g_ensure, in->InputResource.value->Ensure.value, ARRAY_SIZE(g_ensure) - 1);

            // Simulating compliance either way
            is_compliant = MI_TRUE;
        }
        else
        {
            LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Test] Unknown ensure value ('%s')", in->InputResource.value->Ensure.value);
            is_compliant = MI_FALSE;
        }
    }
    
    if (MI_RESULT_OK != (miResult = OSConfig_PrototypeResource_TestTargetResource_Construct(&test_result_object, context)))
    {
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Test] TestTargetResource_Construct failed with %d", miResult);
        goto Exit;
    }

    if (MI_RESULT_OK != (miResult = OSConfig_PrototypeResource_TestTargetResource_Set_MIReturn(&test_result_object, 0)))
    {
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Test] TestTargetResource_Set_MIReturn failed with %d", miResult);
        goto Exit;
    }

    OSConfig_PrototypeResource_TestTargetResource_Set_Result(&test_result_object, is_compliant);
    MI_Context_PostInstance(context, &(test_result_object.__instance));

Exit:
    if ((NULL != miValueResult.instance) && (MI_RESULT_OK != (miResult = MI_Instance_Delete(miValueResult.instance))))
    {
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Test] MI_Instance_Delete failed");
    }

    if (MI_RESULT_OK != (miResult = OSConfig_PrototypeResource_TestTargetResource_Destruct(&test_result_object)))
    {
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Test] TestTargetResource_Destruct failed");
    }

    LogInfo(context, GetLog(), "[OSConfig_PrototypeResource.Test] Test complete with miResult %d", miResult);
    MI_Context_PostResult(context, miResult);
}

void MI_CALL OSConfig_PrototypeResource_Invoke_SetTargetResource(
    _In_opt_ OSConfig_PrototypeResource_Self* self,
    _In_ MI_Context* context,
    _In_opt_z_ const MI_Char* nameSpace,
    _In_opt_z_ const MI_Char* className,
    _In_opt_z_ const MI_Char* methodName,
    _In_ const OSConfig_PrototypeResource* resourceClass,
    _In_opt_ const OSConfig_PrototypeResource_SetTargetResource* in)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(resourceClass);

    MI_Result miResult = MI_RESULT_OK;
    int mpiResult = MPI_OK;

    const char* componentName = "HostName";
    const char* objectName = "desiredName";
    const char* payloadTemplate = "\"%s\"";
    char* payloadString = NULL;
    int payloadSize = 0;

    OSConfig_PrototypeResource_SetTargetResource set_result_object;

    LogInfo(context, GetLog(), "[OSConfig_PrototypeResource.Set] Starting Set");

    if ((NULL == in) || (MI_FALSE == in->InputResource.exists) || (NULL == in->InputResource.value))
    {
        miResult = MI_RESULT_FAILED;
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Set] Invalid argument");
        goto Exit;
    }

    if ((MI_FALSE == in->InputResource.value->PrototypeClassKey.exists) && (NULL != in->InputResource.value->PrototypeClassKey.value))
    {
        miResult = MI_RESULT_FAILED;
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Set] No PrototypeClassKey");
        goto Exit;
    }

    if (MI_RESULT_OK != (miResult = OSConfig_PrototypeResource_SetTargetResource_Construct(&set_result_object, context)))
    {
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Set] SetTargetResource_Construct failed with %d", miResult);
        goto Exit;
    }

    if (MI_RESULT_OK != (miResult = OSConfig_PrototypeResource_SetTargetResource_Set_MIReturn(&set_result_object, 0)))
    {
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Set] SetTargetResource_Set_MIReturn failed with %d", miResult);
        goto Exit;
    }

    MI_Context_PostInstance(context, &(set_result_object.__instance));

    // PrototypeClassKey
    if (strncmp(g_prototypeClassKey, in->InputResource.value->PrototypeClassKey.value, ARRAY_SIZE(g_prototypeClassKey) - 1))
    {
        memset(g_prototypeClassKey, 0, sizeof(g_prototypeClassKey));
        strncpy(g_prototypeClassKey, in->InputResource.value->PrototypeClassKey.value, ARRAY_SIZE(g_prototypeClassKey) - 1);
    }

    // DesiredString
    if (strncmp(g_desiredString, in->InputResource.value->DesiredString.value, ARRAY_SIZE(g_desiredString) - 1))
    {
        memset(g_desiredString, 0, sizeof(g_desiredString));
        strncpy(g_desiredString, in->InputResource.value->DesiredString.value, ARRAY_SIZE(g_desiredString) - 1);

        // Set a desired value for OSConfig, in this case HostName.desiredName

        if (NULL == g_mpiHandle)
        {
            if (!RefreshMpiClientSession())
            {
                miResult = MI_RESULT_FAILED;
                mpiResult = ESRCH;
                LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Set] Failed starting the MPI server (%d)", mpiResult);
            }
        }

        if (g_mpiHandle)
        {
            payloadSize = (int)strlen(g_desiredString) + 1;
            if (NULL != (payloadString = malloc(payloadSize)))
            {
                snprintf(payloadString, payloadSize, payloadTemplate, g_desiredString);

                if (MPI_OK == (mpiResult = CallMpiSet(componentName, objectName, g_desiredString, sizeof(g_desiredString), GetLog())))
                {
                    LogInfo(context, GetLog(), "[OSConfig_PrototypeResource.Set] DesiredString value '%s' successfully applied to device as '%.*s', %d bytes", 
                        g_desiredString, payloadSize, payloadString, payloadSize);
                }
                else
                {
                    miResult = MI_RESULT_FAILED;
                    LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Set] CallMpiSet for '%s' and '%s' failed with %d", componentName, objectName, mpiResult);
                }

                FREE_MEMORY(payloadString);
            }
            else
            {
                miResult = MI_RESULT_FAILED;
                mpiResult = ENOMEM;
                LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Set] Failed to allocate %d bytes", payloadSize);
            }
        }

        if ((MPI_OK != mpiResult) && (0 == g_reportedIntegerStatus) || (0 == strcmp(g_reportedStringResult, "PASS")))
        {
            g_reportedIntegerStatus = mpiResult;
            memset(g_reportedStringResult, 0, sizeof(g_reportedStringResult));
            strncpy(g_reportedStringResult, "FAIL", ARRAY_SIZE(g_reportedStringResult) - 1);
        }
    }

    // DesiredBoolean
    g_desiredBoolean = in->InputResource.value->DesiredBoolean.value;

    //DesiredInteger
    g_desiredInteger = in->InputResource.value->DesiredInteger.value;

    // Set results to report back
    memset(g_reportedStringResult, 0, sizeof(g_reportedStringResult));
    strncpy(g_reportedStringResult, "PASS", ARRAY_SIZE(g_reportedStringResult) - 1);
    g_reportedIntegerStatus = 0;

Exit:
    if (MI_RESULT_OK != miResult)
    {
        memset(g_reportedStringResult, 0, sizeof(g_reportedStringResult));
        strncpy(g_reportedStringResult, "FAIL", ARRAY_SIZE(g_reportedStringResult) - 1);
        g_reportedIntegerStatus = miResult;
    }
    
    if (MI_RESULT_OK != (miResult = OSConfig_PrototypeResource_SetTargetResource_Destruct(&set_result_object)))
    {
        LogError(context, miResult, GetLog(), "[OSConfig_PrototypeResource.Set] SetTargetResource_Destruct failed");
    }
    
    LogInfo(context, GetLog(), "[OSConfig_PrototypeResource.Set] Set complete with miResult %d", miResult);
    MI_Context_PostResult(context, miResult);
}