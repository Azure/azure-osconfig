// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../Common.h"
#include "ComplianceEngineInterface.h"

static MMI_HANDLE gComplianceEngine = NULL;

static const char gComponentName[] = "ComplianceEngine";

int BaselineIsValidResourceIdRuleId(const char* resourceId, const char* ruleId, const char* payloadKey, OsConfigLogHandle log)
{
    UNUSED(resourceId);
    UNUSED(ruleId);
    UNUSED(payloadKey);
    UNUSED(log);
    return 0;
}

void BaselineInitialize(OsConfigLogHandle log)
{
    ComplianceEngineInitialize(log);
    gComplianceEngine = ComplianceEngineMmiOpen(gComponentName, -1);
}

void BaselineShutdown(OsConfigLogHandle log)
{
    UNUSED(log);
    if (NULL == gComplianceEngine)
    {
        return;
    }

    ComplianceEngineMmiClose(gComplianceEngine);
    ComplianceEngineShutdown();
    gComplianceEngine = NULL;
}

int BaselineMmiGet(const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes, OsConfigLogHandle log)
{
    if ((NULL == componentName) || (NULL == objectName))
    {
        OsConfigLogError(log, "BaselineMmiGet called with invalid arguments");
        return EINVAL;
    }

    int result = ComplianceEngineMmiGet(gComplianceEngine, componentName, objectName, payload, payloadSizeBytes);
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
    return ComplianceEngineMmiSet(gComplianceEngine, componentName, objectName, payload, payloadSizeBytes);
}
