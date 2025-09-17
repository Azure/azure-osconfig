// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef ASB_H
#define ASB_H

#ifdef __cplusplus
extern "C"
{
#endif

int AsbIsValidResourceIdRuleId(const char* resourceId, const char* ruleId, const char* payloadKey, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

void AsbInitialize(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
void AsbShutdown(OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

int AsbMmiGet(const char* componentName, const char* objectName, char** payload, int* payloadSizeBytes, unsigned int maxPayloadSizeBytes, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);
int AsbMmiSet(const char* componentName, const char* objectName, const char* payload, const int payloadSizeBytes, OsConfigLogHandle log, OSConfigTelemetryHandle telemetry);

#ifdef __cplusplus
}
#endif

#endif // ASB_H
