// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef BASELINE_H
#define BASELINE_H

#ifdef __cplusplus
extern "C"
{
#endif

int BaselineIsValidResourceIdRuleId(const char* resourceId, const char* ruleId, const char* payloadKey, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int BaselineIsCorrectDistribution(const char* payloadKey, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

void BaselineInitialize(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
void BaselineShutdown(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

int BaselineMmiGet(const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int BaselineMmiSet(const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

#ifdef __cplusplus
}
#endif

#endif // BASELINE_H
