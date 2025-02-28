// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../Common.h"
#include "ComplianceInterface.h"

static MMI_HANDLE gCompliance = NULL;

static const char gComponentName[] = "Compliance";

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
    ComplianceInitialize(log);
    gCompliance = ComplianceMmiOpen(gComponentName, -1);
}

void BaselineShutdown(OsConfigLogHandle log)
{
    UNUSED(log);
    if (NULL == gCompliance)
    {
        return;
    }

    ComplianceMmiClose(gCompliance);
    ComplianceShutdown();
    gCompliance = NULL;
}

int BaselineMmiGet(const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes, OsConfigLogHandle log)
{
    if ((NULL == componentName) || (NULL == objectName))
    {
        OsConfigLogError(log, "BaselineMmiGet called with invalid arguments");
        return EINVAL;
    }

    int result = ComplianceMmiGet(gCompliance, componentName, objectName, payload, payloadSizeBytes);
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
    return ComplianceMmiSet(gCompliance, componentName, objectName, payload, payloadSizeBytes);
}
