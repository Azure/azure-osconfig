// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Common.h"

// The log file for the NRP
#define LOG_FILE "/var/log/osconfig_nrp.log"
#define ROLLED_LOG_FILE "/var/log/osconfig_nrp.bak"

#define MAX_PAYLOAD_LENGTH 0

// OSConfig's MPI server
#define MPI_SERVER "osconfig-platform"

static const char* g_mpiClientName = "OSConfig NRP";
static const char* g_defaultValue = "-";
static const char* g_failValue = "FAIL";

// Desired (write; also reported together with read group)
static char* g_classKey = NULL;
static char* g_componentName = NULL;
static char* g_reportedObjectName = NULL;
static char* g_desiredObjectName = NULL;
static char* g_desiredObjectValue = NULL;

//Reported (read)
static char* g_reportedObjectValue = NULL;
static unsigned int g_reportedMpiResult = 0;

MPI_HANDLE g_mpiHandle = NULL;

static OSCONFIG_LOG_HANDLE g_log = NULL;

OSCONFIG_LOG_HANDLE GetLog(void)
{
    if (NULL == g_log)
    {
        g_log = OpenLog(LOG_FILE, ROLLED_LOG_FILE);
    }
    
    return g_log;
}

static bool RefreshMpiClientSession(void)
{
    bool status = true;

    if (g_mpiHandle && IsDaemonActive(MPI_SERVER, GetLog()))
    {
        return status;
    }

    if (true == (status = EnableAndStartDaemon(MPI_SERVER, GetLog())))
    {
        sleep(1);

        if (NULL == (g_mpiHandle = CallMpiOpen(g_mpiClientName, MAX_PAYLOAD_LENGTH, GetLog())))
        {
            OsConfigLogError(GetLog(), "[OsConfigResource] MpiOpen failed");
            status = false;
        }
    }
    else
    {
        OsConfigLogError(GetLog(), "[OsConfigResource] MPI server could not be started");
    }

    return status;
}

void __attribute__((constructor)) Initialize()
{
    RefreshMpiClientSession();

    g_classKey = DuplicateString(g_defaultValue);
    g_componentName = DuplicateString(g_defaultValue);
    g_reportedObjectName = DuplicateString(g_defaultValue);
    g_desiredObjectName = DuplicateString(g_defaultValue);
    g_desiredObjectValue = DuplicateString(g_failValue);

    OsConfigLogInfo(GetLog(), "[OsConfigResource] Initialized (PID: %d, MPI handle: %p)", getpid(), g_mpiHandle);
}

void __attribute__((destructor)) Destroy()
{
    OsConfigLogInfo(GetLog(), "[OsConfigResource] Terminating (PID: %d, MPI handle: %p)", getpid(), g_mpiHandle);
    
    if (NULL != g_mpiHandle)
    {
        CallMpiClose(g_mpiHandle, GetLog());
        g_mpiHandle = NULL;
    }

    CloseLog(&g_log);

    FREE_MEMORY(g_classKey);
    FREE_MEMORY(g_componentName);
    FREE_MEMORY(g_reportedObjectName);
    FREE_MEMORY(g_desiredObjectName);
    FREE_MEMORY(g_desiredObjectValue);
    FREE_MEMORY(g_reportedObjectValue);
}

void MI_CALL OsConfigResource_Load(
    OsConfigResource_Self** self,
    MI_Module_Self* selfModule,
    MI_Context* context)
{
    MI_UNREFERENCED_PARAMETER(selfModule);

    LogInfo(context, GetLog(), "[OsConfigResource] Load");

    *self = NULL;

    MI_Context_PostResult(context, MI_RESULT_OK);
}

void MI_CALL OsConfigResource_Unload(
    OsConfigResource_Self* self,
    MI_Context* context)
{
    MI_UNREFERENCED_PARAMETER(self);

    LogInfo(context, GetLog(), "[OsConfigResource] Unload");

    MI_Context_PostResult(context, MI_RESULT_OK);
}

void MI_CALL OsConfigResource_EnumerateInstances(
    OsConfigResource_Self* self,
    MI_Context* context,
    const MI_Char* nameSpace,
    const MI_Char* className,
    const MI_PropertySet* propertySet,
    MI_Boolean keysOnly,
    const MI_Filter* filter)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(nameSpace);
    MI_UNREFERENCED_PARAMETER(className);
    MI_UNREFERENCED_PARAMETER(propertySet);
    MI_UNREFERENCED_PARAMETER(keysOnly);
    MI_UNREFERENCED_PARAMETER(filter);

    LogInfo(context, GetLog(), "[OsConfigResource] EnumerateInstances");

    MI_Context_PostResult(context, MI_RESULT_NOT_SUPPORTED);
}

void MI_CALL OsConfigResource_GetInstance(
    OsConfigResource_Self* self,
    MI_Context* context,
    const MI_Char* nameSpace,
    const MI_Char* className,
    const OsConfigResource* instanceName,
    const MI_PropertySet* propertySet)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(nameSpace);
    MI_UNREFERENCED_PARAMETER(className);
    MI_UNREFERENCED_PARAMETER(instanceName);
    MI_UNREFERENCED_PARAMETER(propertySet);

    LogInfo(context, GetLog(), "[OsConfigResource] GetInstance");

    MI_Context_PostResult(context, MI_RESULT_NOT_SUPPORTED);
}

void MI_CALL OsConfigResource_CreateInstance(
    OsConfigResource_Self* self,
    MI_Context* context,
    const MI_Char* nameSpace,
    const MI_Char* className,
    const OsConfigResource* newInstance)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(nameSpace);
    MI_UNREFERENCED_PARAMETER(className);
    MI_UNREFERENCED_PARAMETER(newInstance);

    LogInfo(context, GetLog(), "[OsConfigResource] CreateInstance");

    MI_Context_PostResult(context, MI_RESULT_NOT_SUPPORTED);
}

void MI_CALL OsConfigResource_ModifyInstance(
    OsConfigResource_Self* self,
    MI_Context* context,
    const MI_Char* nameSpace,
    const MI_Char* className,
    const OsConfigResource* modifiedInstance,
    const MI_PropertySet* propertySet)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(nameSpace);
    MI_UNREFERENCED_PARAMETER(className);
    MI_UNREFERENCED_PARAMETER(modifiedInstance);
    MI_UNREFERENCED_PARAMETER(propertySet);

    LogInfo(context, GetLog(), "[OsConfigResource] ModifyInstance");

    MI_Context_PostResult(context, MI_RESULT_NOT_SUPPORTED);
}

void MI_CALL OsConfigResource_DeleteInstance(
    OsConfigResource_Self* self,
    MI_Context* context,
    const MI_Char* nameSpace,
    const MI_Char* className,
    const OsConfigResource* instanceName)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(nameSpace);
    MI_UNREFERENCED_PARAMETER(className);
    MI_UNREFERENCED_PARAMETER(instanceName);

    LogInfo(context, GetLog(), "[OsConfigResource] DeleteInstance");

    MI_Context_PostResult(context, MI_RESULT_NOT_SUPPORTED);
}

static MI_Result GetReportedObjectValueFromDevice(const char* who, MI_Context* context)
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
        if (MPI_OK == (mpiResult = CallMpiGet(g_componentName, g_reportedObjectName, &objectValue, &objectValueLength, GetLog())))
        {
            if (NULL == objectValue)
            {
                mpiResult = ENODATA;
                miResult = MI_RESULT_FAILED;
                LogError(context, miResult, GetLog(), "[%s] CallMpiGet for '%s' and '%s' returned no payload ('%s', %d) (%d)", 
                    who, g_componentName, g_reportedObjectName, objectValue, objectValueLength, mpiResult);
            }
            else
            {
                LogInfo(context, GetLog(), "[%s] CallMpiGet for '%s' and '%s' returned '%s' (%d long)",
                    who, g_componentName, g_reportedObjectName, objectValue, objectValueLength);
                
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

                LogInfo(context, GetLog(), "[%s] ReportedObjectValue value: '%s'", who, g_reportedObjectValue);
                CallMpiFree(objectValue);
            }
        }
    }

    g_reportedMpiResult = mpiResult;

    FREE_MEMORY(payloadString);

    return miResult;
}

static char* GetReasonFromLog(const char* commandTemplate, char separator, const char* keyName, void* log)
{
    char* command = NULL;
    char* textResult = NULL;
    size_t commandLength = 0;
    int status = ENOENT;

    if ((NULL == commandTemplate) || (NULL == keyName))
    {
        OsConfigLogError(log, "GetReasonFromLog called with invalid argument");
        return NULL;
    }

    commandLength = strlen(commandTemplate) + strlen(keyName) + 1;

    if (NULL == (command = (char*)malloc(commandLength)))
    {
        OsConfigLogError(log, "GetReasonFromLog: out of memory");
        return NULL;
    }

    memset(command, 0, commandLength);
    snprintf(command, commandLength, commandTemplate, keyName);

    status = ExecuteCommand(NULL, command, false, false, 0, 0, &textResult, NULL, log);

    FREE_MEMORY(command);

    if (NULL != textResult)
    {
        RemovePrefixUpTo(textResult, separator);
        RemovePrefixBlanks(textResult);
        TruncateAtFirst(textResult, '\n');
    }

    return textResult;
}

struct OsConfigResourceParameters
{
    const char* name;
    int miType;
    const char* stringValue;
    const int integerValue;
};

void MI_CALL OsConfigResource_Invoke_GetTargetResource(
    OsConfigResource_Self* self,
    MI_Context* context,
    const MI_Char* nameSpace,
    const MI_Char* className,
    const MI_Char* methodName,
    const OsConfigResource* instanceName,
    const OsConfigResource_GetTargetResource* in)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(instanceName);

    const char* passCode = "PASS";
    const char* failCode = "FAIL";
    const char* auditPassed = "Audit passed";
    const char* auditFailed = "Audit failed. See /var/log/osconfig*";
    const char* reasonPhraseTemplate = "cat /var/log/osconfig* | grep %s@";
    char reasonPhraseSeparator = '@';

    MI_Result miResult = MI_RESULT_OK;
    MI_Instance* resultResourceObject = NULL;
    MI_Instance* reasonObject = NULL;
    MI_Value miValue = {0};
    MI_Value miValueResult = {0};
    MI_Value miValueResource = {0};
    MI_Value miValueReasonResult = {0};
    MI_Boolean isCompliant = MI_FALSE;

    char* reasonCode = NULL;
    char* reasonPhrase = NULL;

    // Reported values
    struct OsConfigResourceParameters allParameters[] = {
        { "PayloadKey", MI_STRING, in->InputResource.value->PayloadKey.value, 0 },
        { "ComponentName", MI_STRING, in->InputResource.value->ComponentName.value, 0 },
        { "ReportedObjectName", MI_STRING, in->InputResource.value->ReportedObjectName.value, 0 },
        { "ReportedObjectValue", MI_STRING, g_reportedObjectValue, 0 },
        { "DesiredObjectName", MI_STRING, in->InputResource.value->DesiredObjectName.value, 0 },
        { "DesiredObjectValue", MI_STRING, g_desiredObjectValue, 0 },
        { "ReportedMpiResult", MI_UINT32, NULL, g_reportedMpiResult }
    };

    int allParametersSize = ARRAY_SIZE(allParameters);

    OsConfigResource_GetTargetResource get_result_object = {0};

    LogInfo(context, GetLog(), "[OsConfigResource.Get] Starting Get");

    if ((NULL == in) || (MI_FALSE == in->InputResource.exists) || (NULL == in->InputResource.value))
    {
        miResult = MI_RESULT_FAILED;
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] Invalid Get argument");
        goto Exit;
    }

    // Read and refresh the class key from the input resource values

    if ((MI_FALSE == in->InputResource.value->PayloadKey.exists) || (NULL == in->InputResource.value->PayloadKey.value))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] No PayloadKey");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    FREE_MEMORY(g_classKey);

    if (NULL == (g_classKey = DuplicateString(in->InputResource.value->PayloadKey.value)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] DuplicateString(%s) failed", in->InputResource.value->PayloadKey.value);
        g_classKey = DuplicateString(g_defaultValue);
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    LogInfo(context, GetLog(), "[OsConfigResource.Get] Processing key '%s'", g_classKey);

    // Read the MIM component name from the input resource values

    if ((MI_FALSE == in->InputResource.value->ComponentName.exists) && (NULL != in->InputResource.value->ComponentName.value))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] No ComponentName");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    FREE_MEMORY(g_componentName);

    if (NULL == (g_componentName = DuplicateString(in->InputResource.value->ComponentName.value)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] DuplicateString(%s) failed", in->InputResource.value->ComponentName.value);
        g_componentName = DuplicateString(g_defaultValue);
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    LogInfo(context, GetLog(), "[OsConfigResource.Get] Processing ComponentName '%s'", g_componentName);

    // Read the MIM reported object name from the input resource values

    if ((MI_FALSE == in->InputResource.value->ReportedObjectName.exists) && (NULL != in->InputResource.value->ReportedObjectName.value))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] No ReportedObjectName");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    FREE_MEMORY(g_reportedObjectName);

    if (NULL == (g_reportedObjectName = DuplicateString(in->InputResource.value->ReportedObjectName.value)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] DuplicateString(%s) failed", in->InputResource.value->ReportedObjectName.value);
        g_reportedObjectName = DuplicateString(g_defaultValue);
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    LogInfo(context, GetLog(), "[OsConfigResource.Get] Processing ReportedObjectName '%s'", g_reportedObjectName);

    if (MI_RESULT_OK != (miResult = OsConfigResource_GetTargetResource_Construct(&get_result_object, context)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] GetTargetResource_Construct failed with %d", miResult);
        goto Exit;
    }

    if (MI_RESULT_OK != (miResult = OsConfigResource_GetTargetResource_Set_MIReturn(&get_result_object, 0)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] GetTargetResource_Set_MIReturn failed with %d", miResult);
        goto Exit;
    }

    if (MI_RESULT_OK != (miResult = MI_Context_NewInstance(context, &OsConfigResource_rtti, &resultResourceObject)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] MI_Context_NewInstance failed with %d", miResult);
        goto Exit;
    }

    if (MI_RESULT_OK != (miResult = GetReportedObjectValueFromDevice("OsConfigResource.Get", context)))
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
                    LogInfo(context, GetLog(), "[OsConfigResource.Get] MI_Instance_SetElement('%s') to string value '%s' complete with miResult %d", 
                        allParameters[i].name, miValue.string, miResult);
                }
                else
                {
                    LogError(context, miResult, GetLog(), "[OsConfigResource.Get] No string value for '%s'", allParameters[i].name);
                    miResult = MI_RESULT_FAILED;
                }
                break;

            case MI_UINT32:
            default:
                miValue.uint32 = (MI_Uint32)(allParameters[i].integerValue);
                miResult = MI_Instance_SetElement(resultResourceObject, MI_T(allParameters[i].name), &miValue, MI_UINT32, 0);
                LogInfo(context, GetLog(), "[OsConfigResource.Get] MI_Instance_SetElement('%s') to integer value '%d' complete with miResult %d", 
                    allParameters[i].name, miValue.uint32, miResult);
        }

        if (MI_RESULT_OK != miResult)
        {
            LogError(context, miResult, GetLog(), "[OsConfigResource.Get] MI_Instance_SetElement('%s') failed with %d", allParameters[i].name, miResult);
        }

        memset(&miValue, 0, sizeof(miValue));
    }

    // Check if this audit is pass or fail by comparing reported to desired object values

    if ((in->InputResource.value->DesiredObjectValue.exists == MI_TRUE) && (in->InputResource.value->DesiredObjectValue.value != NULL))
    {
        if (0 == strcmp(in->InputResource.value->DesiredObjectValue.value, g_reportedObjectValue))
        {
            isCompliant = MI_TRUE;
            LogInfo(context, GetLog(), "[OsConfigResource.Get] DesiredObjectValue value '%s' matches the current local value",
                in->InputResource.value->DesiredObjectValue.value);
        }
        else
        {
            isCompliant = MI_FALSE;
            LogError(context, miResult, GetLog(), "[OsConfigResource.Get] DesiredObjectValue value '%s' does not match the current local value '%s'",
                in->InputResource.value->DesiredObjectValue.value, g_reportedObjectValue);
        }
    }
    else
    {
        isCompliant = MI_TRUE;
        LogInfo(context, GetLog(), "[OsConfigResource.Get] No DesiredString value, assuming compliance");
    }

    // Generate and report a reason for the result of this audit

    if (MI_TRUE == isCompliant)
    {
        reasonCode = DuplicateString(passCode);
        reasonPhrase = DuplicateString(auditPassed);
    }
    else
    {
        reasonCode = DuplicateString(failCode);
        
        // Search in the OSConfig logs for a trace that starts with this key name followed by reasonPhraseSeparator
        if (NULL == (reasonPhrase = GetReasonFromLog(reasonPhraseTemplate, reasonPhraseSeparator, g_classKey, GetLog())))
        {
            reasonPhrase = DuplicateString(auditFailed);
        }
    }
    
    LogInfo(context, GetLog(), "[OsConfigResource.Get] %s has reason code '%s' and reason phrase '%s'", g_reportedObjectName, reasonCode, reasonPhrase);

    if (MI_RESULT_OK != (miResult = MI_Context_NewInstance(context, &ReasonClass_rtti, &reasonObject)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] MI_Context_NewInstance for a reasons class instance failed with %d", miResult);
        goto Exit;
    }

    miValue.string = (MI_Char*)reasonCode;
    if (MI_RESULT_OK != (miResult = MI_Instance_SetElement(reasonObject, MI_T("Code"), &miValue, MI_STRING, 0)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] MI_Instance_SetElement(ReasonClass.Code) failed with %d", miResult);
        goto Exit;
    }

    miValue.string = (MI_Char*)reasonPhrase;
    if (MI_RESULT_OK != (miResult = MI_Instance_SetElement(reasonObject, MI_T("Phrase"), &miValue, MI_STRING, 0)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] MI_Instance_SetElement(ReasonClass.Phrase) failed with %d", miResult);
        goto Exit;
    }

    miValueReasonResult.instancea.size = 1;
    miValueReasonResult.instancea.data = &reasonObject;
    if (MI_RESULT_OK != (miResult = MI_Instance_SetElement(resultResourceObject, MI_T("Reasons"), &miValueReasonResult, MI_INSTANCEA, 0)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] MI_Instance_SetElement(reason code '%s', phrase '%s') failed with %d", reasonCode, reasonPhrase, miResult);
        goto Exit;
    }
    
    // Set the created output resource instance as the output resource in the GetTargetResource instance
    if (MI_RESULT_OK != (miResult = MI_Instance_SetElement(&get_result_object.__instance, MI_T("OutputResource"), &miValueResource, MI_INSTANCE, 0)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] MI_Instance_SetElement(OutputResource) failed with %d", miResult);
        goto Exit;
    }

    // Post the GetTargetResource instance
    if (MI_RESULT_OK != (miResult = OsConfigResource_GetTargetResource_Post(&get_result_object, context)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] OsConfigResource_GetTargetResource_Post failed with %d", miResult);
        goto Exit;
    }

Exit:
    FREE_MEMORY(reasonPhrase);
    FREE_MEMORY(reasonCode);
    
    // Clean up the reasons class instance
    if ((NULL != reasonObject) && (MI_RESULT_OK != (miResult = MI_Instance_Delete(reasonObject))))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] MI_Instance_Delete(reasonObject) failed");
    }

    // Clean up the Result MI value instance if needed
    if ((NULL != miValueResult.instance) && (MI_RESULT_OK != (miResult = MI_Instance_Delete(miValueResult.instance))))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] MI_Instance_Delete(miValueResult) failed");
    }

    // Clean up the output resource instance
    if ((NULL != resultResourceObject) && (MI_RESULT_OK != (miResult = MI_Instance_Delete(resultResourceObject))))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] MI_Instance_Delete(resultResourceObject) failed");
    }

    // Clean up the GetTargetResource instance
    if (MI_RESULT_OK != (miResult = OsConfigResource_GetTargetResource_Destruct(&get_result_object)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] GetTargetResource_Destruct failed with %d", miResult);
    }

    // Post MI result back to MI to finish
    LogInfo(context, GetLog(), "[OsConfigResource.Get] Get complete with miResult %d", miResult);
    MI_Context_PostResult(context, miResult);
}

void MI_CALL OsConfigResource_Invoke_TestTargetResource(
    OsConfigResource_Self* self,
    MI_Context* context,
    const MI_Char* nameSpace,
    const MI_Char* className,
    const MI_Char* methodName,
    const OsConfigResource* instanceName,
    const OsConfigResource_TestTargetResource* in)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(instanceName);

    OsConfigResource_TestTargetResource test_result_object = {0};

    MI_Result miResult = MI_RESULT_OK;
    MI_Boolean isCompliant = MI_FALSE;

    MI_Value miValueResult;
    memset(&miValueResult, 0, sizeof(MI_Value));

    LogInfo(context, GetLog(), "[OsConfigResource.Test] Starting Test");

    if ((in == NULL) || (in->InputResource.exists == MI_FALSE) || (in->InputResource.value == NULL))
    {
        miResult = MI_RESULT_FAILED;
        LogError(context, miResult, GetLog(), "[OsConfigResource.Test] Invalid Test argument");
        goto Exit;
    }

    // Read and refresh the class key from the input resource values

    if ((MI_FALSE == in->InputResource.value->PayloadKey.exists) || (NULL == in->InputResource.value->PayloadKey.value))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Test] No PayloadKey");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    FREE_MEMORY(g_classKey);

    if (NULL == (g_classKey = DuplicateString(in->InputResource.value->PayloadKey.value)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Test] DuplicateString(%s) failed", in->InputResource.value->PayloadKey.value);
        g_classKey = DuplicateString(g_defaultValue);
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    LogInfo(context, GetLog(), "[OsConfigResource.Test] Processing key '%s'", g_classKey);

    // Read the MIM component name from the input resource values

    if ((MI_FALSE == in->InputResource.value->ComponentName.exists) && (NULL != in->InputResource.value->ComponentName.value))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Test] No ComponentName");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    FREE_MEMORY(g_componentName);

    if (NULL == (g_componentName = DuplicateString(in->InputResource.value->ComponentName.value)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Test] DuplicateString(%s) failed", in->InputResource.value->ComponentName.value);
        g_componentName = DuplicateString(g_defaultValue);
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    LogInfo(context, GetLog(), "[OsConfigResource.Test] Processing ComponentName '%s'", g_componentName);

    // Read the MIM reported object name from the input resource values

    if ((MI_FALSE == in->InputResource.value->ReportedObjectName.exists) && (NULL != in->InputResource.value->ReportedObjectName.value))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Test] No ReportedObjectName");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    FREE_MEMORY(g_reportedObjectName);

    if (NULL == (g_reportedObjectName = DuplicateString(in->InputResource.value->ReportedObjectName.value)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Test] DuplicateString(%s) failed", in->InputResource.value->ReportedObjectName.value);
        g_reportedObjectName = DuplicateString(g_defaultValue);
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    LogInfo(context, GetLog(), "[OsConfigResource.Test] Processing ReportedObjectName '%s'", g_reportedObjectName);

    if (MI_RESULT_OK != (miResult = GetReportedObjectValueFromDevice("OsConfigResource.Test", context)))
    {
        goto Exit;
    }

    if ((in->InputResource.value->DesiredObjectValue.exists == MI_TRUE) && (in->InputResource.value->DesiredObjectValue.value != NULL))
    {
        if (0 == strcmp(in->InputResource.value->DesiredObjectValue.value, g_reportedObjectValue))
        {
            isCompliant = MI_TRUE;
            LogInfo(context, GetLog(), "[OsConfigResource.Test] DesiredObjectValue value '%s' matches the current local value",
                in->InputResource.value->DesiredObjectValue.value);
        }
        else
        {
            isCompliant = MI_FALSE;
            LogError(context, miResult, GetLog(), "[OsConfigResource.Test] DesiredObjectValue value '%s' does not match the current local value '%s'",
                in->InputResource.value->DesiredObjectValue.value, g_reportedObjectValue);
        }
    }
    else
    {
        isCompliant = MI_TRUE;
        LogInfo(context, GetLog(), "[OsConfigResource.Test] No DesiredString value, assuming compliance");
    }

    if (MI_RESULT_OK != (miResult = OsConfigResource_TestTargetResource_Construct(&test_result_object, context)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Test] TestTargetResource_Construct failed with %d", miResult);
        goto Exit;
    }

    if (MI_RESULT_OK != (miResult = OsConfigResource_TestTargetResource_Set_MIReturn(&test_result_object, 0)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Test] TestTargetResource_Set_MIReturn failed with %d", miResult);
        goto Exit;
    }

    OsConfigResource_TestTargetResource_Set_Result(&test_result_object, isCompliant);
    MI_Context_PostInstance(context, &(test_result_object.__instance));

Exit:
    if ((NULL != miValueResult.instance) && (MI_RESULT_OK != (miResult = MI_Instance_Delete(miValueResult.instance))))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Test] MI_Instance_Delete failed");
    }

    if (MI_RESULT_OK != (miResult = OsConfigResource_TestTargetResource_Destruct(&test_result_object)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Test] TestTargetResource_Destruct failed");
    }

    LogInfo(context, GetLog(), "[OsConfigResource.Test] Test complete with miResult %d", miResult);
    MI_Context_PostResult(context, miResult);

}

void MI_CALL OsConfigResource_Invoke_SetTargetResource(
    OsConfigResource_Self* self,
    MI_Context* context,
    const MI_Char* nameSpace,
    const MI_Char* className,
    const MI_Char* methodName,
    const OsConfigResource* instanceName,
    const OsConfigResource_SetTargetResource* in)
{
    MI_UNREFERENCED_PARAMETER(self);
    MI_UNREFERENCED_PARAMETER(instanceName);

    MI_Result miResult = MI_RESULT_OK;
    int mpiResult = MPI_OK;

    const char payloadTemplate[] = "\"%s\"";
    char* payloadString = NULL;
    int payloadSize = 0;

    OsConfigResource_SetTargetResource set_result_object = {0};

    LogInfo(context, GetLog(), "[OsConfigResource.Set] Starting Set");

    if ((NULL == in) || (MI_FALSE == in->InputResource.exists) || (NULL == in->InputResource.value))
    {
        miResult = MI_RESULT_FAILED;
        LogError(context, miResult, GetLog(), "[OsConfigResource.Set] Invalid argument");
        goto Exit;
    }

    if (MI_RESULT_OK != (miResult =OsConfigResource_SetTargetResource_Construct(&set_result_object, context)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Set] SetTargetResource_Construct failed with %d", miResult);
        goto Exit;
    }

    if (MI_RESULT_OK != (miResult =OsConfigResource_SetTargetResource_Set_MIReturn(&set_result_object, 0)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Set] SetTargetResource_Set_MIReturn failed with %d", miResult);
        goto Exit;
    }

    MI_Context_PostInstance(context, &(set_result_object.__instance));

    // Read and refresh the class key from the input resource values

    if ((MI_FALSE == in->InputResource.value->PayloadKey.exists) || (NULL == in->InputResource.value->PayloadKey.value))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Set] No PayloadKey");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    FREE_MEMORY(g_classKey);

    if (NULL == (g_classKey = DuplicateString(in->InputResource.value->PayloadKey.value)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Set] DuplicateString(%s) failed", in->InputResource.value->PayloadKey.value);
        g_classKey = DuplicateString(g_defaultValue);
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    LogInfo(context, GetLog(), "[OsConfigResource.Set] Processing key '%s'", g_classKey);

    // Read the MIM component name from the input resource values

    if ((MI_FALSE == in->InputResource.value->ComponentName.exists) && (NULL != in->InputResource.value->ComponentName.value))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Set] No ComponentName");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    FREE_MEMORY(g_componentName);

    if (NULL == (g_componentName = DuplicateString(in->InputResource.value->ComponentName.value)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Set] DuplicateString(%s) failed", in->InputResource.value->ComponentName.value);
        g_componentName = DuplicateString(g_defaultValue);
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    LogInfo(context, GetLog(), "[OsConfigResource.Set] Processing ComponentName '%s'", g_componentName);

    // Read the MIM desired object name from the input resource values

    if ((MI_FALSE == in->InputResource.value->DesiredObjectName.exists) && (NULL != in->InputResource.value->DesiredObjectName.value))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Set] No DesiredObjectName");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    FREE_MEMORY(g_desiredObjectName);

    if (NULL == (g_desiredObjectName = DuplicateString(in->InputResource.value->DesiredObjectName.value)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Set] DuplicateString(%s) failed", in->InputResource.value->DesiredObjectName.value);
        g_desiredObjectName = DuplicateString(g_defaultValue);
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    LogInfo(context, GetLog(), "[OsConfigResource.Set] Processing DesiredObjectName '%s'", g_desiredObjectName);

    // Read the MIM desired object value from the input resource values

    if ((MI_FALSE == in->InputResource.value->DesiredObjectValue.exists) && (NULL != in->InputResource.value->DesiredObjectValue.value))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Set] No DesiredObjectValue");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    FREE_MEMORY(g_desiredObjectValue);

    if (NULL == (g_desiredObjectValue = DuplicateString(in->InputResource.value->DesiredObjectValue.value)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Set] DuplicateString(%s) failed", in->InputResource.value->DesiredObjectValue.value);
        g_desiredObjectValue = DuplicateString(g_failValue);
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    LogInfo(context, GetLog(), "[OsConfigResource.Set] Processing DesiredObjectValue '%s'", g_desiredObjectValue);

    if (NULL == g_mpiHandle)
    {
        if (!RefreshMpiClientSession())
        {
            miResult = MI_RESULT_FAILED;
            mpiResult = ESRCH;
            LogError(context, miResult, GetLog(), "[OsConfigResource.Set] Failed starting the MPI server (%d)", mpiResult);
        }
    }

    if (g_mpiHandle)
    {
        payloadSize = (int)strlen(g_desiredObjectValue) + 2;
        if (NULL != (payloadString = malloc(payloadSize + 1)))
        {
            memset(payloadString, 0, payloadSize + 1);
            snprintf(payloadString, payloadSize + 1, payloadTemplate, g_desiredObjectValue);

            if (MPI_OK == (mpiResult = CallMpiSet(g_componentName, g_desiredObjectName, payloadString, payloadSize, GetLog())))
            {
                LogInfo(context, GetLog(), "[OsConfigResource.Set] DesiredString value '%s' successfully applied to device as '%.*s', %d bytes",
                    g_desiredObjectValue, payloadSize, payloadString, payloadSize);
            }
            else
            {
                miResult = MI_RESULT_FAILED;
                LogError(context, miResult, GetLog(), "[OsConfigResource.Set] Failed to apply DesiredString value '%s' to device as '%.*s' (%d bytes), miResult %d",
                    g_desiredObjectValue, payloadSize, payloadString, payloadSize, miResult);
            }

            FREE_MEMORY(payloadString);
        }
        else
        {
            miResult = MI_RESULT_FAILED;
            mpiResult = ENOMEM;
            LogError(context, miResult, GetLog(), "[OsConfigResource.Set] Failed to allocate %d bytes", payloadSize);
        }

        if (MPI_OK != mpiResult)
        {
            g_reportedMpiResult = mpiResult;
        }
    }

    // Set results to report back
    g_reportedMpiResult = 0;

Exit:
    if (MI_RESULT_OK != miResult)
    {
        g_reportedMpiResult = miResult;
    }

    if (MI_RESULT_OK != (miResult =OsConfigResource_SetTargetResource_Destruct(&set_result_object)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Set] SetTargetResource_Destruct failed");
    }

    LogInfo(context, GetLog(), "[OsConfigResource.Set] Set complete with miResult %d", miResult);
    MI_Context_PostResult(context, miResult);
}
