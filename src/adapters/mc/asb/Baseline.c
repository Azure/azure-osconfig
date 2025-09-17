// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "../Common.h"
#include <Asb.h>

int BaselineIsValidResourceIdRuleId(const char* resourceId, const char* ruleId, const char* payloadKey, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    UNUSED(telemetry);
    return AsbIsValidResourceIdRuleId(resourceId, ruleId, payloadKey, log, telemetry);
}

int BaselineIsCorrectDistribution(const char* payloadKey, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    UNUSED(telemetry);
    UNUSED(payloadKey);
    UNUSED(log);
    UNUSED(telemetry);
    return 0;
}

void BaselineInitialize(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    UNUSED(telemetry);
    AsbInitialize(log, telemetry);
}

void BaselineShutdown(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    UNUSED(telemetry);
    AsbShutdown(log, telemetry);
}

int BaselineMmiGet(const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    UNUSED(telemetry);
    return AsbMmiGet(componentName, objectName, payload, payloadSizeBytes, maxPayloadSizeBytes, log, telemetry);
}

int BaselineMmiSet(const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry)
{
    UNUSED(telemetry);
    return AsbMmiSet(componentName, objectName, payload, payloadSizeBytes, log, telemetry);
}
