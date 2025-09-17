// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../Common.h"
#include "ComplianceEngineInterface.h"

static MMI_HANDLE gComplianceEngine = NULL;
static const char gComponentName[] = "ComplianceEngine";

int BaselineIsValidResourceIdRuleId(const char* resourceId, const char* ruleId, const char* payloadKey, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    UNUSED(telemetry);
    UNUSED(resourceId);
    UNUSED(ruleId);
    UNUSED(payloadKey);
    UNUSED(log);
    return 0;
}

int BaselineIsCorrectDistribution(const char* payloadKey, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    UNUSED(telemetry);
    return ComplianceEngineCheckApplicability(gComplianceEngine, payloadKey, log);
}

// This function is called in library constructor in OsConfigResource.c
void BaselineInitialize(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    ComplianceEngineInitialize(log, telemetry);
    gComplianceEngine = ComplianceEngineMmiOpen(gComponentName, -1);
}

// This function is called in library destructor in OsConfigResource.c
void BaselineShutdown(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    UNUSED(log);
    UNUSED(telemetry);
    if (NULL == gComplianceEngine)
    {
        return;
    }

    ComplianceEngineMmiClose(gComplianceEngine);
    ComplianceEngineShutdown();
    gComplianceEngine = NULL;
}

int BaselineMmiGet(const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    UNUSED(telemetry);
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

int BaselineMmiSet(const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    UNUSED(log);
    UNUSED(telemetry);
    return ComplianceEngineMmiSet(gComplianceEngine, componentName, objectName, payload, payloadSizeBytes);
}
