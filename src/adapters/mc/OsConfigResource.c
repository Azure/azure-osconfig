// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Common.h"

// Fallback for SSH policy
#include <SshUtils.h>

// The log file for the NRP
#define LOG_FILE "/var/log/osconfig_nrp.log"
#define ROLLED_LOG_FILE "/var/log/osconfig_nrp.bak"

#define MAX_PAYLOAD_LENGTH 0

#define MPI_CLIENT_NAME "OSConfig Universal NRP"

static const char* g_defaultValue = "-";
static const char* g_passValue = SECURITY_AUDIT_PASS;
static const char* g_failValue = SECURITY_AUDIT_FAIL;

// Desired (write; also reported together with read group)
static char* g_classKey = NULL;
static char* g_componentName = NULL;
static char* g_reportedObjectName = NULL;
static char* g_expectedObjectValue = NULL;
static char* g_desiredObjectName = NULL;
static char* g_desiredObjectValue = NULL;

//Reported (read)
static char* g_reportedObjectValue = NULL;
static unsigned int g_reportedMpiResult = 0;

MPI_HANDLE g_mpiHandle = NULL;

static OSCONFIG_LOG_HANDLE g_log = NULL;

const char* g_mpiServer = "osconfig-platform";

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

    if (g_mpiHandle && IsDaemonActive(g_mpiServer, GetLog()))
    {
        return status;
    }

    if (true == (status = EnableAndStartDaemon(g_mpiServer, GetLog())))
    {
        sleep(1);

        if (NULL == (g_mpiHandle = CallMpiOpen(MPI_CLIENT_NAME, MAX_PAYLOAD_LENGTH, GetLog())))
        {
            OsConfigLogError(GetLog(), "[OsConfigResource] MpiOpen failed");
            status = false;
        }
    }
    else
    {
        OsConfigLogError(GetLog(), "[OsConfigResource] The OSConfig Platform service '%s' is not active on this device", g_mpiServer);
    }

    return status;
}

void __attribute__((constructor)) Initialize()
{
    RefreshMpiClientSession();

    g_classKey = DuplicateString(g_defaultValue);
    g_componentName = DuplicateString(g_defaultValue);
    g_reportedObjectName = DuplicateString(g_defaultValue);
    g_expectedObjectValue = DuplicateString(g_passValue);
    g_desiredObjectName = DuplicateString(g_defaultValue);
    g_desiredObjectValue = DuplicateString(g_failValue);

    // Fallback for SSH policy
    InitializeSshAudit(GetLog());

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

    // Fallback for SSH policy
    SshAuditCleanup(GetLog());
    RestartDaemon(g_mpiServer, NULL);

    CloseLog(&g_log);

    FREE_MEMORY(g_classKey);
    FREE_MEMORY(g_componentName);
    FREE_MEMORY(g_reportedObjectName);
    FREE_MEMORY(g_expectedObjectValue);
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
        RefreshMpiClientSession();
    }

    if (NULL != g_mpiHandle)
    {
        if (MPI_OK == (mpiResult = CallMpiGet(g_componentName, g_reportedObjectName, &objectValue, &objectValueLength, GetLog())))
        {
            if (NULL == objectValue)
            {
                mpiResult = ENODATA;
                miResult = MI_RESULT_FAILED;
                LogError(context, miResult, GetLog(), "[%s] CallMpiGet(%s, %s): no payload (%s, %d) (%d)", 
                    who, g_componentName, g_reportedObjectName, objectValue, objectValueLength, mpiResult);
            }
            else
            {
                LogInfo(context, GetLog(), "[%s] CallMpiGet(%s, %s): '%s' (%d)", who, g_componentName, g_reportedObjectName, objectValue, objectValueLength);
                
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
                            if (NULL == (g_reportedObjectValue = DuplicateString(jsonString)))
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

                CallMpiFree(objectValue);
            }
        }
        else
        {
            miResult = MI_RESULT_FAILED;
            LogError(context, miResult, GetLog(), "[%s] CallMpiGet(%s, %s) failed with %d", who, g_componentName, g_reportedObjectName, mpiResult);
        }
    }
    else
    {
        // Fallback for SSH policy
        if (0 == (mpiResult = ProcessSshAuditCheck(g_reportedObjectName, NULL, &objectValue, GetLog())))
        {
            if (NULL == objectValue)
            {
                mpiResult = ENODATA;
                miResult = MI_RESULT_FAILED;
                LogError(context, miResult, GetLog(), "[%s] ProcessSshAuditCheck(%s): no payload (%s, %d) (%d)",
                    who, g_reportedObjectName, objectValue, objectValueLength, mpiResult);
            }
            else
            {
                LogInfo(context, GetLog(), "[%s] ProcessSshAuditCheck(%s): '%s'", who, g_reportedObjectName, objectValue);

                FREE_MEMORY(g_reportedObjectValue);
                if (NULL == (g_reportedObjectValue = DuplicateString(objectValue)))
                {
                    mpiResult = ENOMEM;
                    miResult = MI_RESULT_FAILED;
                    LogError(context, miResult, GetLog(), "[%s] DuplicateString(%s) failed", who, objectValue);
                }

                FREE_MEMORY(objectValue);
            }
        }
        else
        {
            miResult = MI_RESULT_FAILED;
            LogError(context, miResult, GetLog(), "[%s] ProcessSshAuditCheck(%s) failed with %d", who, g_reportedObjectName, mpiResult);
        }
    }

    g_reportedMpiResult = mpiResult;

    return miResult;
}

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

    const char* auditPassed = "Audit passed";
    const char* auditFailed = "Audit failed. See /var/log/osconfig*";

    char* reasonCode = NULL;
    char* reasonPhrase = NULL;

    MI_Result miResult = MI_RESULT_OK;
    MI_Instance* resultResourceObject = NULL;
    MI_Instance* reasonObject = NULL;
    MI_Value miValue = {0};
    MI_Value miValueResult = {0};
    MI_Value miValueResource = {0};
    MI_Value miValueReasonResult = {0};
    MI_Boolean isCompliant = MI_FALSE;

    OsConfigResource_GetTargetResource get_result_object = {0};

    if ((NULL == in) || (MI_FALSE == in->InputResource.exists) || (NULL == in->InputResource.value))
    {
        miResult = MI_RESULT_FAILED;
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] Invalid Get argument");
        goto Exit;
    }

    // Read the class key from the input resource values
    if ((MI_TRUE == in->InputResource.value->PayloadKey.exists) && (NULL != in->InputResource.value->PayloadKey.value))
    {
        FREE_MEMORY(g_classKey);
        if (NULL == (g_classKey = DuplicateString(in->InputResource.value->PayloadKey.value)))
        {
            LogError(context, miResult, GetLog(), "[OsConfigResource.Get] DuplicateString(%s) failed", in->InputResource.value->PayloadKey.value);
            g_classKey = DuplicateString(g_defaultValue);
            miResult = MI_RESULT_FAILED;
            goto Exit;
        }
    }
    else
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] No PayloadKey");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    // Read the MIM component name from the input resource values
    if ((MI_TRUE == in->InputResource.value->ComponentName.exists) && (NULL != in->InputResource.value->ComponentName.value))
    {
        FREE_MEMORY(g_componentName);
        if (NULL == (g_componentName = DuplicateString(in->InputResource.value->ComponentName.value)))
        {
            LogError(context, miResult, GetLog(), "[OsConfigResource.Get] DuplicateString(%s) failed", in->InputResource.value->ComponentName.value);
            g_componentName = DuplicateString(g_defaultValue);
            miResult = MI_RESULT_FAILED;
            goto Exit;
        }
    }
    else
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] No ComponentName");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    // Read the MIM reported object name from the input resource values
    if ((MI_TRUE == in->InputResource.value->ReportedObjectName.exists) && (NULL != in->InputResource.value->ReportedObjectName.value))
    {
        FREE_MEMORY(g_reportedObjectName);
        if (NULL == (g_reportedObjectName = DuplicateString(in->InputResource.value->ReportedObjectName.value)))
        {
            LogError(context, miResult, GetLog(), "[OsConfigResource.Get] DuplicateString(%s) failed", in->InputResource.value->ReportedObjectName.value);
            g_reportedObjectName = DuplicateString(g_defaultValue);
            miResult = MI_RESULT_FAILED;
            goto Exit;
        }
    }
    else
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] No ReportedObjectName");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    // Read the MIM desired object name from the input resource values
    if ((MI_TRUE == in->InputResource.value->DesiredObjectName.exists) && (NULL != in->InputResource.value->DesiredObjectName.value))
    {
        FREE_MEMORY(g_desiredObjectName);
        if (NULL == (g_desiredObjectName = DuplicateString(in->InputResource.value->DesiredObjectName.value)))
        {
            LogError(context, miResult, GetLog(), "[OsConfigResource.Get] DuplicateString(%s) failed", in->InputResource.value->DesiredObjectName.value);
            g_desiredObjectName = DuplicateString(g_defaultValue);
        }
    }
    else
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] No DesiredObjectName");
    }

    // Read the reported MIM object value from the local device
    if (MI_RESULT_OK != (miResult = GetReportedObjectValueFromDevice("OsConfigResource.Get", context)))
    {
        goto Exit;
    }

    // Read the desired MIM object value from the input resource values
    if ((in->InputResource.value->DesiredObjectValue.exists == MI_TRUE) && (in->InputResource.value->DesiredObjectValue.value != NULL))
    {
        FREE_MEMORY(g_desiredObjectValue);
        if (NULL == (g_desiredObjectValue = DuplicateString(in->InputResource.value->DesiredObjectValue.value)))
        {
            LogError(context, miResult, GetLog(), "[OsConfigResource.Get] DuplicateString(%s) failed", in->InputResource.value->DesiredObjectValue.value);
            g_desiredObjectValue = DuplicateString(g_failValue);
        }
    }

    // Read the expected MIM object value from the input resource values, we'll use this to determine compliance
    if ((in->InputResource.value->ExpectedObjectValue.exists == MI_TRUE) && (in->InputResource.value->ExpectedObjectValue.value != NULL))
    {
        FREE_MEMORY(g_expectedObjectValue);
        if (NULL == (g_expectedObjectValue = DuplicateString(in->InputResource.value->ExpectedObjectValue.value)))
        {
            LogError(context, miResult, GetLog(), "[OsConfigResource.Get] DuplicateString(%s) failed", in->InputResource.value->ExpectedObjectValue.value);
            g_expectedObjectValue = DuplicateString(g_passValue);
        }

        isCompliant = (g_expectedObjectValue && (0 == strncmp(g_expectedObjectValue, g_reportedObjectValue, strlen(g_expectedObjectValue)))) ? MI_TRUE : MI_FALSE;
    }
    else
    {
        isCompliant = MI_TRUE;
        LogInfo(context, GetLog(), "[OsConfigResource.Get] %s: no ExpectedObjectValue, assuming compliance", g_classKey);
    }

    // Create the output resource

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

    miValueResource.instance = resultResourceObject;

    // Write the payload key to the output resource values
    miValue.string = (MI_Char*)(g_classKey);
    if (MI_RESULT_OK != (miResult = MI_Instance_SetElement(resultResourceObject, MI_T("PayloadKey"), &miValue, MI_STRING, 0)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] MI_Instance_SetElement(PayloadKey) to string value '%s' failed with miResult %d", miValue.string, miResult);
        goto Exit;
    }

    // Write the MIM component name to the output resource values
    memset(&miValue, 0, sizeof(miValue));
    miValue.string = (MI_Char*)(g_componentName);
    if (MI_RESULT_OK != (miResult = MI_Instance_SetElement(resultResourceObject, MI_T("ComponentName"), &miValue, MI_STRING, 0)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] MI_Instance_SetElement(ComponentName) to string value '%s' failed with miResult %d", miValue.string, miResult);
        goto Exit;
    }

    // Write the reported MIM object name to the output resource values
    memset(&miValue, 0, sizeof(miValue));
    miValue.string = (MI_Char*)(g_reportedObjectName);
    if (MI_RESULT_OK != (miResult = MI_Instance_SetElement(resultResourceObject, MI_T("ReportedObjectName"), &miValue, MI_STRING, 0)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] MI_Instance_SetElement(ReportedObjectName) to string value '%s' failed with miResult %d", miValue.string, miResult);
        goto Exit;
    }

    // Write the reported MIM object value read from local device to the output resource values
    memset(&miValue, 0, sizeof(miValue));
    miValue.string = (MI_Char*)(g_reportedObjectValue);
    if (MI_RESULT_OK != (miResult = MI_Instance_SetElement(resultResourceObject, MI_T("ReportedObjectValue"), &miValue, MI_STRING, 0)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] MI_Instance_SetElement(ReportedObjectValue) to string value '%s' failed with miResult %d", miValue.string, miResult);
        goto Exit;
    }

    // Write the expected MIM object value to the output resource values if present in input resource values
    if ((in->InputResource.value->ExpectedObjectValue.exists == MI_TRUE) && (NULL != in->InputResource.value->ExpectedObjectValue.value))
    {
        memset(&miValue, 0, sizeof(miValue));
        miValue.string = (MI_Char*)(g_expectedObjectValue);
        if (MI_RESULT_OK != (miResult = MI_Instance_SetElement(resultResourceObject, MI_T("ExpectedObjectValue"), &miValue, MI_STRING, 0)))
        {
            LogError(context, miResult, GetLog(), "[OsConfigResource.Get] MI_Instance_SetElement(ExpectedObjectValue) to string value '%s' failed with miResult %d", miValue.string, miResult);
            goto Exit;
        }
    }

    // Write the desired MIM object name to the output resource values if present in input resource values
    if ((MI_TRUE == in->InputResource.value->DesiredObjectName.exists) && (NULL != in->InputResource.value->DesiredObjectName.value))
    {
        memset(&miValue, 0, sizeof(miValue));
        miValue.string = (MI_Char*)(g_desiredObjectName);
        if (MI_RESULT_OK != (miResult = MI_Instance_SetElement(resultResourceObject, MI_T("DesiredObjectName"), &miValue, MI_STRING, 0)))
        {
            LogError(context, miResult, GetLog(), "[OsConfigResource.Get] MI_Instance_SetElement(DesiredObjectName) to string value '%s' failed with miResult %d", miValue.string, miResult);
            goto Exit;
        }
    }

    // Write the desired MIM object value to the output resource values if present in input resource values
    if ((in->InputResource.value->DesiredObjectValue.exists == MI_TRUE) && (NULL != in->InputResource.value->DesiredObjectValue.value))
    {
        memset(&miValue, 0, sizeof(miValue));
        miValue.string = (MI_Char*)(g_desiredObjectValue);
        if (MI_RESULT_OK != (miResult = MI_Instance_SetElement(resultResourceObject, MI_T("DesiredObjectValue"), &miValue, MI_STRING, 0)))
        {
            LogError(context, miResult, GetLog(), "[OsConfigResource.Get] MI_Instance_SetElement(DesiredObjectValue) to string value '%s' failed with miResult %d", miValue.string, miResult);
            goto Exit;
        }
    }

    // Write the MPI result for the MpiGet that returned the reported MIM object value to the output resource values
    memset(&miValue, 0, sizeof(miValue));
    miValue.uint32 = (MI_Uint32)(g_reportedMpiResult);
    if (MI_RESULT_OK != (miResult = MI_Instance_SetElement(resultResourceObject, MI_T("ReportedMpiResult"), &miValue, MI_UINT32, 0)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] MI_Instance_SetElement(ReportedMpiResult) to integer value '%d' failed with miResult %d", miValue.uint32, miResult);
        goto Exit;
    }

    // Generate and report the reason for the result of this audit to the output resource values

    if (MI_TRUE == isCompliant)
    {
        reasonCode = DuplicateString(g_expectedObjectValue);
        if ((0 == strcmp(g_reportedObjectValue, g_expectedObjectValue)) ||
            ((strlen(g_reportedObjectValue) > strlen(g_expectedObjectValue)) && (NULL == (reasonPhrase = DuplicateString(g_reportedObjectValue + strlen(g_expectedObjectValue))))))
        {
            reasonPhrase = DuplicateString(auditPassed);
        }
    }
    else
    {
        reasonCode = DuplicateString(g_failValue);
        if ((0 == strcmp(g_reportedObjectValue, g_failValue)) || (NULL == (reasonPhrase = DuplicateString(g_reportedObjectValue))))
        {
            reasonPhrase = DuplicateString(auditFailed);
        }
    }
    
    LogInfo(context, GetLog(), "[OsConfigResource.Get] %s: '%s', '%s'", g_reportedObjectName, reasonCode, reasonPhrase);

    if (reasonCode && reasonPhrase)
    {
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
    }
    else
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] Failed reporting a reason code and phrase due to low memory");
        miResult = MI_RESULT_FAILED;
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
    FREE_MEMORY(reasonCode);
    FREE_MEMORY(reasonPhrase);
    
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
    
    if (MI_RESULT_OK != miResult)
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Get] Get complete with miResult %d", miResult);
    }
    
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

    if ((in == NULL) || (in->InputResource.exists == MI_FALSE) || (in->InputResource.value == NULL))
    {
        miResult = MI_RESULT_FAILED;
        LogError(context, miResult, GetLog(), "[OsConfigResource.Test] Invalid Test argument");
        goto Exit;
    }

    // Read the class key from the input resource values
    if ((MI_TRUE == in->InputResource.value->PayloadKey.exists) && (NULL != in->InputResource.value->PayloadKey.value))
    {
        FREE_MEMORY(g_classKey);
        if (NULL == (g_classKey = DuplicateString(in->InputResource.value->PayloadKey.value)))
        {
            LogError(context, miResult, GetLog(), "[OsConfigResource.Test] DuplicateString(%s) failed", in->InputResource.value->PayloadKey.value);
            g_classKey = DuplicateString(g_defaultValue);
            miResult = MI_RESULT_FAILED;
            goto Exit;
        }
    }
    else
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Test] No PayloadKey");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    // Read the MIM component name from the input resource values
    if ((MI_TRUE == in->InputResource.value->ComponentName.exists) && (NULL != in->InputResource.value->ComponentName.value))
    {
        FREE_MEMORY(g_componentName);
        if (NULL == (g_componentName = DuplicateString(in->InputResource.value->ComponentName.value)))
        {
            LogError(context, miResult, GetLog(), "[OsConfigResource.Test] DuplicateString(%s) failed", in->InputResource.value->ComponentName.value);
            g_componentName = DuplicateString(g_defaultValue);
            miResult = MI_RESULT_FAILED;
            goto Exit;
        }
    }
    else
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Test] No ComponentName");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    // Read the MIM reported object name from the input resource values
    if ((MI_TRUE == in->InputResource.value->ReportedObjectName.exists) && (NULL != in->InputResource.value->ReportedObjectName.value))
    {
        FREE_MEMORY(g_reportedObjectName);
        if (NULL == (g_reportedObjectName = DuplicateString(in->InputResource.value->ReportedObjectName.value)))
        {
            LogError(context, miResult, GetLog(), "[OsConfigResource.Test] DuplicateString(%s) failed", in->InputResource.value->ReportedObjectName.value);
            g_reportedObjectName = DuplicateString(g_defaultValue);
            miResult = MI_RESULT_FAILED;
            goto Exit;
        }
    }
    else
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Test] No ReportedObjectName");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    // Read the reported MIM object value from the local device
    if (MI_RESULT_OK != (miResult = GetReportedObjectValueFromDevice("OsConfigResource.Test", context)))
    {
        goto Exit;
    }

    // Determine compliance
    if ((in->InputResource.value->ExpectedObjectValue.exists == MI_TRUE) && (in->InputResource.value->ExpectedObjectValue.value != NULL))
    {
        isCompliant = (g_reportedObjectValue && (0 == strncmp(in->InputResource.value->ExpectedObjectValue.value, g_reportedObjectValue, strlen(in->InputResource.value->ExpectedObjectValue.value)))) ? MI_TRUE : MI_FALSE;
        LogInfo(context, GetLog(), "[OsConfigResource.Test] %s: %s", g_classKey, isCompliant ? "compliant" : "incompliant");
    }
    else
    {
        isCompliant = MI_TRUE;
        LogInfo(context, GetLog(), "[OsConfigResource.Test] %s: no ExpectedObjectValue, assuming compliance", g_classKey);
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

    if (MI_RESULT_OK != miResult)
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Test] Test complete with miResult %d", miResult);
    }

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

    char* payloadString = NULL;
    int payloadSize = 0;
    
    JSON_Value* jsonValue = NULL;
    char* serializedValue = NULL;
    
    MI_Result miResult = MI_RESULT_OK;
    int mpiResult = MPI_OK;
    
    OsConfigResource_SetTargetResource set_result_object = {0};

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

    if (MI_RESULT_OK != (miResult = OsConfigResource_SetTargetResource_Set_MIReturn(&set_result_object, 0)))
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Set] SetTargetResource_Set_MIReturn failed with %d", miResult);
        goto Exit;
    }

    MI_Context_PostInstance(context, &(set_result_object.__instance));

    // Read the class key from the input resource values
    if ((MI_TRUE == in->InputResource.value->PayloadKey.exists) && (NULL != in->InputResource.value->PayloadKey.value))
    {
        FREE_MEMORY(g_classKey);
        if (NULL == (g_classKey = DuplicateString(in->InputResource.value->PayloadKey.value)))
        {
            LogError(context, miResult, GetLog(), "[OsConfigResource.Set] DuplicateString(%s) failed", in->InputResource.value->PayloadKey.value);
            g_classKey = DuplicateString(g_defaultValue);
            miResult = MI_RESULT_FAILED;
            goto Exit;
        }
    }
    else
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Set] No PayloadKey");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    // Read the MIM component name from the input resource values
    if ((MI_TRUE == in->InputResource.value->ComponentName.exists) && (NULL != in->InputResource.value->ComponentName.value))
    {
        FREE_MEMORY(g_componentName);
        if (NULL == (g_componentName = DuplicateString(in->InputResource.value->ComponentName.value)))
        {
            LogError(context, miResult, GetLog(), "[OsConfigResource.Set] DuplicateString(%s) failed", in->InputResource.value->ComponentName.value);
            g_componentName = DuplicateString(g_defaultValue);
            miResult = MI_RESULT_FAILED;
            goto Exit;
        }
    }
    else
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Set] No ComponentName");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    // Read the MIM desired object name from the input resource values
    if ((MI_TRUE == in->InputResource.value->DesiredObjectName.exists) && (NULL != in->InputResource.value->DesiredObjectName.value))
    {
        FREE_MEMORY(g_desiredObjectName);
        if (NULL == (g_desiredObjectName = DuplicateString(in->InputResource.value->DesiredObjectName.value)))
        {
            LogError(context, miResult, GetLog(), "[OsConfigResource.Set] DuplicateString(%s) failed", in->InputResource.value->DesiredObjectName.value);
            g_desiredObjectName = DuplicateString(g_defaultValue);
            miResult = MI_RESULT_FAILED;
            goto Exit;
        }
    }
    else
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Set] No DesiredObjectName");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    // Read the MIM desired object value from the input resource values
    if ((MI_TRUE == in->InputResource.value->DesiredObjectValue.exists) && (NULL != in->InputResource.value->DesiredObjectValue.value))
    {
        FREE_MEMORY(g_desiredObjectValue);
        if (NULL == (g_desiredObjectValue = DuplicateString(in->InputResource.value->DesiredObjectValue.value)))
        {
            LogError(context, miResult, GetLog(), "[OsConfigResource.Set] DuplicateString(%s) failed", in->InputResource.value->DesiredObjectValue.value);
            g_desiredObjectValue = DuplicateString(g_failValue);
            miResult = MI_RESULT_FAILED;
            goto Exit;
        }
    }
    else
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Set] No DesiredObjectValue");
        miResult = MI_RESULT_FAILED;
        goto Exit;
    }

    if (NULL == g_mpiHandle)
    {
        RefreshMpiClientSession();
    }

    if (NULL != g_mpiHandle)
    {
        if (NULL == (jsonValue = json_value_init_string(g_desiredObjectValue)))
        {
            miResult = MI_RESULT_FAILED;
            mpiResult = ENOMEM;
            LogError(context, miResult, GetLog(), "[OsConfigResource.Set] json_value_init_string('%s') failed", g_desiredObjectValue);
        }
        else if (NULL == (serializedValue = json_serialize_to_string(jsonValue)))
        {
            miResult = MI_RESULT_FAILED;
            mpiResult = ENOMEM;
            LogError(context, miResult, GetLog(), "[OsConfigResource.Set] json_serialize_to_string('%s') failed", g_desiredObjectValue);
        }
        else
        {
            payloadSize = (int)strlen(serializedValue);
            if (NULL != (payloadString = malloc(payloadSize + 1)))
            {
                memset(payloadString, 0, payloadSize + 1);
                memcpy(payloadString, serializedValue, payloadSize);

                if (MPI_OK == (mpiResult = CallMpiSet(g_componentName, g_desiredObjectName, payloadString, payloadSize, GetLog())))
                {
                    LogInfo(context, GetLog(), "[OsConfigResource.Set] CallMpiSet(%s, %s, '%.*s', %d) ok",
                        g_componentName, g_desiredObjectName, payloadSize, payloadString, payloadSize);
                }
                else
                {
                    miResult = MI_RESULT_FAILED;
                    LogError(context, miResult, GetLog(), "[OsConfigResource.Set] CallMpiSet(%s, %s, '%.*s', %d) failed with %d, miResult %d",
                        g_componentName, g_desiredObjectName, payloadSize, payloadString, payloadSize, mpiResult, miResult);
                }

                FREE_MEMORY(payloadString);
            }
            else
            {
                miResult = MI_RESULT_FAILED;
                mpiResult = ENOMEM;
                LogError(context, miResult, GetLog(), "[OsConfigResource.Set] Failed to allocate %d bytes", payloadSize);
            }
        }

        if (NULL != serializedValue)
        {
            json_free_serialized_string(serializedValue);
        }

        if (NULL != jsonValue)
        {
            json_value_free(jsonValue);
        }

        if (MPI_OK != mpiResult)
        {
            g_reportedMpiResult = mpiResult;
        }
    }
    else
    {
        // Fallback for SSH policy
        if (0 == (mpiResult = ProcessSshAuditCheck(g_desiredObjectName, g_desiredObjectValue, NULL, GetLog())))
        {
            LogInfo(context, GetLog(), "[OsConfigResource.Set] ProcessSshAuditCheck(%s, '%s') ok", g_desiredObjectName, g_desiredObjectValue);
        }
        else
        {
            miResult = MI_RESULT_FAILED;
            LogError(context, miResult, GetLog(), "[OsConfigResource.Set] ProcessSshAuditCheck(%s, '%s') failed with %d", g_desiredObjectName, g_desiredObjectValue, mpiResult);
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

    if (MI_RESULT_OK != miResult)
    {
        LogError(context, miResult, GetLog(), "[OsConfigResource.Set] Set complete with miResult %d", miResult);
    }

    MI_Context_PostResult(context, miResult);
}

