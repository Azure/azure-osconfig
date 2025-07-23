// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../Common.h"
#include "ComplianceEngineInterface.h"

static MMI_HANDLE gComplianceEngine = NULL;

static MMI_HANDLE GetHandle()
{
    static const char sComponentName[] = "ComplianceEngine";
    if(NULL == gComplianceEngine)
    {
        gComplianceEngine = ComplianceEngineMmiOpen(sComponentName, -1);
    }
    return gComplianceEngine;
}

static void FreeHandle()
{
    if(NULL == gComplianceEngine)
    {
        return;
    }

    ComplianceEngineMmiClose(gComplianceEngine);
    gComplianceEngine = NULL;
}

int BaselineIsValidResourceIdRuleId(const char* resourceId, const char* ruleId, const char* payloadKey, OsConfigLogHandle log)
{
    return ComplianceEngineValidatePayload(GetHandle(), resourceId, ruleId, payloadKey, log);
}

void BaselineInitialize(OsConfigLogHandle log)
{
    ComplianceEngineInitialize(log);
}

void BaselineShutdown(OsConfigLogHandle log)
{
    UNUSED(log);
    FreeHandle();
    ComplianceEngineShutdown();
}

int BaselineMmiGet(const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes, OsConfigLogHandle log)
{
    if ((NULL == componentName) || (NULL == objectName))
    {
        OsConfigLogError(log, "BaselineMmiGet called with invalid arguments");
        return EINVAL;
    }

    int result = ComplianceEngineMmiGet(GetHandle(), componentName, objectName, payload, payloadSizeBytes);
    if (MMI_OK != result)
    {
        OsConfigLogError(log, "BaselineMmiGet(%s, %s) failed: %d", componentName, objectName, result);
        return result;
    }

    if ((NULL != *payload) && (*payloadSizeBytes > 0) & (maxPayloadSizeBytes > 0) && ((unsigned)*payloadSizeBytes > maxPayloadSizeBytes))
    {
        OsConfigLogInfo(log, "BaselineMmiGet(%s, %s) payload truncated from %d to %u bytes", componentName, objectName, *payloadSizeBytes, maxPayloadSizeBytes);
        *payloadSizeBytes = (int)maxPayloadSizeBytes;
        *payload[*payloadSizeBytes] = '\0';
    }

    return MMI_OK;
}

int BaselineMmiSet(const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes, OsConfigLogHandle log)
{
    UNUSED(log);
    return ComplianceEngineMmiSet(GetHandle(), componentName, objectName, payload, payloadSizeBytes);
}
